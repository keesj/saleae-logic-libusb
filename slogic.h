// vim: sw=8:ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>
#include <stdbool.h>
#include <stdio.h>

struct slogic_sample_rate {
	const uint8_t pause;	/* sample rates are translated into delays */
	const char *text;	/* A descriptive text for the sample rate ("24MHz") */
	const unsigned int samples_per_second;
};

enum slogic_recording_state {
	RUNNING = 0,
	COMPLETED_SUCCESSFULLY = 1,
	DEVICE_GONE = 2,
	TIMEOUT = 3,
	OVERFLOW = 4,
	UNKNOWN = 100,
};

/*
 * I would really dump the tow method bellow, it not "critical" to the
 * lib so just drop it to put it somewhere in the main.c - KEJO
 *
 * These are reusable utility methods which I think it not a part of
 * the "core" api, but something that a many consumers of a library
 * would like to use so I'd like to keep them. - Trygve
 *
 * The problem here is that some it gets hard to hardcode a sample
 * rate you need to iterate over the sample rates of use what you call
 * a utility method. - KEJO
 *
 * How about adding this:
 *
 * struct slogic_sample_rate *slogic_get_sample_rate(unsigned int samples_per_second);
 *
 * That way you could go like this in your code to get a sample reate
 * of 1MHz:
 *
 * slogic_get_sample_rate(1000000);
 *
 * - Trygve
 */

void slogic_available_sample_rates(struct slogic_sample_rate **sample_rates_out, size_t *size);

struct slogic_sample_rate *slogic_parse_sample_rate(const char *str);

/*
 * Contract between the main program and the utility library
 */
struct slogic_handle;

struct slogic_handle *slogic_open();
void slogic_close(struct slogic_handle *handle);

/*
 * TODO: Should tune have a signature like this instead:
 *
 *     slogic_tune(enum slogic_tunable, void* value)?
 *
 * It would definitely be more future proof. Another option is
 * struct slogic_tunable with a set of unions for each tunable.
 */
void slogic_tune(struct slogic_handle *handle,
		 FILE *debug_file,
		 size_t transfer_buffer_size,
		 unsigned int n_transfer_buffers, unsigned int transfer_timeout, int libusb_debug_level);

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out);

typedef bool(*slogic_on_data_callback) (uint8_t *data, size_t size, void *user_data);

struct slogic_recording {
	struct slogic_sample_rate *sample_rate;
	slogic_on_data_callback on_data_callback;
	/* Updated by slogic when returning from the recording */
	enum slogic_recording_state recording_state;
	void *user_data;
	FILE *debug_file;
};

/*
 * A macro that fills in the required fields of a sampling recording.
 */
static inline void slogic_fill_recording(struct slogic_recording *recording,
					 struct slogic_sample_rate *sample_rate,
					 slogic_on_data_callback on_data_callback, void *user_data)
{
	recording->sample_rate = sample_rate;
	recording->on_data_callback = on_data_callback;
	recording->user_data = user_data;
}

void slogic_execute_recording(struct slogic_handle *handle, struct slogic_recording *recording);

#endif
