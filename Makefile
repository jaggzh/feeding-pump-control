all:
	echo 'Using arduino ide.  See make vi and make tags though'

vi:
	vi Makefile feeding-pump.ino \
		defs.h \
		btn.cpp btn.h \
		pump.cpp pump.h \
		ota.cpp ota.h \
		printutils.cpp printutils.h \
		wifi.cpp wifi.h wifi_config--example.h wifi_config.h

tags: *.cpp *.h *.ino
	ctags *.cpp *.h *.ino *.c

