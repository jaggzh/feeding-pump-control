#define _IN_BTN_C
#include <Arduino.h>
#include <InputDebounce.h>
#include <WiFi.h>
#include "wifi_config.h"
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

// Runtime configuration for function flags
uint16_t function_flags = FEATSET_DEFAULT;

// Runtime configuration for alarm host/ports (hold and toolong events)
char* runtime_alarm_host = NULL;  // NULL = use ALARM_HOLD_HOST default
int runtime_alarm_port_hold = -1;  // -1 = use ALARM_HOLD_PORT default
int runtime_alarm_port_toolong = -1;  // -1 = use ALARM_HOLD_TOOLONG_PORT default

// Runtime configuration for HID host/port (general events)
char* runtime_hid_host = NULL;  // NULL = use ALARM_HOLD_HOST default
int runtime_hid_port = -1;  // -1 = use ALARM_HID_PORT default

unsigned long mot_fwd_on_ms = 0;   // Tracking how long motor on (for safety limit)
unsigned long last_status_ms = 0;  // Reduce serial output
unsigned long last_pot_update = 0; // Reduce pot tests
unsigned long last_safety_ms = 0;  // Reduce frequency of safety tests (I know right)
static InputDebounce btn_fwd;
static InputDebounce btn_rev;
static InputDebounce btn_pat;
float potrate=0;
float potx=0;
float last_potrate_applied=-30;
float last_potx_applied=-30;
bool motorlocked;

#ifdef PAT_BTN_CAPSENSE
	struct SerialDechunk dechunk_real;
	struct SerialDechunk *dechunk = &dechunk_real;
#endif

/********************************************
 * Motor toggles
 * Warning: These do not test other channels!
 *   Our controller disables the motor if two channels are the same, but yours
 *   may not.  Make sure to, for instance, call { a_off(); b_on(); }
 */
void _mot_fwd_set_on(enum UPDATE_TIME_FLAG updtime) {
	_mot_rev_set_off();
	if (!(function_flags & FUNC_PUMP)) {
		spl("_mot_fwd_set_on(): NO ACTION -- FUNC_PUMP disabled");
	} else if (motorlocked) {
		spl("_mot_fwd_set_on(): NO ACTION -- LOCK IS ENABLED");
	} else {
		if (updtime == UPDATE_TIME) {
			sp("  FWD ON Updating time => ");
			mot_fwd_on_ms = millis();
			spl(mot_fwd_on_ms);
		}
		int newval = MAP_POT_VAL(potrate);
		sp("FWD ON (rate:"); sp(newval); spl(')');
		ledcWriteChannel(MOTPWM_FWD_CHAN, newval);
	}
}
void _mot_fwd_set_off() {
	if (function_flags & FUNC_PUMP) {
		spl("FWD OFF");
		ledcWriteChannel(MOTPWM_FWD_CHAN, 0);
	}
}
void _mot_rev_set_on() {
	_mot_fwd_set_off();
	if (!(function_flags & FUNC_PUMP)) {
		sp("_mot_rev_set_on(): NO ACTION -- FUNC_PUMP disabled");
	} else if (motorlocked) {
		sp("_mot_rev_set_on(): NO ACTION -- LOCK IS ENABLED");
	} else {
		int newval = MAP_POT_VAL(potrate);
		sp("REV ON (rate:"); sp(newval); spl(')');
		ledcWriteChannel(MOTPWM_REV_CHAN, newval);
	}
}
void _mot_rev_set_off() {
	if (function_flags & FUNC_PUMP) {
		spl("REV OFF");
		ledcWriteChannel(MOTPWM_REV_CHAN, 0);
	}
}

float readAverage(int pin, int samples, int dly) {
	float total = 0;
	for (int i = 0; i < samples; i++) {
		total += analogRead(pin);
		delay(dly);
	}
	return total / samples;
}

