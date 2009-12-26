// vim: sw=8:ts=8:noexpandtab
#include <libusb.h>

/* Iterates over the usb devices on the usb busses and returns a handle to the
 * first device found that matches the predefined vendor and product id
 */
libusb_device_handle *open_device(libusb_context * ctx, int vendor_id, int product_id);

const char *libusb_transfer_status_to_string(enum libusb_transfer_status transfer_status);

/* TODO: Rename to libusb_error_to_string() */
const char *usbutil_error_to_string(enum libusb_error error);
