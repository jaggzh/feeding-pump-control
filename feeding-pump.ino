#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"
#include "web.h"

void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println("Booted.");
	setup_wifi();
	setup_ota();
	setup_butts();
	setup_web();
	set_cb_press(cap_cb_press);
	set_cb_release(cap_cb_release);
	setup_cap();
}

void loop() {
	/* int gp; */
	unsigned long msnow=millis();
	unsigned long usnow=micros();
	loop_wifi();
	loop_ota_ms(msnow);
	loop_butts_us(usnow);
	delay(5);
}
