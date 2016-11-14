/*
 * input_cluster_table.c
 *
 *  Created on: Jun 15, 2016
 *      Author: michele
 */

#include "input_cluster_table.h"

#define def_min_valid_endpoint 1
#define def_max_valid_endpoint 0xf0
#define def_invalid_endpoint 0


#define def_N_elements_input_cluster_table 128

typedef struct _type_input_cluster_table
{
	uint32_t idx;
	type_input_cluster_table_elem queue[def_N_elements_input_cluster_table];
	int current_endpoint;
	pthread_mutex_t mtx;

}type_input_cluster_table;

static type_input_cluster_table input_cluster_table;

void init_input_cluster_table(void)
{
	memset(&input_cluster_table,0,sizeof(input_cluster_table));
	pthread_mutex_init(&input_cluster_table.mtx, NULL);
}
/**
 * Search for the end-point/cluster_id passed, returns NULL if not found, else returns the pointer to the passed structure, filled with the socket informations
 */
struct sockaddr_in * p_find_socket_of_input_cluster_end_point_command(struct sockaddr_in * p_return, uint8_t endpoint, uint16_t input_cluster_id)
{
	struct sockaddr_in * p = NULL;
	pthread_mutex_lock(&input_cluster_table.mtx);
		unsigned int element_doesnt_exists = 0;
		unsigned int idx;
		for (idx = 0; (idx < input_cluster_table.idx) && !p && !element_doesnt_exists; idx++)
		{
			type_input_cluster_table_elem *p_cur_elem = &input_cluster_table.queue[idx];
			uint8_t cur_endpoint = p_cur_elem->cluster.endpoint;
			if (cur_endpoint == endpoint)
			{
				uint16_t cur_input_cluster_id = p_cur_elem->cluster.input_cluster_id;
				if (cur_input_cluster_id == input_cluster_id)
				{
					p = &p_cur_elem->si_other;
				}
				else if (cur_input_cluster_id > input_cluster_id)
				{
					element_doesnt_exists = 1;
				}
			}
			else if (cur_endpoint > endpoint)
			{
				element_doesnt_exists = 1;
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);
	if (p)
	{
		*p_return = *p;
	}
	return p;
}


static unsigned int is_OK_find_input_cluster_entry(type_input_cluster_table_elem *pe, unsigned int *pui_index)
{
	unsigned int found = 0;
	unsigned int idx;
	for (idx = 0; idx < input_cluster_table.idx && !found; idx++)
	{
		type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
		// if the elements are equal, found!
		if (memcmp(p_elem, pe, sizeof(*pe)) == 0)
		{
			if (pui_index)
			{
				*pui_index = idx;
			}
			found = 1;
		}
	}
	return found;
}
int compare_endp_clusters(const void *a, const void *b)
{
	type_input_cluster_table_elem *pa =(type_input_cluster_table_elem *)a;
	type_input_cluster_table_elem *pb =(type_input_cluster_table_elem *)b;
	if (pa->cluster.endpoint == 0)
	{
		return 1;
	}
	else if (pb->cluster.endpoint == 0)
	{
		return -1;
	}
	else if (pa->cluster.endpoint != pb->cluster.endpoint)
	{
		int endpa = pa->cluster.endpoint;
		int endpb = pb->cluster.endpoint;
		return endpa - endpb;
	}
	else
	{
		int cla = pa->cluster.input_cluster_id;
		int clb = pb->cluster.input_cluster_id;
		return cla - clb;
	}
}

static void sort_input_cluster_table(void)
{
	qsort(&input_cluster_table.queue, def_N_elements_input_cluster_table, sizeof(input_cluster_table.queue[0]), compare_endp_clusters);
}


enum_add_input_cluster_table_retcode add_input_cluster(type_input_cluster_table_elem *pelem_to_add)
{
	enum_add_input_cluster_table_retcode r = enum_add_input_cluster_table_retcode_OK;

	pthread_mutex_lock(&input_cluster_table.mtx);
		if (r == enum_add_input_cluster_table_retcode_OK)
		{
			if (input_cluster_table.idx >= def_N_elements_input_cluster_table)
			{
				r = enum_add_input_cluster_table_retcode_ERR_no_room;
			}
		}

		if (r == enum_add_input_cluster_table_retcode_OK)
		{
			if (pelem_to_add->cluster.endpoint < def_min_valid_endpoint || pelem_to_add->cluster.endpoint > def_max_valid_endpoint)
			{
				r = enum_add_input_cluster_table_retcode_ERR_invalid_endpoint;
			}
		}

		if (r == enum_add_input_cluster_table_retcode_OK)
		{
			// already existing ? nothing to do!
			if (is_OK_find_input_cluster_entry(pelem_to_add, NULL))
			{
				r = enum_add_input_cluster_table_retcode_OK_already_present;
			}
			else
			{
				type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[input_cluster_table.idx];
				*p_elem = *pelem_to_add;
				++input_cluster_table.idx;
				sort_input_cluster_table();
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);

	return r;
}



enum_remove_input_cluster_table_retcode remove_input_cluster(type_input_cluster_table_elem *pelem_to_remove)
{
	enum_remove_input_cluster_table_retcode r = enum_remove_input_cluster_table_retcode_OK;

	pthread_mutex_lock(&input_cluster_table.mtx);
		if (r == enum_remove_input_cluster_table_retcode_OK)
		{
			unsigned int idx_in_table = 0;
			// already existing ? nothing to do!
			if (!is_OK_find_input_cluster_entry(pelem_to_remove, &idx_in_table))
			{
				r = enum_remove_input_cluster_table_retcode_ERR_not_found;
			}
			else
			{
				unsigned int idx;
				for (idx = idx_in_table; idx + 1 < input_cluster_table.idx; idx++)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
					p_elem[0] = p_elem[1];
				}
				// cleanup the freed element
				if (input_cluster_table.idx)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[input_cluster_table.idx];
					memset(p_elem,0,sizeof(*p_elem));
					--input_cluster_table.idx;
				}
				// sort the table
				sort_input_cluster_table();
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);

	return r;
}

void walk_endpoints_clusters_init(void)
{
	input_cluster_table.current_endpoint = -1;
}



unsigned int is_OK_walk_endpoints_clusters_next(type_endpoint_cluster *p)
{
	unsigned int is_OK = 0;
	memset(p,0,sizeof(*p));
	pthread_mutex_lock(&input_cluster_table.mtx);
		unsigned int found = 0;
		unsigned int idx;
		for (idx = 0; idx < input_cluster_table.idx; idx++)
		{
			type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
			if (!found && p_elem->cluster.endpoint > input_cluster_table.current_endpoint)
			{
				found = 1;
				is_OK = 1;
				input_cluster_table.current_endpoint = p_elem->cluster.endpoint;
				p->endpoint = p_elem->cluster.endpoint;
			}
			if (found && input_cluster_table.current_endpoint == p_elem->cluster.endpoint)
			{
				if (p->num_clusters < sizeof(p->clusters_id)/sizeof(p->clusters_id[0]))
				{
					p->clusters_id[p->num_clusters] = p_elem->cluster.input_cluster_id;
					p->num_clusters ++;
				}
				else
				{
					break;
				}
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);
	return is_OK;
}
