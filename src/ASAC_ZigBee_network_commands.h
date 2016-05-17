/*
 * ASAC_ZigBee_network_commands.h
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#ifndef ASAC_ZIGBEE_NETWORK_COMMANDS_H_
#define ASAC_ZIGBEE_NETWORK_COMMANDS_H_

#include <stdint.h>


typedef enum
{
// the commands we send to/receive from the outside world, e.g. the user messages
	enum_ASAC_ZigBee_interface_command_outside_first = 0x10000000,
	enum_ASAC_ZigBee_interface_command_outside_send_message = enum_ASAC_ZigBee_interface_command_outside_first,

// the commands we use to handle the network, e.g. the introduce myself which sets my name in the network
	enum_ASAC_ZigBee_interface_command_network_first = 0x20000000,
	enum_ASAC_ZigBee_interface_command_network_introduce_myself_set = enum_ASAC_ZigBee_interface_command_network_first,
	enum_ASAC_ZigBee_interface_command_network_introduce_myself_get,
// the commands used by the administrator
	enum_ASAC_ZigBee_interface_command_administrator_first = 0x30000000,
	enum_ASAC_ZigBee_interface_command_administrator_firmware_update = enum_ASAC_ZigBee_interface_command_administrator_first,
// the command used to reply to unknown commands
	enum_ASAC_ZigBee_interface_command_unknown = 0xFFFFFFFF,

}enum_ASAC_ZigBee_interface_command;

//
//
//
// SEND OUTSIDE MESSAGE begins here
//
//
//

// the maximum size for a message coming to/ sending to the outside world
#define def_ASAC_ZigBee_interface_max_size_outside_message 4096

typedef struct _type_ASAC_ZigBee_interface_command_outside_send_message_req
{
	uint8_t message[def_ASAC_ZigBee_interface_max_size_outside_message];
}type_ASAC_ZigBee_interface_command_outside_send_message_req;

// the return codes from the send user message outside
typedef enum
{
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK = 0,
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_ERROR_unable_to_push_message, // no room in the radio messages queue
	enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_numof
}enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode;

typedef struct _type_ASAC_ZigBee_interface_command_outside_send_message_reply
{
	uint32_t retcode; // must be of type enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode
	uint32_t id; // the message identifier / handle
}type_ASAC_ZigBee_interface_command_outside_send_message_reply;

//
//
//
// SEND OUTSIDE MESSAGE ends here
//
//
//

//
//
//
// INTRODUCE MYSELF begins here
//
//
//

// maximum number of characters to define my own name
#define def_ASAC_ZigBee_interface_max_char_my_name 128

typedef struct _type_ASAC_ZigBee_interface_command_network_introduce_myself_set_req
{
	uint8_t my_name[def_ASAC_ZigBee_interface_max_char_my_name];
}type_ASAC_ZigBee_interface_command_network_introduce_myself_set_req;

// the return codes from the send user message outside
typedef enum
{
	enum_ASAC_ZigBee_interface_command_outside_introduce_myself_set_reply_retcode_OK = 0,
	enum_ASAC_ZigBee_interface_command_outside_introduce_myself_set_reply_retcode_ERROR,
	enum_ASAC_ZigBee_interface_command_outside_introduce_myself_set_reply_retcode_numof
}enum_ASAC_ZigBee_interface_command_introduce_myself_set_reply_retcode;

typedef struct _type_ASAC_ZigBee_interface_command_network_introduce_myself_set_reply
{
	uint32_t retcode; // must be of type enum_ASAC_ZigBee_interface_command_introduce_myself_set_reply_retcode
}type_ASAC_ZigBee_interface_command_network_introduce_myself_set_reply;

typedef struct _type_ASAC_ZigBee_interface_unknown_reply
{
	uint32_t the_unknown_code; // the unknown code we got
}type_ASAC_ZigBee_interface_unknown_reply;

//
//
//
// INTRODUCE MYSELF ends here
//
//
//


//
//
//
// The ZigBee interface command structure begins here
//
//
//

typedef struct _type_ASAC_Zigbee_interface_command
{
	uint32_t code; // the command code, MUST be one of enum_ASAC_ZigBee_interface_command
	// the union of all of the possibles requests from the clients
	union
	{
		// outside messages requests
		type_ASAC_ZigBee_interface_command_outside_send_message_req outside_send_message;

		// network messages requests
		type_ASAC_ZigBee_interface_command_network_introduce_myself_set_req network_introduce_myself_set;
	}req;
}type_ASAC_Zigbee_interface_command;


typedef struct _type_ASAC_Zigbee_interface_command_reply
{
	uint32_t code; // the command code, MUST be one of enum_ASAC_ZigBee_interface_command
	// the union of all of the possibles replies of the server to the clients requests
	union
	{
		// outside messages replies
		type_ASAC_ZigBee_interface_command_outside_send_message_reply outside_send_message;

		// network messages replies
		type_ASAC_ZigBee_interface_command_network_introduce_myself_set_reply network_introduce_myself;

		// reply to an unknown command
		type_ASAC_ZigBee_interface_unknown_reply unknown_reply;
	}reply;
}type_ASAC_Zigbee_interface_command_reply;

#define def_size_ASAC_Zigbee_interface_code(p_the_struct) (sizeof(p_the_struct->code))

#define def_size_ASAC_Zigbee_interface_req(p_the_struct,the_command_body) (sizeof(p_the_struct->code) + sizeof(p_the_struct->req.the_command_body))
#define def_size_ASAC_Zigbee_interface_reply(p_the_struct,the_command_body) (sizeof(p_the_struct->code) + sizeof(p_the_struct->reply.the_command_body))

//
//
//
// The ZigBee interface command structure ends here
//
//
//

#endif /* ASAC_ZIGBEE_NETWORK_COMMANDS_H_ */
