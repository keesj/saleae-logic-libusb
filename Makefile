sinclude Makefile.local

CFLAGS ?= -g
CFLAGS += -Wall

PKG_CONFIG ?= pkg-config
PKGS = libusb-1.0
CFLAGS += `$(PKG_CONFIG) --cflags $(PKGS)`
LDLIBS += `$(PKG_CONFIG) --libs $(PKGS)`

all: main

run: main
	./main

main: main.o slogic.o firmware.o usbutil.o

firmware.h:
	$(MAKE) -C firmware

clean:
	$(MAKE) -C firmware clean
	rm -rf main .deps
indent_kr:
	indent -kr *.c *.h

sinclude .deps
.deps: $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .deps
