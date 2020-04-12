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
#include "magic.h"
#include "buttons.h"

static void t150_callback(struct urb *urb)
{
	printk(KERN_INFO "t150: Ò fatto la richiesta, ottenuta risposta!\n");
	
}

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

// @TODO Handle errors
error0:
	return error_code;

}

static int t150_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int error_code = 0, i;
	struct t150 *t150;
	struct urb *tmp = 0;

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
	int boh, i = 0;
	uint8_t buffer[15] = {0};

	printk("t150: setup started!");

	for(i = 16; i < magic_packet_length - 2; i++)
	{
		usb_control_msg(
			t150->usb_device,
			usb_sndctrlpipe(t150->usb_device, 0),
			magic_packet[i][1],
			magic_packet[i][0],
			make_word(magic_packet[i][2], magic_packet[i][3]),
			make_word(magic_packet[i][4], magic_packet[i][5]),
			magic_buffer[i],
			make_word(magic_packet[i][6], magic_packet[i][7]),
			1000
		);
	}

	for(i = 0; i < init_win_len - 1; i++)
	{
		usb_interrupt_msg(
			t150->usb_device,
			t150->joy_request_out,
			init_win[i], 15,
			&boh, 1000
		);
		msleep(15);
		usb_interrupt_msg(
			t150->usb_device,
			t150->joy_request_in,
			buffer, 15,
			&boh, 1000
		);
		printP(buffer);
	}

	usb_interrupt_msg(
		t150->usb_device,
		t150->joy_request_in,
		buffer, 15,
		&boh, 1000
	);

	usb_control_msg(
		t150->usb_device,
		usb_sndctrlpipe(t150->usb_device, 0),
		magic_packet[22][1],
		magic_packet[22][0],
		make_word(magic_packet[22][2], magic_packet[22][3]),
		make_word(magic_packet[22][4], magic_packet[22][5]),
		magic_buffer[22],
		make_word(magic_packet[22][6], magic_packet[22][7]),
		1000
	);

	for(i = 0; i < 2; i++)
	{
		usb_interrupt_msg(
			t150->usb_device,
			t150->joy_request_in,
			buffer, 15,
			&boh, 1000
		);
		printP(buffer);
	}
	
	usb_interrupt_msg(
		t150->usb_device,
		t150->joy_request_out,
		range900, 4,
		&boh, 1000
	);

	msleep(250);

	usb_interrupt_msg(
		t150->usb_device,
		t150->joy_request_in,
		buffer, 15,
		&boh, 1000
	);

	printP(buffer);

	usb_control_msg(
		t150->usb_device,
		usb_sndctrlpipe(t150->usb_device, 0),
		magic_packet[23][1],
		magic_packet[23][0],
		make_word(magic_packet[23][2], magic_packet[23][3]),
		make_word(magic_packet[23][4], magic_packet[23][5]),
		magic_buffer[23],
		make_word(magic_packet[23][6], magic_packet[23][7]),
		1000
	);

	t150->joystick->open = t150_open;
}

static void t150_setup_end(struct urb *urb)
{
	usb_free_urb(urb);
}

const char *nameWH = "ThrustMaster T150 steering wheel";
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

static void my(struct urb *urb)
{
	printk(KERN_INFO "t150: eccomi!\n");
}

static int t150_set_rotation(uint8_t angle)
{
	return 0;
}

/********************************************************************
 *			             SYSF STUFF
 *
 *        This section contains the handlers for the
 *          attributes created in the sysfs when a
 *             wheel is plugged into the host
 *******************************************************************/

