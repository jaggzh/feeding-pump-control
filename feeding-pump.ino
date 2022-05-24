#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"
#include "btn.h"

const int pinpwm = MOTPWM_FWD_PIN;

bool pwmstate=false;

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
	if (pwmstate) {
		digitalWrite(pinpwm, HIGH);
		pwmstate = false;
	} else {
		digitalWrite(pinpwm, LOW);
		pwmstate = true;
		delay(100);
	}
	delay(100);
}
