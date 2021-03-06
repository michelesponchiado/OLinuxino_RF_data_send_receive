/*
 * my_queues.c
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */
#include "my_queues.h"

#include <syslog.h>


void init_my_queue(type_my_queue *p)
{
	memset(p,0,sizeof(*p));
	pthread_mutex_init(&p->mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&p->mtx, &mutexattr);
	}
}


#if 0
static unsigned int n_elem_my_queue(type_my_queue *p)
{
	unsigned int n = 0;
	pthread_mutex_lock(p->mtx);
		n= p->w.num_of -p->r.num_of;
	pthread_mutex_unlock(p->mtx);
	return n;
}

static unsigned int is_empty_my_queue(type_my_queue *p)
{
	return n_elem_my_queue(p) == 0 ? 1: 0;
}
static unsigned int is_full_my_queue(type_my_queue *p)
{
	return n_elem_my_queue(p) >= def_N_my_elem_queue ? 1: 0;
}
#endif


static uint32_t get_new_id(uint32_t last_id_used)
{
	if (++last_id_used == def_invalid_id)
	{
		last_id_used = 1;
	}
	return last_id_used;
}
enum_push_my_queue_retcode push_my_queue(type_my_queue *p, uint8_t *p_element_to_push, unsigned int elem_to_push_size, uint32_t * p_id)
{
	enum_push_my_queue_retcode r = enum_push_my_queue_retcode_OK;
	pthread_mutex_lock(&p->mtx);
		if (r == enum_push_my_queue_retcode_OK)
		{
			unsigned int n = 0;
			n= p->w.num_of -p->r.num_of;
			if (n >= def_N_my_elem_queue)
			{
				r = enum_push_my_queue_retcode_no_room;
			}
		}
		if (r == enum_push_my_queue_retcode_OK)
		{
			type_my_queue_indexes * p_idx = &p->w;
			unsigned int idx = p_idx->idx;
			if (idx >= def_N_my_elem_queue)
			{
				idx = 0;
			}
			type_my_queue_elem * p_element_in_queue = &p->queue[idx];
			unsigned int n_bytes_to_copy = elem_to_push_size;
			if (n_bytes_to_copy > def_size_my_elem_queue)
			{
				n_bytes_to_copy = def_size_my_elem_queue;
				p->stats.num_too_big_elements_pushed_ERR++;
				syslog(LOG_ERR, "Too big element to push in the queue, size: %u", n_bytes_to_copy);
#ifdef def_my_queue_too_big_message_is_an_error
				r = enum_push_my_queue_retcode_too_big_element;
#endif
			}
#ifdef def_my_queue_too_big_message_is_an_error
			if (r == enum_push_my_queue_retcode_OK)
#endif
			{
				uint32_t new_id = get_new_id(p->last_id_used);
				memcpy(p_element_in_queue->e, p_element_to_push, n_bytes_to_copy);
				p_element_in_queue->size = n_bytes_to_copy;
				p_element_in_queue->id = new_id;
				*p_id = new_id;
				p->last_id_used = new_id;
				if (++idx >= def_N_my_elem_queue)
				{
					idx = 0;
				}
				p_idx->idx = idx;
				p_idx->num_of++;
			}
		}
		if (r == enum_push_my_queue_retcode_OK)
		{
			p->stats.num_push_OK++;
		}
		else
		{
			p->stats.num_push_ERR++;
		}
	pthread_mutex_unlock(&p->mtx);
	return r;
}



enum_pop_my_queue_retcode pop_my_queue(type_my_queue *p, uint8_t *p_element_to_pop, unsigned int *p_elem_popped_size, unsigned int max_size, uint32_t * p_id)
{
	enum_pop_my_queue_retcode r = enum_pop_my_queue_retcode_OK;
	pthread_mutex_lock(&p->mtx);
		if (r == enum_pop_my_queue_retcode_OK)
		{
			unsigned int n = 0;
			n= p->w.num_of -p->r.num_of;
			if (n == 0)
			{
				r = enum_pop_my_queue_retcode_empty;
			}
		}
		if (r == enum_pop_my_queue_retcode_OK)
		{
			type_my_queue_indexes * p_idx = &p->r;
			unsigned int idx = p_idx->idx;
			if (idx >= def_N_my_elem_queue)
			{
				idx = 0;
			}
			type_my_queue_elem * p_element_in_queue = &p->queue[idx];
			unsigned int n_bytes_to_copy = max_size;
			if (n_bytes_to_copy > def_size_my_elem_queue)
			{
				n_bytes_to_copy = def_size_my_elem_queue;
				p->stats.num_too_big_elements_popped_ERR ++;
				syslog(LOG_ERR, "Too big element popping the queue, size: %u", n_bytes_to_copy);
#ifdef def_my_queue_too_big_message_is_an_error
				r = enum_pop_my_queue_retcode_too_big_element;
#endif
			}
#ifdef def_my_queue_too_big_message_is_an_error
			if (r == enum_pop_my_queue_retcode_OK)
#endif
			{
				if (p_element_to_pop)
				{
					memcpy(p_element_to_pop, p_element_in_queue->e, n_bytes_to_copy);
				}
				if (p_elem_popped_size)
				{
					*p_elem_popped_size = n_bytes_to_copy;
				}
				if (p_id)
				{
					*p_id = p_element_in_queue->id;
				}
			}
			if (++idx >= def_N_my_elem_queue)
			{
				idx = 0;
			}
			p_idx->idx = idx;
			p_idx->num_of++;
		}
		if (r == enum_pop_my_queue_retcode_OK)
		{
			p->stats.num_pop_OK++;
		}
		else
		{
			if (r != enum_pop_my_queue_retcode_empty)
			{
				p->stats.num_pop_ERR++;
			}
		}
	pthread_mutex_unlock(&p->mtx);
	return r;
}
