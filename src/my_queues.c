/*
 * my_queues.c
 *
 *  Created on: May 16, 2016
 *      Author: michele
 */
#include "my_queues.h"


void init_my_queue(type_my_queue *p)
{
	memset(p,0,sizeof(*p));
	pthread_mutex_init(&p->mtx, NULL);

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

enum_push_my_queue_retcode push_my_queue(type_my_queue *p, uint8_t *p_element_to_push, unsigned int elem_to_push_size)
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
				p->too_big_elements_pushed ++;
				r = enum_push_my_queue_retcode_too_big_element;
			}
			memcpy(p_element_in_queue->e, p_element_to_push, n_bytes_to_copy);
			p_element_in_queue->size = n_bytes_to_copy;
			if (++idx >= def_N_my_elem_queue)
			{
				idx = 0;
			}
			p_idx->idx = idx;
			p_idx->num_of++;
		}
	pthread_mutex_unlock(&p->mtx);
	return r;
}



enum_pop_my_queue_retcode pop_my_queue(type_my_queue *p, uint8_t *p_element_to_pop, unsigned int *p_elem_popped_size, unsigned int max_size)
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
				p->too_big_elements_popped ++;
				r = enum_pop_my_queue_retcode_too_big_element;
			}
			memcpy(p_element_to_pop, p_element_in_queue->e, n_bytes_to_copy);
			*p_elem_popped_size = n_bytes_to_copy;
			if (++idx >= def_N_my_elem_queue)
			{
				idx = 0;
			}
			p_idx->idx = idx;
			p_idx->num_of++;
		}
	pthread_mutex_unlock(&p->mtx);
	return r;
}
