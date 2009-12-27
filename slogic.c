// vim: sw=8:ts=8:noexpandtab
#include "firmware/firmware.h"
#include "slogic.h"
#include "usbutil.h"

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

struct slogic_handle {
	/* pointer to the usb handle */
	libusb_device_handle *device_handle;
	libusb_context *context;
	size_t transfer_buffer_size;
	int n_transfer_buffers;
	unsigned int transfer_timeout;
	FILE *debug_file;
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
};

static const int n_sample_rates = sizeof(sample_rates) / sizeof(struct slogic_sample_rate);

void slogic_available_sample_rates(struct slogic_sample_rate **sample_rates_out, size_t *size)
{
	*sample_rates_out = sample_rates;
	*size = n_sample_rates;
}

struct slogic_sample_rate *slogic_parse_sample_rate(const char *str)
{
	int i;

	for (i = 0; i < n_sample_rates; i++) {
		struct slogic_sample_rate *sample_rate = &sample_rates[i];
		if (strcmp(sample_rate->text, str) == 0) {
			return sample_rate;
		}
	}

	return NULL;
}

/*
 * Open / Close
 */

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
static bool slogic_is_firmware_uploaded(struct slogic_handle *handle)
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
struct slogic_handle *slogic_open()
{
	struct slogic_handle *handle = malloc(sizeof(struct slogic_handle));
	assert(handle);

	handle->transfer_buffer_size = DEFAULT_TRANSFER_BUFFER_SIZE;
	handle->n_transfer_buffers = DEFAULT_N_TRANSFER_BUFFERS;
	handle->transfer_timeout = DEFAULT_TRANSFER_TIMEOUT;

	libusb_init(&handle->context);

	libusb_set_debug(handle->context, 3);

	handle->device_handle = open_device(handle->context, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle->device_handle) {
		return NULL;
	}
	if (!slogic_is_firmware_uploaded(handle)) {
		slogic_upload_firmware(handle);
		libusb_close(handle->device_handle);
		libusb_exit(handle->context);
		return NULL;
	}

	return handle;
}

void slogic_close(struct slogic_handle *handle)
{
	libusb_close(handle->device_handle);
	libusb_exit(handle->context);
	free(handle);
}

void slogic_tune(struct slogic_handle *handle, FILE *debug_file, size_t transfer_buffer_size,
		 unsigned int n_transfer_buffers, unsigned int transfer_timeout, int libusb_debug_level)
{
	assert(handle);

	/* TODO: Add validation that these values are sane. If not, don't modify but return an error. */

	if (debug_file) {
		handle->debug_file = debug_file;
	}

	if (transfer_buffer_size) {
		handle->transfer_buffer_size = transfer_buffer_size;
	}

	if (n_transfer_buffers) {
		handle->n_transfer_buffers = n_transfer_buffers;
	}

	if (transfer_timeout) {
		handle->transfer_timeout = transfer_timeout;
	}

	libusb_set_debug(handle->context, libusb_debug_level);
}

int slogic_readbyte(struct slogic_handle *handle, unsigned char *out)
{
	int ret;
	unsigned char command = 0x05;
	int transferred;

	ret = libusb_bulk_transfer(handle->device_handle, COMMAND_OUT_ENDPOINT, &command, 1, &transferred, 100);
	if (ret) {
		fprintf(handle->debug_file, "libusb_bulk_transfer (out): %s\n", usbutil_error_to_string(ret));
		return ret;
	}

	ret = libusb_bulk_transfer(handle->device_handle, COMMAND_IN_ENDPOINT, out, 1, &transferred, 100);
	if (ret) {
		fprintf(handle->debug_file, "libusb_bulk_transfer (in): %s\n", usbutil_error_to_string(ret));
		return ret;
	}
	return 0;
}

static int tcounter = 0;

/* TODO: Rename to slogic_transfer to be consistent - trygvis */
struct stransfer {
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

	struct stransfer *transfers;
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

	internal_recording->shandle = handle;

	internal_recording->n_transfer_buffers = handle->n_transfer_buffers;
	internal_recording->transfers = malloc(sizeof(struct stransfer) * internal_recording->n_transfer_buffers);

	internal_recording->done = false;

	return internal_recording;
}

static void free_internal_recording(struct slogic_internal_recording *internal_recording)
{
	free(internal_recording->transfers);
	free(internal_recording);
}

/*
 * Is some kind of synchronization required here? libusb is not supposed to
 * create its own threads, but I've seen mentions of an event thread in debug
 * output - Trygve
 */
