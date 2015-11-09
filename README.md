# php_http_parser
基于node.js http-parser的PHP扩展，可用于实现纯异步PHP程序

libcurl提供了异步调用方式，有两种风格：
- ONE MULTI HANDLE MANY EASY HANDLES：加入多个easy handle后执行curl_multi_perform方法。该方法在php curl扩展中有对应实现。但最后一步curl_multi_perform是阻塞的。
- MULTI_SOCKET，这个是真正的非阻塞方法，但需要自行实现event loop，且封装较为困难，目前在php中没有对应实现。经过调研，curl_multi_socket_action跟php内核结合困难度很高。

除此之外，基本上没有真正的实现异步http请求的php扩展。