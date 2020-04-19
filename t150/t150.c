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
#include "buttons.h"

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
	t150_init_sysf(t150, interface);

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

const char *nameWH = "ThrustMaster T150RS steering wheel";
/**
 * This function initializes the input system for
 * @t150 pointer to our device
 */
static inline int t150_init_input(struct t150 *t150)
{
	int i;

	t150->joystick = input_allocate_device();
	printk(KERN_INFO "t150: Il signor Kernel mi ha dato %p, che gentile!\n", t150->joystick);

	if(! (t150->joystick))
		return -ENOMEM;

	input_set_drvdata(t150->joystick, t150);

	t150->joystick->name = nameWH;
	t150->joystick->phys = t150->dev_path;
	usb_to_input_id(t150->usb_device, &t150->joystick->id);

	// Assi
	input_set_abs_params(t150->joystick, ABS_GAS,    0x000,  0x3ff,  0, 0); // Gas
	input_set_abs_params(t150->joystick, ABS_BRAKE,  0x000,  0x3ff,  0, 0); // Brake
	input_set_abs_params(t150->joystick, ABS_Z,      0x000,  0x3ff,  0, 0); // Clutch ??
	input_set_abs_params(t150->joystick, ABS_WHEEL, -0x8000, 0x7fff, 0, 0); // Wheel

	input_set_abs_params(t150->joystick, ABS_HAT0X, -1, +1, 0, 0); // d-pad
	input_set_abs_params(t150->joystick, ABS_HAT0Y, -1, +1, 0, 0); // d-pad

	// Buttons
	for(i = 0; i < buttons_state0_assoc_length; i++)
		input_set_capability(t150->joystick, EV_KEY, buttons_state0_assoc[i].key);

	for(i = 0; i < buttons_state1_assoc_length; i++)
		input_set_capability(t150->joystick, EV_KEY, buttons_state1_assoc[i].key);

	//t150->joystick->open = t150_open;
	t150->joystick->close = t150_close;

	usb_fill_int_urb(
		t150->joy_request_in,
		t150->usb_device,
		t150->pipe_in,
		t150->joy_data_in,
		sizeof(struct joy_state_packet),
		t150_update_input,
		t150,
		t150->bInterval_in
	);

	// Start!
	input_register_device(t150->joystick);

	return 0;
}

/** Function */
static int t150_open(struct input_dev *dev)
{
	struct t150 *t150 = input_get_drvdata(dev);
	printk("t150: inizio!");
	//mutex_lock(t150->lock);
	usb_submit_urb(t150->joy_request_in, GFP_ATOMIC);
	return 0;
}

static void t150_close(struct input_dev *dev)
{
	struct t150 *t150 = input_get_drvdata(dev);
	usb_kill_urb(t150->joy_request_in);
}

/**
 * This function updates the current input status of the joystick
 * @t150 target wheel
 * @ss   new status to register
 */
static void t150_update_input(struct urb *urb)
{
	struct joy_state_packet *ss = urb->transfer_buffer;
	struct t150 *t150 = (struct t150*)urb->context;
	struct d_pad_pos d_pad_current_pos;
	int i;

	//mutex_unlock(t150->lock);

	// Reporting axies
	input_report_abs(t150->joystick, ABS_GAS,
		make_word(ss->gas_axis_low, ss->gas_axis_high));

	input_report_abs(t150->joystick, ABS_BRAKE,
		make_word(ss->brake_axis_low, ss->brake_axis_high));

	input_report_abs(t150->joystick, ABS_Z,
		make_word(ss->clutch_axis_low, ss->clutch_axis_high));

	input_report_abs(t150->joystick, ABS_WHEEL,
		(int16_t)
		(make_word(ss->wheel_axis_low, ss->wheel_axis_high) - 0x8000)
	);

	// Reporting d-pad
	d_pad_current_pos = CROSS_POSITIONS[
		(ss->cross_state < 0x08) ? ss->cross_state : 0x08];

	input_report_abs(t150->joystick, ABS_HAT0X, d_pad_current_pos.x);
	input_report_abs(t150->joystick, ABS_HAT0Y, d_pad_current_pos.y);

	// Report buttons
	for(i = 0; i < buttons_state0_assoc_length; i++)
		input_report_key(t150->joystick, buttons_state0_assoc[i].key,
			ss->buttons_state0 & buttons_state0_assoc[i].mask);

	for(i = 0; i < buttons_state1_assoc_length; i++)
		input_report_key(t150->joystick, buttons_state1_assoc[i].key,
			ss->buttons_state1 & buttons_state1_assoc[i].mask);

	input_sync(t150->joystick);

	//mutex_lock(t150->lock);
	usb_submit_urb(t150->joy_request_in, GFP_ATOMIC);
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

	// input deregister
	input_unregister_device(t150->joystick);

	// sysf free
	t150_free_sysf(t150, interface);

	// Free buffers
	kfree(t150->joy_data_in);
	kfree(t150->lock);

	// t150 free
	kfree(t150);
}

