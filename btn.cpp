#define _IN_BTN_C
#define DEBUG
#undef DEBUG
#include "printutils.h"
#include <Arduino.h>
#include <InputDebounce.h>
#include <WiFi.h>
#include <capsense.h>
#include "main.h" // for feeding-pump.ino's stuff
#include "defs.h"
#include "btn.h"
#include "pump.h"

unsigned long last_status_ms = 0;
unsigned long last_pot_update = 0;
unsigned long cap_on_time = 0;

static InputDebounce btn_fwd;
static InputDebounce btn_rev;
static InputDebounce btn_usr;
int potrate_raw=0, potsens_raw=0;
int potrate=0;
float potsens=0;
#ifdef POT_X_PIN
	float potx=0;
#endif

/********************************************
 * Motor toggles
 * Warning: These do not test other channels!
 *   Our controller disables the motor if two channels are the same, but yours
 *   may not.  Make sure to, for instance, call { a_off(); b_on(); }
 */
void mot_fwd_set_on() {
	int newval = MAP_POT_RATE(potrate_raw);
	#if PUMP_DEBUG > 0
		sp("FWD ON (rate:"); sp(newval); spl(')');
	#endif
	ledcWrite(MOTPWM_FWD_CHAN, newval);
}
void mot_fwd_set_off() {
	#if PUMP_DEBUG > 0
		spl("FWD OFF");
	#endif
	ledcWrite(MOTPWM_FWD_CHAN, 0);
}
void mot_rev_set_on() {
	int newval = MAP_POT_RATE(potrate_raw);
	#if PUMP_DEBUG > 0
		sp("FWD ON (rate:"); sp(newval); spl(')');
	#endif
	ledcWrite(MOTPWM_REV_CHAN, newval);
}
void mot_rev_set_off() {
	#if PUMP_DEBUG > 0
		spl("REV OFF");
	#endif
	ledcWrite(MOTPWM_REV_CHAN, 0);
}

void update_pot_controls(int newrate, int newsens, unsigned long now) {
	// Pot RATE
	potrate += ((float)newrate - potrate) / POT_RATE_SMOOTH_DIV;
	if (abs(newrate - (int)potrate) > 1) {
		potrate = newrate;
		if (pumpstate == PUMP_FWD_PULSE ||
				pumpstate == PUMP_FWD_HOLD_START ||
				pumpstate == PUMP_FWD_HOLD)
			mot_fwd_set_on();
		else if (pumpstate == PUMP_REV_PULSE ||
				pumpstate == PUMP_REV_HOLD_START ||
				pumpstate == PUMP_REV_HOLD)
			mot_rev_set_on();
	}
	// Pot SENSITIVITY
	potsens += ((float)MAP_POT_SENS(newsens) - potsens) / POT_SENS_SMOOTH_DIV;
	cp_set_sensitivity(cp1, potsens);
}

/********************************************
 * Button handlers (pressed and released) */
void btn_fwd_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (pumpstate == PUMP_OFF) {
		#if PUMP_DEBUG > 0
			spl("PUMP FWD PULSE MODE");
		#endif
		mot_fwd_set_on();
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			#if PUMP_DEBUG > 0
				spl("PUMP FWD HELD UNTIL HOLD MODE");
			#endif
			pumpstate = PUMP_FWD_HOLD_START;
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		#if PUMP_DEBUG > 0
			spl("PUMP FWD TOGGLED OFF");
		#endif
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_HOLD) {
		#if PUMP_DEBUG > 0
			spl("PUMP REV CANCELLED");
		#endif
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_PULSE || pumpstate == PUMP_REV_HOLD_START) {
		// REV still held down
		#if PUMP_DEBUG > 0
			spl("PUMP REV PULSE MODE LOCKED INTO HOLD (Ignored. Wont lock reverse)");
		#endif
		//pumpstate = PUMP_REV_HOLD; // lock REV on
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF; // lock REV on
	}
}
void btn_fwd_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	#if PUMP_DEBUG > 0
		sp("BTN FWD UP for "); sp(dur); spl("ms");
	#endif
	if (pumpstate == PUMP_FWD_PULSE) {
		pumpstate = PUMP_OFF;
		mot_fwd_set_off();
	} else if (pumpstate == PUMP_FWD_HOLD_START)
		pumpstate = PUMP_FWD_HOLD;
	else if (pumpstate == PUMP_TURNING_OFF)
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
}


