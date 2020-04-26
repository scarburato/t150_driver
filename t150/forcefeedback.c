static int t150_init_ffb(struct t150 *t150)
{
	int errno, i;

	t150->ff_first = kzalloc(sizeof(struct ff_first), GFP_KERNEL);
	if(!t150->ff_first)
		goto err0;
	
	t150->ff_second = kzalloc(sizeof(struct ff_second), GFP_KERNEL);
	if(!t150->ff_second)
		goto err1;

	t150->ff_third = kzalloc(sizeof(struct ff_third), GFP_KERNEL);
	if(!t150->ff_third)
		goto err2;

	//t150->ff_urbs = kzalloc(sizeof(struct urb) * 4, GFP_KERNEL);
	//if(!t150->ff_urbs)
	//	goto err3;

	t150->ff_change_effect_status = kzalloc(sizeof(struct urb), GFP_KERNEL);
	if(!t150->ff_change_effect_status)
		goto err3;

	set_bit(ABS_WHEEL, t150->joystick->ffbit);
	for (i = 0; i < t150_ffb_effects_length; i++)
	{
		set_bit(t150_ffb_effects[i], t150->joystick->ffbit);
		//t150->joystick->ffbit[i] = t150_ffb_effects[i];
		//input_set_capability(t150->joystick, EV_FF, t150_ffb_effects[i]);
	}

	// input core will automatically free force feedback structures when device is destroyed.
	errno = input_ff_create(t150->joystick, FF_MAX_EFFECTS);
	
	if(errno)
	{
		printk(KERN_ERR "t150: error create ff :(\n");
		return errno;
	}
	printk(KERN_INFO "t150: ff created :)\n");


	// may not sleep ?
	//t150->joystick->ff->set_autocenter = t150_ffb_set_autocenter;
	t150->joystick->ff->upload = t150_ff_upload;
	t150->joystick->ff->erase = t150_ff_erase;
	t150->joystick->ff->playback = t150_ff_play;

	return 0;

err3:	kzfree(t150->ff_third);
err2:	kzfree(t150->ff_second);
err1:	kzfree(t150->ff_first);
err0:	return -1;
}

static void t150_close_ffb(struct t150 *t150)
{

}

static int t150_ff_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int boh;
	struct ff_periodic_effect *p_effect = &(effect->u.periodic);

	printk(KERN_INFO "t150: FFB_Uploading...\n");

	t150->ff_first->f0 = cpu_to_le16(0x1c02);
	t150->ff_first->f1 = 0;
	t150->ff_first->attack_length = cpu_to_le16(p_effect->envelope.attack_length);
	t150->ff_first->attack_level  = 0; //p_effect->envelope->attack_level;
	t150->ff_first->fade_length = cpu_to_le16(p_effect->envelope.attack_length);
	t150->ff_first->fade_level  = 0; //p_effect->envelope->fade_level;

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		t150->ff_first,
		sizeof(struct ff_first), &boh,
		1000
	);

	printk(KERN_INFO "t150: Still alive or locked ?\n");

	t150->ff_second->f0 = cpu_to_le16(0x0e04);
	t150->ff_second->magnitude = cpu_to_le16(p_effect->magnitude);
	t150->ff_second->f1 = 0x0000;
	t150->ff_second->period = cpu_to_le16(p_effect->period);

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		t150->ff_second,
		sizeof(struct ff_second), &boh,
		1000
	);

	t150->ff_third->f0 = 0x01;
	t150->ff_third->id = effect->id;
	t150->ff_third->length = cpu_to_le16(effect->replay.length);
	t150->ff_third->f1 = 0;
	t150->ff_third->f2 = cpu_to_le16(0x0e00);
	t150->ff_third->f3 = cpu_to_le16(0x1c00);
	t150->ff_third->f4 = 0;
	t150->ff_third->delay = effect->replay.delay;
	t150->ff_third->f5 = 0;

	switch (effect->type)
	{
	case FF_SINE:
	default:
		t150->ff_third->effect_type = cpu_to_le16(T150_FF_CODE_SINE);
		break;
	case FF_SAW_UP:
		t150->ff_third->effect_type = cpu_to_le16(T150_FF_CODE_SAW_UP);
		break;
	case FF_SAW_DOWN:
		t150->ff_third->effect_type = cpu_to_le16(T150_FF_CODE_SAW_DOWN);
		break;
	}

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		t150->ff_third,
		sizeof(struct ff_third), &boh,
		1000
	);
	
	return 0;
}

/*static void free_callback(struct urb *urb)
{
	kzfree(urb->transfer_buffer);
}*/

static int t150_ff_erase(struct input_dev *dev, int effect_id)
{
	//printk(KERN_WARNING "t150: I should destroy %i now...\n", effect_id);
	struct t150 *t150 = input_get_drvdata(dev);
	int boh;

	t150->ff_change_effect_status->f0 = 0x41;
	t150->ff_change_effect_status->id = effect_id;
	t150->ff_change_effect_status->mode = 0x00;
	t150->ff_change_effect_status->f1 = 0x01;

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		t150->ff_change_effect_status,
		sizeof(struct ff_change_effect_status), &boh,
		1000
	);

	return 0;
}

static int t150_ff_play(struct input_dev *dev, int effect_id, int value)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int boh;

	t150->ff_change_effect_status->f0 = 0x41;
	t150->ff_change_effect_status->id = effect_id;
	t150->ff_change_effect_status->mode = 0x41;
	t150->ff_change_effect_status->f1 = 0x01;

	usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		t150->ff_change_effect_status,
		sizeof(struct ff_change_effect_status), &boh,
		1000
	);

	return 0;
}
