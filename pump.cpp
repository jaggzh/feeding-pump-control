#define _IN_PUMP_C
#include "pump.h"

enum pumpstate pumpstate=PUMP_OFF;

const char *pumpstatestr[] = {
	"PUMP_OFF",
	"PUMP_FWD_PULSE",
	"PUMP_REV_PULSE",
	"PUMP_FWD_HOLD_START",
	"PUMP_FWD_HOLD",
	"PUMP_REV_HOLD_START",
	"PUMP_REV_HOLD",
	"PUMP_TURNING_OFF",
};

