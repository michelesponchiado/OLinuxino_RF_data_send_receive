/*
 * ASACZ_devices_list.c
 *
 *  Created on: Nov 4, 2016
 *      Author: michele
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <sys/timeb.h>  /* ftime, timeb (for timestamp in millisecond) */
#include <sys/time.h>   /* gettimeofday, timeval (for timestamp in microsecond) */
#include "timeout_utils.h"
#include "ASACZ_devices_list.h"

static type_struct_ASACZ_device_list ASACZ_device_list;

typedef struct _type_handle_ASACZ_device_list
{
	pthread_mutex_t mtx; 			//!< the mutex to access the list
	uint32_t	num_of_sequence;	//!< the number of changes in the list, useful for walk operations
}type_handle_ASACZ_device_list;

static type_handle_ASACZ_device_list handle_ASACZ_device_list;


static void init_device_lifecycle(type_struct_ASACZ_device_list_element *p)
{
	if (!p)
	{
		return;
	}
	int64_t t = get_current_epoch_time_ms();
	type_struct_ASACZ_device_lifecycle * p_lifecycle = &(p->lifecycle);
	memset(p_lifecycle, 0, sizeof(*p_lifecycle));
	p_lifecycle->put_in_list_epoch_time_ms = t;
	p_lifecycle->last_msg_rx_epoch_time_ms = t;
}

static void refresh_device_lifecycle_rx(type_struct_ASACZ_device_list_element *p)
{
	if (!p)
	{
		return;
	}
	int64_t t = get_current_epoch_time_ms();
	type_struct_ASACZ_device_lifecycle * p_lifecycle = &(p->lifecycle);
	p_lifecycle->last_msg_rx_epoch_time_ms = t;
}


static type_struct_ASACZ_device_list_element * find_ASACZ_device_list_by_IEEE_address(uint64_t IEEE_address)
{
	type_struct_ASACZ_device_list_element * p_device_found = NULL;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		unsigned int is_device_found = 0;
		unsigned int idx_device;
		for (idx_device = 0; !is_device_found && (idx_device < ASACZ_device_list.num_of_devices); idx_device++)
		{
			type_struct_ASACZ_device_list_element *p_curdevice = &ASACZ_device_list.list[idx_device];
			type_struct_ASACZ_device_header * p_cur_header = &p_curdevice->device_header;
			if (p_cur_header->IEEE_address == IEEE_address)
			{
				p_device_found = p_curdevice;
				is_device_found = 1;
			}
		}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return p_device_found;
}
static type_struct_ASACZ_device_list_element * find_ASACZ_device_list_by_network_short_address(uint16_t network_short_address)
{
	type_struct_ASACZ_device_list_element * p_device_found = NULL;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		unsigned int is_device_found = 0;
		unsigned int idx_device;
		for (idx_device = 0; !is_device_found && (idx_device < ASACZ_device_list.num_of_devices); idx_device++)
		{
			type_struct_ASACZ_device_list_element *p_curdevice = &ASACZ_device_list.list[idx_device];
			type_struct_ASACZ_device_header * p_cur_header = &p_curdevice->device_header;
			if (p_cur_header->network_short_address == network_short_address)
			{
				p_device_found = p_curdevice;
				is_device_found = 1;
			}
		}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return p_device_found;
}

unsigned int is_OK_get_network_short_address_from_IEEE(uint64_t IEEE_address, uint16_t * p_network_short_address)
{
	unsigned int isOK = 1;
	type_struct_ASACZ_device_list_element * p = find_ASACZ_device_list_by_IEEE_address(IEEE_address);
	if (!p)
	{
		isOK = 0;
	}
	if (isOK)
	{
		if (p_network_short_address)
		{
			*p_network_short_address = p->device_header.network_short_address;
		}
	}
	return isOK;
}


unsigned int is_OK_get_IEEE_from_network_short_address(uint16_t network_short_address, uint64_t *p_IEEE_address, enum_device_lifecycle_action e)
{
	unsigned int isOK = 1;
	type_struct_ASACZ_device_list_element * p = find_ASACZ_device_list_by_network_short_address(network_short_address);
	if (!p)
	{
		isOK = 0;
	}
	if (isOK)
	{
		switch(e)
		{
			case enum_device_lifecycle_action_do_nothing:
			default:
			{
				break;
			}
			case enum_device_lifecycle_action_do_refresh_rx:
			{
				refresh_device_lifecycle_rx(p);
				break;
			}
		}
		if (p_IEEE_address)
		{
			*p_IEEE_address = p->device_header.IEEE_address;
		}
	}
	return isOK;
}

