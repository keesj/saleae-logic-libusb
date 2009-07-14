#include <usb.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>


/* This method looks if the kernel already has a driver attached
 * to the device. if so I will take over the device.
 */
static int takeover_device(usb_dev_handle * udev, int interface)
{
    char driver_name[255];

    memset(driver_name, 0, 255);
    int ret = -1;

    ret =
	usb_get_driver_np(udev, interface, driver_name,
			  sizeof(driver_name));
    if (ret == 0) {
	if (0 > usb_detach_kernel_driver_np(udev, interface)) {
	    fprintf(stderr, "Disconnect OS driver: %s\n", usb_strerror());
	} else {
	    fprintf(stderr, "Disconnected OS driver: %s\n",
		    usb_strerror());
	}
    }

    /* claim interface */
    if (usb_claim_interface(udev, interface) < 0) {
	fprintf(stderr, "Claim interface error: %s\n", usb_strerror());
	return -1;
    } else {
	usb_set_configuration(udev, 1);
	usb_set_altinterface(udev, interface);
    }
    return 0;
}

/* Iterates over the usb devices on the usb busses and returns a handle to the
 * first device found that matches the predefined vendor and product id
 */
usb_dev_handle *open_device(int vendor_id, int product_id)
{
    struct usb_bus *bus, *busses;
    struct usb_device *dev;
    usb_dev_handle *device_handle = '\0';

    usb_init();
    usb_find_busses();
    usb_find_devices();
    busses = usb_get_busses();

    for (bus = busses; bus; bus = bus->next) {
	for (dev = bus->devices; dev; dev = dev->next) {
	    if ((dev->descriptor.idVendor == vendor_id) &&
		(dev->descriptor.idProduct == product_id)) {
		device_handle = usb_open(dev);
		if (takeover_device(device_handle, 0) < 0) {
		    fprintf(stderr, "Failed to claim the usb interface\n");
		    return NULL;
		}
		return device_handle;
	    }
	}
    }
    return NULL;
}
