#ifndef _IN_PUMP_H
#define _IN_PUMP_H

// Internal
// /Internal
enum pumpstate {
	PUMP_OFF,
	PUMP_OFF_SAFETY_MODE,
	PUMP_FWD_PULSE,
	PUMP_REV_PULSE,
	PUMP_FWD_HOLD_START,
	PUMP_FWD_HOLD,
	PUMP_REV_HOLD_START,
	PUMP_REV_HOLD,
	PUMP_TURNING_OFF
};

#ifndef _IN_PUMP_C
extern enum pumpstate pumpstate;
extern const char *pumpstatestr[];
#endif // _IN_PUMP_C


#endif

