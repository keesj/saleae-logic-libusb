// vim: ts=8:noexpandtab
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <libusb.h>
#include "slogic.h"
#include "usbutil.h"

int main(int argc, char **argv)
{
	struct slogic_handle *handle = slogic_open();

	assert(handle);

	/* apparently one need to at least read once before the driver continues */
#if 0
	printf("Reading byte\n");
	unsigned char b;
	int ret = slogic_readbyte(handle, &b);
	assert(ret == 0);
	printf("ret = %d, byte = 0x02%x\n", ret, b);
#endif
	int sample_count = 24 * 1000 * 1000;	/* Enough for one second of 24Msps */
	struct slogic_sample_rate *sample_rate =
	    slogic_parse_sample_rate("200kHz");
	sample_rate = slogic_parse_sample_rate("24MHz");
	sample_rate = slogic_parse_sample_rate("250kHz");
	assert(sample_rate);

	uint8_t *buffer = malloc(sample_count);
	assert(buffer);

	printf("slogic_read_samples\n");
	slogic_read_samples(handle, sample_rate, buffer, sample_count);

	slogic_close(handle);

	return 0;
}
