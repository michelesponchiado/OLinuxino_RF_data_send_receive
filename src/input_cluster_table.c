/*
 * input_cluster_table.c
 *
 *  Created on: Jun 15, 2016
 *      Author: michele
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include "input_cluster_table.h"
#include "dataSendRcv.h"

#define def_min_valid_endpoint 1
// the last end point is reserved ASAC
#define def_max_valid_endpoint (0xf0 - 1)
#define def_invalid_endpoint 0


#define def_N_elements_input_cluster_table 128

typedef struct _type_input_cluster_table
{
	uint32_t numof;
	type_input_cluster_table_elem queue[def_N_elements_input_cluster_table];
	int walk_current_endpoint;
	pthread_mutex_t mtx;
	uint32_t idx_walk_socket;

}type_input_cluster_table;

static type_input_cluster_table input_cluster_table;

void init_input_cluster_table(void)
{
	memset(&input_cluster_table,0,sizeof(input_cluster_table));
	pthread_mutex_init(&input_cluster_table.mtx, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&input_cluster_table.mtx, &mutexattr);
	}
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
		for (idx = 0; (idx < input_cluster_table.numof) && !p && !element_doesnt_exists; idx++)
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
	for (idx = 0; idx < input_cluster_table.numof && !found; idx++)
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

static void delete_clusters(type_ASAC_ZigBee_interface_network_input_cluster_register_req *p)
{
	pthread_mutex_lock(&input_cluster_table.mtx);
		unsigned int idx;
		for (idx = 0; (idx < input_cluster_table.numof) ; idx++)
		{
			type_input_cluster_table_elem *p_cur_elem = &input_cluster_table.queue[idx];
			uint8_t cur_endpoint = p_cur_elem->cluster.endpoint;
			if (cur_endpoint == p->endpoint)
			{
				uint16_t cur_input_cluster_id = p_cur_elem->cluster.input_cluster_id;
				if (cur_input_cluster_id == p->input_cluster_id)
				{
					{
	        			char *ip = inet_ntoa(p_cur_elem->si_other.sin_addr);
	        			printf("%s: removed old ep:%i, cluster:%i, ip:%s, port=%u\n", __func__,(int)p_cur_elem->cluster.endpoint,(int)p_cur_elem->cluster.input_cluster_id, ip, (unsigned int)p_cur_elem->si_other.sin_port);
					}
					unsigned int idx_copy;
					for (idx_copy = idx; (idx_copy + 1 < input_cluster_table.numof) ; idx_copy++)
					{
						type_input_cluster_table_elem *p_cur_elem = &input_cluster_table.queue[idx_copy];
						type_input_cluster_table_elem *p_next_elem = &input_cluster_table.queue[idx_copy + 1];
						*p_cur_elem = *p_next_elem;
					}
					type_input_cluster_table_elem *p_cur_elem = &input_cluster_table.queue[idx_copy];
					// cleanup the last element
					memset(p_cur_elem, 0, sizeof(*p_cur_elem));
					if (input_cluster_table.numof > 0)
					{
						input_cluster_table.numof--;
					}
				}
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);

}


static enum_add_input_cluster_table_retcode add_af_user(type_Af_user *p, uint16_t input_cluster_id)
{
	enum_add_input_cluster_table_retcode r = enum_add_input_cluster_table_retcode_OK;
	if (p->AppNumInClusters < sizeof(p->AppInClusterList)/sizeof(p->AppInClusterList[0]))
	{
		p->AppInClusterList[p->AppNumInClusters] = input_cluster_id;
		p->AppNumInClusters++;
		printf("%s: end point %u, adding in cluster %u\n", __func__, (unsigned int)p->EndPoint, (unsigned int)input_cluster_id);
	}
	else
	{
		r = enum_add_input_cluster_table_retcode_ERR_too_many_in_commands;
		printf("%s: ERROR too many in commands specified, end point %u\n", __func__, (unsigned int)p->EndPoint);
		syslog(LOG_ERR, "%s: ERROR too many in commands specified, end point %u\n", __func__, (unsigned int)p->EndPoint);
	}
	if (p->AppNumOutClusters < sizeof(p->AppOutClusterList)/sizeof(p->AppOutClusterList[0]))
	{
		p->AppOutClusterList[p->AppNumOutClusters] = input_cluster_id;
		p->AppNumOutClusters++;
		printf("%s: end point %u, adding out cluster %u\n", __func__, (unsigned int)p->EndPoint, (unsigned int)input_cluster_id);
	}
	else
	{
		r = enum_add_input_cluster_table_retcode_ERR_too_many_out_commands;
		printf("%s: ERROR too many out commands specified, end point %u\n", __func__, (unsigned int)p->EndPoint);
		my_log(LOG_ERR, "%s: ERROR too many out commands specified, end point %u\n", __func__, (unsigned int)p->EndPoint);
	}
	return r;

}


enum_get_next_end_point_command_list_retcode get_next_end_point_command_list(uint8_t prev_end_point, type_Af_user *p)
{
	enum_get_next_end_point_command_list_retcode r = enum_get_next_end_point_command_list_retcode_OK;
	memset(p, 0, sizeof(*p));
	pthread_mutex_lock(&input_cluster_table.mtx);
		unsigned int found_end_point = 0;
		if (r == enum_get_next_end_point_command_list_retcode_OK)
		{
			{
				enum_add_input_cluster_table_retcode add_retcode = enum_add_input_cluster_table_retcode_OK;
				unsigned int idx;
				for (idx = 0; (idx < input_cluster_table.numof) && (add_retcode == enum_add_input_cluster_table_retcode_OK); idx++)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
					if (!found_end_point)
					{
						if (p_elem->cluster.endpoint > prev_end_point)
						{
							found_end_point = 1;
							p->EndPoint = p_elem->cluster.endpoint;
							add_retcode = add_af_user(p, p_elem->cluster.input_cluster_id);
						}
					}
					else
					{
						if (p_elem->cluster.endpoint == p->EndPoint)
						{
							add_retcode = add_af_user(p, p_elem->cluster.input_cluster_id);
						}
					}
				}
			}

		}
		if (!found_end_point)
		{
			r = enum_get_next_end_point_command_list_retcode_empty;
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);

	return r;
}



enum_add_input_cluster_table_retcode add_input_cluster(type_input_cluster_table_elem *pelem_to_add)
{
	enum_add_input_cluster_table_retcode r = enum_add_input_cluster_table_retcode_OK;
	type_Af_user u;

	my_log(1,"%s: adding ep %i/cluster %i\n", __func__, pelem_to_add->cluster.endpoint, pelem_to_add->cluster.input_cluster_id);
	pthread_mutex_lock(&input_cluster_table.mtx);
		if (r == enum_add_input_cluster_table_retcode_OK)
		{
			if (input_cluster_table.numof >= def_N_elements_input_cluster_table)
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
			// delete all of the elements with the same cluster!
			delete_clusters(&pelem_to_add->cluster);
			{
				memset(&u, 0, sizeof(u));
				u.EndPoint = pelem_to_add->cluster.endpoint;
				unsigned int idx;
				for (idx = 0; (idx < input_cluster_table.numof) && (r == enum_add_input_cluster_table_retcode_OK); idx++)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
					if (p_elem->cluster.endpoint == u.EndPoint)
					{
						r = add_af_user(&u, p_elem->cluster.input_cluster_id);
					}
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
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[input_cluster_table.numof];
					r = add_af_user(&u, pelem_to_add->cluster.input_cluster_id);
					if (r == enum_add_input_cluster_table_retcode_OK)
					{
						*p_elem = *pelem_to_add;
						++input_cluster_table.numof;
						// sort the table, the element added is possibly not ordered
						sort_input_cluster_table();
					}
				}
			}
			if (r == enum_add_input_cluster_table_retcode_OK)
			{
    			char *ip = inet_ntoa(pelem_to_add->si_other.sin_addr);
    			my_log(1, "%s: added NEW ep:%i, cluster:%i, ip:%s, port=%u\n", __func__,(int)pelem_to_add->cluster.endpoint, (int)pelem_to_add->cluster.input_cluster_id, ip, (unsigned int)pelem_to_add->si_other.sin_port);
			}
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);
	if (r == enum_add_input_cluster_table_retcode_OK)
	{
		require_network_restart();
		my_log(1,"%s: cluster registered OK, network restart required\n", __func__);
	}
	else
	{
		my_log(1,"%s: cluster register ERROR %i\n", __func__, (int)r);
	}

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
				for (idx = idx_in_table; idx + 1 < input_cluster_table.numof; idx++)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
					p_elem[0] = p_elem[1];
				}
				// cleanup the freed element
				if (input_cluster_table.numof)
				{
					type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[input_cluster_table.numof];
					memset(p_elem,0,sizeof(*p_elem));
					--input_cluster_table.numof;
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
	input_cluster_table.walk_current_endpoint = -1;
}
void walk_sockets_endpoints_clusters_init(void)
{
	input_cluster_table.idx_walk_socket = 0;
}

unsigned int is_OK_walk_sockets_endpoints_clusters_next(struct sockaddr_in  *p_si_other)
{
	unsigned int is_OK = 0;
	pthread_mutex_lock(&input_cluster_table.mtx);
		if (input_cluster_table.idx_walk_socket >= input_cluster_table.numof)
		{
			is_OK = 0;
		}
		else
		{
			is_OK = 1;
			*p_si_other = input_cluster_table.queue[input_cluster_table.idx_walk_socket++].si_other;
		}
	pthread_mutex_unlock(&input_cluster_table.mtx);
	return is_OK;
}


unsigned int is_OK_walk_endpoints_clusters_next(type_endpoint_cluster *p)
{
	unsigned int is_OK = 0;
	memset(p,0,sizeof(*p));
	pthread_mutex_lock(&input_cluster_table.mtx);
		unsigned int found = 0;
		unsigned int idx;
		for (idx = 0; idx < input_cluster_table.numof; idx++)
		{
			type_input_cluster_table_elem *p_elem = &input_cluster_table.queue[idx];
			if (!found && p_elem->cluster.endpoint > input_cluster_table.walk_current_endpoint)
			{
				found = 1;
				is_OK = 1;
				input_cluster_table.walk_current_endpoint = p_elem->cluster.endpoint;
				p->endpoint = p_elem->cluster.endpoint;
			}
			if (found && input_cluster_table.walk_current_endpoint == p_elem->cluster.endpoint)
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
