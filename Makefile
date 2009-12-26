# vim: noexpandtab
sinclude Makefile.local

CFLAGS ?= -g
CFLAGS += -Wall
CFLAGS += `$(PKG_CONFIG) --cflags $(PKGS)`
LDLIBS += `$(PKG_CONFIG) --libs $(PKGS)`

PKG_CONFIG ?= pkg-config
PKGS = libusb-1.0

INDENT ?= indent

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
	$(INDENT) -npro -kr -i8 -ts8 -sob -l120 -ss -ncs -cp1 $(wildcard *.c *.h)

# It would be awesome if git should just spit out a UTC formatted string - trygve
dist:
	date=`git log --date=iso --pretty="format:%ci"|sed -n -e "s,\(....\)-\(..\)-\(..\) \(..\):\(..\).*,\1\2\3-\4\5," -e 1p`; \
	git archive --prefix=saleae-logic-libusb-$$date/ HEAD | bzip2 > ../saleae-logic-libusb-$$date.tar.bz2

sinclude .deps
.deps: $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .deps
	$(MAKE) -C firmware .deps

.PHONY: dist all run
