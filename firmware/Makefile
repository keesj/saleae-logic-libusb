../firmware.h:firmware.h.in firm_cmds.inc firm_data.inc
	m4 firmware.h.in > ../firmware.c
convert:convert.c

firm_cmds.inc:firmware.usbmon
	cat firmware.usbmon | grep "s 40" | cut -d ' ' -f 8,9,10 | awk ' { print "0x"  0x$$1   ", 0x" $$2 ", 0x" $$3 ","} ' > firm_cmds.inc
firm_data.inc:convert
	cat firmware.usbmon | grep "s 40" | cut -d ' ' -f 13-  | sed "s, ,,g" | ./convert > firm_data.inc

clean:
	rm -f convert firm_cmds.inc firm_data.inc
