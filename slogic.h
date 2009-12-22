// vim: ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>

/* sleep time table */
enum slogic_sample_rate {
	sample_rate_24MHz = 1,
	sample_rate_16MHz = 2,
	sample_rate_12MHz = 3,
	sample_rate_8MHz = 5,
	sample_rate_4MHz = 11,
	sample_rate_2MHz = 24,
	sample_rate_1MHz = 47,
	sample_rate_500kHz = 95,
	sample_rate_250kHz = 191,
	sample_rate_200kHz = 239
};

#define BUFFER_SIZE 0x4000	/* 4K */

/**
 * Contract between the main program and the utility library
 **/
struct slogic_handle {
	/* pointer to the usb handle */
	libusb_device_handle *device_handle;
	libusb_context *context;
};

void slogic_upload_firmware(struct slogic_handle *handle);

/* return 1 if the firmware is uploaded 0 if not */
int slogic_is_firmware_uploaded(struct slogic_handle *handle);

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out);

/*
 * slogic_read_samples reads 1800 samples using the streaming 
 * protocol. This methods is really a proof of concept as the
 * data is not exported yet
 */
int slogic_read_samples(struct slogic_handle *handle,
			enum slogic_sample_rate sample_rate);

#endif
