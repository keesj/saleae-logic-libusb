#include "usbutil.h"

#include <stdio.h>

/* This method looks if the kernel already has a driver attached
 * to the device. if so I will take over the device.
 */
static enum libusb_error claim_device(libusb_device_handle * dev, int interface)
{
    enum libusb_error err;
    if (libusb_kernel_driver_active(dev, interface)) {
        fprintf(stderr, "A kernel has claimed the interface, detaching it...\n");
	if ((err = libusb_detach_kernel_driver(dev, interface)) != 0) {
            fprintf(stderr, "Failed to Disconnected the OS driver: %s\n", usbutil_error_to_string(err));
            return err;
	}
    }

    if ((err = libusb_set_configuration(dev, 1))) {
	fprintf(stderr, "libusb_set_configuration: %s\n", usbutil_error_to_string(err));
	return err;
    }

    /* claim interface */
    if ((err = libusb_claim_interface(dev, interface))) {
	fprintf(stderr, "Claim interface error: %s\n", usbutil_error_to_string(err));
	return err;
    }

    libusb_set_interface_alt_setting(dev, interface, 0);

    return LIBUSB_SUCCESS;
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

    if (!found) {
	fprintf(stderr, "Device not found\n");
        libusb_free_device_list(list, 1);
        return NULL;
    }

    if ((err = libusb_open(found, &device_handle))) {
        fprintf(stderr, "Failed OPEN the device: %s\n", usbutil_error_to_string(err));
        libusb_free_device_list(list, 1);
        return NULL;
    }

    libusb_free_device_list(list, 1);

    if ((err = claim_device(device_handle, 0)) != 0) {
        fprintf(stderr, "Failed to claim the usb interface: %s\n", usbutil_error_to_string(err));
        return NULL;
    }

    return device_handle;
}

char* usbutil_error_to_string(enum libusb_error error) {
    switch(error) {
        case LIBUSB_SUCCESS:
            return "LIBUSB_SUCCESS";
        case LIBUSB_ERROR_IO:
            return "LIBUSB_ERROR_IO";
        case LIBUSB_ERROR_INVALID_PARAM:
            return "LIBUSB_ERROR_INVALID_PARAM";
        case LIBUSB_ERROR_ACCESS:
            return "LIBUSB_ERROR_ACCESS";
        case LIBUSB_ERROR_NO_DEVICE:
            return "LIBUSB_ERROR_NO_DEVICE";
        case LIBUSB_ERROR_NOT_FOUND:
            return "LIBUSB_ERROR_NOT_FOUND";
        case LIBUSB_ERROR_BUSY:
            return "LIBUSB_ERROR_BUSY";
        case LIBUSB_ERROR_TIMEOUT:
            return "LIBUSB_ERROR_TIMEOUT";
        case LIBUSB_ERROR_OVERFLOW:
            return "LIBUSB_ERROR_OVERFLOW";
        case LIBUSB_ERROR_PIPE:
            return "LIBUSB_ERROR_PIPE";
        case LIBUSB_ERROR_INTERRUPTED:
            return "LIBUSB_ERROR_INTERRUPTED";
        case LIBUSB_ERROR_NO_MEM:
            return "LIBUSB_ERROR_NO_MEM";
        case LIBUSB_ERROR_NOT_SUPPORTED:
            return "LIBUSB_ERROR_NOT_SUPPORTED";
        case LIBUSB_ERROR_OTHER:
            return "LIBUSB_ERROR_OTHER";
        default:
            return "Unknown error";
    }
}
