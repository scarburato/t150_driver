#define USB_THRUSTMASTER_VENDOR_ID	0x044f
#define USB_T150_PRODUCT_ID		0xb677

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

	// Input api stuff
	char dev_path[128];
	struct input_dev *joystick;

	struct usb_anchor misc_ffb_ops;
	struct urb *update_ffb_urbs[FF_MAX_EFFECTS][3];
	unsigned update_ffb_free_slot;

	struct {
		spinlock_t access_lock;
		unsigned long access_lock_flags;

		uint8_t autocenter_force;
		bool autocenter_enabled;
		uint16_t range;
		uint8_t gain;

		uint8_t firmware_version;
	} settings;
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

static inline void printP(uint8_t const *const bytes, const size_t length)
{
	int i;
	char printstr[202] = "t150: ";

	if(length > 64) // Packet too big :/
		return;

	for(i = 0; i < length; i++)
		sprintf( printstr + (6 + i*3),"%02hhX ", bytes[i]);

	sprintf(printstr + (9 + length*3),"\n");

	printk(printstr);
}