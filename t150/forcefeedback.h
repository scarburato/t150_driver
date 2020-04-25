#define T150_FF_CODE_SINE		0x4022
#define T150_FF_CODE_SAW_UP		0x4023
#define T150_FF_CODE_SAW_DOWN		0x4024

struct __packed ff_first 
{
	uint16_t	f0;
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
	uint16_t	f0;
	/** Do not knwo how it works */
	uint16_t	magnitude;
	/** Do not know how it works */
	uint16_t	f1;
	/** Period in milliseconds */
	uint16_t	period;
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
	uint16_t	f2;
	uint16_t	f3;
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
	uint8_t f1;
};

static int t150_init_ffb(struct t150 *t150);
static void t150_close_ffb(struct t150 *t150);

static int t150_ff_upload(struct input_dev *dev, struct ff_effect *effect, struct ff_effect *old);
static int t150_ff_erase(struct input_dev *dev, int effect_id);
static int t150_ff_play(struct input_dev *dev, int effect_id, int value);

static uint8_t t150_ffb_effects_length = 4;
static const int16_t t150_ffb_effects[] = {
	FF_PERIODIC,
	FF_SINE,
	FF_SAW_UP,
	FF_SAW_DOWN
};
