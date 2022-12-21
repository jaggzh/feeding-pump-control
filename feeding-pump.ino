#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"

void setup() {
	delay(3000);
	Serial.begin(115200);
	setup_wifi();
	setup_ota();
	setup_butts();
}

void loop() {
	int gp;
	unsigned long msnow=millis();
	unsigned long usnow=micros();
	loop_wifi();
	loop_ota_ms(msnow);
	loop_butts_us(usnow);
	delay(50);
}