void btn_rev_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (pumpstate == PUMP_OFF) {
		#if PUMP_DEBUG > 0
			spl("PUMP REV PULSE MODE");
		#endif
		mot_rev_set_on();
		pumpstate = PUMP_REV_PULSE;
	} else if (pumpstate == PUMP_REV_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			#if PUMP_DEBUG > 0
				spl("PUMP REV HELD UNTIL HOLD MODE");
				spl(" (Refusing. We don't hold reverse.)");
			#endif
			//pumpstate = PUMP_REV_HOLD_START;
		}
	} else if (pumpstate == PUMP_REV_HOLD) {
		#if PUMP_DEBUG > 0
			spl("PUMP REV TOGGLED OFF");
		#endif
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_HOLD) {
		#if PUMP_DEBUG > 0
			spl("PUMP FWD CANCELLED");
		#endif
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_PULSE || pumpstate == PUMP_FWD_HOLD_START) {
		// FWD still held down
		#if PUMP_DEBUG > 0
			spl("PUMP FWD PULSE MODE LOCKED INTO HOLD");
		#endif
		pumpstate = PUMP_FWD_HOLD_START; // lock REV on
	}
}

void btn_rev_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	#if PUMP_DEBUG > 0
		sp("BTN REV UP for "); sp(dur); spl("ms");
	#endif
	if (pumpstate == PUMP_REV_PULSE) {
		pumpstate = PUMP_OFF;
		mot_rev_set_off();
	} else if (pumpstate == PUMP_REV_HOLD_START)
		pumpstate = PUMP_REV_HOLD;
	else if (pumpstate == PUMP_TURNING_OFF)
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
}

void btn_usr_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (pumpstate == PUMP_OFF) {
		#if PUMP_DEBUG > 0
			spl("(*USER*) PUMP FWD PULSE MODE");
		#endif
		mot_fwd_set_on();
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			#if PUMP_DEBUG > 0
				spl("(*USER*) PUMP FWD HELD UNTIL HOLD MODE");
			#endif
			pumpstate = PUMP_FWD_HOLD_START;
		}
	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		if (dur >= PUMP_TOO_LONG_PRESS_MS) {
			#if PUMP_DEBUG > 0
				spl("(*USER*) PUMP FWD HELD TOO LONG. SAFETY SHUTOFF");
			#endif
			mot_fwd_set_off();
			pumpstate = PUMP_OFF_SAFETY_MODE;
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		#if PUMP_DEBUG > 0
			spl("(*USER*) PUMP FWD TOGGLED OFF");
		#endif
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_HOLD) {
		#if PUMP_DEBUG > 0
			spl("(*USER*) PUMP FWD CANCELLED");
		#endif
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_PULSE || pumpstate == PUMP_FWD_HOLD_START) {
		// FWD still held down
		#if PUMP_DEBUG > 0
			spl("(*USER*) PUMP FWD PULSE MODE LOCKED INTO HOLD");
		#endif
		pumpstate = PUMP_FWD_HOLD_START; // lock FWD on
	}
}

void btn_usr_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	#if PUMP_DEBUG > 0
		sp("(*USER*) BTN FWD UP for "); sp(dur); spl("ms");
	#endif
	if (pumpstate == PUMP_FWD_PULSE) {
		pumpstate = PUMP_OFF;
		mot_fwd_set_off();
	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		pumpstate = PUMP_FWD_HOLD;
	} else if (pumpstate == PUMP_TURNING_OFF) {
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
	} else if (pumpstate == PUMP_OFF_SAFETY_MODE) {
		/* This is when a button (USR currently) is held down too long.
		 * For safety we consider this someone accidentally holding it, or it
		 * being pressed and unable to be released, or a dysfunction in a button
		 * could cause a short.  Thus, for safety, we will turn the motor off.
		 * ** WARNING ** This is only implemented for the USR button, not the normal
		 * FWD/REV buttons right now. */
		mot_fwd_set_off(); // making sure it's off. It should be already though.
		pumpstate = PUMP_OFF;
	}
}

/* void set_state(enum pumpstate newstate) { */
/* 	if (pumpstate != PUMP_F */
/* 	pumpstate = newst */
/* } */

