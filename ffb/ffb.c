#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/completion.h>

/**
 * When a T150 is connected, it presents itself as a Thrustmaster FFB Wheel.
 * This driver immediately sends a specific control packet that makes the wheel
 * to disconnect and re-connect as a T150.
 *
 * The same effect can be achieved by using a program in the userspace, like
 * withe a Python script
 */

#define USB_THRUSTMASTER_VENDOR_ID	0x044f
#define USB_T150_PRODUCT_ID		0xb65d

/** Copied from the Windows driver beheivor */
struct usb_ctrlrequest full_func =
{
	.bRequestType = 0x41,
	.bRequest = 83,
	.wValue = cpu_to_le16(0x0006),
	.wIndex = 0,
	.wLength = 0
};

struct urb *request_full_func;

static void wheel_callback(struct urb *urb)
{
	printk(KERN_INFO "Ò fatto la richiesta, ottenuta risposta!\n");
}

char fake_buffer[16] = {0};
static int wheel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int error_code = 0;
	struct usb_device *device = interface_to_usbdev(interface);
	struct completion wheel_config_done; init_completion(&wheel_config_done);

	printk(KERN_INFO "Wheel (%04X:%04X) plugged\n", id->idVendor, id->idProduct);

	request_full_func = usb_alloc_urb(0, GFP_KERNEL);
	if(!request_full_func)
		return -EIO;

	/** Send the request to the Wheel*/
	usb_fill_control_urb(
		request_full_func,
		device,
		usb_sndctrlpipe(device, 0),
		(char*)&full_func,
		fake_buffer, 16,
		wheel_callback,
		(void *)&wheel_config_done
	);

	error_code = usb_submit_urb(request_full_func, GFP_KERNEL);
	printk(KERN_INFO "Errore: %d", error_code);

	if(error_code)
		goto error;

	return 0;
error:	usb_free_urb(request_full_func);
	return error_code;
}

static void wheel_disconnect(struct usb_interface *interface)
{
	usb_kill_urb(request_full_func);
	usb_free_urb(request_full_func);
	printk(KERN_INFO "Wheel removed. Bye\n");
}

static struct usb_device_id wheel_table[] =
{
	{ USB_DEVICE(USB_THRUSTMASTER_VENDOR_ID, USB_T150_PRODUCT_ID) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, wheel_table);

static struct usb_driver wheel_driver =
{
	.name = "ffb",
	.id_table = wheel_table,
	.probe = wheel_probe,
	.disconnect = wheel_disconnect,
};

static int __init wheel_init(void)
{
	return usb_register(&wheel_driver);
}

static void __exit wheel_exit(void)
{
	usb_deregister(&wheel_driver);
}

module_init(wheel_init);
module_exit(wheel_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Pagani <>");
MODULE_DESCRIPTION("ThrustMaster T150 steering wheel driver");
