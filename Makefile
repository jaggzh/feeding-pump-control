all:
	echo 'Using arduino ide.  See make vi and make tags though'

vi:
	vi Makefile feeding-pump.ino \
		btn.cpp btn.h \
		pump.cpp pump.h \
		defs.h ota.cpp ota.h \
		printutils.cpp printutils.h \
		wifi.cpp wifi.h wifi_config--example.h wifi_config.h

tags: *.cpp *.h *.ino
	ctags *.cpp *.h *.ino *.c