/**
*/
static uint8_t ss[15];
static inline int t150_start_edit_settings(struct t150 *t150)
{
	int boh, i;

	for(i = 0; i < 3; i++)
		usb_control_msg(
			t150->usb_device,
			usb_sndctrlpipe(t150->usb_device, 0),
			driver_settings_start[1],
			driver_settings_start[0],
			make_word(driver_settings_start[2], driver_settings_start[3]),
			make_word(driver_settings_start[4], driver_settings_start[5]),
			magic_buffer[0], // TODO no
			make_word(driver_settings_start[6], driver_settings_start[7]),
			50
		);

	/*for(i = 0; i < init_win_len; i++)
	{
		usb_interrupt_msg(
			t150->usb_device,
			t150->pipe_out,
			init_win[i],
			15, &boh,
			50
		);
		msleep(20);
		usb_interrupt_msg(
			t150->usb_device,
			t150->pipe_in,
			&ss,
			15, &boh,
			50
		);
		printP(ss);
	}*/

	// We want to modify the Wheel settings
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&start_input_settings,
		sizeof(start_input_settings), &boh,
		20
	);
	return 0;
}

static inline int t150_stop_edit_settings(struct t150 *t150)
{
	int boh, i;

	// Submit & close
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&magic,
		2, &boh,
		1000
	);
	msleep(5);

	for(i = 0; i < magic_apply_len; i++)
	{
		usb_interrupt_msg(
			t150->usb_device,
			t150->pipe_out,
			magic_apply[i],
			15, &boh,
			500
		);
		msleep(15);
		usb_interrupt_msg(
			t150->usb_device,
			t150->pipe_in,
			&ss,
			15, &boh,
			500
		);
		printk(
			KERN_INFO "t150: STOP packet: %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX\n",
			ss[0], ss[1], ss[2], ss[3], ss[4], ss[5], ss[6], ss[7],
			ss[8], ss[9], ss[10],ss[11],ss[12],ss[13],ss[14]
		);
	}

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&apply_input_settings,
		2, &boh,
		1000
	);
	msleep(25);

	usb_control_msg(
		t150->usb_device,
		usb_sndctrlpipe(t150->usb_device, 0),
		driver_settings_start[1],
		driver_settings_start[0],
		make_word(driver_settings_start[2], driver_settings_start[3]),
		make_word(driver_settings_start[4], driver_settings_start[5]),
		magic_buffer[0], // TODO no
		make_word(driver_settings_start[6], driver_settings_start[7]),
		50
	);

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&stop_input_settings,
		2, &boh,
		1000
	);
	msleep(25);

	return 0;
}

static ssize_t t150_read_return_force(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint8_t nforce;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);
	// TODO scrivi
	// FIXME alloca dinamicamente

	// If mallformed input leave...
	if(kstrtou8(buf, 10, &nforce))
		return count;

	if(nforce > 100)
		nforce = 100;

	struct set_return_force srf = {
		0x40,
		0x03,
		nforce,
		0
	};

	t150_start_edit_settings(t150);

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		return_wheel_mode,
		4, &boh,
		1000
	);

	// Send to the wheel desidered return force
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&srf,
		sizeof(struct set_return_force), &boh,
		1000
	);

	t150_stop_edit_settings(t150);

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
	uint8_t use;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);
	//uint8_t *new_setting = kzalloc(sizeof(return_wheel_mode), GFP_KERNEL);

	//memcpy(new_setting, return_wheel_mode, sizeof(return_wheel_mode));

	if(kstrtou8(buf, 10, &use))
		return count;

	return_wheel_mode[2] = use;

	t150_start_edit_settings(t150);
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		return_wheel_mode,
		sizeof(return_wheel_mode), &boh,
		1000
	);
	t150_stop_edit_settings(t150);

	//kzfree(new_setting);

	return count;
}

static ssize_t t150_read_range(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint16_t range;
	int boh;
	struct usb_interface *uif = to_usb_interface(dev);
	struct t150 *t150 = usb_get_intfdata(uif);

	char packet[] = {0x40, 0x11, 0x00, 0x00};

	// If mallformed input leave...
	if(kstrtou16(buf, 10, &range))
		return count;

	*((uint16_t *)&packet[2]) = cpu_to_le16(range);

	//t150_start_edit_settings(t150);

	// Send to the wheel desidered return force
	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		&packet[0],
		4, &boh,
		1000
	);

	//t150_stop_edit_settings(t150);
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

}

static inline int t150_free_sysf(struct t150 *t150, struct usb_interface *uif)
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
