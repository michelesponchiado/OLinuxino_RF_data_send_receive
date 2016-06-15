/*
 * ZigBee_messages.c
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#include "my_queues.h"

type_my_queue my_queue_message_to_ZigBee;

void start_queue_message_to_ZigBee(void)
{
	init_my_queue(&my_queue_message_to_ZigBee);
}

unsigned int is_OK_push_message_to_Zigbee(char *message, unsigned int message_size, uint32_t *p_id)
{
	unsigned int retcode = 0;
	enum_push_my_queue_retcode r;
	r = push_my_queue(&my_queue_message_to_ZigBee, (uint8_t *)message, message_size, p_id);
	switch(r)
	{
		case enum_push_my_queue_retcode_OK:
		{
			retcode = 1;
			break;
		}
		default:
		{
			break;
		}
	}
	return retcode;
}


unsigned int is_OK_pop_message_to_Zigbee(char *message, unsigned int *popped_message_size, unsigned int max_size, uint32_t *p_id)
{
	unsigned int retcode = 0;
	enum_pop_my_queue_retcode r = pop_my_queue(&my_queue_message_to_ZigBee, (uint8_t *)message, popped_message_size, max_size, p_id);
	if (r == enum_pop_my_queue_retcode_OK )
	{
		retcode =1;
	}
	return retcode;
}

unsigned int get_invalid_id(void)
{
	return def_invalid_id;
}

