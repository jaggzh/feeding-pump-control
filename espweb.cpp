#define __IN_ESPWEB_CPP
#include "espweb.h"
#include <WebServer.h>

/* #include "main.h" */
#include "btn.h"
#include "defs.h"

WebServer server(80);

void setup_web() {
	server.on("/", HTTP_GET, http_root);
	/* server.on("/reset", http_reset); */
	server.begin();
}

void loop_web() {
	server.handleClient();
}

void mimehtml() {
	server.sendContent("Content-Type: text/html\r\n\r\n");
}
void mimebmp() {
	server.sendContent("Content-Type: image/bmp\r\n\r\n");
}
void http200plain() {
	server.sendContent("HTTP/1.0 200 OK\r\n");
}
void http200() {
	http200plain();
	mimehtml();
}
void http500() {
	server.sendContent("HTTP/1.0 500 Not OK\r\n");
	mimehtml();
}

void http_reset() {
	server.send(200, "text/html",
		"<html><head><title>On</title>"
		"<meta http-equiv=refresh content='2;url=/' /></head>"
		"<body>Resetting (2s)<br></body></html>");
}

void http_root() {
	char tmp[1001];
	http200();
	server.sendContent(
		"<!DOCTYPE html><html><head><title>Feeding pump</title>"
		"<meta charset=UTF-8 />"
		/* "<meta http-equiv=refresh content=20 />" */
		"<style type=text/css>"
		"</style>"
		"</head><body>");
	snprintf(tmp, 1000,
		"<div>Rate: %f (mapped:%d)</div>"
		"<div>Delay: %f (mapped:%d)</div>"
		"<div>X: %f (mapped:%d)</div>"
		"<div>[ <a href=/reset>On</a> ]</div>"
		"</body>"
		"</html>",
		potrate, MAP_POT_RATE(potrate),
		potdelay, MAP_POT_DELAY(potdelay),
		potx, MAP_POT_DELAY(potx)
		);
	server.sendContent(tmp);
	server.client().stop();
	//server.send(200, "text/html", tmp);
}

