/*
 * ASACSOKET_check.c
 *
 *  Created on: Jun 8, 2016
 *      Author: michele
 */



// TODO: find a better way for all of these defines for the messages size
// 1<<12 = 4096
#define def_max_length_body_ASACSOCKET_msg_numbit_shift 12
#define def_max_length_body_ASACSOCKET_msg (1<<(def_max_length_body_ASACSOCKET_msg_numbit_shift))

// use this to enable footer generation and check
#define def_enable_ASACSOCKET_footer

// the key to put at the very beginning of the message: 32-bit value
#define def_ASACSOCKET_key_header 0xA55A3663

#ifdef def_enable_ASACSOCKET_footer
	// the key to put at the very end of the message: 32-bit value
	#define def_ASACSOCKET_key_footer 0x81c369F0
	typedef struct _type_struct_ASACSOCKET_footer
	{
		uint32_t key;
	}__attribute__((__packed__)) type_struct_ASACSOCKET_footer ;
#endif

// the header put at the very beginning of the message
typedef struct _type_struct_ASACSOCKET_header
{
	uint32_t key;
	uint32_t body_length;
	uint32_t body_crc;
}__attribute__((__packed__)) type_struct_ASACSOCKET_header ;

// the ASACSOCKET_msg structure
typedef struct _type_struct_ASACSOCKET_msg
{
	type_struct_ASACSOCKET_header h;
	unsigned char body[def_max_length_body_ASACSOCKET_msg];
	// at the very end of the message an uint32_t value set to def_ASACSOCKET_key_footer?
#ifdef def_enable_ASACSOCKET_footer
	// ?the key to put at the very end of the message: 32-bit value
	type_struct_ASACSOCKET_footer f;
#endif
}__attribute__((__packed__)) type_struct_ASACSOCKET_msg ;


typedef enum
{
	enum_check_ASACSOCKET_formatted_message_OK = 0,
	enum_check_ASACSOCKET_formatted_message_ERR_key_header,
	enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_header,
	enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_body,
	enum_check_ASACSOCKET_formatted_message_ERR_length_too_short_no_footer,
	enum_check_ASACSOCKET_formatted_message_ERR_length_too_big,
	enum_check_ASACSOCKET_formatted_message_ERR_crc,
	enum_check_ASACSOCKET_formatted_message_ERR_key_footer,
	enum_check_ASACSOCKET_formatted_message_ERR_unknown,
	enum_check_ASACSOCKET_formatted_message_numof
}enum_check_ASACSOCKET_formatted_message;

typedef enum
{
	enum_build_ASACSOCKET_formatted_message_OK = 0,
	enum_build_ASACSOCKET_formatted_message_ERR_too_long_body,
	enum_build_ASACSOCKET_formatted_message_ERR_unknown,
	enum_build_ASACSOCKET_formatted_message_numof
}enum_build_ASACSOCKET_formatted_message;

typedef struct _type_struct_ASACSOCKET_check_stats
{
	uint32_t check_retcodes[enum_check_ASACSOCKET_formatted_message_numof];
	uint32_t num_check_done;
	uint32_t build_retcodes[enum_build_ASACSOCKET_formatted_message_numof];
	uint32_t num_build_done;
}type_struct_ASACSOCKET_check_stats;

// the stats about the various messages checked
extern type_struct_ASACSOCKET_check_stats ASACSOCKET_check_stats;

// useful to check whether a received message is in the proper format or not
enum_check_ASACSOCKET_formatted_message check_ASACSOCKET_formatted_message(char *buffer, int buf_len);

// useful to build a message in the proper format to be sent through ASACSOCKET message
enum_build_ASACSOCKET_formatted_message build_ASACSOCKET_formatted_message(type_struct_ASACSOCKET_msg * p, char *buffer, int buf_len, unsigned int *pui_message_size);
