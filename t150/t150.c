#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/completion.h>
#include <linux/input.h>
#include <linux/usb/input.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/delay.h>

#include "t150.h"
//#include "magic.h"
#include "input.h"
#include "attributes.h"
#include "settings.h"
#include "forcefeedback.h"

/** Init for a t150 data struct
 * @param t150 pointer to the t150 structor to init
 * @param interface pointer to usb interface which the wheel is connected to
 */
static inline int t150_constructor(struct t150 *t150,struct usb_interface *interface)
{
	int i, error_code = 0;
	struct usb_endpoint_descriptor *ep, *ep_irq_in = 0, *ep_irq_out = 0;

	t150->usb_device = interface_to_usbdev(interface);

	// Path used for the input subsystem
	usb_make_path(t150->usb_device, t150->dev_path, sizeof(t150->dev_path));
	strlcat(t150->dev_path, "/input0", sizeof(t150->dev_path));

	// TODO CHECK FOR MEMORY FAIL
	t150->joy_request_in = usb_alloc_urb(0, GFP_KERNEL);
	t150->joy_request_out = usb_alloc_urb(0, GFP_KERNEL);

	t150->joy_data_in = kzalloc(sizeof(struct joy_state_packet), GFP_KERNEL);

	t150->lock = kzalloc(sizeof(struct mutex), GFP_KERNEL);
	//mutex_init(t150->lock);

	t150->setup_task = kthread_create(
		t150_inital_usb_setup,
		t150,
		"setup_task"
	);

	// From xpad.c
	for (i = 0; i < 2; i++)
	{
		ep = &interface->cur_altsetting->endpoint[i].desc;

		if (usb_endpoint_xfer_int(ep))
			if (usb_endpoint_dir_in(ep))
				ep_irq_in = ep;
			else
				ep_irq_out = ep;
	}

	if (!ep_irq_in || !ep_irq_out) {
		printk(KERN_INFO "NIGGER!\n");
		error_code = -ENODEV;
		goto error0;
	}

	t150->pipe_in = usb_rcvintpipe(t150->usb_device, ep_irq_in->bEndpointAddress);
	t150->pipe_out= usb_sndintpipe(t150->usb_device, ep_irq_out->bEndpointAddress);

	t150->bInterval_in = ep_irq_in->bInterval;
	t150->bInterval_out = ep_irq_out->bInterval;

	t150_init_input(t150);
	t150_init_ffb(t150);
	input_register_device(t150->joystick);
	
	t150_init_attributes(t150, interface);

	// Default settings here
	/*t150->current_rotation = 0xffff; // 1080°
	t150->current_return_force = 12; // 12%*/

// @TODO Handle errors
error0:
	return error_code;

}

static int t150_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int error_code = 0;
	struct t150 *t150;

	printk(KERN_INFO "t150: T150 Wheel (%04X:%04X) plugged\n", id->idVendor, id->idProduct);

	// Create new t150 struct
	t150 = kzalloc(sizeof(struct t150), GFP_KERNEL);
	if(!t150)
		return -ENOMEM;

	t150_constructor(t150, interface);

	// Save
	usb_set_intfdata(interface, t150);

	// Start usb setup
	wake_up_process(t150->setup_task);

	return 0;
error0:
	kfree(t150);
	return error_code;
}

/**
 * This function blindly mimic the Windows driver
 */
static int t150_inital_usb_setup(void *data)
{
	struct t150 *t150 = data;

	printk("t150: setup started!");

	t150->joystick->open = t150_open;
	return 0;
}

static void t150_disconnect(struct usb_interface *interface)
{
	struct t150 *t150;

	printk(KERN_INFO "t150: T150 Wheel removed. Bye\n");

	t150 = usb_get_intfdata(interface);

	// Stop al pending requests
	usb_kill_urb(t150->joy_request_in);
	usb_kill_urb(t150->joy_request_out);

	// Free al URBs
	usb_free_urb(t150->joy_request_in);
	usb_free_urb(t150->joy_request_out);

	// Force feedback 
	t150_close_ffb(t150);

	// input deregister
	t150_free_input(t150);

	// sysf free
	t150_free_attributes(t150, interface);

	// Free buffers
	kfree(t150->joy_data_in);
	kfree(t150->lock);

	// t150 free
	kfree(t150);
}

#include "attributes.c"
#include "input.c"
#include "settings.c"
#include "forcefeedback.c"


/********************************************************************
 *			MODULE STUFF
 *
 *******************************************************************/

static struct usb_device_id t150_table[] =
{
	{ USB_DEVICE(USB_THRUSTMASTER_VENDOR_ID, USB_T150_PRODUCT_ID) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, t150_table);

static struct usb_driver t150_driver =
{
	.name = "t150",
	.id_table = t150_table,
	.probe = t150_probe,
	.disconnect = t150_disconnect,
};

static int __init t150_init(void)
{
	return usb_register(&t150_driver);
}

static void __exit t150_exit(void)
{
	usb_deregister(&t150_driver);
}

module_init(t150_init);
module_exit(t150_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Pagani <>");
MODULE_DESCRIPTION("ThrustMaster T150 steering wheel driver");
