#define _IN_BTN_C
#include <Arduino.h>
#include <InputDebounce.h>
#include "defs.h"
#include "btn.h"
#include "printutils.h"
#include "pump.h"

unsigned long last_status_ms = 0;
static InputDebounce btn_fwd;
static InputDebounce btn_rev;
bool mot_fwd = false;
bool mot_rev = false;
float potrate=0;

/********************************************
 * Motor toggles
 * Warning: These do not test other channels!
 *   Our controller disables the motor if two channels are the same, but yours
 *   may not.  Make sure to, for instance, call { a_off(); b_on(); }
 */
void mot_fwd_set_on() {
	int newval;
	if (!mot_fwd) {
		mot_fwd = true;
		int newval = map((int)potrate, 0, 4095, 0, MOTPWM_MAX_DUTY_CYCLE);
		ledcWrite(MOTPWM_FWD_CHAN, newval);
		sp("FWD ON (rate:");
		sp(newval);
		sp(')');
	}
}
void mot_fwd_set_off() {
	if (mot_fwd) {
		mot_fwd = false;
		ledcWrite(MOTPWM_FWD_CHAN, 0);
		spl("FWD OFF");
	}
}
void mot_rev_set_on() {
	if (!mot_rev) {
		mot_rev = true;
		ledcWrite(MOTPWM_REV_CHAN, MOTPWM_MAX_DUTY_CYCLE);
		spl("REV ON");
	}
}
void mot_rev_set_off() {
	if (mot_rev) {
		mot_rev = false;
		ledcWrite(MOTPWM_REV_CHAN, 0);
		spl("REV OFF");
	}
}

void update_pump_rate(int new_value) {
	potrate += (potrate-avg_rate) / POT_SMOOTH_DIV;
	if (abs(new_value - (int)potrate) > 1) {
		potrate = new_value;
		if (pumpstate == PUMP_FWD_PULSE || pumpstate == PUMP_FWD_HOLD_START)
			mot_fwd_set_on();
		else if (pumpstate == PUMP_REV_PULSE || pumpstate == PUMP_REV_HOLD_START)
			mot_rev_set_on();
	}
}

/********************************************
 * Button handlers (pressed and released) */
void btn_fwd_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN FWD DOWN for "); sp(dur); spl("ms");
	if (pumpstate == PUMP_OFF) {
		mot_fwd_set_on();
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS)
			pumpstate = PUMP_FWD_HOLD_START;
	} else if (pumpstate == PUMP_FWD_HOLD) {
		mot_fwd_set_off();
		pumpstate = PUMP_OFF;
	} else if (pumpstate == PUMP_REV_HOLD) {
		mot_rev_set_off();
		pumpstate = PUMP_OFF;
	}
}
void btn_fwd_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN FWD UP for "); sp(dur); spl("ms");
	if (pumpstate = PUMP_FWD_HOLD_START)
		pumpstate = PUMP_FWD_HOLD;
}


void btn_rev_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN REV DOWN for "); sp(dur); spl("ms");
}

void btn_rev_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN REV UP for "); sp(dur); spl("ms");
}


void setup_butts() {
	pinMode(POT_RATE_PIN, INPUT_PULLUP);
	pinMode(POT_DELAY_PIN, INPUT_PULLUP);
	pinMode(POT_X_PIN, INPUT_PULLUP);

	/* Motor pin output tests: */
	/* pinMode(MOTPWM_FWD_PIN, OUTPUT); */
	/* pinMode(MOTPWM_REV_PIN, OUTPUT); */
	/* digitalWrite(MOTPWM_FWD_PIN, HIGH); */
	/* digitalWrite(MOTPWM_REV_PIN, HIGH); */

	btn_fwd.registerCallbacks(NULL, NULL, btn_fwd_cb_pressed_dur, btn_fwd_cb_released_dur);
	btn_rev.registerCallbacks(NULL, NULL, btn_rev_cb_pressed_dur, btn_rev_cb_released_dur);
	btn_fwd.setup(BTN_FWD_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_rev.setup(BTN_REV_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);

	ledcSetup(MOTPWM_FWD_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_FWD_PIN, MOTPWM_FWD_CHAN);
	//ledcWrite(MOTPWM_FWD_CHAN, 0);

	ledcSetup(MOTPWM_REV_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_REV_PIN, MOTPWM_REV_CHAN);
	//ledcWrite(MOTPWM_REV_CHAN, MOTPWM_MAX_DUTY_CYCLE);
}

void loop_butts() {
	unsigned long now = millis();
	int new_potrate, potdelay, potx;
	int motfwd_duty;
	int motrev_duty;

	btn_fwd.process(now);
	btn_rev.process(now);

	if (now - last_status_ms > BTN_STATUS_DISPLAY_MS) {
		last_status_ms = now;
		new_potrate = analogRead(POT_RATE_PIN);

		potdelay = analogRead(POT_DELAY_PIN);
		potx = analogRead(POT_X_PIN);
		motfwd_duty = ledcRead(MOTPWM_FWD_CHAN);
		motrev_duty = ledcRead(MOTPWM_REV_CHAN);
		sp("BTN(Go:"); sp(btn_fwd.isPressed() ? "DOWN" : "UP"); sp(" ");
		sp("Rev:"); sp(btn_rev.isPressed() ? "DOWN" : "UP"); sp(") ");
		sp("POT(Rate:"); sp(new_potrate); sp(" ");
		sp("Delay:"); sp(potdelay); sp(" ");
		sp("X:"); sp(potx); sp(")");
		sp(" Duty(Fwd:"); sp(motfwd_duty);
		sp(" Rev:"); sp(motrev_duty); sp(")");
		spl("");
		update_pump_rate(new_potrate);
	}
}

