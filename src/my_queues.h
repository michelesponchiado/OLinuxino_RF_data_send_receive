/*
 * my_queues.h
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */

#ifndef MY_QUEUES_H_
#define MY_QUEUES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>

#define def_my_queue_too_big_message_is_an_error

#define def_invalid_id 0

typedef struct _type_my_queue_indexes
{
	unsigned int idx;
	unsigned int num_of;
}type_my_queue_indexes;

#define def_size_my_elem_queue 4096
typedef struct _type_my_queue_elem
{
	uint32_t id;
	unsigned int size;
	uint8_t e[def_size_my_elem_queue];
}type_my_queue_elem;

#define def_N_my_elem_queue 16
typedef struct _type_my_queue
{
	uint32_t last_id_used;
	type_my_queue_indexes r,w;
	type_my_queue_elem queue[def_N_my_elem_queue];
	pthread_mutex_t mtx;

	unsigned int too_big_elements_pushed;
	unsigned int too_big_elements_popped;
}type_my_queue;


void init_my_queue(type_my_queue *p);
typedef enum
{
	enum_push_my_queue_retcode_OK = 0,
	enum_push_my_queue_retcode_no_room,
	enum_push_my_queue_retcode_too_big_element,
	enum_push_my_queue_retcode_numof
}enum_push_my_queue_retcode;

enum_push_my_queue_retcode push_my_queue(type_my_queue *p, uint8_t *p_element_to_push, unsigned int elem_to_push_size, uint32_t * p_id);



typedef enum
{
	enum_pop_my_queue_retcode_OK = 0,
	enum_pop_my_queue_retcode_empty,
	enum_pop_my_queue_retcode_too_big_element,
	enum_pop_my_queue_retcode_numof
}enum_pop_my_queue_retcode;

enum_pop_my_queue_retcode pop_my_queue(type_my_queue *p, uint8_t *p_element_to_pop, unsigned int *p_elem_popped_size, unsigned int max_size, uint32_t *p_id);
#endif /* MY_QUEUES_H_ */
