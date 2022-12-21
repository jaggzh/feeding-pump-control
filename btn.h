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

// Internal
// /Internal

#ifndef _IN_BTN_C
#endif // _IN_BTN_C


#endif
