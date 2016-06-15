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


#define def_N_elements_input_cluster_table 32

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
	if (pa->request.endpoint == 0)
	{
		return 1;
	}
	else if (pb->request.endpoint == 0)
	{
		return -1;
	}
	else if (pa->request.endpoint != pb->request.endpoint)
	{
		int endpa = pa->request.endpoint;
		int endpb = pb->request.endpoint;
		return endpa - endpb;
	}
	else
	{
		int cla = pa->request.input_cluster_id;
		int clb = pb->request.input_cluster_id;
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
			if (pelem_to_add->request.endpoint < def_min_valid_endpoint || pelem_to_add->request.endpoint > def_max_valid_endpoint)
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
			if (!found && p_elem->request.endpoint > input_cluster_table.current_endpoint)
			{
				found = 1;
				is_OK = 1;
				input_cluster_table.current_endpoint = p_elem->request.endpoint;
				p->endpoint = p_elem->request.endpoint;
			}
			if (found && input_cluster_table.current_endpoint == p_elem->request.endpoint)
			{
				if (p->num_clusters < sizeof(p->clusters_id)/sizeof(p->clusters_id[0]))
				{
					p->clusters_id[p->num_clusters] = p_elem->request.input_cluster_id;
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