void init_ASACZ_device_list(void)
{
	memset(&ASACZ_device_list, 0, sizeof(ASACZ_device_list));
	pthread_mutex_init(&handle_ASACZ_device_list.mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_ASACZ_device_list.mtx, &mutexattr);
	}
}

enum_add_ASACZ_device_list_header_retcode add_ASACZ_device_list_header(type_struct_ASACZ_device_header *p_device_header)
{
	enum_add_ASACZ_device_list_header_retcode r = enum_add_ASACZ_device_list_header_retcode_OK;
	type_struct_ASACZ_device_list_element *p_device = NULL;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		if (r == enum_add_ASACZ_device_list_header_retcode_OK)
		{
			p_device = find_ASACZ_device_list_by_IEEE_address(p_device_header->IEEE_address);
			if (!p_device)
			{
				if (ASACZ_device_list.num_of_devices >= sizeof(ASACZ_device_list.list)/sizeof(ASACZ_device_list.list[0]))
				{
					r = enum_add_ASACZ_device_list_header_retcode_ERR_no_room;
				}
				else
				{
					unsigned int idx_new_device = ASACZ_device_list.num_of_devices;
					p_device = &ASACZ_device_list.list[idx_new_device];
					ASACZ_device_list.num_of_devices++;
				}
			}
		}
		if (r == enum_add_ASACZ_device_list_header_retcode_OK)
		{
			if (p_device)
			{
				// reset the device structure
				memset(p_device, 0, sizeof(*p_device));
				// store the header informations
				p_device->device_header = *p_device_header;
				init_device_lifecycle(p_device);
				handle_ASACZ_device_list.num_of_sequence++;
			}
			else
			{
				r = enum_add_ASACZ_device_list_header_retcode_ERR_unable_to_add;
			}
		}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return r;
}

enum_add_ASACZ_device_list_end_points_retcode add_ASACZ_device_list_end_points(uint16_t	network_short_address, uint8_t num_of_end_points, uint8_t *list_end_points)
{
	enum_add_ASACZ_device_list_end_points_retcode r = enum_add_ASACZ_device_list_end_points_retcode_OK;
	type_struct_ASACZ_device_list_element *p_device = NULL;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		if (r == enum_add_ASACZ_device_list_end_points_retcode_OK)
		{
			p_device = find_ASACZ_device_list_by_network_short_address(network_short_address);
			if (!p_device)
			{
				r = enum_add_ASACZ_device_list_end_points_retcode_ERR_network_short_address_not_found;
			}
		}
		if (r == enum_add_ASACZ_device_list_end_points_retcode_OK)
		{
			refresh_device_lifecycle_rx(p_device);
			type_struct_ASACZ_end_point_list *p_end_point_list = &p_device->end_point_list;
			if (num_of_end_points >= sizeof(p_end_point_list->list)/sizeof(p_end_point_list->list[0]))
			{
				r = enum_add_ASACZ_device_list_end_points_retcode_ERR_too_many_end_points;
			}
			else
			{
				// copy the end-points list
				unsigned int idx_end_point;
				for (idx_end_point = 0; idx_end_point < num_of_end_points; idx_end_point++)
				{
					p_end_point_list->list[idx_end_point].end_point = list_end_points[idx_end_point];
				}
				// set the number of end-points
				p_end_point_list->num_of_active_end_points = num_of_end_points;
				handle_ASACZ_device_list.num_of_sequence++;
			}
		}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return r;
}

enum_add_ASACZ_device_list_single_end_point_retcode add_ASACZ_device_list_single_end_point(uint16_t	network_short_address, type_struct_ASACZ_endpoint_list_element *p)
{
	enum_add_ASACZ_device_list_single_end_point_retcode r = enum_add_ASACZ_device_list_single_end_point_retcode_OK;
	type_struct_ASACZ_device_list_element *p_device = NULL;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		if (r == enum_add_ASACZ_device_list_single_end_point_retcode_OK)
		{
			p_device = find_ASACZ_device_list_by_network_short_address(network_short_address);
			if (!p_device)
			{
				r = enum_add_ASACZ_device_list_single_end_point_retcode_ERR_network_short_address_not_found;
			}
		}
		if (r == enum_add_ASACZ_device_list_single_end_point_retcode_OK)
		{
			type_struct_ASACZ_endpoint_list_element * p_found_end_point = NULL;
			type_struct_ASACZ_end_point_list *p_end_point_list = &p_device->end_point_list;
			// search the end-point
			unsigned int idx_end_point;
			for (idx_end_point = 0; !p_found_end_point && (idx_end_point < p_end_point_list->num_of_active_end_points); idx_end_point++)
			{
				type_struct_ASACZ_endpoint_list_element * p_cur_end_point = &p_end_point_list->list[idx_end_point];
				if (p_cur_end_point->end_point == p->end_point)
				{
					p_found_end_point = p_cur_end_point;
				}
			}
			if (!p_found_end_point)
			{
				r = enum_add_ASACZ_device_list_end_points_retcode_ERR_end_point_undefined;
			}
			else
			{
				*p_found_end_point = *p;
				handle_ASACZ_device_list.num_of_sequence++;
			}
		}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return r;
}



