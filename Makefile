# vim: noexpandtab
sinclude Makefile.local

VERSION = 1.0

CFLAGS ?= -g
CFLAGS += -Wall
CFLAGS += `$(PKG_CONFIG) --cflags $(PKGS)`
LDLIBS += `$(PKG_CONFIG) --libs $(PKGS)`

PKG_CONFIG ?= pkg-config
PKGS = libusb-1.0

INDENT ?= indent

all: main

run: main
	./main -f out.log -r 16MHz

main: main.o slogic.o firmware/firmware.o usbutil.o log.o

firmware/firmware.o:
	$(MAKE) -C firmware firmware.o

clean:
	$(MAKE) -C firmware clean
	rm -rf main .deps $(wildcard *.o *~)

indent:
	$(INDENT) -npro -kr -i8 -ts8 -sob -l120 -ss -ncs -cp1 $(wildcard *.c *.h)

sinclude .deps
.deps: $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .deps
	$(MAKE) -C firmware .deps

# Misc

test:

install:
	mkdir -p $(DESTDIR)/usr/bin
	cp main $(DESTDIR)/usr/bin/slogic
	chmod +x $(DESTDIR)/usr/bin/slogic

dist:
	date=`git log --date=iso --pretty="format:%ci"|sed -n -e "s,\(....\)-\(..\)-\(..\) \(..\):\(..\).*,\1\2\3\4\5," -e 1p`; \
		 echo "Creating archive in ../saleae-logic-libusb-$(VERSION)-$$date.tar.gz"; \
		git archive --prefix=saleae-logic-libusb-$(VERSION)-$$date/ HEAD | gzip > ../saleae-logic-libusb-$(VERSION)-$$date.tar.gz

.PHONY: dist all run
