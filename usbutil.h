#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <libusb.h>

/* This method looks if the kernel already has a driver attached
 * to the device. if so I will take over the device.
 */
static int takeover_device(libusb_device_handle * dev, int interface)
{
	if (libusb_kernel_driver_active(dev, interface)) {
		if (!libusb_detach_kernel_driver(dev, interface)) {
			fprintf(stderr,
				"Failed to Disconnected the OS driver: n");
		}
	}

	/* claim interface */
	if (libusb_claim_interface(dev, interface) != 0) {
		fprintf(stderr, "Claim interface error:\n");
		return -1;
	} else {
		libusb_set_configuration(dev, 1);
		libusb_set_interface_alt_setting(dev, interface, 0);
	}
	return 0;
}

/* Iterates over the usb devices on the usb busses and returns a handle to the
 * first device found that matches the predefined vendor and product id
 */
libusb_device_handle *open_device(libusb_context * ctx, int vendor_id,
				  int product_id)
{

	// discover devices
	libusb_device **list;
	libusb_device *found = NULL;
	libusb_device_handle *device_handle = NULL;
	struct libusb_device_descriptor descriptor;

	size_t cnt = libusb_get_device_list(ctx, &list);
	size_t i = 0;
	int err = 0;
	if (cnt < 0) {
		fprintf(stderr, "Failed to get a list of devices\n");
		return NULL;
	}

	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		libusb_get_device_descriptor(device, &descriptor);
		if ((descriptor.idVendor == vendor_id) &&
		    (descriptor.idProduct == product_id)) {
			found = device;
			break;
		}
	}

	if (found) {
		err = libusb_open(found, &device_handle);
		if (err) {
			fprintf(stderr,
				"Failed OPEN the device libusb error %i\n",
				err);
			return NULL;
		}
		if (takeover_device(device_handle, 0) < 0) {
			fprintf(stderr, "Failed to claim the usb interface\n");
			return NULL;
		}
	} else {
		fprintf(stderr, "Device not found\n");
	}

	libusb_free_device_list(list, 1);

	return device_handle;
}
