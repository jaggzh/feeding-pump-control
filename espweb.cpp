#define __IN_ESPWEB_CPP
#include "espweb.h"
#include <WebServer.h>

#include "main.h"
#include "btn.h"
#include "defs.h"
#include "pump.h"
#include <capsense.h>

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

void http_capclosed_on() { http_redirect("/", "Closed data on"); stg_show_closed = 1; }
void http_capclosed_off() { http_redirect("/", "Closed data off"); stg_show_closed = 0; }

void http_capopen_on() { http_redirect("/", "Open data on"); stg_show_open = 1; }
void http_capopen_off() { http_redirect("/", "Open data off"); stg_show_open = 0; }

void http_set() {
	struct web_set_floats {
		char *cginame;
		float *dest;
	};
	struct web_set_floats fsets[] = {
		{ "thresh_diff", &cp1->thresh_diff },
	const char *pname;
	pname = "thresh_diff";
	if (server.hasArg(pname)) {
		String val = server.arg(pname);
		cp1->thresh_diff = strtof(val.c_str(), NULL);
	}
	pname = "thresh_integ";
	if (server.hasArg(pname)) {
		String val = server.arg(pname);
		cp1->thresh_integ = strtof(val.c_str(), NULL);
	}
	pname = "leak_integ";
	if (server.hasArg(pname)) {
		String val = server.arg(pname);
		cp1->leak_integ = strtof(val.c_str(), NULL);
	}
	pname = "leak_integ_fail";
	if (server.hasArg(pname)) {
		String val = server.arg(pname);
		cp1->leak_integ_no = strtof(val.c_str(), NULL);
	}
	http_redirect("/", "Set value");
}

void setup_web() {
	server.on("/", HTTP_GET, http_root);
	server.on("/sdata_on", HTTP_GET, http_sdata_on);
	server.on("/sdata_off", HTTP_GET, http_sdata_off);
	server.on("/cap_closed_on", HTTP_GET, http_capclosed_on);
	server.on("/cap_closed_off", HTTP_GET, http_capclosed_off);
	server.on("/cap_open_on", HTTP_GET, http_capopen_on);
	server.on("/cap_open_off", HTTP_GET, http_capopen_off);
	server.on("/reset", HTTP_GET, http_reset);
	server.on("/set", HTTP_GET, http_set);
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
	Serial.println("Connection to / received");
	http200();
	server.sendContent(
		"<!DOCTYPE html><html><head><title>Feeding pump</title>"
		"<meta charset=UTF-8 />"
		/* "<meta http-equiv=refresh content=20 />" */
		"<style type=text/css>"
		"</style>"
		"</head><body>");
	snprintf(tmp, 1000,
		"<div>Rate: %d (mapped:%d)</div>"
		"<div>Sensitivity: %d (mapped:%f)</div>"
		#ifdef POT_X_PIN
			"<div>X: %f (mapped:%d)</div>"
		#endif
		"<div>[ <a href=/reset>Reset</a> ]</div>"
		"<div>[ Sensor data <a href=/sdata_on>On</a>, <a href=/sdata_off>Off</a> ]</div>"
		"<div>[ Cap data: [Closed <a href=/cap_closed_on>on</a>, <a href=/cap_closed_off>off</a> ]"
		                 "[Open <a href=/cap_open_on>on</a>, <a href=/cap_open_off>off</a> ]</div>"
		"<div>Pump State: %s</div>",
		potrate_raw, potrate,
		potsens_raw, potsens,
		#ifdef POT_X_PIN
			potx, MAP_POT_DELAY(potx)
		#endif
		pumpstatestr[pumpstate]
		);
	server.sendContent(tmp);
	snprintf(tmp, 1000,
		"<div><form action=/set>Diff thresh: <input name=thresh_diff value=%f></form></div>"
		"<div><form action=/set>Integ thresh: <input name=thresh_integ value=%f></form></div>"
		"<div><form action=/set>Leak integral: <input name=leak_integ value=%f></form></div>"
		"<div><form action=/set>Leak integral inactive: <input name=leak_integ_no value=%f></form></div>"
		"</body>"
		"</html>",
		cp1->thresh_diff,
		cp1->thresh_integ,
		cp1->leak_integ,
		cp1->leak_integ_no
		);
	server.sendContent(tmp);
	server.client().stop();
	//server.send(200, "text/html", tmp);
}

