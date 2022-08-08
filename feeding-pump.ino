#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"
#include "espweb.h"

void setup() {
	delay(3000);
	Serial.begin(115200);
	setup_wifi();
	setup_ota();
	setup_butts();
	setup_web();
}

void loop() {
	int gp;
	loop_wifi();
	loop_ota();
	loop_butts();
	loop_web();
}
