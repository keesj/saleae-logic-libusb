// vim: sw=8:ts=8:noexpandtab
#include "firmware/firmware.h"
#include "slogic.h"
#include "usbutil.h"
#include "log.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_N_TRANSFER_BUFFERS 4
#define DEFAULT_TRANSFER_BUFFER_SIZE (4 * 1024)
#define DEFAULT_TRANSFER_TIMEOUT 1000

/*
 * define EP1 OUT , EP1 IN, EP2 IN and EP6 OUT
 */
#define COMMAND_OUT_ENDPOINT 0x01
#define COMMAND_IN_ENDPOINT 0x81
#define STREAMING_DATA_IN_ENDPOINT 0x82
#define STREAMING_DATA_OUT_ENDPOINT 0x06

/* Bus 006 Device 006: ID 0925:3881 Lakeview Research */
#define USB_VENDOR_ID 0x0925
#define USB_PRODUCT_ID 0x3881

static struct logger logger = {
	.name = __FILE__,
	.verbose = 0,
};

/*
 * Sample Rates
 */
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
	{0, NULL, 0},
};

struct slogic_sample_rate *slogic_get_sample_rates()
{
	return sample_rates;
}

struct slogic_sample_rate *slogic_parse_sample_rate(const char *str)
{
	struct slogic_sample_rate *sample_rate = slogic_get_sample_rates();
	while (sample_rate->text != NULL) {
		if (strcmp(sample_rate->text, str) == 0) {
			return sample_rate;
		}
		sample_rate++;
	}
	return NULL;
}

void slogic_upload_firmware(struct slogic_handle *handle)
{
	int counter;
	unsigned int *current;
	int data_start;
	int cmd_size = 3;
	int cmd_count = slogic_firm_cmds_size() / sizeof(*slogic_firm_cmds) / cmd_size;
	int timeout = 4;

	current = slogic_firm_cmds;
	data_start = 0;
	for (counter = 0; counter < cmd_count; counter++) {
		libusb_control_transfer(handle->device_handle, 0x40, 0XA0,
					current[INDEX_CMD_REQUEST],
					current[INDEX_CMD_VALUE],
					&slogic_firm_data[data_start], current[INDEX_PAYLOAD_SIZE], timeout);
		data_start += current[2];
		current += cmd_size;
	}
	sleep(1);
}

/* return 1 if the firmware is uploaded 0 if not */
bool slogic_is_firmware_uploaded(struct slogic_handle *handle)
{
	/* just try to perform a normal read, if this fails we assume the firmware is not uploaded */
	unsigned char out_byte = 0x05;
	int transferred;
	int ret = libusb_bulk_transfer(handle->device_handle, COMMAND_OUT_ENDPOINT, &out_byte, 1, &transferred, 100);
	return ret == 0;	/* probably the firmware is uploaded */
}

/*
 * TODO: An error code is probably required here as there can be many reasons
 * to why open() fails. The handle should probably be an argument passed as
 * a pointer to a pointer.
 */
struct slogic_handle *slogic_init()
{
	struct slogic_handle *handle = malloc(sizeof(struct slogic_handle));
	assert(handle);

	handle->transfer_buffer_size = DEFAULT_TRANSFER_BUFFER_SIZE;
	handle->n_transfer_buffers = DEFAULT_N_TRANSFER_BUFFERS;
	handle->transfer_timeout = DEFAULT_TRANSFER_TIMEOUT;
	libusb_init(&handle->context);

	return handle;
}

int slogic_open(struct slogic_handle *handle)
{
	handle->device_handle = open_device(handle->context, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle->device_handle) {
		log_printf(&logger, ERR, "Failed to open the device\n");
		return -1;
	}
	return 0;
}

void slogic_close(struct slogic_handle *handle)
{
	libusb_close(handle->device_handle);
	libusb_exit(handle->context);
	free(handle);
}

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out)
{
	int ret;
	unsigned char command = 0x05;
	int transferred;

	ret = libusb_bulk_transfer(handle->device_handle, COMMAND_OUT_ENDPOINT, &command, 1, &transferred, 100);
	if (ret) {
		log_printf(&logger, ERR, "libusb_bulk_transfer (out): %s\n", usbutil_error_to_string(ret));
		return ret;
	}

	ret = libusb_bulk_transfer(handle->device_handle, COMMAND_IN_ENDPOINT, out, 1, &transferred, 100);
	if (ret) {
		log_printf(&logger, ERR, "libusb_bulk_transfer (in): %s\n", usbutil_error_to_string(ret));
		return ret;
	}
	return 0;
}

static int tcounter = 0;

/* TODO: Rename to slogic_transfer to be consistent - trygvis */
struct slogic_transfer {
	struct slogic_internal_recording *internal_recording;
	struct libusb_transfer *transfer;
	int seq;
};

