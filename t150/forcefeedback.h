#define T150_FF_CODE_CONSTANT		0x4000
#define T150_FF_CODE_SINE		0x4022
#define T150_FF_CODE_SAW_UP		0x4023
#define T150_FF_CODE_SAW_DOWN		0x4024

struct __packed ff_periodic
{
	/** Truncked value of maginute on 16 bit */
	int8_t		magnitude;
	/** Trunked value of offset  */
	int8_t		offset;
	/** Phase where 0x00 = 0° and 0xff = 360° */
	uint8_t		phase;
	/** Period in milliseconds */
	uint16_t	period;
};

struct __packed ff_constant
{
	/** the level of the effect  */
	int8_t		level;
};

struct __packed ff_first 
{
	uint8_t		f0;
	/** Seems is effect_id * 0x1c + 0x1c */
	uint8_t		pk_id0;
	uint8_t		f1;
	/** Attack length in milliseconds */
	uint16_t	attack_length;
	/** Do not know how it works */
	uint8_t		attack_level;
	/** Fade length in milliseconds */
	uint16_t	fade_length;
	/** Do not know how it works */
	uint8_t		fade_level;
};

struct __packed ff_second 
{
	/** 0x04 for periodic, 0x03 for const*/
	uint8_t		effect_class;
	/** seems is effect_id * 0x1c + 0x0e */
	uint8_t		pk_id1;
	uint8_t		f1;

	/** Fields specific to effect class */
	union {
		struct ff_periodic periodic;
		struct ff_constant constant;
	} effect;
};

struct __packed ff_third
{
	uint8_t		f0;
	uint8_t		id;
	/** Effect code */
	uint16_t	effect_type;
	/** Length of the effect in milliseconds */
	uint16_t	length;
	/** Do not know how it works */
	uint16_t	f1;
	uint8_t		f2;
	/** seems is effect_id * 0x1c + 0x0e */
	uint8_t		pk_id1;
	uint8_t		f3;
	/** Seems is effect_id * 0x1c + 0x1c */
	uint8_t		pk_id0;
	uint8_t		f4;
	/** Delay in milliseconds */
	uint8_t		delay;
	uint8_t		f5;
};

struct __packed ff_change_effect_status
{
	uint8_t f0;
	uint8_t id;
	uint8_t mode;
	uint8_t times;
};

struct __packed ff_change_gain
{
	uint8_t f0;
	uint8_t gain;
};

union __packed ff_change
{
	struct ff_change_effect_status effect;
	struct ff_change_gain gain;
};

static int t150_init_ffb(struct t150 *t150);
static void t150_close_ffb(struct t150 *t150);

static int t150_ff_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old);
static int t150_ff_erase(struct input_dev *dev, int effect_id);
static int t150_ff_play(struct input_dev *dev, int effect_id, int value);
static void t150_ff_set_gain(struct input_dev *dev, uint16_t gain);

static uint8_t t150_ffb_effects_length = 6;
static const int16_t t150_ffb_effects[] = {
	FF_GAIN,

	FF_PERIODIC,
	FF_SINE,
	FF_SAW_UP,
	FF_SAW_DOWN,

	FF_CONSTANT
};
