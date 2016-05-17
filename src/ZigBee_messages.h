/*
 * ZigBee_messages.h
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#ifndef ZIGBEE_MESSAGES_H_
#define ZIGBEE_MESSAGES_H_

#define def_too_big_message_is_an_error

void start_queue_message_to_ZigBee(void);
unsigned int is_OK_push_message_to_Zigbee(char *message, unsigned int message_size, uint32_t *p_id);
unsigned int is_OK_pop_message_to_Zigbee(char *message, unsigned int *popped_message_size, unsigned int max_size, uint32_t *p_id);
unsigned int get_invalid_id(void);

#endif /* ZIGBEE_MESSAGES_H_ */
