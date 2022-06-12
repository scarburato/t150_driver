#define STATE_PACKET_INPUT 0x07

/**
 * All data is stored in little endian
 */
struct __packed t150_state_packet
{
	/** 0x07 if this packet contains the wheel current input status */
	uint8_t		type;
};