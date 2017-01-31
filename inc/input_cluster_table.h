/*
 * input_cluster_table.h
 *
 *  Created on: Jun 15, 2016
 *      Author: michele
 */

#ifndef INC_INPUT_CLUSTER_TABLE_H_
#define INC_INPUT_CLUSTER_TABLE_H_
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>
#include <ASAC_ZigBee_network_commands.h>

#include "dataSendRcv.h"
#include "update_end_point_list.h"

typedef struct _type_input_cluster_table_elem
{
	type_ASAC_ZigBee_interface_network_input_cluster_register_req cluster;
	struct sockaddr_in  si_other;
}type_input_cluster_table_elem;

typedef enum
{
	enum_add_input_cluster_table_retcode_OK = 0,
	enum_add_input_cluster_table_retcode_OK_already_present,
	enum_add_input_cluster_table_retcode_ERR_no_room,
	enum_add_input_cluster_table_retcode_ERR_invalid_endpoint,
	enum_add_input_cluster_table_retcode_ERR_too_many_in_commands,
	enum_add_input_cluster_table_retcode_ERR_too_many_out_commands,
	enum_add_input_cluster_table_retcode_ERR_unknwon,
	enum_add_input_cluster_table_retcode_numof
}enum_add_input_cluster_table_retcode;
enum_add_input_cluster_table_retcode add_input_cluster(type_input_cluster_table_elem *pelem_to_add);

struct sockaddr_in * p_find_socket_of_input_cluster_end_point_command(struct sockaddr_in * p_return, uint8_t endpoint, uint16_t input_cluster_id);

typedef enum
{
	enum_remove_input_cluster_table_retcode_OK = 0,
	enum_remove_input_cluster_table_retcode_ERR_not_found,
	enum_remove_input_cluster_table_retcode_numof
}enum_remove_input_cluster_table_retcode;
enum_remove_input_cluster_table_retcode remove_input_cluster(type_input_cluster_table_elem *pelem_to_remove);


typedef struct _type_endpoint_cluster
{
	unsigned int endpoint;
	unsigned int num_clusters;
	unsigned int clusters_id[16];
}type_endpoint_cluster;

void walk_endpoints_clusters_init(void);
unsigned int is_OK_walk_endpoints_clusters_next(type_endpoint_cluster *p);
void walk_sockets_endpoints_clusters_init(void);
unsigned int is_OK_walk_sockets_endpoints_clusters_next(struct sockaddr_in  *p_si_other);

void init_input_cluster_table(void);

typedef enum
{
	enum_get_next_end_point_command_list_retcode_OK = 0,
	enum_get_next_end_point_command_list_retcode_empty,
	enum_get_next_end_point_command_list_retcode_numof
}enum_get_next_end_point_command_list_retcode;
enum_get_next_end_point_command_list_retcode get_next_end_point_command_list(uint8_t prev_end_point, type_Af_user *p);

// returns the pointer to the end points update list
type_update_end_point_list *get_ptr_to_update_end_point_list(void);
unsigned int is_OK_fill_end_point_command_list(uint8_t end_point, type_Af_user *p);

#endif /* INC_INPUT_CLUSTER_TABLE_H_ */
