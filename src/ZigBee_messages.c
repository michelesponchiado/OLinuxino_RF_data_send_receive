/*
 * ZigBee_messages.c
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "my_queues.h"
#include "ASACZ_message_history.h"

typedef struct _type_handle_messages_to_Zigbee
{
	type_my_queue queue;//!< the message queue
	pthread_mutex_t mtx; 			//!< the mutex to access both the message queue and the message history
}type_handle_messages_to_Zigbee;
static type_handle_messages_to_Zigbee handle_messages_to_Zigbee;

void start_queue_message_to_ZigBee(void)
{
	init_my_queue(&handle_messages_to_Zigbee.queue);
	pthread_mutex_init(&handle_messages_to_Zigbee.mtx, NULL);
}

unsigned int is_OK_push_message_to_Zigbee(char *message, unsigned int message_size, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_to_Zigbee.mtx);
		enum_push_my_queue_retcode r;
		r = push_my_queue(&handle_messages_to_Zigbee.queue, (uint8_t *)message, message_size, p_id);
		switch(r)
		{
			case enum_push_my_queue_retcode_OK:
			{
				uint32_t message_id = * p_id;
				// add the new message id to the history list, with status enum_message_history_status_in_waiting_queue
				message_history_tx_add_id(message_id, enum_message_history_status_in_waiting_queue);
				retcode = 1;
				break;
			}
			default:
			{
				break;
			}
		}
	pthread_mutex_unlock(&handle_messages_to_Zigbee.mtx);
	return retcode;
}


unsigned int is_OK_pop_message_to_Zigbee(char *message, unsigned int *popped_message_size, unsigned int max_size, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_to_Zigbee.mtx);
		enum_pop_my_queue_retcode r = pop_my_queue(&handle_messages_to_Zigbee.queue, (uint8_t *)message, popped_message_size, max_size, p_id);
		if (r == enum_pop_my_queue_retcode_OK )
		{
			retcode =1;
		}
	pthread_mutex_unlock(&handle_messages_to_Zigbee.mtx);
	return retcode;
}

unsigned int get_invalid_id(void)
{
	return def_invalid_id;
}

