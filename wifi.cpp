#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define WIFI_CONFIG_GET_IPS
#define __WIFI_CPP
#include "wifi_config.h"
#define _IN_WIFI_CPP
#include "wifi.h"
#include "printutils.h"
#include "espweb.h"

uint16_t wifi_connflags = 0;

/* WiFiEventHandler wifiConnectHandler; */
/* WiFiEventHandler wifiDisconnectHandler; */
/* WiFiEventHandler wifiGotIPHandler; */
unsigned long int wifi_start_delay_future=0;
unsigned long int wifi_last_connect_test=0;
#define WIFI_CONNECT_TEST_PERIOD 10000
bool last_wifi_was_connected = false;

void loop_wifi(unsigned long int now) {
	if (wifi_start_delay_future) {
		if (wifi_start_delay_future <= now) {
			wifi_start_delay_future = 0;
			Serial.println("Delayed WiFi.begin is happening");
			setup_wifi();
		}
	}
	if ((now - wifi_last_connect_test >= WIFI_CONNECT_TEST_PERIOD)) {
		if ((WiFi.status() != WL_CONNECTED)) {
			spl("Reconnecting to WiFi...");
			last_wifi_was_connected = false;
			WiFi.disconnect();
			WiFi.reconnect();
			wifi_last_connect_test = now;
		} else {
			if (!last_wifi_was_connected) {
				spl("WiFi connection successful");
				last_wifi_was_connected = true;
			}
		}
	}
	/* long rssi = WiFi.RSSI(); */
	/* unsigned long cur_millis = millis(); */
	/* static unsigned long last_wifi_strength = cur_millis; */
	/* if (cur_millis - last_wifi_strength > 500) { */
	/* 	last_wifi_strength = cur_millis; */
	/* 	Serial.print("WiFi strength: "); */
	/* 	Serial.println(rssi); */
	/* } */
}

void setup_wifi(int delay) {
	if (delay) wifi_start_delay_future = millis() + delay;
}

void setup_wifi(void) {
	WiFi.mode(WIFI_STA);
	WiFi.config(ip, gw, nm);
	/* WiFi.setOutputPower(20.5); // 0 - 20.5 (multiples of .25) */
	spl(F("Connecting to wife (WiFi.begin())..."));
	/* WiFi.onEvent(onWifiConnect, WIFI_EVENT_STA_CONNECTED); */
	/* WiFi.onEvent(onWifiDisconnect, WIFI_EVENT_STA_DISCONNECTED); */
	/* WiFi.onEvent(onWifiGotIP, IP_EVENT_STA_GOT_IP); */

	WiFi.setAutoReconnect(true);
	WiFi.persistent(true);       // reconnect to prior access point
	WiFi.setTxPower(WIFI_POWER_19_5dBm); // 19.5 max
	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi ..");
	#define WIFI_CONNECT_ATTEMPTS 5
	int tries=0;
	for (; tries<WIFI_CONNECT_ATTEMPTS; tries++) {
		if (WiFi.status() == WL_CONNECTED) break;
		Serial.print('.');
		delay(1000);
	}
	if (tries >= WIFI_CONNECT_ATTEMPTS) {
		Serial.println("Couldn't connect yet. Moving on.");
	} else {
		last_wifi_was_connected = true;
	}
	Serial.println(WiFi.localIP());
	/* while (WiFi.waitForConnectResult() != WL_CONNECTED) { */
	/* 	//spl(F("Conn. fail! Rebooting...")); */
	/* 	delay(500); */
	/* 	spl(F("Conn. fail!")); */
	/* 	//ESP.restart(); */
	/* } */
	WiFi.setAutoReconnect(true);
	WiFi.persistent(true);       // reconnect to prior access point
	/* delay(500); */
	/* werr = WiFi.esp_wifi_set_max_tx_power(78); */
}

/* void onWifiGotIP(const WiFiEventStationModeGotIP& event) { */
/* 	Serial.println(F("EVENT: IP established sucessfully.")); */
/* 	Serial.print(F("IP address: ")); */
/* 	Serial.println(WiFi.localIP()); */
/* 	wifi_connflags = WIFI_FLAG_CONNECTED | WIFI_FLAG_IP; */
/* } */


/* void onWifiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) */
/* 	Serial.println(F("EVENT: Disconnected from Wi-Fi. Auto-reconnect should happen.")); */
/* 	/1* WiFi.disconnect(); *1/ */
/* 	/1* WiFi.begin(ssid, password); *1/ */
/* 	wifi_connflags = 0; */
/* } */

/* void onWifiConnect(WiFiEvent_t event, WiFiEventInfo_t info) */
/* 	long rssi = WiFi.RSSI(); */
/* 	Serial.print(F("EVENT: Connected to Wi-Fi sucessfully. Strength: ")); */
/* 	Serial.println(rssi); */
/* } */

// Optional call to use if trying to requiring wifi during setup()
// Wait max of passed seconds for wifi
// Returns flags immediately upon success (eg. WIFI_FLAG_CONNECTED)
// Return flags of 0 means NOT connected for timeout period
uint16_t setup_wait_wifi(unsigned int timeout_s) {
	unsigned long mil = millis();
	bool ret;
	while (((millis() - mil)/1000) < timeout_s) {
		ret = loop_check_wifi(); // after 3s this fn will start printing to serial
		if (ret) return ret;
		delay(99);
	}
	return 0;
}

uint16_t loop_check_wifi() {
	static int connected=false;
	unsigned long cur_millis = millis();
	static unsigned long last_wifi_millis = cur_millis;
	/* static unsigned long last_connect_millis = 0; */
	static unsigned long last_reconnect_millis = 0;
	if (cur_millis - last_wifi_millis < 2000) {
		return WIFI_FLAG_IGNORE;
	} else {
		last_wifi_millis = cur_millis;
		if (WiFi.status() == WL_CONNECTED) {
			if (!connected) { // only if we toggled state
				connected = true;
				/* last_connect_millis = cur_millis; */
				sp(F("Just connected to "));
				sp(ssid);
				sp(F(". IP: "));
				spl(WiFi.localIP());
				WiFi.setAutoReconnect(true);
				WiFi.persistent(true);       // reconnect to prior access point
				return (WIFI_FLAG_CONNECTED | WIFI_FLAG_RECONNECTED);
			} else {
				return WIFI_FLAG_CONNECTED;
			}
		} else {
			if (!connected) {
				#ifndef PLOT_TO_SERIAL
					sp(F("Not connected to wifi. millis="));
					sp(cur_millis);
					sp(F(", cur-last="));
					spl(cur_millis - last_wifi_millis);
				#endif
				if (cur_millis - last_reconnect_millis > MAX_MS_BEFORE_RECONNECT) {
					#ifndef PLOT_TO_SERIAL
						spl(F("  Not connected to WiFi. Reconnecting (disabled)"));
					#endif
					last_reconnect_millis = cur_millis;
					// WiFi.reconnect();
				}
			} else { // only if we toggled state
				connected=false;
				spl(F("Lost WiFi connection. Will try again."));
			}
		}
	}
	return 0; // not connected
}