void slogic_read_samples_callback(struct libusb_transfer *transfer)
{
	struct stransfer *stransfer = transfer->user_data;
	struct slogic_internal_recording *internal_recording = stransfer->internal_recording;
	struct slogic_recording *recording = internal_recording->recording;
	assert(stransfer);

	if (internal_recording->done) {
		/*
		 * This will happen if there was more incoming transfers when the
		 * callback wanted to stop recording. The outer method will handle
		 * the cleanup so just return here.
		 */
		return;
	}

	stransfer->internal_recording->transfer_counter++;

	/*
	 * Handle the success as a special case, the failure logic is basically: abort.
	 * Note that this does not indicate that the entire amount of requested data was transferred.
	 */
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		internal_recording->sample_count += transfer->actual_length;

		bool more =
		    recording->on_data_callback(transfer->buffer, transfer->actual_length, recording->user_data);

		if (!more) {
			internal_recording->recording->recording_state = COMPLETED_SUCCESSFULLY;
			internal_recording->done = true;
			fprintf(internal_recording->recording->debug_file, "Callback signalled completion\n");
			return;
		}

		int old_seq = stransfer->seq;
		stransfer->seq = tcounter++;
		int ret = libusb_submit_transfer(stransfer->transfer);
		if (ret) {
			fprintf(internal_recording->recording->debug_file, "libusb_submit_transfer: %s\n",
				usbutil_error_to_string(ret));
			internal_recording->recording->recording_state = UNKNOWN;
			internal_recording->done = true;
			return;
		}

		fprintf(internal_recording->recording->debug_file, "Rescheduled transfer %d as %d\n", old_seq,
			stransfer->seq);
		return;
	}

	internal_recording->done = true;

	fprintf(internal_recording->recording->debug_file, "Transfer failed: %s\n", libusb_transfer_status_to_string(transfer->status));

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
		internal_recording->recording->recording_state = UNKNOWN;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		internal_recording->recording->recording_state = TIMEOUT;
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		internal_recording->recording->recording_state = DEVICE_GONE;
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		internal_recording->recording->recording_state = OVERFLOW;
		break;
	}
}

void slogic_execute_recording(struct slogic_handle *handle, struct slogic_recording *recording)
{
	/* TODO: validate recording */

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

	fprintf(recording->debug_file, "Starting recording...\n");
	fprintf(recording->debug_file, "Transfer buffers:     %d\n", internal_recording->n_transfer_buffers);
	fprintf(recording->debug_file, "Transfer buffer size: %zu\n", handle->transfer_buffer_size);
	fprintf(recording->debug_file, "Transfer timeout:     %u\n", handle->transfer_timeout);

	/* Pre-allocate transfers */
	for (counter = 0; counter < internal_recording->n_transfer_buffers; counter++) {
		buffer = malloc(handle->transfer_buffer_size);
		assert(buffer);

		transfer = libusb_alloc_transfer(0);
		if (transfer == NULL) {
			fprintf(recording->debug_file, "libusb_alloc_transfer failed\n");
			recording->recording_state = UNKNOWN;
			return;
		}
		libusb_fill_bulk_transfer(transfer, handle->device_handle,
					  STREAMING_DATA_IN_ENDPOINT, buffer,
					  handle->transfer_buffer_size,
					  slogic_read_samples_callback,
					  &internal_recording->transfers[counter], handle->transfer_timeout);
		internal_recording->transfers[counter].internal_recording = internal_recording;
		internal_recording->transfers[counter].transfer = transfer;
	}

	/* Submit all transfers */
	for (counter = 0; counter < internal_recording->n_transfer_buffers; counter++) {
		internal_recording->transfers[counter].seq = tcounter++;
		ret = libusb_submit_transfer(internal_recording->transfers[counter].transfer);
		if (ret) {
			fprintf(recording->debug_file, "libusb_submit_transfer: %s\n", usbutil_error_to_string(ret));
			recording->recording_state = UNKNOWN;
			return;
		}
	}

	recording->recording_state = RUNNING;
	internal_recording->done = false;

	/* Switch the logic to streaming read mode */
	/* KEJO: pause as in delay between samples.. */
	fprintf(recording->debug_file, "pause=%d\n", recording->sample_rate->pause);
	unsigned char command[] = { 0x01, recording->sample_rate->pause };
	int transferred;
	ret = libusb_bulk_transfer(handle->device_handle, COMMAND_OUT_ENDPOINT, command, 2, &transferred, 100);
	if (ret) {
		fprintf(recording->debug_file, "libusb_bulk_transfer (set streaming read mode): %s\n",
			usbutil_error_to_string(ret));
		recording->recording_state = UNKNOWN;
		return;
	}
	assert(transferred == 2);

	struct timeval start;
	assert(gettimeofday(&start, NULL) == 0);

	struct timeval timeout = { 1, 0 };
	while (!internal_recording->done) {
		fprintf(recording->debug_file, "Processing events...\n");
		ret = libusb_handle_events_timeout(handle->context, &timeout);
		if (ret) {
			fprintf(recording->debug_file, "libusb_handle_events: %s\n", usbutil_error_to_string(ret));
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
		fprintf(recording->debug_file, "FAIL! recording_state=%d\n", internal_recording->recording->recording_state);
	} else {
		fprintf(recording->debug_file, "SUCCESS!\n");
	}

	fprintf(recording->debug_file, "Total number of samples read: %i\n", internal_recording->sample_count);
	fprintf(recording->debug_file, "Total number of transfers: %i\n", internal_recording->transfer_counter);

	int sec = end.tv_sec - start.tv_sec;
	int usec = (end.tv_usec - start.tv_usec) / 1000;
	if(usec < 0) {
		sec--;
		usec = 1 - usec;
	}
	fprintf(recording->debug_file, "Time elapsed: %d.%03ds\n", sec, usec);

	free_internal_recording(internal_recording);
}
