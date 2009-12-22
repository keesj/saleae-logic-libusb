#firmware.o: firmware.c firmware.h firm_cmds.inc firm_data.inc

firm_cmds.inc: firmware.usbmon
	cat firmware.usbmon | \
        grep "s 40" | \
        cut -d ' ' -f 8,9,10 | \
        awk ' { print "0x"  0x$$1   ", 0x" $$2 ", 0x" $$3 ","} ' \
        > firm_cmds.inc

firm_data.inc: firmware.usbmon
	cat firmware.usbmon | \
        awk -F " " '$$5 == "s" && $$6 == 40 { if($$11 > 16) exit 1; printf $$13 $$14 $$15 $$16 }' | \
        sed -n -e "s/\(................\)/\1_/g" -e "s/\([0-9a-f][0-9a-f]\)/0x\1, /gp" | \
        tr "_" "\n" \
        > firm_data.inc

sinclude .deps
.deps: $(wildcard *.c) $(wildcard *.h)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM *.c > .deps

clean:
	rm -f $(wildcard *.inc) $(wildcard *.o)