/********************************************************************
 *			             SYSF STUFF
 *
 *        This section contains the handlers for the
 *          attributes created in the sysfs when a
 *             wheel is plugged into the host
 *******************************************************************/

/**/
static ssize_t t150_read_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(sizeof(set_return_force), GFP_KERNEL);
	memcpy(set, set_return_force, sizeof(set_return_force));

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		goto ret;

	if(nforce > 100)
		nforce = 100;

	set[2] = nforce;

	// Send to the wheel desidered return force
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		set,
		4, &boh,
		0
	);

ret:	kfree(set);
	return count;
}

static ssize_t t150_write_return_force(struct device *dev, struct device_attribute *attr,char * buf )
{
	// TODO Read from wheel with USB request
	return sscanf(buf, no_str);
}

static ssize_t t150_read_simulate_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	/**uint8_t use;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);**/
	
	// TODO

	return count;
}

static ssize_t t150_read_range(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t range;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char *set = kzalloc(sizeof(set_range), GFP_KERNEL);
	memcpy(set, set_range, sizeof(set_range));

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &range))
		goto ret;

	if(range < 270)
		range = 270;
	else if (range > 1080)
		range = 1080;
	
	range = (range * 0xffff) / 1080;

	*((uint16_t *)&set[2]) = cpu_to_le16(range);

	// Send to the wheel desidered return force
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		set,
		4, &boh,
		1000
	);

ret:	kfree(set);
	return count;
}

static ssize_t t150_write_range(struct device *dev, struct device_attribute *attr,char * buf )
{
	// TODO Read from wheel with USB request
	return sscanf(buf, no_str);
}

/** Attribute used to set how much strong is the simulated "spring" that makes
 * the wheel center back when steered.
 * Input is a decimal value between 0 and 100*/
static DEVICE_ATTR(return_force, 0660, t150_write_return_force, t150_read_return_force);

/** Attribute used to set if the wheel has to activate the auto-centering of the
 *  wheel.
 * Input is a numerical value. 0 disabled, not 0 enabled*/
static DEVICE_ATTR(simulate_return_force, 0660, t150_write_return_force, t150_read_simulate_return_force);

/** Attribute used to set the wheel max rotation in degrees.
 * Input is a decimal value between between 270 and 1080*/
static DEVICE_ATTR(range, 0660, t150_write_range, t150_read_range);

static inline int t150_init_sysf(struct t150 *t150, struct usb_interface *uif)
{
	device_create_file(&uif->dev, &dev_attr_return_force);
	device_create_file(&uif->dev, &dev_attr_simulate_return_force);
	device_create_file(&uif->dev, &dev_attr_range);

	return 0;
}

static inline void t150_free_sysf(struct t150 *t150, struct usb_interface *uif)
{
	device_remove_file(&uif->dev, &dev_attr_return_force);
	device_remove_file(&uif->dev, &dev_attr_simulate_return_force);
	device_remove_file(&uif->dev, &dev_attr_range);
}


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
