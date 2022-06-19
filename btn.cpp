#define _IN_BTN_C
#include <Arduino.h>
#include <InputDebounce.h>
#include "defs.h"
#include "btn.h"
#include "printutils.h"
#include "pump.h"
#define MAGICCHUNK_DEBUG
#include "MagicSerialDechunk.h"
#include <PrintHero.h>

/* If the PATIENT button was used for this, let's keep track of that
 * We don't use a whole state for this, just this flag (see pumpstate
 * for the normal and more-elaborate state tracking) */
bool triggered_by_patient=false;

unsigned long mot_fwd_on_ms = 0;   // Tracking how long motor on (for safety limit)
unsigned long last_status_ms = 0;  // Reduce serial output
unsigned long last_pot_update = 0; // Reduce pot tests
unsigned long last_safety_ms = 0;  // Reduce frequency of safety tests (I know right)
static InputDebounce btn_fwd;
static InputDebounce btn_rev;
static InputDebounce btn_patient;
float potrate=0, potdelay=0, potx=0;

struct SerialDechunk dechunk_real;
struct SerialDechunk *dechunk = &dechunk_real;

/********************************************
 * Motor toggles
 * Warning: These do not test other channels!
 *   Our controller disables the motor if two channels are the same, but yours
 *   may not.  Make sure to, for instance, call { a_off(); b_on(); }
 */
void mot_fwd_set_on() {
	mot_fwd_on_ms = millis();
	int newval = MAP_POT_VAL(potrate);
	sp("FWD ON (rate:"); sp(newval); spl(')');
	ledcWrite(MOTPWM_FWD_CHAN, newval);
}
void mot_fwd_set_off() {
	spl("FWD OFF");
	ledcWrite(MOTPWM_FWD_CHAN, 0);
}
void mot_rev_set_on() {
	int newval = MAP_POT_VAL(potrate);
	sp("REV ON (rate:"); sp(newval); spl(')');
	ledcWrite(MOTPWM_REV_CHAN, newval);
}
void mot_rev_set_off() {
	spl("REV OFF");
	ledcWrite(MOTPWM_REV_CHAN, 0);
}

void update_pump_rate(unsigned long now,
		int new_potrate, int new_potdelay, int new_potx) {
	if (now - last_pot_update > DELAY_MS_POT_UPDATE) {
		last_pot_update = now;
		potrate += ((float)new_potrate - potrate) / POT_SMOOTH_DIV;
		if (abs(new_potrate - (int)potrate) > 2) {
			potrate = new_potrate;
			if (pumpstate == PUMP_FWD_PULSE ||
					pumpstate == PUMP_FWD_HOLD_START ||
					pumpstate == PUMP_FWD_HOLD)
				mot_fwd_set_on();
			else if (pumpstate == PUMP_REV_PULSE ||
					pumpstate == PUMP_REV_HOLD_START ||
					pumpstate == PUMP_REV_HOLD)
				mot_rev_set_on();
		}
	}
}

/********************************************
 * Button handlers (pressed and released) */
void btn_fwd_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (pumpstate == PUMP_OFF) {
		spl("PUMP FWD PULSE MODE");
		triggered_by_patient = false;
		mot_fwd_set_on();
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("PUMP FWD HELD UNTIL HOLD MODE");
			pumpstate = PUMP_FWD_HOLD_START;
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("PUMP FWD TOGGLED OFF");
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("PUMP REV CANCELLED");
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_PULSE || pumpstate == PUMP_REV_HOLD_START) {
		// REV still held down
		spl("PUMP REV PULSE MODE LOCKED INTO HOLD (Ignored. Wont lock reverse)");
		//pumpstate = PUMP_REV_HOLD; // lock REV on
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF; // lock REV on
	}
}
void btn_fwd_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN FWD UP for "); sp(dur); spl("ms");
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
		spl("PUMP REV PULSE MODE");
		triggered_by_patient = false;
		mot_rev_set_on();
		pumpstate = PUMP_REV_PULSE;
	} else if (pumpstate == PUMP_REV_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("PUMP REV HELD UNTIL HOLD MODE");
			spl(" (Refusing. We don't hold reverse.)");
			//pumpstate = PUMP_REV_HOLD_START;
		}
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("PUMP REV TOGGLED OFF");
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("PUMP FWD CANCELLED");
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		// FWD still held down
		spl("PUMP FWD PULSE MODE LOCKED INTO HOLD");
		pumpstate = PUMP_FWD_HOLD_START; // lock REV on
	}
}

void btn_rev_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("BTN REV UP for "); sp(dur); spl("ms");
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

void btn_patient_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (pumpstate == PUMP_OFF) {
		spl("(*USER*) PUMP FWD PULSE MODE");
		triggered_by_patient = true;
		mot_fwd_set_on();
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("(*USER*) PUMP FWD HELD UNTIL HOLD MODE");
			pumpstate = PUMP_FWD_HOLD_START;
		}
	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		if (dur >= PUMP_TOO_LONG_PRESS_MS) {
			spl("(*USER*) PUMP FWD HELD TOO LONG. SAFETY SHUTOFF");
			mot_fwd_set_off();
			pumpstate = PUMP_OFF_SAFETY_MODE;
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("(*USER*) PUMP FWD TOGGLED OFF");
		mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("(*USER*) PUMP FWD CANCELLED");
		mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	}
}

void btn_patient_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("(*USER*) BTN FWD UP for "); sp(dur); spl("ms");
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
		/* This is when a button (PATIENT currently) is held down too long.
		 * For safety we consider this someone accidentally holding it, or it
		 * being pressed and unable to be released, or a dysfunction in a button
		 * could cause a short.  Thus, for safety, we will turn the motor off.
		 * ** WARNING ** This is only implemented for the PATIENT button, not the normal
		 * FWD/REV buttons right now. */
		mot_fwd_set_off(); // making sure it's off. It should be already though.
		pumpstate = PUMP_OFF;
	}
}