struct slogic_internal_recording {
	/* A reference to the user's part of the recording */
	struct slogic_recording *recording;

	/* Number of samples collected so far */
	int sample_count;

	struct slogic_handle *shandle;
	/* Number of USB transfers */
	int transfer_counter;
	int timeout_counter;

	struct slogic_transfer *transfers;
	unsigned int n_transfer_buffers;

	bool done;
};

static struct slogic_internal_recording *allocate_internal_recording(struct slogic_handle *handle,
								     struct slogic_recording *recording)
{
	struct slogic_internal_recording *internal_recording = malloc(sizeof(struct slogic_internal_recording));
	assert(internal_recording);

	internal_recording->recording = recording;
	internal_recording->transfer_counter = 0;
	internal_recording->timeout_counter = 0;

	internal_recording->shandle = handle;

	internal_recording->n_transfer_buffers = handle->n_transfer_buffers;
	internal_recording->transfers = malloc(sizeof(struct slogic_transfer) * internal_recording->n_transfer_buffers);

	internal_recording->done = false;

	return internal_recording;
}

static void free_internal_recording(struct slogic_internal_recording *internal_recording)
{
	free(internal_recording->transfers);
	free(internal_recording);
}

void slogic_read_samples_callback_start_log(struct libusb_transfer *transfer)
{
	struct slogic_recording *recording = transfer->user_data;
	printf("start log\n");
	recording->recording_state = RUNNING;
	libusb_free_transfer(transfer);
	/* free data? */
};

/*
 * Is some kind of synchronization required here? libusb is not supposed to
 * create its own threads, but I've seen mentions of an event thread in debug
 * output - Trygve
 */
void slogic_read_samples_callback(struct libusb_transfer *transfer)
{
	struct slogic_transfer *slogic_transfer = transfer->user_data;
	struct slogic_internal_recording *internal_recording = slogic_transfer->internal_recording;
	struct slogic_recording *recording = internal_recording->recording;
	assert(slogic_transfer);

	if (internal_recording->done) {
		/*
		 * This will happen if there was more incoming transfers when the
		 * callback wanted to stop recording. The outer method will handle
		 * the cleanup so just return here.
		 */
		return;
	}

	slogic_transfer->internal_recording->transfer_counter++;

	/*
	 * Handle the success as a special case, the failure logic is basically: abort.
	 * Note that this does not indicate that the entire amount of requested data was transferred.
	 */
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED || recording->recording_state == RUNNING) {
		internal_recording->sample_count += transfer->actual_length;

		bool more =
		    recording->on_data_callback(transfer->buffer, transfer->actual_length, recording->user_data);

		if (!more) {
			internal_recording->recording->recording_state = COMPLETED_SUCCESSFULLY;
			internal_recording->done = true;
			log_printf(&logger, DEBUG, "Callback signalled completion\n");
			return;
		}

		int old_seq = slogic_transfer->seq;
		slogic_transfer->seq = tcounter++;
		int ret = libusb_submit_transfer(slogic_transfer->transfer);
		if (ret) {
			log_printf(&logger, ERR, "libusb_submit_transfer: %s\n", usbutil_error_to_string(ret));
			internal_recording->recording->recording_state = UNKNOWN;
			internal_recording->done = true;
			return;
		}

		log_printf(&logger, DEBUG, "Rescheduled transfer %d as %d\n", old_seq, slogic_transfer->seq);
		return;
	}
	
	if (internal_recording->transfer_counter == 200) {
			struct libusb_transfer *transfer;
			unsigned char command[] = { 0x01, recording->sample_rate->sample_delay };
			transfer = libusb_alloc_transfer(0 /* we use bulk */ );
			assert(transfer);
			libusb_fill_bulk_transfer(transfer,
						  internal_recording->shandle->device_handle, 0x01,
						  command, 2, slogic_read_samples_callback_start_log, recording, 40);
			libusb_submit_transfer(transfer);
	}
	if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT) {
		if (recording->recording_state == RUNNING){
			slogic_transfer->internal_recording->timeout_counter++;
		}
		if (internal_recording->timeout_counter < 1000){
			slogic_transfer->seq = tcounter++;
			int ret = libusb_submit_transfer(slogic_transfer->transfer);
			if (ret) {
				log_printf(&logger, ERR, "libusb_submit_transfer: %s\n", usbutil_error_to_string(ret));
				internal_recording->recording->recording_state = UNKNOWN;
				internal_recording->done = true;
			}
			return;
		}
	}

	internal_recording->done = true;

	log_printf(&logger, ERR, "Transfer failed: %s\n", usbutil_transfer_status_to_string(transfer->status));

	switch (transfer->status) {
	default:
		/* Make the compiler shut up */
	case LIBUSB_TRANSFER_COMPLETED:
		/*
		 * This shouldn't happen in slogic.
		 * From libusb docs:
		 * For bulk/interrupt endpoints: halt condition detected (endpoint stalled).
		 * For control endpoints: control request not supported.
		 */
	case LIBUSB_TRANSFER_STALL:
		/* We don't cancel transfers without setting should_run = 0 so this should not happen */
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_ERROR:
		recording->recording_state = UNKNOWN;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		recording->recording_state = TIMEOUT;
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		recording->recording_state = DEVICE_GONE;
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		recording->recording_state = OVERFLOW;
		break;
	}
}

