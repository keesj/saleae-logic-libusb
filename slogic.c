#include "slogic.h"
#include "usbutil.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

void slogic_upload_firmware(struct slogic_handle *handle)
{
    int counter;
    unsigned int *current;
    int data_start;
    int cmd_size = 3;
    int cmd_count =
	slogic_firm_cmds_size() / sizeof(*slogic_firm_cmds) / cmd_size;

    current = slogic_firm_cmds;
    data_start = 0;
    for (counter = 0; counter < cmd_count; counter++) {
	/* int libusb_control_transfer(libusb_device_handle *dev_handle,
	 * int8_t request_type, uint8_t request, uint16_t value, uint16_t index,
	 * unsigned char *data, uint16_t length, unsigned int timeout); 
	 */
	libusb_control_transfer(handle->device_handle, 0x40, 0XA0,
				current[INDEX_CMD_REQUEST],
				current[INDEX_CMD_VALUE],
				&slogic_firm_data[data_start],
				current[INDEX_PAYLOAD_SIZE], 4);
	data_start += current[2];
	current += cmd_size;
    }
    sleep(1);
}

/* return 1 if the firmware is uploaded 0 if not */
int slogic_is_firmware_uploaded(struct slogic_handle *handle)
{
    /* just try to perform a normal read, if this fails we assume the firmware is not uploaded */
    int count;
    unsigned char out_byte = 0x05;
    int transferred;
    count =
	libusb_bulk_transfer(handle->device_handle, 0x01, &out_byte, 1,
			     &transferred, 100);
    if (count < 0) {
	return 0;
    }
    return 1;			/* probably the firmware is uploaded */
}

char slogic_readbyte(struct slogic_handle *handle)
{
    int ret;
    unsigned char out_byte = 0x05;
    unsigned char in_byte = 0x00;
    int transferred;
    ret =
	libusb_bulk_transfer(handle->device_handle, 0x01, &out_byte, 1,
			     &transferred, 100);
    if(ret) {
        fprintf(stderr, "libusb_bulk_transfer: %s\n", usbutil_error_to_string(ret));
    }
    assert(ret == 0);
    ret =
	libusb_bulk_transfer(handle->device_handle,
			     0x01 | LIBUSB_ENDPOINT_IN, &in_byte, 1,
			     &transferred, 100);
    //assert(count > 0 );
    return in_byte;
}

static int tcounter = 0;
static struct stransfer transfers[TRANSFER_BUFFERS];

/* keep track of the amount of transfeered data */
static int sample_counter = 0;

void slogic_read_samples_callback_start_log(struct libusb_transfer
					    *transfer)
{
    printf("start log\n");
    libusb_free_transfer(transfer);
    /* free data? */
};

void slogic_read_samples_callback(struct libusb_transfer *transfer)
{
    struct stransfer *stransfer = transfer->user_data;
    assert(stransfer);
    if (tcounter == 200) {
	struct libusb_transfer *transfer;
	unsigned char cmd[2];
	cmd[0] = 0x01;
	//cmd[1] = DELAY_FOR_12000000;
	//cmd[1] = DELAY_FOR_8000000;
	cmd[1] = DELAY_FOR_4000000;
	//cmd[1] = DELAY_FOR_200000;
	transfer = libusb_alloc_transfer(0 /* we use bulk */ );
	assert(transfer);
	libusb_fill_bulk_transfer(transfer,
				  stransfer->shandle->device_handle, 0x01,
				  cmd, 2,
				  slogic_read_samples_callback_start_log,
				  NULL, 10);
	libusb_submit_transfer(transfer);
    }

    sample_counter += transfer->actual_length;
#if 0
    if (tcounter < 2000) {
#endif
	stransfer->seq = tcounter++;
	libusb_submit_transfer(stransfer->transfer);
#if 0
    } else {
	libusb_free_transfer(transfer);
	/* free data? */
    }
#endif
}

/*
 * slogic_read_samples reads 1800 samples using the streaming 
 * protocol. This methods is really a proof of concept as the
 * data is not exported yet
 */
void slogic_read_samples(struct slogic_handle *handle)
{
    struct libusb_transfer *transfer;
    unsigned char *buffer;
    int counter;

    for (counter = 0; counter < TRANSFER_BUFFERS; counter++) {
	buffer = malloc(BUFFER_SIZE);
	assert(buffer);

	transfer = libusb_alloc_transfer(0 /* we use bulk */ );
	assert(transfer);
	libusb_fill_bulk_transfer(transfer, handle->device_handle,
				  0x02 | LIBUSB_ENDPOINT_IN, buffer,
				  BUFFER_SIZE,
				  slogic_read_samples_callback,
				  &transfers[counter], 4);
	transfers[counter].transfer = transfer;
	transfers[counter].shandle = handle;
    }

    for (counter = 0; counter < TRANSFER_BUFFERS; counter++) {
	transfers[counter].seq = tcounter++;
	libusb_submit_transfer(transfers[counter].transfer);
    }

    while (tcounter < 20000 - TRANSFER_BUFFERS) {
	libusb_handle_events(handle->context);
    }
    printf("Total number of Samples red is %i\n", sample_counter);
}
