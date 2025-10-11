#ifndef _IN_BTN_H
#define _IN_BTN_H

#define BTN_DEBOUNCE_MS   40
#define BTN_STATUS_DISPLAY_MS  2000  // display status log frequency
#define SAFETY_TEST_DELAY_MS   50

#define DELAY_MS_POT_UPDATE  5
#define POT_SMOOTH_DIV 8

#define PAT_SERIAL_DATA_CHUNKSIZE 8

enum OPERATION_MODE {
	MODE_PUMP_ONLY = 0,   // Motor only, no TCP signals
	MODE_SIGNAL_ONLY,     // TCP signals only, no motor
	MODE_BOTH             // Both motor and TCP signals (default)
};

void setup_butts();
void loop_butts_us(unsigned long usecsnow);

void set_all_off();
void set_fwd_hold();
void set_rev_hold();
void trigger_send_value(const char *server, int svrport, char *lbl, float value);
void trigger_remote_alarm(const char *server, int svrport); // hits remote alarm for now

// Internal. DON'T CALL DIRECTLY -- they don't set the pumpstate variable
void _mot_fwd_set_off();
void _mot_rev_set_off();

// Internal
// /Internal

enum UPDATE_TIME_FLAG {
	NO_UPDATE_TIME=0,
	UPDATE_TIME
};


#ifndef _IN_BTN_C
	extern bool motorlocked;
	extern float potrate;
	extern float potx;
	extern enum OPERATION_MODE operation_mode;
	extern char* runtime_alarm_host;
	extern int runtime_alarm_port_hid;
	extern int runtime_alarm_port_hold;
	extern int runtime_alarm_port_toolong;
#endif // _IN_BTN_C


#endif
