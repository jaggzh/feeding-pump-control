#ifndef __WIFI_CONFIG_H
#define __WIFI_CONFIG_H

//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include "defs.h"

#define MDNS_NAME "LanDevice"
#define SSID_NAME "YourSSID"
#define SSID_PW   "YourPASSWORD"

#ifdef WIFI_CONFIG_GET_IPS
	IPAddress ip(192, 168, 0, 10);
	IPAddress gw(192, 168, 0, 1);
	IPAddress nm(255, 255, 255, 0);
#endif

// Alarm host (for hold and toolong events)
#define HOST_ALARM "192.168.0.20"
#define PORT_ALARM_HOLD 7
#define PORT_ALARM_TOOLONG 8

// HID host (for general events like pat-press, potx, etc.)
#define HOST_HID "192.168.0.20"
#define PORT_HID 10101

// The below might not be implemented
#define WEBUPDATE_USER "webuser"
#define WEBUPDATE_PW   "webpw"

#define MAX_MS_BEFORE_RECONNECT 10000

#ifdef __WIFI_CPP
const char *ssid = SSID_NAME;
//#define SSPW {33+22, 3, 62+22, 4+2, 6+129, 0}
char password[] = SSID_PW;

const char *update_user = WEBUPDATE_USER; // HTTP auth user for OTA http update
const char *update_pw = WEBUPDATE_PW;  // HTTP auth password

#else // like: ifndef __MAIN_INO__
extern const char *ssid;
extern char password[];
extern const char *update_user; // HTTP auth user for OTA http update
extern const char *update_pw;  // HTTP auth password
#endif

#endif
