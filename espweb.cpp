#define __IN_ESPWEB_CPP
#include "espweb.h"
#include <WebServer.h>

/* #include "main.h" */
#include "btn.h"
#include "defs.h"
#include <capproc.h>

WebServer server(80);
bool web_initted=false;

void http_redirect(const char *where, const char *msg) {
	http200plain();
	mimehtml();
	server.sendContent("<html><head><title>Reset</title>"
		"<meta http-equiv=refresh content='2;url=");
	server.sendContent(where);
	server.sendContent("' /></head><body>");
	server.sendContent(msg);
	server.sendContent("</body></html>");
}

void http_sdata_on()  { http_redirect("/", "Data on"); cp_sense_debug_data = 1; }
void http_sdata_off() { http_redirect("/", "Data off"); cp_sense_debug_data = 0; }


void setup_web() {
	server.on("/", HTTP_GET, http_root);
	server.on("/sdata_on", HTTP_GET, http_sdata_on);
	server.on("/sdata_off", HTTP_GET, http_sdata_off);
	server.on("/reset", http_reset);
	server.begin();
	web_initted=true;
}

void loop_web() {
	if (web_initted) server.handleClient();
}

void mimetext() {
	server.sendContent("Content-Type: text/plain;charset=UTF-8\r\n\r\n");
}
void mimehtml() {
	server.sendContent("Content-Type: text/html;charset=UTF-8\r\n\r\n");
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
	http_redirect("/", "Reeet");
	for (int i=0; i<10; i++) delay(10); // give time for page send
	ESP.restart();
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
		"<div>[ <a href=/reset>Reset</a> ]</div>"
		"<div>[ Sensor data <a href=/sdata_on>On</a>, <a href=/sdata_off>Off</a>"
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

