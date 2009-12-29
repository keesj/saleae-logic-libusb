// vim: sw=8:ts=8:noexpandtab
#include "usbutil.h"

#include <stdio.h>

/*
 * Data structure debugging.
 */

static void dump_endpoint_descriptor(FILE * file, int i, const struct libusb_endpoint_descriptor *endpoint_descriptor)
{
	char *direction = ((endpoint_descriptor->bEndpointAddress & 0x80) == LIBUSB_ENDPOINT_IN) ? "in" : "out";

	fprintf(file, "     Endpoint #%d\n", i);
	fprintf(file, "      Address: %d, direction=%s\n", endpoint_descriptor->bEndpointAddress & 0x0f, direction);
	fprintf(file, "      Attributes: %02x\n", endpoint_descriptor->bmAttributes);
	fprintf(file, "      Max packet size: %u\n", endpoint_descriptor->wMaxPacketSize);
	fprintf(file, "      Poll interval: %d\n", endpoint_descriptor->bInterval);
	fprintf(file, "      Refresh: %d\n", endpoint_descriptor->bRefresh);
	fprintf(file, "      Sync address: %d\n", endpoint_descriptor->bSynchAddress);
}

static void dump_interface(FILE * file, int i, const struct libusb_interface *interface)
{
	fprintf(file, "  Interface #%d: Descriptors: (%d)\n", i, interface->num_altsetting);
	const struct libusb_interface_descriptor *interface_descriptor = interface->altsetting;
	int j, k;
	for (j = 0; j < interface->num_altsetting; j++, interface_descriptor++) {
		fprintf(file, "   Descriptor #%d:\n", j);
		fprintf(file, "    Interface class/sub-class: %d/%d\n",
			interface_descriptor->bInterfaceClass, interface_descriptor->bInterfaceSubClass);
		fprintf(file, "    Protocol: %d\n", interface_descriptor->bInterfaceProtocol);
		fprintf(file, "    Endpoints: (%d)\n", interface_descriptor->bNumEndpoints);
		const struct libusb_endpoint_descriptor
		*endpoint_descriptor = interface_descriptor->endpoint;
		for (k = 0; k < interface_descriptor->bNumEndpoints; k++, endpoint_descriptor++) {
			dump_endpoint_descriptor(file, k, endpoint_descriptor);
		}
	}
}

void usbutil_dump_config_descriptor(FILE * file, struct libusb_config_descriptor *config_descriptor)
{
	/* TODO: Decode bytes to strings */
	fprintf(file, "Configuration descriptor:\n");
	fprintf(file, " Configuration id: %d\n", config_descriptor->bConfigurationValue);
	fprintf(file, " Interfaces (%d):\n", config_descriptor->bNumInterfaces);

	const struct libusb_interface *interface = config_descriptor->interface;
	int i;
	for (i = 0; i < config_descriptor->bNumInterfaces; i++, interface++) {
		dump_interface(file, i, interface);
	}
}

void usbutil_dump_device_descriptor(FILE * file, struct libusb_device_descriptor *device_descriptor)
{
	/* TODO: Decode bytes to strings */
	fprintf(file, "Device descriptor:\n");
	fprintf(file, " Class/Sub-class: %04x/%04x\n", device_descriptor->bDeviceClass,
		device_descriptor->bDeviceSubClass);
	fprintf(file, " Protocol: %d\n", device_descriptor->bDeviceProtocol);
	fprintf(file, " Vendor id / product id: %04x / %04x\n", device_descriptor->idVendor,
		device_descriptor->idProduct);
	fprintf(file, " Number of possible configurations: %d\n", device_descriptor->bNumConfigurations);
}

/*
 * Open / Close
 */

/*
 * This method looks if the kernel already has a driver attached to the device. if so I will take over the device.
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
	fprintf(stderr, "libusb_set_configuration: %s\n", usbutil_error_to_string(err));

	/* claim interface */
	if ((err = libusb_claim_interface(dev, interface))) {
		fprintf(stderr, "Claim interface error: %s\n", usbutil_error_to_string(err));
		return err;
	}
	fprintf(stderr, "libusb_claim_interface: %s\n", usbutil_error_to_string(err));

	if ((err = libusb_set_interface_alt_setting(dev, interface, 0))) {
		fprintf(stderr, "libusb_set_interface_alt_setting: %s\n", usbutil_error_to_string(err));
		return err;
	}
	fprintf(stderr, "libusb_set_interface_alt_setting: %s\n", usbutil_error_to_string(err));

	return LIBUSB_SUCCESS;
}

/*
 * Iterates over the usb devices on the usb busses and returns a handle to the
 * first device found that matches the predefined vendor and product id
 */
libusb_device_handle *open_device(libusb_context * ctx, int vendor_id, int product_id)
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
		err = libusb_get_device_descriptor(device, &descriptor);
		if (err) {
			fprintf(stderr, "libusb_get_device_descriptor: %s\n", usbutil_error_to_string(err));
			libusb_free_device_list(list, 1);
			return NULL;
		}
		if ((descriptor.idVendor == vendor_id) && (descriptor.idProduct == product_id)) {
			found = device;
			usbutil_dump_device_descriptor(stderr, &descriptor);
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
	fprintf(stderr, "libusb_open: %s\n", usbutil_error_to_string(err));

	libusb_free_device_list(list, 1);

	if ((err = claim_device(device_handle, 0)) != 0) {
		fprintf(stderr, "Failed to claim the usb interface: %s\n", usbutil_error_to_string(err));
		return NULL;
	}

	struct libusb_config_descriptor *config_descriptor;
	err = libusb_get_active_config_descriptor(found, &config_descriptor);
	if (err) {
		fprintf(stderr, "libusb_get_active_config_descriptor: %s\n", usbutil_error_to_string(err));
		return NULL;
	}
	fprintf(stderr, "Active configuration:%d\n", config_descriptor->bConfigurationValue);
	libusb_free_config_descriptor(config_descriptor);

	fprintf(stderr, "Available configurations (%d):\n", descriptor.bNumConfigurations);
	for (i = 0; i < descriptor.bNumConfigurations; i++) {
		err = libusb_get_config_descriptor(found, i, &config_descriptor);
		if (err) {
			fprintf(stderr, "libusb_get_config_descriptor: %s\n", usbutil_error_to_string(err));
			return NULL;
		}

		usbutil_dump_config_descriptor(stderr, config_descriptor);
		libusb_free_config_descriptor(config_descriptor);
	}

	return device_handle;
}

const char *usbutil_transfer_status_to_string(enum libusb_transfer_status transfer_status)
{
	switch (transfer_status) {
	case LIBUSB_TRANSFER_COMPLETED:
		return "Completed.";
	case LIBUSB_TRANSFER_ERROR:
		return "Error.";
	case LIBUSB_TRANSFER_TIMED_OUT:
		return "Timeout.";
	case LIBUSB_TRANSFER_CANCELLED:
		return "Cancelled.";
	case LIBUSB_TRANSFER_STALL:
		return "Stalled.";
	case LIBUSB_TRANSFER_NO_DEVICE:
		return "No device.";
	case LIBUSB_TRANSFER_OVERFLOW:
		return "Overflow.";
	default:
		return "libusb_transfer_status: Unkown.";
	}
}

const char *usbutil_error_to_string(enum libusb_error error)
{
	switch (error) {
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
		return "libusb_error: Unknown error";
	}
}
