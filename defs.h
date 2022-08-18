/** Normal user settings **/
/* How long longpress (to lock pumping on) */
#define PUMP_LONG_PRESS_MS 2000
#define PUMP_TOO_LONG_PRESS_MS 4000

/** Less-adjustable project settings **/
#define BTN_FWD_PIN  21
#define BTN_REV_PIN  22
#define BTN_USR_PIN  5

#define POT_RATE_PIN  34  // these no longer match schematic for now
#define POT_SENSI_PIN 36  // these no longer match schematic for now
/* #define POT_X_PIN     35  // Comment out to disable */

#define SPEAKER_PIN   23

#define MOTPWM_FWD_CHAN 0
#define MOTPWM_REV_CHAN 2
#define MOTPWM_FREQ 8000
#define MOTPWM_RES  8

#define MOTPWM_FWD_PIN  19
#define MOTPWM_REV_PIN  18

//#define MOTPWM_MAX_DUTY_CYCLE ((int)(pow(2, MOTPWM_RES) - 1))
#define MOTPWM_MAX_DUTY_CYCLE 254
#define MOTADC_MAX 4095 // This must be changed if you change the analog resolution

#define MOTPWM_MIN 155
#define MOTPWM_MAX 235
#if MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE
	#error "MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE"
#endif
#define MAP_POT_RATE(v)  map((int)v, 0, MOTADC_MAX, MOTPWM_MIN, MOTPWM_MAX)
//#define MAP_POT_SENS(v) map((int)v, 0, MOTADC_MAX, 0.0, 5.0)
#define MAP_POT_SENS(v) (((float)v)*2.5/MOTADC_MAX)
#define MAP_POT_X(v) map((int)v, 0, MOTADC_MAX, 0, 1024)


