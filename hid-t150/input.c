/**
 * This function initializes the input system for
 * @t150 pointer to our device
 */
static inline int t150_init_input(struct t150 *t150)
{
	struct hid_input *hidinput = list_entry(t150->hid_device->inputs.next, struct hid_input, list);
	t150->joystick = hidinput->input;
	
	input_set_drvdata(t150->joystick, t150);

	t150->joystick->open = t150_input_open;
	t150->joystick->close = t150_input_close;

	return 0;
}

static inline void t150_free_input(struct t150 *t150)
{
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

	if(packet->type != STATE_PACKET_INPUT)
	{
		hid_warn(hdev, "recived a packet that is not an input state :/\n");
		printP(packet_raw, size);
		return -1; // @TODO 
	}

	return 0;
}
