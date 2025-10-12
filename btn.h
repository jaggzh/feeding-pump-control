#ifndef _IN_BTN_H
#define _IN_BTN_H

#define BTN_DEBOUNCE_MS   40
#define BTN_STATUS_DISPLAY_MS  2000  // display status log frequency
#define SAFETY_TEST_DELAY_MS   50

#define DELAY_MS_POT_UPDATE  5
#define POT_SMOOTH_DIV 8

#define PAT_SERIAL_DATA_CHUNKSIZE 8

// Function flags (16-bit)
#define FUNC_PUMP         0x01    // Motor/pump control
#define FUNC_NET_ALARMS   0x02    // Alarm signals (hold, toolong)
#define FUNC_NET_HID      0x04    // HID event signals (pat-press, pat-release, potx, etc.)
#define FUNC_NET_ANY      (FUNC_NET_ALARMS | FUNC_NET_HID)

// Feature sets (convenient presets)
#define FEATSET_HIDONLY    (FUNC_NET_HID)                                    // 0x04
#define FEATSET_PUMPONLY   (FUNC_PUMP | FUNC_NET_ALARMS)                    // 0x03
#define FEATSET_PUMPHID    (FUNC_PUMP | FUNC_NET_ALARMS | FUNC_NET_HID)    // 0x07

// Default mode on boot
#define FEATSET_DEFAULT    FEATSET_PUMPHID

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
	extern uint16_t function_flags;
	extern char* runtime_alarm_host;
	extern int runtime_alarm_port_hold;
	extern int runtime_alarm_port_toolong;
	extern char* runtime_hid_host;
	extern int runtime_hid_port;
#endif // _IN_BTN_C


#endif