float readMedian(int pin, int samples, int dly) {
	int readings[samples];
	
	int i;
	int j;
	for (j=0, i = 0; i < samples; i++) {
		int r;
		r=analogRead(pin);
		if (r) {
			readings[j] = r;
			delay(dly);
			j++;
		}
	}
	if (!j) return 0.0;
	samples = j;
	
	// Sort
	for (i = 0; i < samples - 1; i++) {
		for (int j = i + 1; j < samples; j++) {
			if (readings[i] > readings[j]) {
				int temp = readings[i];
				readings[i] = readings[j];
				readings[j] = temp;
			}
		}
	}

	/* for (i = 0; i < samples; i++) { */
	/* 		if (i) sp(' '); */
	/* 		sp(readings[i]); */
	/* } */
	/* spl(""); */
	
	// Return median
	if (samples % 2 == 0) {
		return (readings[samples / 2 - 1] + readings[samples / 2]) / 2.0;
	} else {
		return readings[samples / 2];
	}
}

void update_pump_x(int new_potx) {
	potx += (((float)new_potx) - potx) / (POT_SMOOTH_DIV);
	if (abs((int)(last_potx_applied - potx)) > 15) {
		trigger_send_value(HOST_HID, PORT_HID, "potx", potx);
		last_potx_applied = potx;
	}
}

void update_pump_rate(int new_potrate) {
	potrate += (((float)new_potrate) - potrate) / (POT_SMOOTH_DIV);
	if (abs((int)(last_potrate_applied - potrate)) > 10) {
		if (pumpstate == PUMP_FWD_PULSE ||
				pumpstate == PUMP_FWD_HOLD_START ||
				pumpstate == PUMP_FWD_HOLD)
			_mot_fwd_set_on(NO_UPDATE_TIME);
		else if (pumpstate == PUMP_REV_PULSE ||
				pumpstate == PUMP_REV_HOLD_START ||
				pumpstate == PUMP_REV_HOLD)
			_mot_rev_set_on();
		last_potrate_applied = potrate;
	}
}

/********************************************
 * Button handlers (pressed and released) */
void btn_fwd_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	if (debuglevel>0) {
		sp("btn_fwd_cb_pressed_dur(");
		sp(pinIn); sp(", "); sp(dur); spl(" ms)");
	}
	if (pumpstate == PUMP_OFF) {
		spl("PUMP FWD PULSE MODE");
		triggered_by_patient = false;
		_mot_fwd_set_on(UPDATE_TIME);
		pumpstate = PUMP_FWD_PULSE;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("PUMP FWD HELD UNTIL HOLD MODE");
			pumpstate = PUMP_FWD_HOLD_START;
			trigger_remote_alarm(HOST_ALARM, PORT_ALARM_HOLD);
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("PUMP FWD TOGGLED OFF");
		_mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("PUMP REV CANCELLED");
		_mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_REV_PULSE || pumpstate == PUMP_REV_HOLD_START) {
		// REV still held down
		spl("PUMP REV PULSE MODE LOCKED INTO HOLD (Ignored. Wont lock reverse)");
		//pumpstate = PUMP_REV_HOLD; // lock REV on
		_mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF; // lock REV on
	}
}
void btn_fwd_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	int pstate=0;
	if (pumpstate == PUMP_FWD_PULSE) {
		pumpstate = PUMP_OFF;
		_mot_fwd_set_off();
		pstate=1;

	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		pumpstate = PUMP_FWD_HOLD;
		pstate=2;
	} else if (pumpstate == PUMP_TURNING_OFF) {
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
		pstate=3;
	}
	if (pstate) {
		sp("btn_fwd_cb_pressed_dur(");
		sp(pinIn); sp(", "); sp(dur);
		sp(F(" ms, cb_state=")); sp(pstate); spl(")");
	}
}

