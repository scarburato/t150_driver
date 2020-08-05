/** Callback to free urb when a ffb request is completed */
static void t150_ff_free_urb(struct urb *urb) 
{
	//struct t150 *t150 = urb->context;
	kzfree(urb->transfer_buffer);
	usb_free_urb(urb);
}

/**
 * Creates an usb URB to be sent to wheel for ffb operations
 * @param t150 our wheel
 * @param buffer_size how large alloc the urb
 * @returns a ptr to URB if no error, 0 otherwise
 */
static struct urb* t150_ff_alloc_urb(struct t150 *t150, const size_t buffer_size)
{
	struct urb *urb;

	void *buffer = kzalloc(buffer_size, GFP_KERNEL);
	if(!buffer)
		return 0;

	urb = usb_alloc_urb(0, GFP_KERNEL);
	if(!urb)
	{
		kzfree(buffer);
		return 0;
	}

	usb_fill_int_urb(
		urb,
		t150->usb_device,
		t150->pipe_out,
		buffer,
		buffer_size,
		t150_ff_free_urb,
		t150,
		t150->bInterval_out
	); 

	return urb;
			
}

/** 
 * This macro is called in the probe function when the wheel input
 * is beign setted up
 * @param t150 a pointer to our wheel
 * @returns 0 if no error, less than 0 if an error occured. 
 */
static inline int t150_init_ffb(struct t150 *t150)
{
	int errno, i;

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

	//t150->joystick->ff->set_autocenter = t150_ffb_set_autocenter;
	t150->joystick->ff->upload = t150_ff_upload;
	t150->joystick->ff->erase = t150_ff_erase;
	t150->joystick->ff->playback = t150_ff_play;
	t150->joystick->ff->set_gain = t150_ff_set_gain;

	return 0;
}

/**
 * macro to clean up the ffb stuff of a whell. It's to be called
 * when probe failed to init or when the wheel is disconnected
 * @param t150 a pointer to our wheel
 */
static inline void t150_free_ffb(struct t150 *t150)
{
	;
}

/**
 * This function prepares an update packet to update an already uploaded effected
 * or when we're uploading a new effect
 * @param t150 our wheel
 * @param effect the effect to be updated
 * 
 * @returns 0 error, urb ptr otherwise
 */
static struct urb* t150_ff_prepare_update(struct t150 *t150, struct ff_effect *effect)
{
	struct ff_update *ff_update;
	struct urb *urb = t150_ff_alloc_urb(t150, sizeof(struct ff_update));
	int32_t level = 0;

	if(!urb)
		return 0;
	ff_update = urb->transfer_buffer;

	ff_update->pk_id1 = effect->id * 0x1c + 0x0e;
	ff_update->f1 = 0x00;

	switch (effect->type)
	{
	case FF_SINE:
	case FF_SAW_UP:
	case FF_SAW_DOWN:
	default:
		ff_update->effect_class = 0x04;

		ff_update->effect.periodic.magnitude = word_high(effect->u.periodic.magnitude);
		ff_update->effect.periodic.offset = word_high(effect->u.periodic.offset);
		ff_update->effect.periodic.phase = (effect->u.periodic.phase * 0xff) / (360*100); // Check if correct
		ff_update->effect.periodic.period = cpu_to_le16(effect->u.periodic.period);
		break;
	case FF_CONSTANT:
		ff_update->effect_class = 0x03;

		level = effect->u.constant.level * fixp_sin16(effect->direction * 360 / 0xFFFF) * +1;
		level >>= 15;

		ff_update->effect.constant.level = (level / 0x01ff);
		break;
	}

	return urb;	
}

/**
 * Function to be called by when an user wants to an effect to the wheel.
 * A period effect - at least if it's periodic - has to be sent to the wheel
 * fragmented in 3 usb request.
 * @param dev the input_dev
 * @param effect the effect to upload
 * @param old If I have to update an already uploaded effect this is not 0
 * 
 * @return 0 if no errors occured
 * @TODO refactor
 */
