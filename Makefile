all:run

run:main
	./main
main:main.c slogic.h firmware.h usbutil.h
	gcc `pkg-config --libs --cflags libusb `  -Wall -g -o main main.c 
firmware.h:
	$(MAKE) -C firmware

clean:
	$(MAKE) -C firmware clean
	rm -rf main 
indent_kr:
	indent -kr *.c *.h
