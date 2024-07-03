#ifndef _IN_BTN_H
#define _IN_BTN_H

#define BTN_DEBOUNCE_MS   40
#define BTN_STATUS_DISPLAY_MS  2000  // display status log frequency
#define SAFETY_TEST_DELAY_MS   50

#define DELAY_MS_POT_UPDATE  50
#define POT_SMOOTH_DIV 512

#define PAT_SERIAL_DATA_CHUNKSIZE 8

void setup_butts();
void loop_butts_us(unsigned long usecsnow);

void set_all_off();
void set_fwd_hold();
void set_rev_hold();

// Internal. DON'T CALL DIRECTLY -- they don't set the pumpstate variable
void _mot_fwd_set_off();
void _mot_rev_set_off();

// Internal
// /Internal

#ifndef _IN_BTN_C
extern bool motorlocked;

#endif // _IN_BTN_C


#endif
