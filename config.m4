dnl $Id$
dnl config.m4 for extension http_parser

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(http_parser, for http_parser support,
Make sure that the comment is aligned:
[  --with-http_parser             Include http_parser support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(http_parser, whether to enable http_parser support,
dnl Make sure that the comment is aligned:
dnl [  --enable-http_parser           Enable http_parser support])

if test "$PHP_HTTP_PARSER" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-http_parser -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/http_parser.h"  # you most likely want to change this
  dnl if test -r $PHP_HTTP_PARSER/$SEARCH_FOR; then # path given as parameter
  dnl   HTTP_PARSER_DIR=$PHP_HTTP_PARSER
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for http_parser files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       HTTP_PARSER_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$HTTP_PARSER_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the http_parser distribution])
  dnl fi

  dnl # --with-http_parser -> add include path
  dnl PHP_ADD_INCLUDE($HTTP_PARSER_DIR/include)

  dnl # --with-http_parser -> check for lib and symbol presence
  dnl LIBNAME=http_parser # you may want to change this
  dnl LIBSYMBOL=http_parser # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $HTTP_PARSER_DIR/$PHP_LIBDIR, HTTP_PARSER_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_HTTP_PARSERLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong http_parser lib version or lib not found])
  dnl ],[
  dnl   -L$HTTP_PARSER_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(HTTP_PARSER_SHARED_LIBADD)

  PHP_NEW_EXTENSION(http_parser, http_parser.c lib/http-parser/http_parser.c, $ext_shared)
  PHP_ADD_BUILD_DIR($ext_builddir/lib/http-parser)
fi


if test -z "$PHP_DEBUG"; then 
	AC_ARG_ENABLE(debug,
	[	--enable-debug			compile with debugging symbols],[
		PHP_DEBUG=$enableval
	],[	PHP_DEBUG=no
	])
fi
