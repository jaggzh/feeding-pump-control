#define VER_INPUT_TEENSY_ACCGYRO // undefine one
//#define VERSION_PM16_CAPSENSE

/** Normal user settings **/
/* How long longpress (to lock pumping on) */
#define PUMP_LONG_PRESS_MS 2000
#define PUMP_TOO_LONG_PRESS_MS 6000
#define PUMP_PATIENT_TOO_LONG_RUNNING_MS  20000
#define PUMP_ADMIN_TOO_LONG_RUNNING_MS    30000
#define MSECS_OTA_CHECK          40
#define USECS_SERIALBTN_CHECK    950000  // to handle the baud. the baud I tell you.
#define MSECS_LOGICBTN_CHECK     70
#define MSECS_BTN_ROT_CB      31   // Between calls to button press NON-events with "duration"

/** Less-adjustable project settings **/
#define BTN_FWD_PIN  21
#define BTN_REV_PIN  22
#define BTN_PATIENT_PIN  32

#define POT_RATE_PIN  36
#define POT_DELAY_PIN 35
#define POT_X_PIN     34

#define MOTPWM_FWD_CHAN 0
#define MOTPWM_REV_CHAN 2
#define MOTPWM_FREQ 8000
#define MOTPWM_RES  8

// 17, 16 matches PCB.
// For testing with a separate ESP we're re-assigning to some
// unused pins at 18, 19
/* #define MOTPWM_FWD_PIN  17  // This should be 17 */
/* #define MOTPWM_REV_PIN  16  // This should be 16 */
#define MOTPWM_FWD_PIN  18  // This should be 17
#define MOTPWM_REV_PIN  19  // This should be 16
#ifdef VER_INPUT_TEENSY_ACCGYRO
	/* #define PAT_SERIAL_RX_PIN   17 */
	/* #define PAT_SERIAL_TX_PIN   16 // unused (NC) currently */
	/* #define PAT_BTN_SERIAL_BOOL */
	#warning "We're here, properly"
	#define PAT_BTN_LOGICAL
	#define PAT_BTN_LOGIC_PIN   16
	//#define PAT_BTN_CAPSENSE // no. not used on teensy. teensy is for gyro.
	/* #define PAT_BTN_SERIAL_BAUD 9600 */
#else
	#warning "Not using TEENSY as trigger input. Pins might not be set right."
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

#define MOTPWM_MIN 150
#define MOTPWM_MAX 254 // 254 max right now. bug in something
#if MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE
	#error "MOTPWM_MAX > MOTPWM_MAX_DUTY_CYCLE"
#endif
#define MAP_POT_VAL(v) map((int)v, 0, MOTADC_MAX, MOTPWM_MIN, MOTPWM_MAX)


