/**
 * @param t150 ptr to t150
 * @param gain a value between 0x00 and 0x80 where 0x80 is 100% gain
 * @return 0 on success @see usb_interrupt_msg for return codes
 */
static int t150_set_gain(struct t150 *t150, uint8_t gain)
{
	int boh, errno;
	uint8_t *buffer = kzalloc(2, GFP_KERNEL);
	unsigned long flags;

	buffer[0] = 0x43;
	buffer[1] = gain;

	mutex_lock(&t150->lock);

	// Send to the wheel desidered return force
	errno = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		buffer,
		2, &boh,
		SETTINGS_TIMEOUT
	);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.gain = gain;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	} else {
		printk(KERN_ERR "t150: Operation set gain failed with code %d", errno);
	}

	mutex_unlock(&t150->lock);

	kzfree(buffer);

	return errno;
}

/**
 * @param autocenter_force a value between 0 and 100, is the strength of the autocenter effect
 */
static __always_inline int t150_set_autocenter(struct t150 *t150, uint8_t autocenter_force)
{
	uint8_t *buffer = kzalloc(4, GFP_KERNEL);
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_RETURN_FORCE, autocenter_force, buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.autocenter_force = autocenter_force;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);

	kzfree(buffer);
	return errno;
}

/**
 * @param enable true if the autocenter effect is to be kept enabled when the input
 * 	is opened. The autocentering effect is always active while no input are open
 */
static __always_inline int t150_set_enable_autocenter(struct t150 *t150, bool enable)
{
	uint8_t *buffer = kzalloc(4, GFP_KERNEL);
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_USE_RETURN_FORCE, enable, buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.autocenter_enabled = enable;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);

	kzfree(buffer);
	return errno;
}

/**
 * @param range a value between 0x0000 and 0xffff where 0xffff is 1080°
 * 	wheel range
 */
static __always_inline int t150_set_range(struct t150 *t150, uint16_t range)
{
	uint8_t *buffer = kzalloc(4, GFP_KERNEL);
	int errno;
	unsigned long flags;

	mutex_lock(&t150->lock);

	errno = t150_settings_set40(t150, SET40_RANGE, range, buffer);

	if(!errno) {
		spin_lock_irqsave(&t150->settings.access_lock, flags);
		t150->settings.range = range;
		spin_unlock_irqrestore(&t150->settings.access_lock, flags);
	}

	mutex_unlock(&t150->lock);

	kzfree(buffer);
	return errno;
}


/**
 * @t150 pointer to t150
 * @operation number of operation
 * @argument the argument to pass with the request
 * @buffer a buffer of at least 4 BYTES!
 * @return 0 on success @see usb_interrupt_msg for return codes
 */
static int t150_settings_set40(
	struct t150 *t150, operation_t operation, uint16_t argument, void *buffer_
)
{
	int boh, errno;
	struct operation40 *buffer = buffer_;
	buffer->code = 0x40;
	buffer->operation = operation;
	buffer->argument = cpu_to_le16(argument);

	// Send to the wheel desidered return force
	errno = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		buffer,
		sizeof(struct operation40), &boh,
		SETTINGS_TIMEOUT
	);

	if(errno)
		printk(
			KERN_ERR "t150: errno %d during operation 0x40 0x%02hhX with argument (big endian) %04hhX",
			errno, operation, argument);

	return errno;
}

static int t150_setup_task(struct t150 *t150)
{
	int errno = 0;
	uint8_t *fw_version;

	fw_version = kzalloc(8, GFP_KERNEL);

	// Retrive current version
	mutex_lock(&t150->lock);
	
	errno = usb_control_msg(
		t150->usb_device,
		usb_rcvctrlpipe(t150->usb_device, 0),
		86, 0xc1, 0, 0, fw_version, 8, SETTINGS_TIMEOUT
	);

	mutex_unlock(&t150->lock);

	if(errno < 0)
		printk(KERN_ERR "t150: Error %d while sending the control URB to retrive firmware version\n", errno);
	else
		t150->settings.firmware_version = fw_version[1];

	errno = t150_set_gain(t150, 80);
	if(errno)
		printk(KERN_ERR "t150: Error %d while setting the t150 default gain\n", errno);

	errno = t150_set_enable_autocenter(t150, false);
	if(errno)
		printk(KERN_ERR "t150: Error %d while setting the t150 default enable_autocenter\n", errno);

	errno = t150_set_autocenter(t150, 50);
	if(errno)
		printk(KERN_ERR "t150: Error %d while setting the t150 default autocenter\n", errno);

	errno = t150_set_range(t150, 0xffff);
	if(errno)
		printk(KERN_ERR "t150: Error %d while setting the t150 default range\n", errno);

	printk(KERN_INFO "t150: setup completed! Firmware version is %d\n", t150->settings.firmware_version);

	kzfree(fw_version);
	return errno;
}
