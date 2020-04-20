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

/**
 * @t150 
 * @argument a number between 0x00 and 0x80
 * @buffer a buffer of at least 2 BYTES
 */
static int t150_settings_set43(struct t150 *t150, uint8_t argument, void *buffer_)
{
	int boh, errno;
	uint8_t *buffer = buffer_;
	buffer[0] = 0x43;
	buffer[1] = argument;

	// Send to the wheel desidered return force
	errno = usb_interrupt_msg(
		t150->usb_device,
		t150->pipe_out,
		buffer,
		2, &boh,
		SETTINGS_TIMEOUT
	);

	if(errno)
		printk(
			KERN_ERR "t150: errno %d during operation 0x43 argument (big endian) %02hhX",
			errno,  argument);

	return errno;
	
}
