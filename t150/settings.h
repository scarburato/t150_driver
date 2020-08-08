#define SETTINGS_TIMEOUT 1000

typedef uint8_t operation_t;
#define SET40_RETURN_FORCE		0x03
#define SET40_USE_RETURN_FORCE		0x04
#define SET40_RANGE			0x11

struct operation40
{
	uint8_t		code; // MUST BE 0x40
	operation_t	operation;
	uint16_t	argument;
};

/** [SEEMS LIKE IT'S NOT NEEDED :P] Packet to send to the Wheel when we do not want to edit its settings anymore **/
#define SET42_STOP_WHEEL_SETTINGS	0x00
/** [SEEMS LIKE IT'S NOT NEEDED :P] Packet to send to the Wheel when we want to edit its settings **/
#define SET42_START_WHEEL_SETTINGS	0x04
/** [SEEMS LIKE IT'S NOT NEEDED :P] Packet to send to the Wheel when we want to apply the new settings **/
#define SET42_APPLY_WHEEL_SETTINGS	0x05

struct opertation42
{
	uint8_t		code; // MUST BE 0x42
	operation_t	operation;
};

static int t150_settings_set40(struct t150 *t150, operation_t operation, 
	uint16_t argument, void *buffer);

static int t150_set_gain(struct t150 *t150, uint8_t gain);
static __always_inline int t150_set_autocenter(struct t150 *t150, uint8_t autocenter_force);
static __always_inline int t150_set_enable_autocenter(struct t150 *t150, bool enable);
static __always_inline int t150_set_range(struct t150 *t150, uint16_t range);