static void walk_init_ASACZ_device_list(type_walk_ASACZ_device_list *p)
{
	p->offset_before = 0;
	p->offset_after = 0;
	p->num_of_sequence = handle_ASACZ_device_list.num_of_sequence;
	p->bytes_copied = 0;
}

/**
 * Call this routine with offset_before == 0 to start walking, then at every loop set offset_before = offset_after until end_of_walk is set
 * or an error is returned
 */
enum_walk_ASACZ_device_list_retcode walk_ASACZ_device_list(type_walk_ASACZ_device_list *p, uint8_t *p_buffer, unsigned int max_bytes)
{
	enum_walk_ASACZ_device_list_retcode r = enum_walk_ASACZ_device_list_retcode_OK;
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
		if (p->offset_before == 0)
		{
			walk_init_ASACZ_device_list(p);
		}
		uint32_t bytes_copied = 0;
		if (p->num_of_sequence != handle_ASACZ_device_list.num_of_sequence)
		{
			r = enum_walk_ASACZ_device_list_retcode_ERR_sequence_changed;
		}
		else
		{
			int bytes_remaining = sizeof(ASACZ_device_list);
			bytes_remaining -= p->offset_before;
			if (bytes_remaining > 0)
			{
				int bytes2copy = max_bytes;
				if (bytes2copy > bytes_remaining)
				{
					bytes2copy = bytes_remaining;
				}
				memcpy(p_buffer, ((uint8_t *)&ASACZ_device_list) + p->offset_before, bytes2copy);
				bytes_copied = bytes2copy;
			}
		}
		p->bytes_copied = bytes_copied;
		p->offset_after = p->offset_before + bytes_copied;
		p->end_of_walk = (p->offset_after >= sizeof(ASACZ_device_list))? 1 : 0;
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	return r;
}

uint32_t get_device_list_sequence_number(void)
{
	return handle_ASACZ_device_list.num_of_sequence;
}

unsigned int is_OK_get_device_IEEE_list(uint32_t start_index, uint32_t sequence, type_struct_device_list *p, uint32_t max_num_of_devices_to_return)
{
	unsigned int is_OK = 1;
	memset(p, 0, sizeof(*p));
	pthread_mutex_lock(&handle_ASACZ_device_list.mtx);
	if (is_OK)
	{
		if ((start_index > 0) && (sequence != handle_ASACZ_device_list.num_of_sequence))
		{
			is_OK = 0;
		}
	}
	if (is_OK)
	{
		p->start_index 		= start_index;
		p->sequence 		= handle_ASACZ_device_list.num_of_sequence;
		p->sequence_valid 	= 1;
		uint32_t num_devices_in_chunk = 0;
#define def_max_device_to_find (sizeof(p->IEEE_chunk) / sizeof(p->IEEE_chunk[0]))
		unsigned int idx_device;
		for (idx_device = start_index; (idx_device < ASACZ_device_list.num_of_devices) && (num_devices_in_chunk < def_max_device_to_find) && (num_devices_in_chunk < max_num_of_devices_to_return); idx_device++)
		{
			type_struct_ASACZ_device_list_element *p_curdevice = &ASACZ_device_list.list[idx_device];
			p->IEEE_chunk[num_devices_in_chunk++] = p_curdevice->device_header.IEEE_address;
		}
		p->num_devices_in_chunk = num_devices_in_chunk;
		p->list_ends_here = (idx_device >= ASACZ_device_list.num_of_devices) ? 1: 0;
	}
	pthread_mutex_unlock(&handle_ASACZ_device_list.mtx);
	if (!is_OK)
	{
		p->sequence_valid = 0;
	}
	return is_OK;
}


