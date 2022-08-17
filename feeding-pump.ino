#define _IN_FEEDING_PUMP_C // agree
#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"
#include "espweb.h"
#include <capsense.h>

cp_st *cp1;

void setup() {
	delay(2000);
	Serial.begin(115200);
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
	cp_set_cb_press(cp1, cap_cb_press);
	cp_set_cb_release(cp1, cap_cb_release);
}

void loop() {
	unsigned long now = millis();
	loop_wifi(now);
	loop_ota();
	loop_butts();
	loop_cap(cp1, now);
}
