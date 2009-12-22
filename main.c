// vim: ts=8:noexpandtab
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include <libusb.h>
#include "slogic.h"
#include "usbutil.h"

//Bus 006 Device 006: ID 0925:3881 Lakeview Research
#define USB_VENDOR_ID 0x0925
#define USB_PRODUCT_ID 0x3881

int main(int argc, char **argv)
{
	struct slogic_handle handle;

	libusb_init(&handle.context);

	libusb_set_debug(handle.context, 3);

	handle.device_handle =
	    open_device(handle.context, USB_VENDOR_ID, USB_PRODUCT_ID);
	if (!handle.device_handle) {
		fprintf(stderr, "Failed to find the device\n");
		return -1;
	}
//    if (!slogic_is_firmware_uploaded(&handle)) {
//      printf("Uploading firmware restart program\n");
//      slogic_upload_firmware(&handle);
//      libusb_close(handle.device_handle);
//      libusb_exit(handle.context);
//      return -1;
//    }
	/* apparently one need to at least read once before the driver continues */
	slogic_readbyte(&handle);

	slogic_read_samples(&handle);
	printf("0x%2x\n", slogic_readbyte(&handle));

	libusb_close(handle.device_handle);
	libusb_exit(handle.context);
	return 0;
}
