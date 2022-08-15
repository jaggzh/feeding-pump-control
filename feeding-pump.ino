#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"
#include "espweb.h"
#include <capsense.h>

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
	set_cb_press(cap_cb_press);
	set_cb_release(cap_cb_release);
	setup_cap();
}

void loop() {
	int gp;
	unsigned long now = millis();
	loop_wifi(now);
	loop_ota();
	loop_cap(now);
	loop_butts();
	loop_web();
}
