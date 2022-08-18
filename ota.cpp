#include "wifi_config.h"
#undef DEBUG
#include "printutils.h"
#include <WiFi.h>
#include <mdns.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

void setup_ota(void) {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(MDNS_NAME);
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    dbsp(F("Start updating "));
    dbspl(type);
  });

  ArduinoOTA.onEnd([]() {
    dbspl(F("\nEnd"));
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    dbsp(F("Progress: "));
    dbsp(progress / (total / 100));
    dbspl('%');
  });

  ArduinoOTA.onError([](ota_error_t error) {
    dbsp(F("Error[")); dbsp(error); dbsp(F("]: "));
    if (error == OTA_AUTH_ERROR) {
      dbspl(F("Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      dbspl(F("Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      dbspl(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      dbspl(F("Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      dbspl(F("End Failed"));
    }
  });

  ArduinoOTA.begin();
  dbspl(F("OTA Ready. IP: "));
  dbspl(WiFi.localIP());
}

void loop_ota(void) {
	ArduinoOTA.handle();
}