static int t150_ff_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int errno, i;

	struct urb *urbs[3];
	struct ff_first *ff_first;
	struct ff_commit *ff_commit;

	struct ff_periodic_effect *p_effect = &(effect->u.periodic);

	//printk(KERN_INFO "t150: Uploading effect with id %i...\n", effect->id);

	// Alloc first urb
	urbs[0] = t150_ff_alloc_urb(t150, sizeof(struct ff_first));
	if(!urbs[0])
		return -ENOMEM;
	ff_first = urbs[0]->transfer_buffer;

	// Alloc second urb
	urbs[1] = t150_ff_prepare_update(t150, effect);
	if(!urbs[1])
	{
		t150_ff_free_urb(urbs[0]);
		return -ENOMEM;
	}

	// Alloc third urb
	urbs[2] = t150_ff_alloc_urb(t150, sizeof(struct ff_commit));
	if(!urbs[2])
	{
		t150_ff_free_urb(urbs[0]);
		t150_ff_free_urb(urbs[1]);
		return -ENOMEM;
	}
	ff_commit = urbs[2]->transfer_buffer;
	

	/** Preparing effect */
	ff_first->f0 = 0x02;
	ff_first->pk_id0 = effect->id * 0x1c + 0x1c;
	ff_first->f1 = 0;
	ff_first->attack_length = cpu_to_le16(p_effect->envelope.attack_length);
	ff_first->attack_level  = p_effect->envelope.attack_level / 0x1fff;
	ff_first->fade_length = cpu_to_le16(p_effect->envelope.attack_length);
	ff_first->fade_level  = p_effect->envelope.fade_level / 0x1fff;
	
	ff_commit->f0 = 0x01;
	ff_commit->id = effect->id;
	ff_commit->length = cpu_to_le16(effect->replay.length);
	ff_commit->f1 = 0;
	ff_commit->f2 = 0;
	ff_commit->pk_id1 = effect->id * 0x1c + 0x0e;
	ff_commit->f3 = 0;
	ff_commit->pk_id0 = effect->id * 0x1c + 0x1c;
	ff_commit->f4 = 0;
	ff_commit->delay = word_high(effect->replay.delay);
	ff_commit->f5 = 0;

	switch (effect->type)
	{
	case FF_SINE:
	default:
		ff_commit->effect_type = cpu_to_le16(T150_FF_CODE_SINE);
		break;
	case FF_SAW_UP:
		ff_commit->effect_type = cpu_to_le16(T150_FF_CODE_SAW_UP);
		break;
	case FF_SAW_DOWN:
		ff_commit->effect_type = cpu_to_le16(T150_FF_CODE_SAW_DOWN);
		break;
	case FF_CONSTANT:
		ff_commit->effect_type = cpu_to_le16(T150_FF_CODE_CONSTANT);
		break;
	}

	/** Submiting the effect to the wheel */
	for(i = 0; i < 3; i++)
	{
		errno = usb_submit_urb(urbs[i], GFP_ATOMIC);
		if(errno)
		{
			printk(KERN_ERR "t150: submitting urb, error %i\n", errno);
			return errno;
		}
	}	
	return 0;
}

/**
 * Function used to erase an effect already uploaded to the Wheel
 * @param dev 
 * @param effect_id the ID of the effect to be erased
 * 
 * @return 0 if no errors occured
 */
static int t150_ff_erase(struct input_dev *dev, int effect_id)
{
	struct t150 *t150 = input_get_drvdata(dev);
	struct urb *urb;
	struct ff_change_effect_status *ff_change;
	int errno;

	printk(KERN_WARNING "t150: I should destroy %i now...\n", effect_id);

	// Alloc urb
	urb = t150_ff_alloc_urb(t150, sizeof(struct ff_first));
	if(!urb)
		return -ENOMEM;
	ff_change = urb->transfer_buffer;

	ff_change->f0 = 0x41;
	ff_change->id = effect_id;
	ff_change->mode = 0x00;
	ff_change->times = 0x01;

	errno = usb_submit_urb(urb, GFP_KERNEL);
	if(errno)
	{
		printk(KERN_ERR "t150: unable to send URB, errno %i\n", errno);
	}

	return errno;
}

/**
 * Function used to play an effect already uploaded to the Wheel
 * If times==0 then the function will return without sending any usb
 * request to the wheel and without signaling any errors.
 * @param dev 
 * @param effect_id the ID of the effect to be played
 * @param times how many times the effect should be played. If the effect
 * 	is beign erased a play request in times=0 is also sent.
 * 
 * @return 0 if no errors occured
 */
static int t150_ff_play(struct input_dev *dev, int effect_id, int times)
{
	struct t150 *t150 = input_get_drvdata(dev);
	struct urb *urb;
	struct ff_change_effect_status *ff_change;
	int errno;

	//printk(KERN_INFO "t150: I have to reproduce the effect %i for %i time(s)\n",effect_id, times);


	// Alloc urb
	urb = t150_ff_alloc_urb(t150, sizeof(struct ff_change_effect_status));
	if(!urb)
		return -ENOMEM;
	ff_change = urb->transfer_buffer;

	ff_change->f0 = 0x41;
	ff_change->id = effect_id;
	ff_change->mode = 0x41;
	ff_change->times = times;

	errno = usb_submit_urb(urb, GFP_KERNEL);
	if(errno)
	{
		printk(KERN_ERR "t150: unable to send URB, errno %i\n", errno);
	}

	return errno;
}

/**
 * @param dev
 * @param gain 0xFFFF = 100% of gain 
 */
static void t150_ff_set_gain(struct input_dev *dev, uint16_t gain)
{
	struct t150 *t150 = input_get_drvdata(dev);
	int errno;
	struct urb *urb;
	struct ff_change_gain *ff_change;

	urb = t150_ff_alloc_urb(t150, sizeof(struct ff_change_gain));
	if(!urb)
		return; // -NOMEM

	ff_change = urb->transfer_buffer;
	
	ff_change->f0 = 0x43;
	ff_change->gain = gain / 0x1ff;

	errno = usb_submit_urb(urb, GFP_KERNEL);
	if(errno)
	{
		printk(KERN_ERR "t150: unable to send URB, errno %i\n", errno);
	}
	
	//return errno;
}
