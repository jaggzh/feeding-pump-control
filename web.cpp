#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "btn.h"
#include "pump.h"
#include "printutils.h"
#include "wifi_config.h"

AsyncWebServer server(80);

const char* get_function_flags_str(uint16_t flags) {
	if (flags == FEATSET_HIDONLY) return "HIDONLY";
	if (flags == FEATSET_PUMPONLY) return "PUMPONLY";
	if (flags == FEATSET_PUMPHID) return "PUMPHID";
	return "CUSTOM";
}

void notFound(AsyncWebServerRequest *request) {
	request->send(404, "text/plain", "Not found");
}

void setup_web() {
	if (WiFi.status() != WL_CONNECTED)
		Serial.println("WiFi not connected. Doing web setup anyway...");
	else 
		Serial.println("Connected to WiFi");

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		String html = "<html><head><title>ESP Pump Control</title></head>\n"
			"<style>"
			"h1, p, div {font-size:200%;margin:0;padding:0 0 .1em 0;}"
			"#log{font-size:130%;}"
			".st{font-size:90%;}"
			"</style>"
			"<body>\n";

		html += "<script>\n";
		html += "function sa(url) {\n";
		html += " fetch(url).then(response => {\n";
		html += "  if(response.ok) {\n";
		html += "   logAction('Action ' + url + ' executed');\n";
		html += "  }\n";
		html += "  return response.text();\n";
		html += " }).then(data => {\n";
		html += "  console.log(data); // Log response from the server if needed\n";
		html += " }).catch(err => console.error('Error with ' + url + ':', err));\n";
		html += "}\n";
		html += "function logAction(message) {\n";
		html += " var log = document.getElementById('log');\n";
		html += " var newLogEntry = new Date().toLocaleTimeString() + ' - ' + message + '<br>';\n";
		html += " log.innerHTML += newLogEntry;\n";
		html += " log.scrollTop = log.scrollHeight;\n";
		html += "}\n";
		html += "</script>\n";

		html += "<h1>Pump Control Panel - v2025-10-12 03:13</h1>\n";
		html += "<p class=st>Status at page load: " + String(pumpstatestr[pumpstate]) +
		        " [" + String((motorlocked ? "LOCKED" : "Unlocked")) + "]";
		html += " Functions: " + String(get_function_flags_str(function_flags)) +
		        " (0x" + String(function_flags, HEX) + ")";
		html += " Pump:" + String((function_flags & FUNC_PUMP) ? "ON" : "off") +
		        " Alarms:" + String((function_flags & FUNC_NET_ALARMS) ? "ON" : "off") +
		        " HID:" + String((function_flags & FUNC_NET_HID) ? "ON" : "off");
		html += " [RateRaw: " + String(potrate) + "]";
		html += " ManualSpd: " + String(manual_speed_enabled ? "ON val=" + String(manual_speed_val) : "off") + "</p>\n";
		
		html += "<p class=st>Alarm Host: ";
		html += (runtime_alarm_host != NULL) ? String(runtime_alarm_host) : String(HOST_ALARM) + " (default)";
		html += " Ports: HOLD=";
		html += (runtime_alarm_port_hold != -1) ? String(runtime_alarm_port_hold) : String(PORT_ALARM_HOLD) + " (def)";
		html += " TOOLONG=";
		html += (runtime_alarm_port_toolong != -1) ? String(runtime_alarm_port_toolong) : String(PORT_ALARM_TOOLONG) + " (def)";
		html += "</p>\n";
		
		html += "<p class=st>HID Host: ";
		html += (runtime_hid_host != NULL) ? String(runtime_hid_host) : String(HOST_HID) + " (default)";
		html += " Port: ";
		html += (runtime_hid_port != -1) ? String(runtime_hid_port) : String(PORT_HID) + " (def)";
		html += "</p>\n";

		html += "<div>"
			"[<a href='javascript:void(0);' onclick='sa(\"/lock\");'>/lock</a> "
			"| <a href='javascript:void(0);' onclick='sa(\"/unlock\");'>/unlock</a>] "
			"&mdash; [Debug "
			"<a href='javascript:void(0);' onclick='sa(\"/debug_inc\");'>+</a> / "
			"<a href='javascript:void(0);' onclick='sa(\"/debug_dec\");'>-</a>]"
			"</div>\n";
		html += "<div>[Mode: "
			"<a href='javascript:void(0);' onclick='sa(\"/hidmode?1\");'>HID</a> | "
			"<a href='javascript:void(0);' onclick='sa(\"/pumpmode?1\");'>PUMP</a> | "
			"<a href='javascript:void(0);' onclick='sa(\"/pumphidmode?1\");'>PUMPHID</a>]</div>\n";
		html += "<div>[Fine: "
			"<a href='javascript:void(0);' onclick='sa(\"/func_pump?1\");'>Pump+</a>/"
			"<a href='javascript:void(0);' onclick='sa(\"/func_pump?0\");'>-</a> "
			"<a href='javascript:void(0);' onclick='sa(\"/func_net_alarms?1\");'>Alarms+</a>/"
			"<a href='javascript:void(0);' onclick='sa(\"/func_net_alarms?0\");'>-</a> "
			"<a href='javascript:void(0);' onclick='sa(\"/func_net_hid?1\");'>HID+</a>/"
			"<a href='javascript:void(0);' onclick='sa(\"/func_net_hid?0\");'>-</a>]</div>\n";
		html += "<div>[<a href='javascript:void(0);' onclick='sa(\"/fwdon\");'>/fwdon</a> | <a href='javascript:void(0);' onclick='sa(\"/revon\");'>/revon</a>] [<a href='javascript:void(0);' onclick='sa(\"/off\");'>/off</a>]</div>\n";
		
		// Manual speed control
		String onStyle  = manual_speed_enabled ? "font-weight:bold;font-style:italic;" : "";
		String offStyle = manual_speed_enabled ? "" : "font-weight:bold;font-style:italic;";
		html += "<div>[Manual Speed: "
			"<a href='javascript:void(0);' onclick='sa(\"/manual_speed_on\");' style='" + onStyle + "'>on</a>"
			" | "
			"<a href='javascript:void(0);' onclick='sa(\"/manual_speed_off\");' style='" + offStyle + "'>off</a>"
			"] "
			"<input type='number' id='manualSpeedVal' min='0' max='254' value='" + String(manual_speed_val) + "' style='font-size:80%;width:4em;'> "
			"<a href='javascript:void(0);' onclick='sa(\"/set_manual_speed?val=\" + document.getElementById(\"manualSpeedVal\").value);'>Set</a>"
			"</div>\n";
		html += "<div>[Hosts: <a href='javascript:void(0);' onclick='sa(\"/resethosts_all\");'>Reset all to defaults</a>]"
			" &mdash; [<a href='javascript:void(0);' onclick='if(confirm(\"Reboot?\")) sa(\"/reboot\");'>Reboot</a>]</div>\n";
		html += "<div id='log' style='height:400px;overflow:auto;background:#f0f0f0;padding:10px;'></div>\n";

		html += "<script>\n";
		html += "var priorStatus = '';  // To store the last status\n";
		html += "var maxLines = 250;    // Maximum number of lines in the log\n";
		html += "setInterval(function() {\n";
		html += " fetch('/status').then(response => response.text()).then(data => {\n";
		html += "  var log = document.getElementById('log');\n";
		html += "  var newData = data.substring(data.indexOf('Motor:'));\n";
		html += "  if (priorStatus !== newData) {\n";
		html += "   priorStatus = newData;\n";
		html += "   log.innerHTML += data + '<br>';\n";
		html += "   log.scrollTop = log.scrollHeight;\n";
		html += "   var entries = log.innerHTML.split('<br>');\n";
		html += "   if (entries.length > maxLines) {\n";
		html += "    log.innerHTML = entries.slice(entries.length - maxLines).join('<br>');\n";
		html += "   }\n";
		html += "  }\n";
		html += " }).catch(err => console.error(err));\n";
		html += "}, 1000);\n";
		html += "</script>\n";
		html += "</body></html>";
		request->send(200, "text/html", html);
	});

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
		String status = String(millis())
			+ " ms Motor:" + String(pumpstatestr[pumpstate])
			+ " [" + (motorlocked ? "LOCKED" : "Unlocked")
			+ "] Func:" + String(get_function_flags_str(function_flags))
			+ "(0x" + String(function_flags, HEX) + ")"
			+ " PotRate:" + String(potrate) + " PotX:" + String(potx)
			+ " ManualSpd:" + String(manual_speed_enabled ? "ON val=" + String(manual_speed_val) : "off");
		request->send(200, "text/plain", status);
	});
	
	server.on("/funcstatus", HTTP_GET, [](AsyncWebServerRequest *request){
		String status = "Function Flags: 0x" + String(function_flags, HEX) + " (" + String(get_function_flags_str(function_flags)) + ")\n";
		status += "FUNC_PUMP: " + String((function_flags & FUNC_PUMP) ? "ENABLED" : "disabled") + "\n";
		status += "FUNC_NET_ALARMS: " + String((function_flags & FUNC_NET_ALARMS) ? "ENABLED" : "disabled") + "\n";
		status += "FUNC_NET_HID: " + String((function_flags & FUNC_NET_HID) ? "ENABLED" : "disabled") + "\n";
		request->send(200, "text/plain", status);
	});
	
	server.on("/lock", HTTP_GET, [](AsyncWebServerRequest *request){
		motorlocked = true;
		request->send(200, "text/plain", "Locked\n");
		spl("/LOCK");
	});
	server.on("/unlock", HTTP_GET, [](AsyncWebServerRequest *request){
		motorlocked = false;
		request->send(200, "text/plain", "Unlocked\n");
		spl("/UNLOCK");
	});
	
	// Convenient preset modes
	server.on("/hidmode", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("0") || (request->hasParam("value") && request->getParam("value")->value() == "0")) {
			function_flags &= ~FUNC_NET_HID;
			request->send(200, "text/plain", "HID mode disabled\n");
			spl("/HIDMODE?0");
		} else {
			function_flags = FEATSET_HIDONLY;
			set_all_off();  // Turn off pump when switching to HID-only
			request->send(200, "text/plain", "Mode set to HIDONLY\n");
			spl("/HIDMODE?1");
		}
	});
	
	server.on("/pumpmode", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("0") || (request->hasParam("value") && request->getParam("value")->value() == "0")) {
			function_flags &= ~FUNC_PUMP;
			set_all_off();
			request->send(200, "text/plain", "Pump mode disabled\n");
			spl("/PUMPMODE?0");
		} else {
			function_flags = FEATSET_PUMPONLY;
			request->send(200, "text/plain", "Mode set to PUMPONLY\n");
			spl("/PUMPMODE?1");
		}
	});
	
	server.on("/pumphidmode", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("1") || !request->hasParam("0")) {
			function_flags = FEATSET_PUMPHID;
			request->send(200, "text/plain", "Mode set to PUMPHID\n");
			spl("/PUMPHIDMODE?1");
		} else {
			request->send(200, "text/plain", "Invalid parameter\n");
		}
	});
	
	// Fine-grained function control
	server.on("/func_pump", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("0") || (request->hasParam("value") && request->getParam("value")->value() == "0")) {
			function_flags &= ~FUNC_PUMP;
			set_all_off();
			request->send(200, "text/plain", "FUNC_PUMP disabled\n");
			spl("/FUNC_PUMP?0");
		} else {
			function_flags |= FUNC_PUMP;
			request->send(200, "text/plain", "FUNC_PUMP enabled\n");
			spl("/FUNC_PUMP?1");
		}
	});
	
	server.on("/func_net_alarms", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("0") || (request->hasParam("value") && request->getParam("value")->value() == "0")) {
			function_flags &= ~FUNC_NET_ALARMS;
			request->send(200, "text/plain", "FUNC_NET_ALARMS disabled\n");
			spl("/FUNC_NET_ALARMS?0");
		} else {
			function_flags |= FUNC_NET_ALARMS;
			request->send(200, "text/plain", "FUNC_NET_ALARMS enabled\n");
			spl("/FUNC_NET_ALARMS?1");
		}
	});
	
	server.on("/func_net_hid", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("0") || (request->hasParam("value") && request->getParam("value")->value() == "0")) {
			function_flags &= ~FUNC_NET_HID;
			request->send(200, "text/plain", "FUNC_NET_HID disabled\n");
			spl("/FUNC_NET_HID?0");
		} else {
			function_flags |= FUNC_NET_HID;
			request->send(200, "text/plain", "FUNC_NET_HID enabled\n");
			spl("/FUNC_NET_HID?1");
		}
	});
	
	// Host configuration - alarm host
	server.on("/sethost_alarm", HTTP_GET, [](AsyncWebServerRequest *request){
		bool changed = false;
		String response = "Alarm host configuration updated:\n";
		
		if (request->hasParam("ip")) {
			String ip = request->getParam("ip")->value();
			if (runtime_alarm_host != NULL) {
				free(runtime_alarm_host);
			}
			runtime_alarm_host = (char*)malloc(ip.length() + 1);
			strcpy(runtime_alarm_host, ip.c_str());
			response += "  Host: " + ip + "\n";
			changed = true;
		}
		
		if (request->hasParam("port_hold")) {
			runtime_alarm_port_hold = request->getParam("port_hold")->value().toInt();
			response += "  Port HOLD: " + String(runtime_alarm_port_hold) + "\n";
			changed = true;
		}
		
		if (request->hasParam("port_toolong")) {
			runtime_alarm_port_toolong = request->getParam("port_toolong")->value().toInt();
			response += "  Port TOOLONG: " + String(runtime_alarm_port_toolong) + "\n";
			changed = true;
		}
		
		if (changed) {
			request->send(200, "text/plain", response);
			sp("/SETHOST_ALARM: "); spl(response);
		} else {
			request->send(400, "text/plain", "No parameters provided. Use: /sethost_alarm?ip=IP&port_hold=N&port_toolong=N\n");
		}
	});
	
	// Host configuration - HID host
	server.on("/sethost_hid", HTTP_GET, [](AsyncWebServerRequest *request){
		bool changed = false;
		String response = "HID host configuration updated:\n";
		
		if (request->hasParam("ip")) {
			String ip = request->getParam("ip")->value();
			if (runtime_hid_host != NULL) {
				free(runtime_hid_host);
			}
			runtime_hid_host = (char*)malloc(ip.length() + 1);
			strcpy(runtime_hid_host, ip.c_str());
			response += "  Host: " + ip + "\n";
			changed = true;
		}
		
		if (request->hasParam("port")) {
			runtime_hid_port = request->getParam("port")->value().toInt();
			response += "  Port: " + String(runtime_hid_port) + "\n";
			changed = true;
		}
		
		if (changed) {
			request->send(200, "text/plain", response);
			sp("/SETHOST_HID: "); spl(response);
		} else {
			request->send(400, "text/plain", "No parameters provided. Use: /sethost_hid?ip=IP&port=N\n");
		}
	});
	
	// Host reset functions
	server.on("/resethost_alarm", HTTP_GET, [](AsyncWebServerRequest *request){
		if (runtime_alarm_host != NULL) {
			free(runtime_alarm_host);
			runtime_alarm_host = NULL;
		}
		runtime_alarm_port_hold = -1;
		runtime_alarm_port_toolong = -1;
		request->send(200, "text/plain", "Alarm host reset to defaults\n");
		spl("/RESETHOST_ALARM");
	});
	
	server.on("/resethost_hid", HTTP_GET, [](AsyncWebServerRequest *request){
		if (runtime_hid_host != NULL) {
			free(runtime_hid_host);
			runtime_hid_host = NULL;
		}
		runtime_hid_port = -1;
		request->send(200, "text/plain", "HID host reset to defaults\n");
		spl("/RESETHOST_HID");
	});
	
	server.on("/resethosts_all", HTTP_GET, [](AsyncWebServerRequest *request){
		if (runtime_alarm_host != NULL) {
			free(runtime_alarm_host);
			runtime_alarm_host = NULL;
		}
		runtime_alarm_port_hold = -1;
		runtime_alarm_port_toolong = -1;
		if (runtime_hid_host != NULL) {
			free(runtime_hid_host);
			runtime_hid_host = NULL;
		}
		runtime_hid_port = -1;
		request->send(200, "text/plain", "All hosts reset to defaults\n");
		spl("/RESETHOSTS_ALL");
	});
	
	server.on("/fwdon", HTTP_GET, [](AsyncWebServerRequest *request){
		set_fwd_hold();
		request->send(200, "text/plain", "Forward Enabled\n");
		spl("/FWDON");
	});
	server.on("/revon", HTTP_GET, [](AsyncWebServerRequest *request){
		set_rev_hold();
		request->send(200, "text/plain", "Reverse Enabled\n");
		spl("/REVON");
	});
	server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
		set_all_off();
		request->send(200, "text/plain", "Pump turned OFF\n");
		spl("/OFF");
	});
	server.on("/debug_inc", HTTP_GET, [](AsyncWebServerRequest *request){
		if (debuglevel<100) debuglevel++;
		request->send(200, "text/plain", "Increased debug\n");
		spl("/debug_inc");
	});
	server.on("/debug_dec", HTTP_GET, [](AsyncWebServerRequest *request){
		if (debuglevel>0) debuglevel--;
		request->send(200, "text/plain", "Decreased debug\n");
		spl("/debug_dec");
	});
	server.on("/manual_speed_on", HTTP_GET, [](AsyncWebServerRequest *request){
		manual_speed_enabled = true;
		request->send(200, "text/plain", "Manual speed ON (val=" + String(manual_speed_val) + ")\n");
		sp("/MANUAL_SPEED_ON val="); spl(String(manual_speed_val).c_str());
	});
	server.on("/manual_speed_off", HTTP_GET, [](AsyncWebServerRequest *request){
		manual_speed_enabled = false;
		request->send(200, "text/plain", "Manual speed OFF\n");
		spl("/MANUAL_SPEED_OFF");
	});
	server.on("/set_manual_speed", HTTP_GET, [](AsyncWebServerRequest *request){
		if (request->hasParam("val")) {
			int v = request->getParam("val")->value().toInt();
			if (v < 0) v = 0;
			if (v > 254) v = 254;
			manual_speed_val = v;
			request->send(200, "text/plain", "Manual speed set to " + String(manual_speed_val) + "\n");
			sp("/SET_MANUAL_SPEED val="); spl(String(manual_speed_val).c_str());
		} else {
			request->send(400, "text/plain", "Missing val param. Use /set_manual_speed?val=0-254\n");
		}
	});
	server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(200, "text/plain", "Rebooting...\n");
		spl("/REBOOT");
		delay(200);
		ESP.restart();
	});
	server.onNotFound(notFound);
	server.begin();
}
