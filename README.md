# php_http_parser PHP解析HTTP扩展
基于node.js http-parser的PHP扩展，可用于实现纯异步PHP程序

libcurl提供了异步调用方式，有两种风格：
- ONE MULTI HANDLE MANY EASY HANDLES：加入多个easy handle后执行curl_multi_perform方法。该方法在php curl扩展中有对应实现。但最后一步curl_multi_perform是阻塞的。
- MULTI_SOCKET，这个是真正的非阻塞方法，但需要自行实现event loop，且封装较为困难，目前在php中没有对应实现。经过调研，curl_multi_socket_action跟php内核结合困难度很高。

除此之外，基本上没有真正的实现异步http请求的php扩展。目前仅有部分纯php实现的版本，比如[tsf](https://github.com/tencent-php/tsf)中的http client实现。
使用纯php实现的问题主要受限于http解析的性能。因此考虑将这一模块用扩展的方式来实现。node.js [http-parser](https://github.com/nodejs/http-parser)就是一个很好的c语言的http解析库。
php_http_parser就是对其做的一个封装，在php中暴露出相应的接口。

为了实现真正的非阻塞请求，仍然需要自己实现event loop。目前推荐结合[swoole](http://www.swoole.com)使用，以获得更好的性能。

## 使用方式

```php
$buffs = array("HTTP/1.1 301 Moved Permanently\r\n"
,"Location: http://www.google.com/\r\n"
,"Content-Type: text/html; charset=UTF-8\r\n"
,"Date: Sun, 26 Apr 2009 11:11:49 GMT\r\n"
,"Expires: Tue, 26 May 2009 11:11:49 GMT\r\n"
,"Cache-Control: public, max-age=2592000\r\n"
,"Server: gws\r\n"
,"Content-Length: 193\r\n"
,"\r\n"
,"<HTML><HEAD><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\">\n"
,"<TITLE>301 Moved</TITLE></HEAD><BODY>\n"
,"<H1>301 Moved</H1>\n"
,"The document has moved\n"
,"<A HREF=\"http://www.google.com/\">here</A>.\r\n"
	,"<A HREF=\"http://www.google.com/\">here</A>.\r\n"
	,"<A HREF=\"http://www.google.com/\">here</A>.\r\n"
	,"<A HREF=\"http://www.google.com/\">here</A>.\r\n"
	,"<A HREF=\"http://www.google.com/\">here</A>.\r\n"
,"</BODY></HTML>\r\n");


$hp = new HttpParser();
$hp->on('body', function($data){
	echo $data;
});

global $buffs;

foreach($buffs as $buff){
	$hp->execute($buff);
}
```

虽然http请求可能分包发送，HttpParser会将所有包合并在一起后，出发body事件，然后调用相应的回调方法。
诸如header回调，目前暂未实现。另外，此处需要自行实现timeout逻辑。

示例代码是结合swoole_client与[swPromise](https://github.com/coooold/swPromise)框架实现的一个异步http client。
籍此可以实现真正的非阻塞的PHP程序。

```php
class HttpClientFuture implements FutureIntf {
	protected $url = null;
	protected $post = null;

	public $rspHeaders = array();		//返回头信息
	public $body = '';
	
	protected $buffer = '';
	protected $isFinish = false;
	protected $trunk_length = 0;
	
	protected $proxy = false;
	
	public function __construct($url, $post = array(), $proxy = array()) {
		$this->url = $url;
		$this->post = $post;
		if($proxy){
			$this->proxy = $proxy;
		}
	}
	public function run(Promise &$promise) {
		$cli = new \swoole_client ( SWOOLE_TCP, SWOOLE_SOCK_ASYNC );
		$urlInfo = parse_url ( $this->url );
		if(!isset($urlInfo ['port']))$urlInfo ['port'] = 80;
		
		$httpParser = new \HttpParser();
		$httpParser->on('body', function($data)use(&$promise){
			//echo $data;
			$promise->accept(['http_data' => $data]);
		});
		
		$cli->on ( "connect", function ($cli)use($urlInfo){
			$host = $urlInfo['host'];
			if($urlInfo['port'])$host .= ':'.$urlInfo['port'];
			$req = array();
			$req[] = "GET {$this->url} HTTP/1.1\r\n";
			$req[] = "User-Agent: PHP swAsync\r\n";
			$req[] = "Host:{$host}\r\n";
			$req[] = "Connection:close\r\n";
			$req[] = "\r\n";
			$req = implode('', $req);
			$cli->send ( $req );
		} );
		$cli->on ( "receive", function ($cli, $data = "") use(&$httpParser) {
			$httpParser->execute($data);
		} );
		$cli->on ( "error", function ($cli) use(&$promise) {
			$promise->reject ();
		} );
		$cli->on ( "close", function ($cli) {
		} );
		
		
		if($this->proxy){
			$cli->connect ( $this->proxy['host'], $this->proxy ['port'], 1 );
		}else{
			$cli->connect ( $urlInfo ['host'], $urlInfo ['port'], 1 );
		}
	}
}
```


## 其他
经测试，扩展版本比纯php版性能不强多少。也可以理解为，http协议主要解析的时间都在header部分，后面其实没什么了。。。
就当练手吧。

