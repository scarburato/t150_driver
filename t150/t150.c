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
#include <linux/fixp-arith.h>
#include <linux/spinlock.h>

#include "t150.h"
#include "input.h"
#include "attributes.h"
#include "settings.h"
#include "forcefeedback.h"
#include "packet.h"

static void donothing_callback(struct urb *urb) {}

/** Init for a t150 data struct
 * @param t150 pointer to the t150 structor to init
 * @param interface pointer to usb interface which the wheel is connected to
 */
static inline int t150_constructor(struct t150 *t150,struct usb_interface *interface)
{
	int i, error_code = 0;
	struct usb_endpoint_descriptor *ep, *ep_irq_in = 0, *ep_irq_out = 0;

	t150->usb_device = interface_to_usbdev(interface);
	dev_set_drvdata(&t150->usb_device->dev, t150);

	// Path used for the input subsystem
	usb_make_path(t150->usb_device, t150->dev_path, sizeof(t150->dev_path));
	strlcat(t150->dev_path, "/input0", sizeof(t150->dev_path));

	t150->joy_request_in = usb_alloc_urb(0, GFP_KERNEL);
	if(!t150->joy_request_in)
		return -ENOMEM;

	t150->joy_request_out = usb_alloc_urb(0, GFP_KERNEL);
	if(!t150->joy_request_out)
	{
		error_code = -ENOMEM;
		goto error0;
	}

	t150->joy_data_in = kzalloc(sizeof(struct t150_state_packet), GFP_KERNEL);
	if(!t150->joy_data_in)
	{
		error_code = -ENOMEM;
		goto error1;
	}
	mutex_init(&t150->lock);
	spin_lock_init(&t150->settings.access_lock);

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

	if (!ep_irq_in || !ep_irq_out) 
	{
		error_code = -ENODEV;
		goto error3;
	}

	t150->pipe_in = usb_rcvintpipe(t150->usb_device, ep_irq_in->bEndpointAddress);
	t150->pipe_out= usb_sndintpipe(t150->usb_device, ep_irq_out->bEndpointAddress);

	t150->bInterval_in = ep_irq_in->bInterval;
	t150->bInterval_out = ep_irq_out->bInterval;

	error_code = t150_init_input(t150);
	if(error_code)
		goto error4;

	error_code = t150_init_ffb(t150);
	if(error_code)
		goto error5;

	error_code = input_register_device(t150->joystick);
	if(error_code)
		goto error6;
	
	error_code = t150_init_attributes(t150, interface);
	if(error_code)
		goto error7;

	return 0;

error7:	input_free_device(t150->joystick);
error6: t150_free_ffb(t150);
error5: t150_free_input(t150);
error4:	;
error3: kzfree(t150->joy_data_in);
error1: usb_free_urb(t150->joy_request_out);
error0:	usb_free_urb(t150->joy_request_in);
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

	error_code = t150_constructor(t150, interface);
	if(error_code)
		goto error0;

	// Save
	usb_set_intfdata(interface, t150);

	return 0;

error0: kfree(t150);
	return error_code;
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
	t150_free_ffb(t150);

	// input deregister
	t150_free_input(t150);

	// sysf free
	t150_free_attributes(t150, interface);

	// Free buffers
	kfree(t150->joy_data_in);

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
	int errno = -ENOMEM;
	packet_input_open = kzalloc(sizeof(uint16_t), GFP_KERNEL);
	if(!packet_input_open)
		goto err0;

	packet_input_what = kzalloc(sizeof(uint16_t), GFP_KERNEL);
	if(!packet_input_what)
		goto err1;

	packet_input_close = kzalloc(sizeof(uint16_t), GFP_KERNEL);
	if(!packet_input_close)
		goto err2;

	*packet_input_open = cpu_to_le16(0x0442);
	*packet_input_what = cpu_to_le16(0x0542);
	*packet_input_close = cpu_to_le16(0x0042);

	errno = usb_register(&t150_driver);
	if(errno)
		goto err3;
	else
		return 0;

err3:	kzfree(packet_input_close);
err2:	kzfree(packet_input_what);
err1:	kzfree(packet_input_open);
err0:	return errno;
}

static void __exit t150_exit(void)
{
	kzfree(packet_input_open);
	kzfree(packet_input_what);
	kzfree(packet_input_close);

	usb_deregister(&t150_driver);
}

module_init(t150_init);
module_exit(t150_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dario Pagani <>");
MODULE_DESCRIPTION("ThrustMaster T150 steering wheel driver");
