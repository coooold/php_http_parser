<?php
ini_set('memory_limit', '128M');
ini_set('display_errors',1);
error_reporting(E_ALL);

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


function test(){
	global $buffs;
	$hp = new HttpParser();
	$hp->on('body', function($data){
			});

	

	foreach($buffs as $buff){
			$hp->execute($buff);
	}
}

$i=0;
while($i++<10000){test();}
