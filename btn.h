#ifndef _IN_BTN_H
#define _IN_BTN_H

#define BTN_DEBOUNCE_MS   40
#define BTN_STATUS_DISPLAY_MS  500  // display status log frequency

#define POT_SMOOTH_DIV 4

void setup_butts();
void loop_butts();

// Internal
// /Internal

#ifndef _IN_BTN_C
#endif // _IN_BTN_C


#endif
