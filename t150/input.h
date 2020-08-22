struct t150_state_packet;
struct __packed t150_input_state_packet
{
	/** Range from 0x0000 (full left) to 0xffff (full right)
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

struct button_mask
{
    uint16_t key;
    uint8_t mask;
};

static inline int t150_init_input(struct t150 *t150);
static inline void t150_free_input(struct t150 *t150);
static int t150_input_open(struct input_dev *dev);
static void t150_input_close(struct input_dev *dev);
static int t150_update_input(struct hid_device *hdev, struct hid_report *report, uint8_t *packet_raw, int size);

static char const *const nameWH = "Thrustmaster T150 steering wheel";
static uint16_t *packet_input_open = 0;
/** It seems it's used to purge all uploaded effects from the wheel, not sure */
static uint16_t *packet_input_what = 0;
static uint16_t *packet_input_close = 0; 

static const size_t buttons_state0_assoc_length = 8;
static const struct button_mask buttons_state0_assoc[] = {
	{ BTN_GEAR_UP,		0b00000001},
	{ BTN_GEAR_DOWN,	0b00000010},
	{ BTN_NORTH,		0b00000100},
	{ BTN_WEST,		0b00001000},
	{ BTN_EAST,		0b00010000},
	{ BTN_SOUTH,		0b00100000},
	{ BTN_SELECT,		0b01000000},
	{ BTN_START,		0b10000000}
};

static const size_t buttons_state1_assoc_length = 5;
static const struct button_mask buttons_state1_assoc[] = {
	{ BTN_TR,		0b00000001},
	{ BTN_TL,		0b00000010},
	{ BTN_TR2,		0b00000100},
	{ BTN_TL2,		0b00001000},
	{ BTN_JOYSTICK,		0b00010000}
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