void btn_rev_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	sp("btn_rev_cb_pressed_dur(");
	sp(pinIn); sp(", "); sp(dur); spl(" ms)");
	if (pumpstate == PUMP_OFF) {
		spl("PUMP REV PULSE MODE");
		triggered_by_patient = false;
		_mot_rev_set_on();
		pumpstate = PUMP_REV_PULSE;
	} else if (pumpstate == PUMP_REV_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("PUMP REV HELD UNTIL HOLD MODE");
			spl(" (Refusing. We don't hold reverse.)");
			//pumpstate = PUMP_REV_HOLD_START;
		}
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("PUMP REV TOGGLED OFF");
		_mot_rev_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("PUMP FWD CANCELLED");
		_mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
	} else if (pumpstate == PUMP_FWD_PULSE) {
		// FWD still held down
		spl("PUMP FWD PULSE MODE LOCKED INTO HOLD");
		pumpstate = PUMP_FWD_HOLD_START; // lock REV on
	}
}

void btn_rev_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	sp("btn_rev_cb_released_dur(");
	sp(pinIn); sp(", "); sp(dur); spl(" ms)");
	if (pumpstate == PUMP_REV_PULSE) {
		pumpstate = PUMP_OFF;
		_mot_rev_set_off();
	} else if (pumpstate == PUMP_REV_HOLD_START)
		pumpstate = PUMP_REV_HOLD;
	else if (pumpstate == PUMP_TURNING_OFF)
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
}

void set_fwd_hold() {
	_mot_fwd_set_on(UPDATE_TIME);
	pumpstate = PUMP_FWD_HOLD;
}

void set_rev_hold() {
	_mot_rev_set_on();
	pumpstate = PUMP_REV_HOLD;
}

void set_all_off() {
	_mot_rev_set_off();
	_mot_fwd_set_off();
	pumpstate = PUMP_OFF;
}

void btn_pat_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	sp("btn_pat_cb_pressed_dur(");
	sp(pinIn); sp(", "); sp(dur); spl(" ms)");
	if (pumpstate == PUMP_OFF) {
		spl("(*USER*) PUMP FWD PULSE MODE");
		triggered_by_patient = true;
		_mot_fwd_set_on(UPDATE_TIME);
		pumpstate = PUMP_FWD_PULSE;
		trigger_send_value(HOST_HID, PORT_HID, "pat-press", 1.0);
	} else if (pumpstate == PUMP_FWD_PULSE) {
		if (dur >= PUMP_LONG_PRESS_MS) {
			spl("(*USER*) PUMP FWD HELD UNTIL HOLD MODE");
			pumpstate = PUMP_FWD_HOLD_START;
			trigger_send_value(HOST_HID, PORT_HID, "pat-hold", 1.0);
			trigger_remote_alarm(HOST_ALARM, PORT_ALARM_HOLD);
		}
	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		if (dur >= PUMP_TOO_LONG_PRESS_MS) {
			spl("(*USER*) PUMP FWD HELD TOO LONG. SAFETY SHUTOFF");
			_mot_fwd_set_off();
			pumpstate = PUMP_OFF_SAFETY_MODE;
			trigger_send_value(HOST_HID, PORT_HID, "pat-safety", 1.0);
			trigger_remote_alarm(HOST_ALARM, PORT_ALARM_TOOLONG);
		}
	} else if (pumpstate == PUMP_FWD_HOLD) {
		spl("(*USER*) PUMP FWD TOGGLED OFF");
		_mot_fwd_set_off();
		pumpstate = PUMP_TURNING_OFF;
		trigger_send_value(HOST_HID, PORT_HID, "pat-release", 1.0);
	} else if (pumpstate == PUMP_REV_HOLD) {
		spl("(*USER*) PUMP FWD CANCELLED");
		_mot_rev_set_off();
		trigger_send_value(HOST_HID, PORT_HID, "pat-rev-hold--cancel-by-pat-press", 1.0);
		pumpstate = PUMP_TURNING_OFF;
	}
}

