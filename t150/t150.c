#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/usb.h>
#include <linux/usb/ch9.h>
#include <linux/completion.h>
#include <linux/input.h>
#include <linux/usb/input.h>

#include "t150.h"

struct urb *js_read;

static int t150_set_rotation(uint8_t angle)
{
	return 0;
}

static void t150_callback(struct urb *urb)
{
	printk(KERN_INFO "t150: Ò fatto la richiesta, ottenuta risposta!\n");
}

static int t150_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	int error_code = 0, i;
	struct t150 *t150;
	struct usb_endpoint_descriptor *ep_irq_in = 0, *ep_irq_out = 0;

	printk(KERN_INFO "t150: T150 Wheel (%04X:%04X) plugged\n", id->idVendor, id->idProduct);

	// Create new t150 struct
	t150 = kzalloc(sizeof(struct t150), GFP_KERNEL);
	if(!t150)
		return -ENOMEM;

	// Save
	usb_set_intfdata(interface, t150);

	t150->usb_device = interface_to_usbdev(interface);

	// Apro dev
	usb_make_path(t150->usb_device, t150->dev_path, sizeof(t150->dev_path));
	strlcat(t150->dev_path, "/input0", sizeof(t150->dev_path));

	printk(KERN_INFO "t150: Ò ottentuo %s\n", t150->dev_path);

	// TODO CHECK FOR MEMORY FAIL
	t150->joy_request_in = usb_alloc_urb(0, GFP_KERNEL);
	t150->joy_request_out = usb_alloc_urb(0, GFP_KERNEL);

	t150->joy_data_in = kzalloc(sizeof(struct joy_state_packet), GFP_KERNEL);


	// From xpad.c
	for (i = 0; i < 2; i++) {
		struct usb_endpoint_descriptor *ep =
				&interface->cur_altsetting->endpoint[i].desc;

		if (usb_endpoint_xfer_int(ep)) {
			if (usb_endpoint_dir_in(ep))
				ep_irq_in = ep;
			else
				ep_irq_out = ep;
		}
	}

	if (!ep_irq_in || !ep_irq_out) {
		printk(KERN_INFO "NIGGER!\n");
		error_code = -ENODEV;
		goto error0;
	}

	/*void usb_fill_int_urb(struct urb *urb, struct usb_device *dev,
                      unsigned int pipe, void *transfer_buffer,
                      int buffer_length, usb_complete_t complete,
                      void *context, int interval);*/

	t150_init_input(t150);
	usb_fill_int_urb(
		t150->joy_request_in,
		t150->usb_device,
		usb_rcvintpipe(t150->usb_device, ep_irq_in->bEndpointAddress),
		t150->joy_data_in,
		sizeof(struct joy_state_packet),
		t150_update_input,
		t150,
		//interface->cur_altsetting->endpoint->desc.bInterval
		// This is a rondom number I choose, I do not understand
		// whats this field does :P
		0x0f
	);

	usb_submit_urb(t150->joy_request_in, GFP_ATOMIC);

	return 0;
error0:
	kfree(t150);
	return error_code;
}

const char *nameWH = "ThrustMaster T150 steering wheel";
static inline int t150_init_input(struct t150 *t150)
{
	t150->joystick = input_allocate_device();
	printk(KERN_INFO "t150: Il signor Kernel mi ha dato %p, che gentile!\n", t150->joystick);

	if(! (t150->joystick))
		return -ENOMEM;

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
	input_set_capability(t150->joystick, EV_KEY, BTN_NORTH);  // Triangle
	input_set_capability(t150->joystick, EV_KEY, BTN_EAST);   // Circle
	input_set_capability(t150->joystick, EV_KEY, BTN_SOUTH);  // Cross
	input_set_capability(t150->joystick, EV_KEY, BTN_WEST);   // Square

	input_set_capability(t150->joystick, EV_KEY, BTN_GEAR_DOWN); // L1
	input_set_capability(t150->joystick, EV_KEY, BTN_GEAR_UP);   // R1
	input_set_capability(t150->joystick, EV_KEY, BTN_TL);        // L2
	input_set_capability(t150->joystick, EV_KEY, BTN_TR);        // R2
	input_set_capability(t150->joystick, EV_KEY, BTN_TL2);       // L3
	input_set_capability(t150->joystick, EV_KEY, BTN_TR2);       // R3

	input_set_capability(t150->joystick, EV_KEY, BTN_START); // ST / options
	input_set_capability(t150->joystick, EV_KEY, BTN_SELECT); // SE / share
	input_set_capability(t150->joystick, EV_KEY, BTN_GAMEPAD); // PS

	input_register_device(t150->joystick);

	return 0;
}

static inline uint16_t make_word(const uint8_t low, const uint8_t high)
{
	return ((uint16_t)low | ((uint8_t)(high) << 8));
}

static void t150_update_input(struct urb *urb)
{
	struct joy_state_packet *ss = urb->transfer_buffer;
	struct t150 *t150 = (struct t150*)urb->context;

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
	struct d_pad_pos d_pad_current_pos = CROSS_POSITIONS[
		(ss->cross_state < 0x08) ? ss->cross_state : 0x08];

	input_report_abs(t150->joystick, ABS_HAT0X, d_pad_current_pos.x);
	input_report_abs(t150->joystick, ABS_HAT0Y, d_pad_current_pos.y);

	// Report buttons
	input_report_key(t150->joystick, BTN_GEAR_UP,
		ss->buttons_state & BTN_GEAR_UP_MASK);

	input_report_key(t150->joystick, BTN_GEAR_DOWN,
		ss->buttons_state & BTN_GEAR_DOWN_MASK);

	input_sync(t150->joystick);

	// Restart
	usb_submit_urb(urb, GFP_ATOMIC);
}

static void t150_disconnect(struct usb_interface *interface)
{
	struct t150 *t150;

	printk(KERN_INFO "t150: T150 Wheel removed. Bye\n");

	t150 = usb_get_intfdata(interface);

	// input deregister
	input_unregister_device(t150->joystick);

	// Stop al pending requests
	usb_kill_urb(t150->joy_request_in);
	usb_kill_urb(t150->joy_request_out);

	// Free al URBs
	usb_free_urb(t150->joy_request_in);
	usb_free_urb(t150->joy_request_out);

	// Free buffers
	kfree(t150->joy_data_in);

	// t150 free
	kfree(t150);
}

static struct usb_device_id t150_table[] =
{
	{ USB_DEVICE(USB_THRUSTMASTER_VENDOR_ID, USB_T150_PRODUCT_ID) },
	{} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, t150_table);

static struct usb_driver t150_driver =
{
	.name = "Thrustmaster T150 Wheel driver",
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
