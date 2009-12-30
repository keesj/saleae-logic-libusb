// vim: sw=8:ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>
#include <stdbool.h>
#include <stdio.h>

struct slogic_sample_rate {
	const uint8_t sample_delay;	/* sample rates are translated into sampling delays */
	const char *text;	/* A descriptive text for the sample rate ("24MHz") */
	const unsigned int samples_per_second;
};

enum slogic_recording_state {
	WARMING_UP = 0,
	RUNNING = 1,
	COMPLETED_SUCCESSFULLY = 1,
	DEVICE_GONE = 2,
	TIMEOUT = 3,
	OVERFLOW = 4,
	UNKNOWN = 100,
};

/* returns an array of available sample rates. The array is terminates with an entry having a
0 sample_delay , a NULL text and a 0 samples_per_second */
struct slogic_sample_rate *slogic_get_sample_rates();
struct slogic_sample_rate *slogic_parse_sample_rate(const char *str);

/*
 * Contract between the main program and the utility library
 */
struct slogic_handle {
	/* pointer to the usb handle */
	libusb_device_handle *device_handle;
	libusb_context *context;
	/* TODO add doc about when we can change these values */
	size_t transfer_buffer_size;
	int n_transfer_buffers;
	unsigned int transfer_timeout;
};

struct slogic_handle *slogic_init();
int slogic_open(struct slogic_handle *handle);

void slogic_close(struct slogic_handle *handle);

bool slogic_is_firmware_uploaded(struct slogic_handle *handle);
void slogic_upload_firmware(struct slogic_handle *handle);

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out);

typedef bool(*slogic_on_data_callback) (uint8_t * data, size_t size, void *user_data);

struct slogic_recording {
	struct slogic_sample_rate *sample_rate;
	slogic_on_data_callback on_data_callback;
	/* Updated by slogic when returning from the recording */
	enum slogic_recording_state recording_state;
	void *user_data;
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

/* return 0 on success */
int slogic_execute_recording(struct slogic_handle *handle, struct slogic_recording *recording);

#endif