void btn_pat_cb_released_dur(uint8_t pinIn, unsigned long dur) {
	char pstate=0;
	if (pumpstate == PUMP_FWD_PULSE) {
		if (triggered_by_patient) {
			pumpstate = PUMP_OFF;
			_mot_fwd_set_off();
			pstate=1;
			trigger_send_value(HOST_HID, PORT_HID, "pat-release-from-press", 1.0);
		}
	} else if (pumpstate == PUMP_FWD_HOLD_START) {
		if (triggered_by_patient) {
			pumpstate = PUMP_FWD_HOLD;
			pstate=2;
			trigger_send_value(HOST_HID, PORT_HID, "pat-release-from-hold-start", 1.0);
		}
	} else if (pumpstate == PUMP_TURNING_OFF) {
		/* This is when a HOLD was terminated by a press. It's already off
		 * so we're just changing the state once they release. */
		pumpstate = PUMP_OFF;
		pstate=3;
	} else if (pumpstate == PUMP_OFF_SAFETY_MODE) {
		/* This is when a button (PATIENT currently) is held down too long.
		 * For safety we consider this someone accidentally holding it, or it
		 * being pressed and unable to be released, or a dysfunction in a button
		 * could cause a short.  Thus, for safety, we will turn the motor off.
		 * ** WARNING ** This is only implemented for the PATIENT button, not the normal
		 * FWD/REV buttons right now. */
		_mot_fwd_set_off(); // making sure it's off. It should be already though.
		pumpstate = PUMP_OFF;
		pstate=4;
		trigger_send_value(HOST_HID, PORT_HID, "pat-release-from-safety", 1.0);
	}
	if (pstate) {
		sp(F("btn_pat_cb_released_dur("));
		sp(pinIn); sp(", "); sp(dur);
		sp(F(" ms, cb_state=")); sp(pstate); spl(")");
	}
}

void safety_tests(unsigned long now) {
	if (pumpstate == PUMP_FWD_HOLD) {
		if (now - last_safety_ms > SAFETY_TEST_DELAY_MS) {
			last_safety_ms = now;
			if ( (triggered_by_patient  && (now-mot_fwd_on_ms > PUMP_PATIENT_TOO_LONG_RUNNING_MS))) {
				spl("PUMP (PATIENT MODE) RUNNING TOO LONG, TURNING OFF.");
				_mot_fwd_set_off();
				pumpstate = PUMP_OFF;
			} else if (!triggered_by_patient && (now-mot_fwd_on_ms > PUMP_ADMIN_TOO_LONG_RUNNING_MS)) {
				spl("PUMP (ADMIN MODE) RUNNING TOO LONG, TURNING OFF.");
				sp("  (now="); sp(now);
				sp("; mot_fwd_on_ms="); sp(mot_fwd_on_ms);
				sp(". Diff=");
				sp(now-mot_fwd_on_ms);
				sp(" > too_long=");
				spl(PUMP_ADMIN_TOO_LONG_RUNNING_MS);
				_mot_fwd_set_off();
				pumpstate = PUMP_OFF;
			}
		}
	}
}

