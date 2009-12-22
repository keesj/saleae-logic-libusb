// vim: ts=8:noexpandtab
#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <libusb.h>
#include "firmware.h"

/* sleep time table */
#define DELAY_FOR_24000000   1
#define DELAY_FOR_16000000   2
#define DELAY_FOR_12000000   3
#define DELAY_FOR_8000000    5
#define DELAY_FOR_4000000   11
#define DELAY_FOR_2000000   24
#define DELAY_FOR_1000000   47
#define DELAY_FOR_500000    95
#define DELAY_FOR_250000   191
#define DELAY_FOR_200000   239

#define BUFFER_SIZE 0x4000	/* 4K */

#define TRANSFER_BUFFERS 4

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

char slogic_readbyte(struct slogic_handle *handle);

struct stransfer {
	struct libusb_transfer *transfer;
	int seq;
	struct slogic_handle *shandle;
};

void slogic_read_samples_callback_start_log(struct libusb_transfer
					    *transfer);

void slogic_read_samples_callback(struct libusb_transfer *transfer);

/*
 * slogic_read_samples reads 1800 samples using the streaming 
 * protocol. This methods is really a proof of concept as the
 * data is not exported yet
 */
void slogic_read_samples(struct slogic_handle *handle);

#endif
