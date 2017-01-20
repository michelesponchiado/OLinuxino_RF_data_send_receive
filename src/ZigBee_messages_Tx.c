/*
 * ZigBee_messages.c
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */
#include <ASAC_ZigBee_network_commands.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "../inc/ASACZ_message_history.h"
#include "../inc/my_queues.h"

typedef struct _type_handle_messages_Tx
{
	type_my_queue queue;//!< the message queue
	pthread_mutex_t mtx; 			//!< the mutex to access both the message queue and the message history
}type_handle_messages_Tx;
static type_handle_messages_Tx handle_messages_Tx;

void start_queue_message_Tx(void)
{
	init_my_queue(&handle_messages_Tx.queue);
	pthread_mutex_init(&handle_messages_Tx.mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_messages_Tx.mtx, &mutexattr);
	}
}

unsigned int is_OK_push_Tx_outside_message(type_ASAC_ZigBee_interface_command_outside_send_message_req *p, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_Tx.mtx);
		enum_push_my_queue_retcode r;
		r = push_my_queue(&handle_messages_Tx.queue, (uint8_t *)p, NUM_BYTES_IN_outside_send_message_req(p), p_id);
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
	pthread_mutex_unlock(&handle_messages_Tx.mtx);
	return retcode;
}


unsigned int is_OK_pop_Tx_outside_message(type_ASAC_ZigBee_interface_command_outside_send_message_req *p, uint32_t *p_id)
{
	unsigned int retcode = 0;
	pthread_mutex_lock(&handle_messages_Tx.mtx);
		unsigned int ui_size_of_popped_element = 0;
		enum_pop_my_queue_retcode r = pop_my_queue(&handle_messages_Tx.queue, (uint8_t *)p, &ui_size_of_popped_element, sizeof(*p), p_id);
		if (r == enum_pop_my_queue_retcode_OK )
		{
			retcode =1;
		}
	pthread_mutex_unlock(&handle_messages_Tx.mtx);
	return retcode;
}

unsigned int get_invalid_id(void)
{
	return def_invalid_id;
}

