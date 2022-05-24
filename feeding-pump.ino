#include <Arduino.h>
#include "defs.h"
#include "wifi.h"
#include "ota.h"

/* struct pindef { */
/* 	char *s; // name */
/* 	int  i;  // gpio# */
/* }; */
/* struct pindef pindefs[] = { }; */
int pindefs_io[] = { 16, 17, 18, 19, 21, 22, 23, 32, 33 };
int pindefs_ionly[] = { 34, 35, 36, 39 };
const int pinpwm = MOTPWM_FWD_PIN;

#define IOLEN (sizeof(pindefs_io) / sizeof(*pindefs_io))
#define IONLYLEN (sizeof(pindefs_ionly) / sizeof(*pindefs_ionly))
#define sp(v) Serial.print(v)
#define spl(v) Serial.println(v)

bool pwmstate=false;

void setup() {
	delay(3000);
	Serial.begin(115200);
	setup_wifi();
	setup_ota();
	setup_butts();
}

void setup_butts() {
	pinMode(BTN_GO_PIN, INPUT_PULLUP);
	pinMode(BTN_REV_PIN, INPUT_PULLUP);
	pinMode(POT_RATE_PIN, INPUT_PULLUP);
	pinMode(POT_DELAY_PIN, INPUT_PULLUP);
	pinMode(POT_X_PIN, INPUT_PULLUP);
}

void loop_butts() {
	sp("BTN(Go:"); sp(digitalRead(BTN_GO_PIN)); sp(" ");
	sp("Rev:"); sp(digitalRead(BTN_REV_PIN)); sp(") ");
	sp("POT(Rate:"); sp(analogRead(POT_RATE_PIN)); sp(" ");
	sp("Delay:"); sp(analogRead(POT_DELAY_PIN)); sp(" ");
	sp("X:"); sp(analogRead(POT_X_PIN)); sp(")");
	spl("");
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