void trigger_send_value(const char *server, int svrport, char *lbl, float value) {
	// Determine if this is an alarm or HID event based on port
	bool is_alarm = (svrport == PORT_ALARM_HOLD || svrport == PORT_ALARM_TOOLONG);
	bool is_hid = (svrport == PORT_HID);
	
	// Check if we should send this type of signal
	if (is_alarm && !(function_flags & FUNC_NET_ALARMS)) {
		return;  // Alarm signals disabled
	}
	if (is_hid && !(function_flags & FUNC_NET_HID)) {
		return;  // HID signals disabled
	}
	
	// Determine target host and port
	const char *target_host;
	int target_port;
	
	if (is_alarm) {
		target_host = (runtime_alarm_host != NULL) ? runtime_alarm_host : HOST_ALARM;
		// Port selection for alarms
		if (svrport == PORT_ALARM_HOLD && runtime_alarm_port_hold != -1) {
			target_port = runtime_alarm_port_hold;
		} else if (svrport == PORT_ALARM_TOOLONG && runtime_alarm_port_toolong != -1) {
			target_port = runtime_alarm_port_toolong;
		} else {
			target_port = svrport;
		}
	} else {  // is_hid
		target_host = (runtime_hid_host != NULL) ? runtime_hid_host : HOST_HID;
		target_port = (runtime_hid_port != -1) ? runtime_hid_port : svrport;
	}
	
	WiFiClient client;
	if (client.connect(target_host, target_port)) {
		sp(F("Connection to server established"));
		client.printf("%s=%.2f\n", lbl, value);
	} else {
		sp(F("Connection failed"));
	}
	client.stop(); // Close the connection
}

void trigger_remote_alarm(const char *server, int svrport) {
	// Check if alarm signals are enabled
	if (!(function_flags & FUNC_NET_ALARMS)) {
		return;  // Alarm signals disabled
	}
	
	// Determine target host and port
	const char *target_host = (runtime_alarm_host != NULL) ? runtime_alarm_host : HOST_ALARM;
	int target_port;
	
	if (svrport == PORT_ALARM_HOLD && runtime_alarm_port_hold != -1) {
		target_port = runtime_alarm_port_hold;
	} else if (svrport == PORT_ALARM_TOOLONG && runtime_alarm_port_toolong != -1) {
		target_port = runtime_alarm_port_toolong;
	} else {
		target_port = svrport;
	}
	
	WiFiClient client;
	if (client.connect(target_host, target_port)) {
		sp(F("Connection to server established"));
	} else {
		sp(F("Connection failed"));
	}
	client.stop(); // Close the connection
}

#ifdef PAT_BTN_CAPSENSE
	void serial_dechunk_cb(struct SerialDechunk *sp) {
		DSP(" {");
		for (int i=0; i<sp->chunksize; i++) {
			/* printf("%d ", sp->b[i]); */
			DSP(sp->b[i]);
			DSP(',');
		}
		DSPL("} ");
	}
#endif

