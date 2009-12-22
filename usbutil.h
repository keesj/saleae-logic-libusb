#include <libusb.h>

/* Iterates over the usb devices on the usb busses and returns a handle to the
 * first device found that matches the predefined vendor and product id
 */
libusb_device_handle *open_device(libusb_context * ctx, int vendor_id,
				  int product_id);

char *usbutil_error_to_string(enum libusb_error error);
