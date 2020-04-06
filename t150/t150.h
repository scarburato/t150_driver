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

	// Device file
	char dev_path[128];

	struct input_dev *joystick;
	uint8_t current_rotation;
};

struct joy_state_packet
{
	uint8_t		__padding0; // UNKNOWN

	/* Range from 0x0000 (full left) to 0xffff (full right)
	The range is relative to the current max rotation
	**/
	uint8_t		wheel_axis_low;
	uint8_t		wheel_axis_high;

	/** 0x000 when pedal released to 0x3ff when the pedal is fully pressed */
	uint8_t		brake_axis_low;
	uint8_t		brake_axis_high;

	/** 0x000 when pedal released to 0x3ff when the pedal is fully pressed */
	uint8_t		gas_axis_low;
	uint8_t		gas_axis_high;

	// FIXME This in absumtion!
	/** 0x00 when pedal released to 0xff when the pedal is fully pressed */
	uint8_t		clutch_axis_low;
	uint8_t		clutch_axis_high;

	uint8_t		__padding1; // UNKNOWN
	uint8_t		__padding2; // UNKNOWN
	uint8_t		__padding3; // UNKNOWN
	uint8_t		__padding4; // UNKNOWN
	uint8_t		__padding5; // UNKNOWN

	uint8_t		cross_state;
};

const struct d_pad_pos
{
	int8_t x;
	int8_t y;
} CROSS_POSITIONS[] = {
	{+1,  0}, // 0x00 north
	{+1, +1}, // 0x01 north-est
	{0 , +1}, // 0x02 test
	{-1, +1}, // 0x03 south-est
	{-1,  0}, // 0x04 south
	{-1, -1}, // 0x05 south-west
	{0 , -1}, // 0x06 west
	{+1, -1}, // 0x07 nort-west
	{0, 0}
};


/** Function declearations **/
static int t150_init_input(struct t150 *t150);
static void t150_update_input(struct urb *urb);
