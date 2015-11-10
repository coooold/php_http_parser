/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <malloc.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_http_parser.h"
#include "lib/http-parser/http_parser.h"

//#define hp_debug(s) do{printf("HP_DEBUG: %s\n", s);}while(0)
#define hp_debug(s)

/* If you declare any globals in php_http_parser.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(http_parser)
*/

zend_class_entry *http_parser_ce;

/* True global resources - no need for thread safety here */
static int le_http_parser;

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("http_parser.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_http_parser_globals, http_parser_globals)
    STD_PHP_INI_ENTRY("http_parser.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_http_parser_globals, http_parser_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_http_parser_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_http_parser_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "http_parser", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_http_parser_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_http_parser_init_globals(zend_http_parser_globals *http_parser_globals)
{
	http_parser_globals->global_value = 0;
	http_parser_globals->global_string = NULL;
}
*/
/* }}} */



ZEND_BEGIN_ARG_INFO(arg_http_parser_on, 0)
	ZEND_ARG_INFO(0, event)
	ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arg_http_parser_execute, 0)
	ZEND_ARG_INFO(0, buff)
ZEND_END_ARG_INFO()

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(http_parser)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(http_parser)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(http_parser)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(http_parser)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "http_parser support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ http_parser_functions[]
 *
 * Every user visible function must have an entry in http_parser_functions[].
 */
const zend_function_entry http_parser_functions[] = {
	PHP_FE(confirm_http_parser_compiled,	NULL)		/* For testing, remove later. */
	PHP_ME(HttpParser, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
	PHP_ME(HttpParser, __destruct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_DTOR)
	PHP_ME(HttpParser, on, arg_http_parser_on, ZEND_ACC_PUBLIC)
	PHP_ME(HttpParser, execute, arg_http_parser_execute, ZEND_ACC_PUBLIC)
	PHP_FE_END	/* Must be the last line in http_parser_functions[] */
};
/* }}} */


typedef struct{
	char* buff;
	size_t size;
	size_t len;
} hp_buff;

http_parser_settings *settings;

typedef struct {
	http_parser parser;
	hp_buff body;
	hp_buff header;
	zval* this;
} hp_context_t;



void hp_buff_init(hp_buff *buff){
	hp_debug("\thp_buff_init()");
	buff->buff = (char*)malloc(1024*4*sizeof(char*));
	buff->len = 0;
	buff->size = 1024*4*sizeof(char*);
	//bzero(buff->buff, buff->size);
}

void hp_buff_concat(hp_buff *buff, const char* str, size_t str_len){
	hp_debug("\thp_buff_concat()");
	if(buff->len + str_len >= buff->size){
		size_t old_size = buff->size;
		size_t new_size = old_size*2;
		char* old_buff = buff->buff;
		char* new_buff = (char*)realloc(old_buff, new_size);
		if(!new_buff)return;
		//bzero(new_buff + old_size, new_size); 
		buff->buff = new_buff;
		buff->size = new_size;
	}

	memcpy(buff->buff + buff->len, str, str_len);
	buff->len += str_len;
}

void hp_buff_dtor(hp_buff *buff){
	hp_debug("\thp_buff_dtor()");
	free(buff->buff);
	buff->size = 0;
	buff->len = 0;
}

/* {{{ object bind */

typedef struct{
	void **array;
	uint32_t size;
} hp_object_bind_t;
__thread hp_object_bind_t hp_object_bind;

void hp_object_init(){
	hp_debug("hp_object_init");
	hp_object_bind.size = 65536;
	hp_object_bind.array = malloc(hp_object_bind.size * sizeof(void*));
	memset(hp_object_bind.array, 0, hp_object_bind.size * sizeof(void*));
}

static void* hp_object_get(zval *object){
	int handle = (int) Z_OBJ_HANDLE(*object);
	assert(handle < hp_object_bind.size);
	hp_debug("hp_object_get()");
//printf("object handle %d\n", handle);
	return hp_object_bind.array[handle];
}

void hp_object_set(zval *object, void *ptr){
	int handle = (int) Z_OBJ_HANDLE(*object);
	
	hp_debug("hp_object_set");
//printf("object handle %d\n", handle);
	if(handle >= hp_object_bind.size){
		uint32_t old_size = hp_object_bind.size;
		uint32_t new_size = hp_object_bind.size * 2;
		
		void *old_array = hp_object_bind.array;
		void *new_array = realloc(old_array, sizeof(void*) * new_size);
		
		if(!new_array){
			return;
		}
		bzero(new_array + (old_size * sizeof(void*)), (new_size - old_size) * sizeof(void*));
		
		hp_object_bind.array = new_array;
		hp_object_bind.size = new_size;
	}
	
	hp_object_bind.array[handle] = ptr;
}

/* object bind end }}} */

int _on_body_cb(http_parser* parser, const char *at, size_t length){
	hp_debug("_on_ body_cb() start");
	zval *cb;
	hp_context_t *ctx;
	zval *instance;
	zval *retval;
	zval **args[1];
	zval *buff;
	int ret = 0;

	ctx = (hp_context_t*)parser->data;
	instance = ctx->this;

	hp_buff_concat(&ctx->body, at, length);

	if(http_body_is_final(parser)){
		MAKE_STD_ZVAL(buff);
		ZVAL_STRINGL(buff, ctx->body.buff, (int)ctx->body.len, 1);
		args[0] = &buff;

		cb = zend_read_property(Z_OBJCE_P(instance), instance, ZEND_STRL("bodyCallback"), 1 TSRMLS_DC);

		//char *func_name = NULL; 
		//if(zend_is_callable(cb, 0, &func_name TSRMLS_CC)){ 
		if(cb->type != IS_NULL){
			call_user_function_ex(NULL, NULL, cb, &retval, 1, args, 0, NULL TSRMLS_CC);
			ret = (int)Z_LVAL_P(retval);
			zval_ptr_dtor(&retval);
		}
		//efree(func_name);
		zval_ptr_dtor(buff);
	}
	hp_debug("_on_ body_cb() end");
	return 0;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(http_parser)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	zend_class_entry http_parser;
	INIT_CLASS_ENTRY(http_parser, "HttpParser", http_parser_functions);
	http_parser_ce = zend_register_internal_class_ex(&http_parser, NULL, NULL TSRMLS_CC);
	
	zend_declare_property_null(http_parser_ce, ZEND_STRL("headerCallback"), ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(http_parser_ce, ZEND_STRL("bodyCallback"), ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(http_parser_ce, ZEND_STRL("errorCallback"), ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(http_parser_ce, ZEND_STRL("header"), ZEND_ACC_PRIVATE TSRMLS_CC);
	zend_declare_property_null(http_parser_ce, ZEND_STRL("body"), ZEND_ACC_PRIVATE TSRMLS_CC);
	
	hp_object_init();

	settings = (http_parser_settings*)pemalloc(sizeof(http_parser_settings), 1);
	settings->on_message_begin = NULL;
	settings->on_header_field = NULL;
	settings->on_header_value = NULL;
	settings->on_url = NULL;
	settings->on_status = NULL;
	settings->on_body = _on_body_cb;
	settings->on_headers_complete = NULL;
	settings->on_message_complete = NULL;
	settings->on_chunk_header = NULL;
	settings->on_chunk_complete = NULL;
	
	return SUCCESS;
}
/* }}} */

/* {{{ http_parser_module_entry
 */
zend_module_entry http_parser_module_entry = {
	STANDARD_MODULE_HEADER,
	"http_parser",
	http_parser_functions,
	PHP_MINIT(http_parser),
	PHP_MSHUTDOWN(http_parser),
	PHP_RINIT(http_parser),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(http_parser),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(http_parser),
	PHP_HTTP_PARSER_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_HTTP_PARSER
ZEND_GET_MODULE(http_parser)
#endif



PHP_METHOD(HttpParser, __construct){
	hp_debug("HttpParser::__construct() start");
	hp_context_t *hp_ctx;
	hp_ctx = (hp_context_t*)pemalloc(sizeof(hp_context_t), 1);

	hp_ctx->this = getThis();
	hp_buff_init(&hp_ctx->body);
	hp_buff_init(&hp_ctx->header);

	http_parser_init(&(hp_ctx->parser), HTTP_RESPONSE);
	hp_ctx->parser.data = hp_ctx;
	hp_object_set(getThis(), hp_ctx);
	hp_debug("HttpParser::__construct() end");
}


PHP_METHOD(HttpParser, __destruct){
	hp_debug("HttpParser::__destruct() start");
	hp_context_t *hp_ctx;

	hp_ctx = (hp_context_t*)hp_object_get(getThis());
	hp_buff_dtor(&hp_ctx->body);
	hp_buff_dtor(&hp_ctx->header);
	pefree(hp_ctx, 1);
	hp_debug("HttpParser::__destruct() end");
}


PHP_METHOD(HttpParser, on){
	hp_debug("HttpParser::on() start");
	char *event = NULL;
	int event_len;
	
	zval *cb;
	zval *val;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &event, &event_len, &cb) == FAILURE){
		WRONG_PARAM_COUNT;
	}
	
	if(0 == strncmp(event, "header", strlen("header"))){
		zend_update_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("headerCallback"), cb TSRMLS_CC);
	}else if(0 == strncmp(event, "body", strlen("body"))){
		zend_update_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("bodyCallback"), cb TSRMLS_CC);
	}else if(0 == strncmp(event, "error", strlen("error"))){
		zend_update_property(Z_OBJCE_P(getThis()), getThis(), ZEND_STRL("errorCallback"), cb TSRMLS_CC);
	}

	hp_debug("HttpParser::on() end");
}


PHP_METHOD(HttpParser, execute){
	char *buff;
	int buff_len;
	size_t nparsed;
	hp_context_t *hp_ctx;
	
	hp_debug("execute start");
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &buff, &buff_len) == FAILURE){
		WRONG_PARAM_COUNT;
	}

	hp_ctx = (hp_context_t*)hp_object_get(getThis());
	nparsed = http_parser_execute(&(hp_ctx->parser), settings, buff, (size_t)buff_len);
	
	ZVAL_LONG(return_value, nparsed);

	hp_debug("execute end");
}












/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

