#ifndef PRINTUTILS_H
#define PRINTUTILS_H
//#define DEBUG 1
/* #ifdef DEBUG */
/* 	#undef DEBUG */
/* #endif */
#if defined(ARDUINO) || defined(ESP_PLATFORM)
	#include <Arduino.h>
	#define sp(a)  (Serial.print(a))
	#define spl(a) (Serial.println(a))
	#ifdef DEBUG
		#define dbsp(a)  (Serial.print(a))
		#define dbspl(a) (Serial.println(a))
		#define dbprintf(...) // printf(__VA_ARGS__) // not in mcu, sorry
	#else // versions that do nothing if no DEBUG
		#define dbsp(a)
		#define dbspl(a)
		#define dbprintf(...)
	#endif
#else
	#define sp(a)  (cout << (a))
	#define spl(a) (cout << (a) << "\n")
	#ifdef DEBUG
		#define dbsp(a)  (cout << (a))
		#define dbspl(a) (cout << (a) << "\n")
		#define dbprintf(...) printf(__VA_ARGS__)
	#else // versions that do nothing if no DEBUG
		#define dbsp(a)
		#define dbspl(a)
		#define dbprintf(...)
	#endif
#endif

enum spt_type {
	T_END=0, // they'll terminate args with 0 so make it clear
	T_CHAR,
	T_UCHAR,
	T_CHARP,
	T_UCHARP,
	T_FLOAT,
	T_LONG,
	T_ULONG,
	T_LONGP,
	T_ULONGP
};
void spt(enum spt_type type, ...);

#ifndef _IN_PRINTUTILS_C
#endif

#endif // PRINTUTILS_H
#if 0
// alternative version:
#ifdef ARDUINO
	template<typename T> void sp(const T &a) { Serial.print(a); }
	template<typename T> void spl(const T &a) { Serial.println(a); }
#else
	template<typename T> void sp(const T &a) { std::cout << a; }
	template<typename T> void spl(const T &a) { std::cout << a << '\n'; }
	void sp(std::uint8_t a) { std::cout << static_cast<int>(a); }
	void spl(std::uint8_t a) { std::cout << static_cast<int>(a) << '\n'; }
#endif
#endif


