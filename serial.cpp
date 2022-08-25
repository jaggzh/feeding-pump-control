#include <Arduino.h>
#include "main.h"

#define SERBUFLEN 10
unsigned long last_serial_check=0;

char *get_serial_str() {
	static char buf[SERBUFLEN+1];
	int i=0;
	
	unsigned long start=millis();
	while (millis()-start < 50) {
		if (Serial.available()) {
			int c = Serial.read();
			if (c == '\n' || c == '\r') {
				buf[i]=0;
				return buf;
			} else {
				buf[i] = c;
				i++;
				if (i >= SERBUFLEN) {
					buf[i] = 0;
					return buf;
				}
			}
		}
	}
	// did not receive full line in time
	buf[0]=0;
	return buf;
}

void loop_serial(unsigned long now) {
	if (now - last_serial_check > 10) {
		last_serial_check = now;
		if (Serial.available()) {
			char *s = get_serial_str();
			float f=0;
			if (*s) {
				if (s[1]) f = strtof(s+1, NULL);
				if (*s == 'h') {
					Serial.println("(d#)diffthr (i#)integ (l#)ileak (n#)ileakno");
				} else if (*s == 'd') {
					cp1->thresh_diff = f;
					Serial.print(F("Set diff threshold: "));
					Serial.println(f);
				} else if (*s == 'i') {
					cp1->thresh_integ = f;
					Serial.print(F("Set integ threshold: "));
					Serial.println(f);
				} else if (*s == 'l') {
					cp1->leak_integ = f;
					Serial.print(F("Set integ leak: "));
					Serial.println(f);
				} else if (*s == 'n') {
					cp1->leak_integ_no = f;
					Serial.print(F("Set integ leak no: "));
					Serial.println(f);
				}
			}
		}
	}
}

