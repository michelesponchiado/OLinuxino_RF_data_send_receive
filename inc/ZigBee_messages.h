/*
 * ZigBee_messages.h
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#ifndef INC_ZIGBEE_MESSAGES_H_
#define INC_ZIGBEE_MESSAGES_H_
#include <ASAC_ZigBee_network_commands.h>

#define def_too_big_message_is_an_error

void start_queue_message_Tx(void);
unsigned int is_OK_push_Tx_outside_message(type_ASAC_ZigBee_interface_command_outside_send_message_req *p, uint32_t *p_id);
unsigned int is_OK_pop_Tx_outside_message(type_ASAC_ZigBee_interface_command_outside_send_message_req *p, uint32_t *p_id);

void start_queue_message_Rx(void);
unsigned int is_OK_push_Rx_outside_message(type_ASAC_ZigBee_interface_command_received_message_callback *p, uint32_t *p_id);
unsigned int is_OK_pop_Rx_outside_message(type_ASAC_ZigBee_interface_command_received_message_callback *p, uint32_t *p_id);

unsigned int get_invalid_id(void);

#endif /* INC_ZIGBEE_MESSAGES_H_ */
