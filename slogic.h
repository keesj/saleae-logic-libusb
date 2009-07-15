#ifndef __SLOGIC_H__
#define __SLOGIC_H__

#include <malloc.h>
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

//#define BUFFER_SIZE 0x4000    /* 4K */
#define BUFFER_SIZE 512

/**
 * Contract between the main program and the utility library
 **/
struct slogic_handle {
    /* pointer to the usb handle */
    libusb_device_handle *device_handle;

    libusb_context *context;
};

void slogic_upload_firmware(struct slogic_handle *handle)
{
    int counter;
    unsigned int *current;
    int data_start;
    int cmd_size = 3;
    int cmd_count =
	sizeof(slogic_firm_cmds) / sizeof(*slogic_firm_cmds) / cmd_size;

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
				current[INDEX_PAYLOAD_SIZE], 500);
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
    int count;
    unsigned char out_byte = 0x05;
    unsigned char in_byte = 0x00;
    int transferred;
    count =
	libusb_bulk_transfer(handle->device_handle, 0x01, &out_byte, 1,
			     &transferred, 100);
    assert(count > 0);
    count =
	libusb_bulk_transfer(handle->device_handle, 0x01, &in_byte, 1,
			     &transferred, 100);
    //assert(count > 0 );
    return in_byte;
}

/*
 * slogic_read_samples reads 1800 samples using the streaming 
 * protocol. This methods is really a proof of concept as the
 * data is not exported because of the following reasons
 * -a libusb 0.x limitation we can not queue request upfront
 * -a libusb 0.x limitation we can not know the size of the tranfered data
 */
void slogic_read_samples(struct slogic_handle *handle)
{
    int counter;
    int count;
    int transferred;

    unsigned char *buffer;
    unsigned char cmd[2];

    buffer = malloc(BUFFER_SIZE);
    assert(buffer);

    cmd[0] = 0x01;
    cmd[1] = DELAY_FOR_250000;
//    cmd[1] = DELAY_FOR_8000000;
    for (counter = 0; counter < 2000; counter++) {
	count =
	    libusb_bulk_transfer(handle->device_handle, 0x02, buffer,
				 BUFFER_SIZE, &transferred, 1);
	if (counter == 200) {
	    libusb_bulk_transfer(handle->device_handle, 0x01, cmd, 2,
				 &transferred, 1);
	}
    }
}

#endif
