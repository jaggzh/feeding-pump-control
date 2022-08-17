#ifndef _IN_BTN_H
#define _IN_BTN_H

#include "defs.h"
#include <capsense.h>

#define BTN_DEBOUNCE_MS   40
#define BTN_STATUS_DISPLAY_MS  500  // display status log frequency

#define DELAY_MS_POT_UPDATE  20
#define POT_RATE_SMOOTH_DIV 256   // This one's used on the raw 0-4095 int
                                  //  ^ pre-map()'ing
#define POT_SENS_SMOOTH_DIV 16    // This one's used on a small float
                                  //  ^ post-map()'ing

void setup_butts();
void loop_butts();

void cap_cb_press(cp_st *cp);
void cap_cb_release(cp_st *cp);

// Internal
// /Internal

#ifndef _IN_BTN_C
	extern int potrate_raw, potsens_raw;
	extern int potrate;
	extern float potsens;
	#ifdef POT_X_PIN
		extern float potx;
	#endif
#endif // _IN_BTN_C


#endif
