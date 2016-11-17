/*
 * ZigBee_messages_Rx.c
 *
 *  Created on: Nov 9, 2016
 *      Author: michele
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "my_queues.h"
#include "ASACZ_message_history.h"
#include "ASAC_ZigBee_network_commands.h"

typedef struct _type_handle_messages_Rx
{
	type_my_queue queue;//!< the message queue
	pthread_mutex_t mtx; 			//!< the mutex to access both the message queue and the message history
}type_handle_messages_Rx;
static type_handle_messages_Rx handle_messages_Rx;

void start_queue_message_Rx(void)
{
	init_my_queue(&handle_messages_Rx.queue);
	pthread_mutex_init(&handle_messages_Rx.mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_messages_Rx.mtx, &mutexattr);
	}
}

unsigned int is_OK_push_Rx_outside_message(type_ASAC_ZigBee_interface_command_received_message_callback *p, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_Rx.mtx);
		enum_push_my_queue_retcode r;
		r = push_my_queue(&handle_messages_Rx.queue, (uint8_t *)p, NUM_BYTES_IN_command_received(p), p_id);
		switch(r)
		{
			case enum_push_my_queue_retcode_OK:
			{
				//uint32_t message_id = * p_id;
				retcode = 1;
				break;
			}
			default:
			{
				break;
			}
		}
	pthread_mutex_unlock(&handle_messages_Rx.mtx);
	return retcode;
}

unsigned int is_OK_pop_Rx_outside_message(type_ASAC_ZigBee_interface_command_received_message_callback *p, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_Rx.mtx);
		unsigned int ui_size_of_popped_element = 0;
		enum_pop_my_queue_retcode r = pop_my_queue(&handle_messages_Rx.queue, (uint8_t *)p, &ui_size_of_popped_element, sizeof(*p), p_id);
		if (r == enum_pop_my_queue_retcode_OK )
		{
			retcode =1;
		}
	pthread_mutex_unlock(&handle_messages_Rx.mtx);
	return retcode;
}
