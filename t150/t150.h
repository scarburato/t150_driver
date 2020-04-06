#define USB_THRUSTMASTER_VENDOR_ID	0x044f
#define USB_T150_PRODUCT_ID		0xb677

/** **/
struct joy_state_packet;
struct t150
{
	// USB stuff
	struct usb_device *usb_device;
	struct urb *joy_request_out;
	struct urb *joy_request_in;
	struct joy_state_packet *joy_data_in;
	struct joy_state_packet *joy_data_in_dma;

	struct input_dev *joystick;
	uint8_t current_rotation;
};

struct joy_state_packet
{
	uint8_t		__padding0; // UNKNOWN

	/* Range from 0x0000 (full left) to 0xffff (full right)
	The range is relative to the current max rotation
	**/
	uint8_t		stering_wheel_axis_high;
	uint8_t		stering_wheel_axis_low;

	/** 0x00 when pedal released to 0xff when the pedal is fully pressed */
	uint8_t		brake_axis;
	/** 0b00 full pressed; 0b10 partialy pressed; 0b11 released */
	uint8_t		brake_flags;

	/** 0x00 when pedal released to 0xff when the pedal is fully pressed */
	uint8_t		gas_axis;
	/** 0b00 full pressed; 0b10 partialy pressed; 0b11 released */
	uint8_t		gas_flags;

	// FIXME This in absumtion!
	/** 0x00 when pedal released to 0xff when the pedal is fully pressed */
	uint8_t		clutch_axis;
	/** 0b00 full pressed; 0b10 partialy pressed; 0b11 released */
	uint8_t		clutch_flags;

	uint8_t		__padding1; // UNKNOWN
	uint8_t		__padding2; // UNKNOWN
	uint8_t		__padding3; // UNKNOWN
	uint8_t		__padding4; // UNKNOWN
	uint8_t		__padding5; // UNKNOWN

	uint8_t		cross;
};


/** Function declearations **/
static int t150_init_input(struct t150 *t150);
static void t150_update_input(struct urb *urb);
