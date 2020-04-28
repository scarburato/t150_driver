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

	t150->ff_change_effect_status = kzalloc(sizeof(struct urb), GFP_KERNEL);
	if(!t150->ff_change_effect_status)
		goto err3;

	for(i = 0; i < 3; i++)
	{
		t150->ff_upload_urbs[i] = usb_alloc_urb(0, GFP_KERNEL);
		if(!t150->ff_upload_urbs[i])
			goto err4;
	}

	t150->ff_change_urbs = usb_alloc_urb(0, GFP_KERNEL);
	if(!t150->ff_change_urbs)
		goto err5;

	usb_fill_int_urb(
		t150->ff_upload_urbs[0],
		t150->usb_device,
		t150->pipe_out,
		t150->ff_first,
		sizeof(struct ff_first),
		NULL,
		t150,
		t150->bInterval_out
	);

	usb_fill_int_urb(
		t150->ff_upload_urbs[1],
		t150->usb_device,
		t150->pipe_out,
		t150->ff_second,
		sizeof(struct ff_second),
		NULL,
		t150,
		t150->bInterval_out
	);

	usb_fill_int_urb(
		t150->ff_upload_urbs[2],
		t150->usb_device,
		t150->pipe_out,
		t150->ff_third,
		sizeof(struct ff_third),
		NULL,
		t150,
		t150->bInterval_out
	);

	usb_fill_int_urb(
		t150->ff_change_urbs,
		t150->usb_device,
		t150->pipe_out,
		t150->ff_change_effect_status,
		sizeof(struct ff_change_effect_status),
		NULL,
		t150,
		t150->bInterval_out
	);

	for (i = 0; i < t150_ffb_effects_length; i++)
		set_bit(t150_ffb_effects[i], t150->joystick->ffbit);

	// input core will automatically free force feedback structures when device is destroyed.
	errno = input_ff_create(t150->joystick, FF_MAX_EFFECTS);
	
	if(errno)
	{
		printk(KERN_ERR "t150: error create ff :(. errno=%i\n", errno);
		return errno;
	}
	printk(KERN_INFO "t150: ff created :)\n");


	// may not sleep ?
	//t150->joystick->ff->set_autocenter = t150_ffb_set_autocenter;
	t150->joystick->ff->upload = t150_ff_upload;
	t150->joystick->ff->erase = t150_ff_erase;
	t150->joystick->ff->playback = t150_ff_play;

	return 0;

err5:	usb_free_urb(t150->ff_change_urbs);
err4:	for(i--; i >= 0; i--)
		usb_free_urb(t150->ff_upload_urbs[i]);
err3:	kzfree(t150->ff_third);
err2:	kzfree(t150->ff_second);
err1:	kzfree(t150->ff_first);
err0:	return -1;
}

static void t150_close_ffb(struct t150 *t150)
{
	int i;

	usb_free_urb(t150->ff_change_urbs);
	for(i = 0; i < 3; i++)
		usb_free_urb(t150->ff_upload_urbs[i]);

	kzfree(t150->ff_third);
	kzfree(t150->ff_second);
	kzfree(t150->ff_first);
}

static int t150_ff_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int errno, i;
	struct ff_periodic_effect *p_effect = &(effect->u.periodic);

	printk(KERN_INFO "t150: FFB_Uploading...\n");

	t150->ff_first->f0 = cpu_to_le16(0x1c02);
	t150->ff_first->f1 = 0;
	t150->ff_first->attack_length = cpu_to_le16(p_effect->envelope.attack_length);
	t150->ff_first->attack_level  = 0; //p_effect->envelope->attack_level;
	t150->ff_first->fade_length = cpu_to_le16(p_effect->envelope.attack_length);
	t150->ff_first->fade_level  = 0; //p_effect->envelope->fade_level;

	t150->ff_second->f0 = cpu_to_le16(0x0e04);
	t150->ff_second->f1 = 0x00;
	t150->ff_second->magnitude = word_high(p_effect->magnitude);
	t150->ff_second->offset = word_high(p_effect->offset);
	t150->ff_second->phase = (p_effect->phase * 0xff) / (360*100); // Check if correct
	t150->ff_second->period = cpu_to_le16(p_effect->period);

	t150->ff_third->f0 = 0x01;
	t150->ff_third->id = effect->id;
	t150->ff_third->length = cpu_to_le16(effect->replay.length);
	t150->ff_third->f1 = 0;
	t150->ff_third->f2 = cpu_to_le16(0x0e00);
	t150->ff_third->f3 = cpu_to_le16(0x1c00);
	t150->ff_third->f4 = 0;
	t150->ff_third->delay = word_high(effect->replay.delay);
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

	for(i = 0; i < 3; i++)
	{
		errno = usb_submit_urb(t150->ff_upload_urbs[i], GFP_ATOMIC);
		if(errno)
			return errno;
	}
	
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
	t150->ff_change_effect_status->times = 0x01;

	usb_submit_urb(t150->ff_change_urbs, GFP_KERNEL);
}

static int t150_ff_play(struct input_dev *dev, int effect_id, int times)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int boh;

	printk(KERN_INFO "t150: I have to reproduce the effect %i for %i time(s)",effect_id, times);

	if(times == 0)
		return;

	// @TODO Check if the wheel can play infinte times
	if(times > 0xff)
		times = 0xee;

	t150->ff_change_effect_status->f0 = 0x41;
	t150->ff_change_effect_status->id = effect_id;
	t150->ff_change_effect_status->mode = 0x41;
	t150->ff_change_effect_status->times = times;

	usb_submit_urb(t150->ff_change_urbs, GFP_KERNEL);
}
