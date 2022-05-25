#define _IN_PUMP_C
#include "pump.h"

/* How long longpress (to lock pumping on) */
#define PUMP_LONG_PRESS_MS 2000

enum pumpstate pumpstate=PUMP_OFF;