int slogic_execute_recording(struct slogic_handle *handle, struct slogic_recording *recording)
{
	/* TODO: validate recording */
	int retval =0;

	struct libusb_transfer *transfer;
	unsigned char *buffer;
	int counter;
	int ret;

	struct slogic_internal_recording *internal_recording = allocate_internal_recording(handle, recording);

	/*
	 * TODO: We probably want to tune the transfer buffer size to a sane
	 * default based on the sampling rate. If transfer_buffer_size is
	 * samples per second / 10 we can expect 10 transfers per second which
	 * should be sufficient for a user application to be responsive while
	 * not having too many transfers per second.
	 *  - Trygve
	 */

	log_printf(&logger, DEBUG, "Starting recording...\n");
	log_printf(&logger, DEBUG, "Transfer buffers:     %d\n", internal_recording->n_transfer_buffers);
	log_printf(&logger, DEBUG, "Transfer buffer size: %zu\n", handle->transfer_buffer_size);
	log_printf(&logger, DEBUG, "Transfer timeout:     %u\n", handle->transfer_timeout);

	/* Pre-allocate transfers */
	for (counter = 0; counter < internal_recording->n_transfer_buffers; counter++) {
		buffer = malloc(handle->transfer_buffer_size);
		assert(buffer);

		transfer = libusb_alloc_transfer(0);
		if (transfer == NULL) {
			log_printf(&logger, ERR, "libusb_alloc_transfer failed\n");
			recording->recording_state = UNKNOWN;
			retval = 1;
			return retval;
		}
		libusb_fill_bulk_transfer(transfer, handle->device_handle,
					  STREAMING_DATA_IN_ENDPOINT, buffer,
					  handle->transfer_buffer_size,
					  slogic_read_samples_callback,
					  &internal_recording->transfers[counter], handle->transfer_timeout);
		internal_recording->transfers[counter].internal_recording = internal_recording;
		internal_recording->transfers[counter].transfer = transfer;
	}

	recording->recording_state = WARMING_UP;
	internal_recording->done = false;

	/* Submit all transfers */
	for (counter = 0; counter < internal_recording->n_transfer_buffers; counter++) {
		internal_recording->transfers[counter].seq = tcounter++;
		ret = libusb_submit_transfer(internal_recording->transfers[counter].transfer);
		if (ret) {
			log_printf(&logger, ERR, "libusb_submit_transfer: %s\n", usbutil_error_to_string(ret));
			recording->recording_state = UNKNOWN;
			retval =1;
			return retval;
		}
	}

	log_printf(&logger, DEBUG, "sample_delay=%d\n", recording->sample_rate->sample_delay);

	struct timeval start;
	assert(gettimeofday(&start, NULL) == 0);

	struct timeval timeout = { 1, 0 };
	while (!internal_recording->done) {
		ret = libusb_handle_events_timeout(handle->context, &timeout);
		if (ret) {
			log_printf(&logger, ERR, "libusb_handle_events: %s\n", usbutil_error_to_string(ret));
			break;
		}
	}

	struct timeval end;
	assert(gettimeofday(&end, NULL) == 0);

	for (counter = 0; counter < internal_recording->n_transfer_buffers; counter++) {
		libusb_cancel_transfer(internal_recording->transfers[counter].transfer);
		libusb_free_transfer(internal_recording->transfers[counter].transfer);
		internal_recording->transfers[counter].transfer = NULL;
	}

	if (internal_recording->recording->recording_state != COMPLETED_SUCCESSFULLY) {
		log_printf(&logger, ERR, "FAIL! recording_state=%d\n", internal_recording->recording->recording_state);
		retval =1;
	} else {
		log_printf(&logger, DEBUG, "SUCCESS!\n");
	}

	log_printf(&logger, DEBUG, "Total number of samples read: %i\n", internal_recording->sample_count);
	log_printf(&logger, DEBUG, "Total number of transfers: %i\n", internal_recording->transfer_counter);

	int sec = end.tv_sec - start.tv_sec;
	int usec = (end.tv_usec - start.tv_usec) / 1000;
	if (usec < 0) {
		sec--;
		usec = 1 - usec;
	}
	log_printf(&logger, DEBUG, "Time elapsed: %d.%03ds\n", sec, usec);

	free_internal_recording(internal_recording);
	return retval;
}
