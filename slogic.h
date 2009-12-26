// vim: sw=8:ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>
#include <stdio.h>
#include <stdbool.h>

/* KEJO: the name of the struct and it's content doesn't seam to match */
/* Trygve: To the user these are what they think about as a sample
rate. The "pause" field is definitely implementation specific which
is why I first had it as an internal-only struct but as the text
was useful for usage() it went public. How about creating a public
sample_logic which is referenced from an internal one? That way it
would be easier to swap the underlying implementation. */
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
just drop it to put it somewhere in the main.c - KEJO

These are reusable utility methods which I think it not a part of the "core"
api, but something that a many consumers of a library would like to use so I'd
like to keep them. - Trygve
*/

enum slogic_recording_state {
	RUNNING = 0,
	COMPLETED_SUCCESSFULLY = 1,
	DEVICE_GONE = 2,
	TIMEOUT = 3,
	OVERFLOW = 4,
	UNKNOWN = 100,
};

void slogic_available_sample_rates(struct slogic_sample_rate **sample_rates_out,
				   size_t * size);

struct slogic_sample_rate *slogic_parse_sample_rate(const char *str);

/**
 * Contract between the main program and the utility library
 */
struct slogic_handle;

struct slogic_handle *slogic_open();
void slogic_close(struct slogic_handle *handle);

/* TODO: Should tune have a signature like this instead: 
 * slogic_tune(enum slogic_tunable, void* value)? It would definitely be more
 * future proof.
 *
 * Another option is struct slogic_tunable with a set of unions for each tunable.
 * 
 */
void slogic_tune(struct slogic_handle *handle,
		FILE* debug_file,
		size_t transfer_buffer_size,
		unsigned int n_transfer_buffers,
		unsigned int transfer_timeout,
		int libusb_debug_level);

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out);

typedef bool (*slogic_on_data_callback)(uint8_t* data, size_t size, void* user_data);

struct slogic_recording {
	struct slogic_sample_rate *sample_rate;
	slogic_on_data_callback on_data_callback;
	/* Updated by slogic when returning from the recording */
	enum slogic_recording_state recording_state;
	void* user_data;
	FILE* debug_file;
};

/*
 * A macro that fills in the required fields of a sampling recording.
 */
static inline void slogic_fill_recording(
		struct slogic_recording* recording, 
		struct slogic_sample_rate *sample_rate,
		slogic_on_data_callback on_data_callback,
		void *user_data) {
	recording->sample_rate = sample_rate;
	recording->on_data_callback = on_data_callback;
	recording->user_data = user_data;
}

void slogic_execute_recording(struct slogic_handle *handle,
		struct slogic_recording *recording);

#endif
