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

main: main.o slogic.o firmware/firmware.o usbutil.o

firmware/firmware.o:
	$(MAKE) -C firmware firmware.o

clean:
	$(MAKE) -C firmware clean
	rm -rf main .deps $(wildcard *.o *~)

indent:
	Lindent $(wildcard *.c *.h)

sinclude .deps
.deps: $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .deps
	$(MAKE) -C firmware .deps