void setup_butts() {
	pinMode(POT_RATE_PIN, INPUT);
	potrate = (float)analogRead(POT_RATE_PIN);
	pinMode(POT_X_PIN, INPUT);
	potx = (float)analogRead(POT_X_PIN);

	/* Motor pin output tests: */
	/* pinMode(MOTPWM_FWD_PIN, OUTPUT); */
	/* pinMode(MOTPWM_REV_PIN, OUTPUT); */
	/* digitalWrite(MOTPWM_FWD_PIN, HIGH); */
	/* digitalWrite(MOTPWM_REV_PIN, HIGH); */

	btn_fwd.registerCallbacks(NULL, NULL, btn_fwd_cb_pressed_dur, btn_fwd_cb_released_dur);
	btn_rev.registerCallbacks(NULL, NULL, btn_rev_cb_pressed_dur, btn_rev_cb_released_dur);
	btn_pat.registerCallbacks(NULL, NULL, btn_pat_cb_pressed_dur, btn_pat_cb_released_dur);
	btn_fwd.setup(BTN_FWD_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_rev.setup(BTN_REV_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);
	btn_pat.setup(BTN_PAT_PIN, BTN_DEBOUNCE_MS, InputDebounce::PIM_INT_PULL_UP_RES);

	ledcAttachChannel(MOTPWM_FWD_PIN, MOTPWM_FREQ, MOTPWM_RES, MOTPWM_FWD_CHAN);
	ledcAttachChannel(MOTPWM_REV_PIN, MOTPWM_FREQ, MOTPWM_RES, MOTPWM_REV_CHAN);


	// Old, pre esp32 core channel 2.x
	//ledcSetup(MOTPWM_FWD_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	//ledcAttach(MOTPWM_FWD_PIN, MOTPWM_FWD_CHAN);
	////ledcWriteChannel(MOTPWM_FWD_CHAN, 0);

	// Old, pre esp32 core channel 2.x
	//ledcSetup(MOTPWM_REV_CHAN, MOTPWM_FREQ, MOTPWM_RES);
	//ledcAttachPin(MOTPWM_REV_PIN, MOTPWM_REV_CHAN);
	////ledcWriteChannel(MOTPWM_REV_CHAN, MOTPWM_MAX_DUTY_CYCLE);

	#ifdef PAT_BTN_SER_BOOL
		Serial2.begin(PAT_BTN_SERIAL_BAUD, SERIAL_8N1, PAT_SERIAL_RX_PIN, PAT_SERIAL_TX_PIN);
	#endif
	#ifdef PAT_BTN_LOGICAL
		pinMode(PAT_BTN_LOGIC_PIN, INPUT_PULLUP);
	#endif
	#ifdef PAT_BTN_CAPSENSE
		serial_dechunk_init(dechunk, PAT_SERIAL_DATA_CHUNKSIZE, serial_dechunk_cb);
	#endif
}

#ifdef PAT_BTN_LOGICAL
	// logic state is inversed due to pullups
	#define PAT_BTN_LOGIC_STATE_ON LOW
	#define PAT_BTN_LOGIC_STATE_OFF HIGH
	void patient_button_logical_process(unsigned long msnow, uint8_t invstate) {
		static uint8_t last_inv_state=PAT_BTN_LOGIC_STATE_OFF;
		static unsigned long last_ms_start=0;
		static unsigned long last_ms_callback_start=0;
		unsigned long dur=0;
		if (invstate != last_inv_state) {
			sp("ROT BTN STATE CHANGE => ");
			spl(invstate == PAT_BTN_LOGIC_STATE_OFF ? "off" : "on");
			if (invstate == PAT_BTN_LOGIC_STATE_OFF) btn_pat_cb_released_dur(0, dur);
			else btn_pat_cb_pressed_dur(0, dur);
			last_inv_state = invstate;
			last_ms_callback_start = last_ms_start = msnow;
		} else {
			if (msnow - last_ms_callback_start > MSECS_BTN_ROT_CB) {
				last_ms_callback_start = msnow;
				dur = msnow - last_ms_start;
				if (invstate == PAT_BTN_LOGIC_STATE_OFF) btn_pat_cb_released_dur(0, dur);
				else btn_pat_cb_pressed_dur(0, dur);
			}
		}
		//void btn_pat_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	}
#endif

#ifdef PAT_BTN_SER_BOOL
	void patient_button_bool_process(unsigned long msnow, uint8_t state) {
		static uint8_t laststate=0;
		static unsigned long last_ms_start=0;
		static unsigned long last_ms_callback_start=0;
		unsigned long dur=0;
		if (state != laststate) {
			sp("ROT BTN STATE CHANGE => ");
			spl(state ? "ON" : "off");
			if (!state) btn_pat_cb_released_dur(0, dur);
			else btn_pat_cb_pressed_dur(0, dur);
			laststate = state;
			last_ms_callback_start = last_ms_start = msnow;
		} else {
			if (msnow - last_ms_callback_start > MSECS_BTN_ROT_CB) {
				last_ms_callback_start = msnow;
				dur = msnow - last_ms_start;
				if (!state) btn_pat_cb_released_dur(0, dur);
				else btn_pat_cb_pressed_dur(0, dur);
			}
		}
		//void btn_pat_cb_pressed_dur(uint8_t pinIn, unsigned long dur) {
	}
#endif

#if defined(PAT_BTN_SER_BOOL) || defined(PAT_BTN_CAPSENSE)
void loop_butts_serial_ms(unsigned long msnow) {
	unsigned long usnow = micros();
	static int wrap=0;
	static unsigned long last_ser_check_us=0;
	if (usnow - last_ser_check_us > USECS_SERIALBTN_CHECK) {
		last_ser_check_us = usnow;
		if (Serial2.available()) {
			uint8_t c = Serial2.read();
			sp("Ser read: ");
			spl(c);
			DSP(c);
			#ifdef PAT_BTN_SER_BOOL
				patient_button_bool_process(msnow, c);
			#elif defined(PAT_BTN_CAPSENSE)
				dechunk->add(dechunk, c);
			#else
				#warning "No PAT_BTN_* method chosen."
			#endif
		} else {
			DSP("No ser.available()\n");
		}
	}
}
#endif

void loop_butts_patient_logical_ms(unsigned long msnow) {
	/* static int wrap=0; */
	static unsigned long last_check_ms=0;
	if (msnow - last_check_ms > MSECS_LOGICBTN_CHECK) {
		last_check_ms = msnow;
		int invstate = digitalRead(PAT_BTN_LOGIC_PIN);
		/* sp("Read: "); */
		/* spl(invstate); */
		patient_button_logical_process(msnow, invstate);
	}
}

void loop_butts_us(unsigned long usecsnow) {
	unsigned long msnow = millis();
	int new_potx;
	int new_potrate;
	int motfwd_duty;
	int motrev_duty;
	/* sp("PAT BUT(32) "); spl(digitalRead(BTN_PAT_PIN)); */

	btn_fwd.process(msnow);
	btn_rev.process(msnow);
	btn_pat.process(msnow);

	#if defined(PAT_BTN_SER_BOOL) || defined(PAT_BTN_CAPSENSE)
		#error "Not using these methods ^^ right now"
		loop_butts_serial_ms(msnow);
	#endif
	#if defined(PAT_BTN_LOGICAL)
		loop_butts_patient_logical_ms(msnow);
	#endif

	// The pot update is faster than our serial output.
	// To ensure our variables are set in serial output
	// we have the serial within the pot update:
	if (msnow - last_pot_update > DELAY_MS_POT_UPDATE) {
		last_pot_update = msnow;
		// get preliminary value for smoothing for final value
		/* new_potrate = analogRead(POT_RATE_PIN); */
		new_potrate = readMedian(POT_RATE_PIN, 13, 2);
		new_potx = readMedian(POT_X_PIN, 13, 2);
		update_pump_rate(new_potrate);
		update_pump_x(new_potx);
		// these two aren't used or smoothed. we'll assign them
		//  directly:
		/* sp(potrate); sp(' '); */
		/* spl(""); */
		/* sp("r:"); sp(new_potrate); sp("\tsr:"); sp(potrate); */
		/* spl(""); */
/* #if 0 */

		if (msnow - last_status_ms > BTN_STATUS_DISPLAY_MS) {
			last_status_ms = msnow;
			motfwd_duty = ledcRead(MOTPWM_FWD_CHAN);
			motrev_duty = ledcRead(MOTPWM_REV_CHAN);
			sp("[PUMP STATE:"); sp(pumpstatestr[pumpstate]); sp("] ");
			sp("BTN(Go:"); sp(btn_fwd.isPressed() ? '1' : '0'); sp(", ");
			sp("Rev:"); sp(btn_rev.isPressed() ? '1' : '0'); sp(", ");
			sp("Usr:"); sp(btn_pat.isPressed() ? '1' : '0'); sp(") ");
			sp("POT{{Rate:"); sp(new_potrate); sp("["); sp(potrate); sp("] ");
			sp("POT{{X:"); sp(new_potx); sp("["); sp(potrate); sp("] ");
			sp(" Duty(Fwd:"); sp(motfwd_duty);
			sp(" Rev:"); sp(motrev_duty); sp(")");
			spl("");
		}
/* #endif */
	}
	safety_tests(millis());
}

