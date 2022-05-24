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
	loop_wifi();
	loop_ota();
	loop_butts();
	delay(50);
}
