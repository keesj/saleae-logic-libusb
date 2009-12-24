// vim: sw=8:ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>

/* KEJO: the name of the struct and it's content doesn't seam to match */
struct slogic_sample_rate {
	const uint8_t pause;
	const char *text;
	const unsigned int samples_per_second;
};

/* KEJO: it's realy enough to put the slogic_sample_rate (that is located in the  slogic.c 
here to make clear to everybody what rates one can get
struct slogic_sample_rate sample_rates[] = {
	{1, "24MHz", 24000000},
	{2, "16MHz", 16000000},
	{3, "12MHz", 12000000},
	{5, "8MHz", 8000000},
	{11, "4MHz", 4000000},
	{24, "2MHz", 2000000},
	{47, "1MHz", 1000000},
	{95, "500kHz", 500000},
	{191, "250kHz", 250000},
	{239, "200kHz", 200000},
};
Using a enum like you did before is even better but use CAPITALS 
enum sample_rates {
	SAMPLE_RATE_24MHZ  =1;
	SAMPLE_RATE_16MHZ  =2;
	SAMPLE_RATE_12MHZ  =3;
}

I would really dump the tow method bellow, it not "critical" to the lib so 
just drop it to put it somewhere in the main.c
*/


void slogic_available_sample_rates(struct slogic_sample_rate **sample_rates_out,
				   size_t * size);

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
/* KEJO: see comment in main.c about the API */
int slogic_read_samples(struct slogic_handle *handle,
			struct slogic_sample_rate *sample_rate,
			uint8_t * samples, size_t recording_size);

#endif
