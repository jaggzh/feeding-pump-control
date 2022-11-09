#define _IN_FEEDING_PUMP_C // agree
#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"
#include "espweb.h"
#include "serial.h"
#include <capsense.h>
#include <PrintHero.h>

cp_st *cp1;

void setup() {
	delay(2000);
	Serial.begin(115200);
	/* Serial.println(F("Hallooo!")); */
	/* Serial.println("Setting delayed wifi start for 5s from now"); */

	/* const int freq = 500; */
	/* const int ledChannel = 0; */
	/* const int resolution = 8; */
	/* ledcSetup(ledChannel, freq, resolution); */
	/* ledcAttachPin(SPEAKER_PIN, ledChannel); */
	/* delay(500); */
	/* ledcWrite(SPEAKER_PIN, 0); */
	/* ledcDetachPin(SPEAKER_PIN); */
	/* pinMode(SPEAKER_PIN, INPUT); */

	setup_wifi();
	setup_ota();
	setup_butts();
	setup_web();
	cp1 = capnew();
	/* Set up Cap sensor callbacks and thresholds and stuff.
	   The later are going to be device specific and I don't yet have an
	   auto-calibration method implemented. */
	cp_set_cb_press(cp1, cap_cb_press);
	cp_set_cb_release(cp1, cap_cb_release);
	/* capsense_debug_data_on(); */
	capsense_debug_data_off(); // in case it's default on
	/* capsense_debug_off(); */
	capsense_debug_on();
	cp_set_thresh_diff(cp1, .01f);
	cp_set_thresh_integ(cp1, .99f);
	cp_set_thresh_leak_closed(cp1, 0.999f);
	cp_set_thresh_leak_open(cp1, 0.93f);
	PIN_ESP_LED_SETUP();
}

void loop() {
	unsigned long now = millis();
	static unsigned long lasttime = millis();
	/* spl("loop()"); */
	loop_wifi(now);
	loop_ota();
	loop_butts();
	loop_cap_serial(cp1, now);
	loop_serial(now);
	loop_web();
	/* if (now  - lasttime > 1000) { ESP32_LED_OFF(); lasttime=now; } */
	/* else if (now  - lasttime > 500) { ESP32_LED_ON(); } */
}

