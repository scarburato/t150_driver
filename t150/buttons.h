struct button_mask
{
    uint16_t key;
    uint8_t mask;
};

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
	{ BTN_GAMEPAD,		0b00010000}
};