void safety_tests(unsigned long now) {
	if (pumpstate == PUMP_FWD_HOLD) {
		if (now - last_safety_ms > SAFETY_TEST_DELAY_MS) {
			last_safety_ms = now;
			if (      (triggered_by_patient  && (now-mot_fwd_on_ms > PUMP_PATIENT_TOO_LONG_RUNNING_MS))) {
				spl("PUMP (PATIENT MODE) RUNNING TOO LONG, TURNING OFF.");
				mot_fwd_set_off();
				pumpstate = PUMP_OFF;
			} else if (!triggered_by_patient && (now-mot_fwd_on_ms > PUMP_ADMIN_TOO_LONG_RUNNING_MS)) {
				spl("PUMP (ADMIN MODE) RUNNING TOO LONG, TURNING OFF.");
				mot_fwd_set_off();
				pumpstate = PUMP_OFF;
			}
		}
	}
}

void serial_dechunk_cb(struct SerialDechunk *sp) {
	DSP(" {");
	for (int i=0; i<sp->chunksize; i++) {
		/* printf("%d ", sp->b[i]); */
		DSP(sp->b[i]);
		DSP(',');
	}
	DSPL("} ");
}

void setup_butts() {
	pinMode(POT_RATE_PIN, INPUT_PULLUP);
	pinMode(POT_DELAY_PIN, INPUT_PULLUP);
	pinMode(POT_X_PIN, INPUT_PULLUP);
	potrate = (float)analogRead(POT_RATE_PIN);
	potdelay = (float)analogRead(POT_DELAY_PIN);
	potx = (float)analogRead(POT_X_PIN);

	/* Motor pin output tests: */
	/* pinMode(MOTPWM_FWD_PIN, OUTPUT); */
	/* pinMode(MOTPWM_REV_PIN, OUTPUT); */
	/* digitalWrite(MOTPWM_FWD_PIN, HIGH); */
	/* digitalWrite(MOTPWM_REV_PIN, HIGH); */

	btn_fwd.registerCallbacks(NULL, NULL, btn_fwd_cb_pressed_dur, btn_fwd_cb_released_dur);
	btn_rev.registerCallbacks(NULL, NULL, btn_rev_cb_pressed_dur, btn_rev_cb_released_dur);
	btn_patient.registerCallbacks(NULL, NULL, btn_patient_cb_pressed_dur, btn_patient_cb_released_dur);
	btn_fwd.setup(BTN_FWD_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_rev.setup(BTN_REV_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_patient.setup(BTN_PATIENT_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);

	ledcSetup(MOTPWM_FWD_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_FWD_PIN, MOTPWM_FWD_CHAN);
	//ledcWrite(MOTPWM_FWD_CHAN, 0);

	ledcSetup(MOTPWM_REV_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	ledcAttachPin(MOTPWM_REV_PIN, MOTPWM_REV_CHAN);
	//ledcWrite(MOTPWM_REV_CHAN, MOTPWM_MAX_DUTY_CYCLE);

	Serial2.begin(9600, SERIAL_8N1, PAT_SERIAL_RX_PIN, PAT_SERIAL_TX_PIN);
	serial_dechunk_init(dechunk, PAT_SERIAL_DATA_CHUNKSIZE, serial_dechunk_cb);
}

void loop_butts_serial(unsigned long now) {
	static int wrap=0;
	static unsigned long last_ser_check=now;
	if (now - last_ser_check > 0) {
		last_ser_check = now;
		if (Serial2.available()) {
			uint8_t c = Serial2.read();
			DSP(c);
			++wrap;
			if (wrap == 8) DSP("  ");
			else if (wrap >= 16) { wrap=0; DSP('\n'); }
			else DSP(' ');
			dechunk->add(dechunk, c);
		} else {
			DSP("No ser.available()\n");
		}
	}
}

void loop_butts() {
	unsigned long now = millis();
	int new_potrate, new_potdelay, new_potx;
	int motfwd_duty;
	int motrev_duty;

	btn_fwd.process(now);
	btn_rev.process(now);
	btn_patient.process(now);

	loop_butts_serial(now);

	if (now - last_status_ms > BTN_STATUS_DISPLAY_MS) {
		last_status_ms = now;
		new_potrate = analogRead(POT_RATE_PIN);
		new_potdelay = analogRead(POT_DELAY_PIN);
		potx = analogRead(POT_X_PIN);
		motfwd_duty = ledcRead(MOTPWM_FWD_CHAN);
		motrev_duty = ledcRead(MOTPWM_REV_CHAN);
		sp("[PUMP STATE:"); sp(pumpstatestr[pumpstate]); sp("] ");
		sp("BTN(Go:"); sp(btn_fwd.isPressed() ? '1' : '0'); sp(", ");
		sp("Rev:"); sp(btn_rev.isPressed() ? '1' : '0'); sp(", ");
		sp("Usr:"); sp(btn_patient.isPressed() ? '1' : '0'); sp(") ");
		sp("POT(Rate:"); sp(new_potrate); sp("["); sp(potrate); sp("] ");
		sp("Delay:"); sp(new_potdelay); sp(" "); sp(potdelay); sp("] ");
		sp("X:"); sp(new_potx); sp(")"); sp(potx); sp("] ");
		sp(" Duty(Fwd:"); sp(motfwd_duty);
		sp(" Rev:"); sp(motrev_duty); sp(")");
		spl("");
		update_pump_rate(now, new_potrate, new_potdelay, new_potx);
	}
	safety_tests(now);
}

