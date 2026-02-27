#ifndef _DEFS_H
#define _DEFS_H

// #define PCBVER 2022
#define PCBVER 2024

#define VER_LOGIC_ACCGYRO_METHOD // undefine one
//#define VERSION_PM16_CAPSENSE

/** Normal user settings **/
/* How long longpress (to lock pumping on) */
#define PUMP_LONG_PRESS_MS 2000
#define PUMP_TOO_LONG_PRESS_MS 6000
#define PUMP_PATIENT_TOO_LONG_RUNNING_MS  40000
#define PUMP_ADMIN_TOO_LONG_RUNNING_MS    30000
#define MSECS_OTA_CHECK          40
#define USECS_SERIALBTN_CHECK    950000  // to handle the baud. the baud I tell you.
#define MSECS_LOGICBTN_CHECK     70
#define MSECS_BTN_ROT_CB      31   // Between calls to button press NON-events with "duration"

/** Less-adjustable project settings **/
// #define BTN_FWD_PIN  21  /* new board */
// #define BTN_REV_PIN  22  /* new board */
#if PCBVER == 2024
	IPAddress ip(192, 168, 2, 131);
	IPAddress gw(192, 168, 2, 1);
	IPAddress nm(255, 255, 255, 0);

	#define BTN_FWD_PIN  21
	#define BTN_REV_PIN  22
	#define MOTPWM_FWD_PIN  19
	#define MOTPWM_REV_PIN  18
	#define POT_RATE_PIN  35
	#define BTN_PAT_PIN  5
	#ifdef VER_LOGIC_ACCGYRO_METHOD
		#define PAT_BTN_LOGICAL
		// #define PAT_BTN_LOGIC_RX_PIN   16 // Unused, but it is connected to USB-B
		#define PAT_BTN_LOGIC_PIN   17
	#endif
#else // 2022
	IPAddress ip(192, 168, 2, 130);
	IPAddress gw(192, 168, 2, 1);
	IPAddress nm(255, 255, 255, 0);

	#define BTN_FWD_PIN  21
	#define BTN_REV_PIN  22
	// MOTPWM PWM Control FWD/REV: 17, 16 matches PCB.
	// For testing with a separate ESP we're re-assigning to some
	// unused pins at 18, 19
	#define MOTPWM_FWD_PIN  17
	#define MOTPWM_REV_PIN  16
	#define POT_RATE_PIN  34
	#define BTN_PAT_PIN  5
	#ifdef VER_LOGIC_ACCGYRO_METHOD
		#define PAT_BTN_LOGICAL
		// #define PAT_BTN_LOGIC_RX_PIN   ?? // Unused, but it is connected to USB-B
		#define PAT_BTN_LOGIC_PIN   16
	#endif
#endif

#define POT_X_PIN     36
/* #define POT_DELAY_PIN 36 */

#define MOTPWM_FWD_CHAN 0
#define MOTPWM_REV_CHAN 2
#define MOTPWM_FREQ 8000
#define MOTPWM_RES  8

#ifndef VER_LOGIC_ACCGYRO_METHOD
	/* #warning "Not using TEENSY as trigger input. Pins might not be set right." */
	#error "We're in the capsense version. This doesn't work."
	/* #define PAT_SERIAL_RX_PIN   16 */
	/* #define PAT_SERIAL_TX_PIN   17 */
	//#define PAT_BTN_SER_BOOL
	#define PAT_BTN_CAPSENSE
	#define PAT_BTN_SERIAL_BAUD 9600
#endif
#if !defined(PAT_BTN_SER_BOOL) && !defined(PAT_BTN_CAPSENSE) && !defined(PAT_BTN_LOGICAL)
	#error "No PAT_BTN_* method chosen. Patient button won't work."
#endif
#if defined(PAT_BTN_SER_BOOL) + defined(PAT_BTN_CAPSENSE) + !defined(PAT_BTN_LOGICAL) > 1
	#warning "These PAT_BTN_ options might conflict, all using the PS2 connector. Careful."
#endif

//#define MOTPWM_MAX_DUTY_CYCLE ((int)(pow(2, MOTPWM_RES) - 1))
#define MOTPWM_MAX_DUTY_CYCLE 254
#define MOTADC_MAX 4095 // This must be changed if you change the analog resolution

#define MOTPWM_MIN 150 // this was 120 for years and was too low for the pump to function
#define MOTPWM_MAX 245 // 254 max right now. bug in something
#if MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE
	#error "MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE"
#endif
#define MAP_POT_VAL(v) map((int)v, 0, MOTADC_MAX, MOTPWM_MIN, MOTPWM_MAX)


#endif // _DEFS_H
