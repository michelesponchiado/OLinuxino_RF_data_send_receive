/*
 * update_end_point_list.h
 *
 *  Created on: Jan 16, 2017
 *      Author: michele
 */

#ifndef INC_UPDATE_END_POINT_LIST_H_
#define INC_UPDATE_END_POINT_LIST_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>
// number of bits occupied by the number of elements in the update queue
#define UPDATE_END_POINT_INDEX_NUMBIT 7
// 7 bits --> 128 elements
#define def_N_elements_update_end_point_list (1 << UPDATE_END_POINT_INDEX_NUMBIT)
// 7 bits --> mask is 127
// this is the mask to use to re-circulate the index
#define UPDATE_END_POINT_INDEX_MASK (def_N_elements_update_end_point_list - 1)

typedef struct _type_update_end_point_list_stats
{
	unsigned int num_err_overflow;
}type_update_end_point_list_stats;
typedef struct _type_update_end_point_list
{
	unsigned int idx_w, num_w;
	unsigned int idx_r, num_r;
	uint8_t update_list[def_N_elements_update_end_point_list];
	pthread_mutex_t mtx;
	type_update_end_point_list_stats stats;
}type_update_end_point_list;


void init_update_end_point_list(type_update_end_point_list *p);
unsigned int is_full_update_end_point_list(type_update_end_point_list *p);
unsigned int is_empty_update_end_point_list(type_update_end_point_list *p);
unsigned int num_elem_update_end_point_list(type_update_end_point_list *p);
uint8_t *p_find_update_end_point_list(type_update_end_point_list *p, uint8_t e);
unsigned int  is_OK_add_to_update_end_point_list(type_update_end_point_list *p, uint8_t e);
unsigned int is_OK_get_next_update_end_point_list(type_update_end_point_list *p, uint8_t *p_elem);

#endif /* INC_UPDATE_END_POINT_LIST_H_ */
