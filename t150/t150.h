#define USB_THRUSTMASTER_VENDOR_ID	0x044f
#define USB_T150_PRODUCT_ID		0xb677
static const char *no_str = "Non implementato :/ \n";

struct joy_state_packet;
struct ff_first;
struct ff_second;
struct ff_third;
union ff_change;

struct t150
{
	// USB stuff
	struct usb_device *usb_device;

	// Stuff to read from the wheel
	struct joy_state_packet *joy_data_in;
	struct urb *joy_request_in;
	int pipe_in;
	uint8_t bInterval_in;

	// Stuff to write to the wheel
	struct joy_state_packet *joy_data_in_dma;
	struct urb *joy_request_out;
	int pipe_out;
	uint8_t bInterval_out;

	// packets to be used with ffb
	//struct mutex ff_mutex;

	/** Used to run the initial wheel setup */
	struct task_struct *setup_task;

	// sysf STUFF
	struct kobject *kobj_my_dir;

	// Input api stuff
	char dev_path[128];
	struct input_dev *joystick;

	// Force feedback effects packages
	struct urb *ff_packets[FF_MAX_EFFECTS][2][3];
	uint8_t current_ff_packet[FF_MAX_EFFECTS];

	/** Mutex used to allow one operation at time on the Wheel */
	struct mutex *lock;

	volatile uint16_t current_rotation;
	volatile uint8_t current_return_force;
};

//structs about packets entering the host
/**
 * All data is stored in little endian
 */
struct __attribute__((packed)) joy_state_packet
{
	/** 0x07 if this packet contains the wheel current input status */
	uint8_t		packet_flags;

	/* Range from 0x0000 (full left) to 0xffff (full right)
	The range is relative to the current max rotation
	**/
	uint16_t	wheel_axis;

	/** 0x000 when pedal released to 0x3ff when the pedal is fully pressed */
	uint16_t	brake_axis;

	/** 0x000 when pedal released to 0x3ff when the pedal is fully pressed */
	uint16_t	gas_axis;

	// FIXME This in absumtion!
	/** 0x00 when pedal released to 0x3ff when the pedal is fully pressed */
	uint16_t	clutch_axis;

	uint8_t		__padding1; // UNKNOWN
	uint8_t		__padding2; // UNKNOWN

	/** Some buttons */
	uint8_t		buttons_state0;
	uint8_t		buttons_state1;

	uint8_t		__padding5; // UNKNOWN

	uint8_t		cross_state;
};

const struct d_pad_pos
{
	int8_t y;
	int8_t x;
} CROSS_POSITIONS[] = {
	{-1,  0}, // 0x00 north
	{-1, +1}, // 0x01 north-est
	{0 , +1}, // 0x02 test
	{+1, +1}, // 0x03 south-est
	{+1,  0}, // 0x04 south
	{+1, -1}, // 0x05 south-west
	{0 , -1}, // 0x06 west
	{-1, -1}, // 0x07 north-west
	{0, 0}
};

/**
 * Simple macro to make a word from two bytes
 * @low the low part of a word
 * @high the high part of a word
 */
static inline uint16_t make_word(const uint8_t low, const uint8_t high)
{
	return ((uint16_t)low | ((uint8_t)(high) << 8));
}

static inline uint8_t word_high(const uint16_t word)
{
	return word >> 8;
}

static inline uint8_t word_low(const uint16_t word)
{
	return word;
}

static inline void printP(uint8_t const *const print)
{
	int i;
	char printstr[55] = "t150: ";
	for(i = 0; i < 15; i++)
		sprintf(&printstr[6 + i*3],"%02hhX ", print[i]);

	printk(printstr);
}