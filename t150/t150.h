#define USB_THRUSTMASTER_VENDOR_ID	0x044f
#define USB_T150_PRODUCT_ID		0xb677
static const char *no_str = "Non implementato :/ \n";
/** **/

struct joy_state_packet;
struct t150
{
	// USB stuff
	struct usb_device *usb_device;

	// Stuff to read from the wheel
	struct joy_state_packet *joy_data_in;
	struct urb *joy_request_in;
	int pipe_in;

	// Stuff to write to the wheel
	struct joy_state_packet *joy_data_in_dma;
	struct urb *joy_request_out;
	int pipe_out;

	// sysf STUFF
	struct kobject *kobj_my_dir;

	// Input api stuff
	char dev_path[128];
	struct input_dev *joystick;

	/** Mutex used to allow one operation at time on the Wheel */
	struct mutex *lock;

	volatile uint8_t current_rotation;
	volatile uint8_t return_force;
};

/** [?] Packet to send to the Wheel when we want to edit its settings **/
static const uint8_t start_input_settings[] = {0x42, 0x04};

/** [?] Packet to send to the Wheel when we want to apply the new settings **/
static const uint8_t apply_input_settings[] = {0x42, 0x05};

/** [?] Packet to send to the Wheel when we do not want to edit its settings anymore **/
static const uint8_t stop_input_settings[] = {0x42, 0x00};

//structs about packets entering the host
/**
 *
 */
struct joy_state_packet
{
	/** 0x07 if this packet contains the wheel current input status */
	uint8_t		packet_flags;

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

	/** Some buttons */
	uint8_t		buttons_state;

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

// structs about packets entering the wheel
struct set_return_force
{
	/** Must be 0x40 to set the wheel pos*/
	const uint8_t code0;
	/** Must be 0x03*/
	const uint8_t code1;

	/** The force between 0x00 and 0x64 */
	uint8_t return_force;

	uint8_t zero;
};

#define BTN_GEAR_UP_MASK	0b00000001
#define BTN_GEAR_DOWN_MASK	0b00000010

/**
 * Simple macro to make a word from two bytes
 * @low the low part of a word
 * @high the high part of a word
 */
static inline uint16_t make_word(const uint8_t low, const uint8_t high)
{
	return ((uint16_t)low | ((uint8_t)(high) << 8));
}

/** Function declearatioinit_inpuns **/
static inline int t150_init_input(struct t150 *t150);
static void t150_setup_end(struct urb *urb);

static int t150_open(struct input_dev *dev);
static void t150_close(struct input_dev *dev);

static void t150_update_input(struct urb *urb);
static inline int t150_free_sysf(struct t150 *t150, struct usb_interface *uif);
static inline int t150_init_sysf(struct t150 *t150, struct usb_interface *uif);
