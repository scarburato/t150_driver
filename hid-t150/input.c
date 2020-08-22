/**
 * This function initializes the input system for
 * @t150 pointer to our device
 */
static inline int t150_init_input(struct t150 *t150)
{
	int i;

	t150->joystick = input_allocate_device();

	if(! (t150->joystick))
		return -ENOMEM;

	input_set_drvdata(t150->joystick, t150);

	t150->joystick->name = nameWH;
	t150->joystick->phys = t150->dev_path;
	usb_to_input_id(t150->usb_device, &t150->joystick->id);
	//t150->joystick->id.product = USB_T150_PRODUCT_ID;
	//t150->joystick->id.vendor = USB_THRUSTMASTER_VENDOR_ID;
	t150->joystick->dev.parent = &t150->usb_device->dev;

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

	t150->joystick->open = t150_input_open;
	t150->joystick->close = t150_input_close;

	return 0;
}

static inline void t150_free_input(struct t150 *t150)
{
	input_unregister_device(t150->joystick);
}

static int t150_input_open(struct input_dev *dev)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int boh, ret;

	ret = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		packet_input_open, 2, &boh,
		8
	);

	if(ret)
		return ret;

	ret = hid_hw_open(t150->hid_device);

	return ret;
}

static void t150_input_close(struct input_dev *dev)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int boh, i;

	hid_hw_close(t150->hid_device);

	// Send magic codes
	for(i = 0; i < 2; i++)
		usb_interrupt_msg(
			t150->usb_device,
			t150->pipe_out,
			packet_input_what, 2, &boh,
			8
		);

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		packet_input_close, 2, &boh,
		8
	);
}

/**
 * This function updates the current input status of the joystick
 * @t150 target wheel
 * @ss   new status to register
 */
static int t150_update_input(struct hid_device *hdev, struct hid_report *report, uint8_t *packet_raw, int size)
{
	struct t150_state_packet *packet = (struct t150_state_packet*)packet_raw;
	struct t150_input_state_packet *ss = &packet->data.input;
	struct t150 *t150 = hid_get_drvdata(hdev);
	struct d_pad_pos d_pad_current_pos;
	int i;

	if(packet->type != STATE_PACKET_INPUT)
	{
		hid_warn(hdev, "recived a packet that is not an input state :/\n");
		printP(packet_raw, size);
		return -1; // @TODO 
	}

	// Reporting axies
	input_report_abs(t150->joystick, ABS_GAS,
		0x3ff - le16_to_cpu(ss->gas_axis));

	input_report_abs(t150->joystick, ABS_BRAKE,
		0x3ff - le16_to_cpu(ss->brake_axis));

	input_report_abs(t150->joystick, ABS_Z,
		0x3ff - le16_to_cpu(ss->clutch_axis));

	input_report_abs(t150->joystick, ABS_WHEEL,
		(int16_t)
		(le16_to_cpu(ss->wheel_axis) - 0x8000)
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
	return 0;
}
