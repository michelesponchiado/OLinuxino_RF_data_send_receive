/*
 * update_end_point_list.c
 *
 *  Created on: Jan 16, 2017
 *      Author: michele
 */
#include "../inc/update_end_point_list.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include "dbgPrint.h"

#include "../inc/dataSendRcv.h"
#include "../inc/input_cluster_table.h"


#if UPDATE_END_POINT_INDEX_NUMBIT > 8
#error too big update end point list!
#endif

void init_update_end_point_list(type_update_end_point_list *p)
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

unsigned int num_elem_update_end_point_list(type_update_end_point_list *p)
{
	pthread_mutex_lock(&p->mtx);
		unsigned int num_w = p->num_w;
		unsigned int num_r = p->num_r;
	pthread_mutex_unlock(&p->mtx);
	unsigned int n_elem = (num_w - num_r);
	if (n_elem > def_N_elements_update_end_point_list)
	{
		n_elem = def_N_elements_update_end_point_list;
		p->stats.num_err_overflow++;
	}
	return n_elem;
}

unsigned int is_full_update_end_point_list(type_update_end_point_list *p)
{
	unsigned int nelem = num_elem_update_end_point_list(p);
	if (nelem >= def_N_elements_update_end_point_list)
	{
		return 1;
	}
	return 0;
}

unsigned int is_empty_update_end_point_list(type_update_end_point_list *p)
{
	unsigned int nelem = num_elem_update_end_point_list(p);
	if (nelem == 0)
	{
		return 1;
	}
	return 0;
}

uint8_t *p_find_update_end_point_list(type_update_end_point_list *p, uint8_t e)
{
	uint8_t * p_elem = NULL;
	pthread_mutex_lock(&p->mtx);
		unsigned int n_elem = num_elem_update_end_point_list(p);
		unsigned int loop_elem;
		for (loop_elem = 0; loop_elem  < n_elem; loop_elem++)
		{
			unsigned int idx_elem = ((p->idx_r + loop_elem) & (UPDATE_END_POINT_INDEX_MASK));
			uint8_t * p_cur_elem = &p->update_list[idx_elem];
			if (*p_cur_elem == e)
			{
				p_elem = p_cur_elem;
				break;
			}
		}
	pthread_mutex_unlock(&p->mtx);
	return p_elem;
}

unsigned int is_OK_get_next_update_end_point_list(type_update_end_point_list *p, uint8_t *p_elem)
{
	unsigned int is_OK = 0;
	pthread_mutex_lock(&p->mtx);
		if (!is_empty_update_end_point_list(p))
		{
			unsigned int idx_elem_read = ((p->idx_r) & (UPDATE_END_POINT_INDEX_MASK));
			// store in the return value the next value present in the read queue
			*p_elem = p->update_list[idx_elem_read];
			// blank the value in the list
			memset(&p->update_list[idx_elem_read], 0, sizeof(p->update_list[idx_elem_read]));
			// update the read index
			p->idx_r = ((idx_elem_read + 1) & (UPDATE_END_POINT_INDEX_MASK));
			// update the number of reads
			p->num_r ++;
			// the get must return OK
			is_OK = 1;
		}
	pthread_mutex_unlock(&p->mtx);
	return is_OK;
}


unsigned int is_OK_add_to_update_end_point_list(type_update_end_point_list *p, uint8_t e)
{
	unsigned int is_OK = 1;
	pthread_mutex_lock(&p->mtx);
	// if already present in the list, nothing to do, the update has been already planned
	if (!p_find_update_end_point_list(p, e))
	{
		// if the queue is full, we can't add a new element!
		if (is_full_update_end_point_list(p))
		{
			is_OK = 0;
		}
		else
		{
			unsigned int idx_elem_write = ((p->idx_w) & (UPDATE_END_POINT_INDEX_MASK));
			uint8_t * p_write_elem = &p->update_list[idx_elem_write];
			// store the new element
			*p_write_elem = e;
			// update the write index
			p->idx_w = ((idx_elem_write + 1) & (UPDATE_END_POINT_INDEX_MASK));
			// update the number of writes
			p->num_w ++;
		}
	}
	pthread_mutex_unlock(&p->mtx);
	return is_OK;
}