void cap_cb_press(cp_st *cp) {
	dbspl(F("Cap sensor pressed"));
	/* pumpstate = PUMP_FWD_PULSE; */
	cap_on_time = millis();
	btn_usr_cb_pressed_dur(0, 0);
	/* mot_fwd_set_on(); */
}
void cap_cb_release(cp_st *cp) {
	cap_on_time = 0;
	dbspl(F("Cap sensor RELEASED"));
	btn_usr_cb_released_dur(0, 0);
	/* mot_fwd_set_off(); */
	/* pumpstate = PUMP_OFF; */
}
void evaluate_cap_state(unsigned long now) {
	// Evaluate capacitive sensor with respect to pump state:
	if (cap_on_time) {
		unsigned long int dur = now - cap_on_time;
		btn_usr_cb_pressed_dur(0, dur);
	}
}

void setup_butts() {
	pinMode(POT_RATE_PIN, INPUT);
	pinMode(POT_SENSI_PIN, INPUT);
	#ifdef POT_X_PIN
		pinMode(POT_X_PIN, INPUT);
	#endif
	/* analogSetAttenuation(ADC_ATTEN_DB_11); */
	potrate_raw = (float)analogRead(POT_RATE_PIN);
	potsens_raw = (float)analogRead(POT_SENSI_PIN);
	#ifdef POT_X_PIN
		potx = (float)analogRead(POT_X_PIN);
	#endif

	/* Motor pin output tests: */
	/* pinMode(MOTPWM_FWD_PIN, OUTPUT); */
	/* pinMode(MOTPWM_REV_PIN, OUTPUT); */
	/* digitalWrite(MOTPWM_FWD_PIN, HIGH); */
	/* digitalWrite(MOTPWM_REV_PIN, HIGH); */

	btn_fwd.registerCallbacks(NULL, NULL, btn_fwd_cb_pressed_dur, btn_fwd_cb_released_dur);
	btn_rev.registerCallbacks(NULL, NULL, btn_rev_cb_pressed_dur, btn_rev_cb_released_dur);
	btn_usr.registerCallbacks(NULL, NULL, btn_usr_cb_pressed_dur, btn_usr_cb_released_dur);
	pinMode(BTN_FWD_PIN, INPUT_PULLUP);
	pinMode(BTN_REV_PIN, INPUT_PULLUP);
	pinMode(BTN_USR_PIN, INPUT_PULLUP);
	btn_fwd.setup(BTN_FWD_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_rev.setup(BTN_REV_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_usr.setup(BTN_USR_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);

	ledcSetup(MOTPWM_FWD_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_FWD_PIN, MOTPWM_FWD_CHAN);
	//ledcWrite(MOTPWM_FWD_CHAN, 0);

	ledcSetup(MOTPWM_REV_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_REV_PIN, MOTPWM_REV_CHAN);
	//ledcWrite(MOTPWM_REV_CHAN, MOTPWM_MAX_DUTY_CYCLE);
}

void loop_butts() {
	unsigned long now = millis();
	int motfwd_duty;
	int motrev_duty;

	btn_fwd.process(now);
	btn_rev.process(now);
	btn_usr.process(now);

	if (now - last_pot_update > DELAY_MS_POT_UPDATE) {
		last_pot_update = now;
		potsens_raw = analogRead(POT_SENSI_PIN);
		potrate_raw = analogRead(POT_RATE_PIN);
		#ifdef POT_X_PIN
			potx = analogRead(POT_X_PIN);
		#endif
		update_pot_controls(potrate_raw, potsens_raw, now);
		evaluate_cap_state(now);
	}
	if (now - last_status_ms > BTN_STATUS_DISPLAY_MS) {
		last_status_ms = now;
		motfwd_duty = ledcRead(MOTPWM_FWD_CHAN);
		motrev_duty = ledcRead(MOTPWM_REV_CHAN);

		#if PUMP_DEBUG > 0
			sp("[PUMP STATE:"); sp(pumpstatestr[pumpstate]); sp("] ");
			sp("BTN(Go:"); sp(btn_fwd.isPressed() ? '1' : '0'); sp(", ");
			sp("Rev:"); sp(btn_rev.isPressed() ? '1' : '0'); sp(", ");
			sp("Usr:"); sp(btn_usr.isPressed() ? '1' : '0'); sp(") ");
			sp("POT(Rate:"); sp(potrate_raw); sp("["); sp(potrate); sp("] ");
			sp("*Sensi:"); sp(potsens_raw); sp("["); sp(potsens); sp("], ");
			#ifdef POT_X_PIN
				sp("X:"); sp(potx);
			#endif
			sp(") Duty(Fwd:"); sp(motfwd_duty);
			sp(" Rev:"); sp(motrev_duty); sp(")");
			sp(" WiFi:"); sp(WiFi.status() == WL_CONNECTED);
			spl("");
		#endif
	}
}

