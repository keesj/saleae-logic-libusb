// vim: sw=8:ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>

struct slogic_sample_rate {
	const uint8_t pause;	/* sample rates are translated into delays */
	const char *text;
	const unsigned int samples_per_second;
};

/*
I would really dump the tow method bellow, it not "critical" to the lib so 
just drop it to put it somewhere in the main.c - KEJO

These are reusable utility methods which I think it not a part of the "core"
api, but something that a many consumers of a library would like to use so I'd
like to keep them. - Trygve

The problem here is that some it gets hard to hardcode a sample rate
you need to iterate over the sample rates of use what you call a utility method. - KEJO
*/

void slogic_available_sample_rates(struct slogic_sample_rate **sample_rates_out, size_t * size);

struct slogic_sample_rate *slogic_parse_sample_rate(const char *str);

/**
 * Contract between the main program and the utility library
 */
struct slogic_handle;

struct slogic_handle *slogic_open();
void slogic_close(struct slogic_handle *handle);

void slogic_tune(struct slogic_handle *handle, size_t transfer_buffer_size,
		 unsigned int n_transfer_buffers, int libusb_debug_level);

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out);

/*
 * slogic_read_samples reads 1800 samples using the streaming 
 * protocol. This methods is really a proof of concept as the
 * data is not exported yet
 */
int slogic_read_samples(struct slogic_handle *handle,
			struct slogic_sample_rate *sample_rate, uint8_t * samples, size_t recording_size);

#endif
