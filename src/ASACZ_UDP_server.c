/*
 * simple_server.c
 *
 *  Created on: Apr 20, 2016
 *      Author: michele
 */

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#include "ts_util.h"
#include "timeout_utils.h"
#include "server_thread.h"
#include "simple_server.h"
#include "dbgPrint.h"
#include "ASACSOCKET_check.h"
#include <ASAC_ZigBee_network_commands.h>
#include "input_cluster_table.h"
#include "ZigBee_messages.h"
#include "ASACZ_devices_list.h"
#include "ASACZ_firmware_version.h"
#include "ASACZ_app.h"

typedef struct _type_handle_socket_in
{
	struct sockaddr_in si_other;
	socklen_t slen;
}type_handle_socket_in;

typedef struct _type_handle_ASACZ_request
{
	type_ASAC_Zigbee_interface_request *pzmessage_rx;
	type_ASAC_Zigbee_interface_command_reply *pzmessage_tx;
	uint32_t zmessage_size;
	type_handle_socket_in *phs_in;
	type_handle_server_socket *phss;
	uint32_t send_reply;
	uint32_t send_unknown;
}type_handle_ASACZ_request;

unsigned int is_OK_send_ASACSOCKET_formatted_message_ZigBee(type_ASAC_Zigbee_interface_command_reply *p, unsigned int message_size, int socket_fd, struct sockaddr_in *p_socket_dst);

static void error(const char *msg)
{
	char message[256];
	int err_save = errno;
	snprintf(message,sizeof(message),"%s Error code is %i (%s)",msg, err_save, strerror(err_save));

    perror(message);
    exit(1);
}

static void init_header_protocol(type_ASAC_Zigbee_interface_protocol_header *p)
{
	p->major_protocol_id = def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MAJOR_VERSION;
	p->minor_protocol_id = def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MINOR_VERSION;
}
static void init_header(type_ASAC_Zigbee_interface_header *p, uint32_t command_version, uint32_t command_code, uint32_t link_id)
{
	init_header_protocol(&p->p);
	type_ASAC_Zigbee_interface_command_header *pc = &p->c;
	pc->command_version = command_version;
	pc->command_code = command_code;
	pc->command_link_id = link_id;
}

static void init_header_reply(type_ASAC_Zigbee_interface_header *p_reply, type_ASAC_Zigbee_interface_request *pzmessage_rx, uint32_t max_command_handled_version)
{
	init_header_protocol(&p_reply->p);
	type_ASAC_Zigbee_interface_command_header *p_command_reply = &p_reply->c;
	type_ASAC_Zigbee_interface_command_header *p_command_received = &pzmessage_rx->h.c;
	// gets the lower version number between the required and the available
	uint32_t reply_command_version = max_command_handled_version;
	if (reply_command_version > p_command_received->command_version)
	{
		reply_command_version = p_command_received->command_version;
	}
	p_command_reply->command_version = reply_command_version;
	p_command_reply->command_code = p_command_received->command_code;
	p_command_reply->command_link_id = p_command_received->command_link_id;
}

static void build_and_send_unknown_command(uint32_t the_unknown_code,type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	type_ASAC_Zigbee_interface_command_reply zmessage_tx;
	memset(&zmessage_tx, 0, sizeof(zmessage_tx));
	init_header(&zmessage_tx.h, 0, enum_ASAC_ZigBee_interface_command_unknown, defReservedLinkId_notAReply);

	zmessage_tx.h.c.command_code = enum_ASAC_ZigBee_interface_command_unknown;
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),unknown);
	type_ASAC_ZigBee_interface_unknown_reply * p_unknown_reply = &zmessage_tx.reply.unknown;

	p_unknown_reply->the_unknown_code = the_unknown_code;

	if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &phs_in->si_other))
	{
		my_log(LOG_INFO,"%s: reply sent OK", __func__);
	}
	else
	{
		my_log(LOG_ERR,"%s: unable to send the reply", __func__);
	}
}






#if 0
static void increment_server_stat(type_handle_server_socket *p, enum_socket_server_stat e)
{
	if (e >= enum_socket_server_stat_numof)
	{
		e = enum_socket_server_stat_unknown;
	}
	p->stats[e]++;
}

static unsigned int is_available_free_connection(type_handle_server_socket *p)
{
	if (p->num_of_active_connections < sizeof(p->active_connections)/sizeof(p->active_connections[0]))
	{
		return 1;
	}
	return 0;
}
static type_thread_info * p_get_new_thread_info(type_handle_server_socket *p)
{
	type_thread_info *p_thread_free = NULL;
	if (p->num_of_active_connections < sizeof(p->active_connections)/sizeof(p->active_connections[0]))
	{
		int i;
		for (i = 0; !p_thread_free && i < sizeof(p->active_connections)/sizeof(p->active_connections[0]); i++)
		{
			type_thread_info *p_t = &p->active_connections[i];
			if (p_t->num_of_thread_starts == p_t->num_of_thread_ends)
			{
				p_thread_free = p_t;
				break;
			}
		}
	}
	return p_thread_free;
}


#endif

static void increment_thread_exit_status_stat(type_handle_server_socket *p, enum_thread_exit_status e)
{
	if (e >= enum_thread_exit_status_numof)
	{
		e = enum_thread_exit_status_ERR_unknown;
	}
	p->thread_exit_status_stats[e]++;
}


void connection_housekeeping(type_handle_server_socket *p_handle_server_socket)
{
	// active threads housekeeping
	if (!p_handle_server_socket->num_of_active_connections)
	{
		return;
	}
	unsigned int change_detected = 0;
	unsigned int i;
	for (i = 0; i < sizeof(p_handle_server_socket->active_connections)/sizeof(p_handle_server_socket->active_connections[0]); i++)
	{
		type_thread_info *p_t = &p_handle_server_socket->active_connections[i];
		// is the thread just ended?
		if (is_ended_thread_info(p_t))
		{
#ifdef def_enable_debug
			printf("thread ends with code: %s\n",get_thread_exit_status_string(p_t->thread_exit_status));
#endif
			unmark_as_started_thread_info(p_t);
			// update statistics
			increment_thread_exit_status_stat(p_handle_server_socket, p_t->thread_exit_status);
			// update number of active connections
			if (p_handle_server_socket->num_of_active_connections)
			{
				p_handle_server_socket->num_of_active_connections--;
			}
			change_detected = 1;
		}
	}
#ifdef def_enable_debug
	if (change_detected)
	{
		printf("# of active connections: %i\n",p_handle_server_socket->num_of_active_connections);
	}
#endif
}

unsigned int is_OK_shutdown_server(type_handle_server_socket *p)
{
	p->shutdown = 1;
	struct timespec ts_timeout;
	memset(&ts_timeout, 0, sizeof(ts_timeout));
	if (clock_gettime(CLOCK_REALTIME, &ts_timeout) == -1)
	{
		error("ERROR getting current time while waiting for a free thread from the pool!");
	}
	// adds 2 seconds margin waiting for the thread to stop
	ts_timeout.tv_sec += def_max_ms_wait_server_shutdown_seconds + 2;
	while(!p->is_terminated)
	{
		usleep(2000);
		struct timespec ts_now;
		memset(&ts_now, 0, sizeof(ts_now));
		if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1)
		{
			error("ERROR getting current time while waiting for a free thread from the pool!");
		}
		if (tsCompare(ts_now, ts_timeout) >0 )
		{
			break;
		}
	}

	return p->is_terminated;
}


#define def_local_ASACSOCKET_test 1
//#define def_local_preformatted_test 1
#if def_local_preformatted_test
void prepare_formatted_message(char *buffer, unsigned int buffer_size, unsigned int message_index)
{
	int res = buffer_size;
	int used = 0;
	char * pc = buffer;
	used += snprintf(pc,res,"STX %4i ETX",message_index);
	pc += used;
	res -= used;
	if(res > 0)
	{
		unsigned int crc = 0;
		unsigned int idx;
		for (idx = 0; idx < used; idx++)
		{
			crc ^= buffer[idx];
		}
		crc ^= 0xff;
		crc &= 0xff;
		used += snprintf(pc,res,"%4u",crc);
	}
}

unsigned int is_OK_check_formatted_message(char *buffer, unsigned int buffer_size)
{
	int index;
	unsigned int crc_rx;
	int n_args = sscanf(buffer,"STX %i ETX%u",&index,&crc_rx);
	if (n_args != 2)
	{
		return 0;
	}
	{
		unsigned int crc = 0;
		unsigned int idx;
		for (idx = 0; idx < strlen(buffer) - 4; idx++)
		{
			crc ^= buffer[idx];
		}
		crc ^= 0xff;
		crc &= 0xff;
		if (crc != crc_rx)
		{
			return 0;
		}
	}
	return 1;
}

#endif



unsigned int is_OK_send_ASACSOCKET_formatted_message_ZigBee(type_ASAC_Zigbee_interface_command_reply *p, unsigned int message_size, int socket_fd, struct sockaddr_in *p_socket_dst)
{
	unsigned int  is_OK = 0;
	type_struct_ASACSOCKET_msg amessage_tx;
	memset(&amessage_tx,0,sizeof(amessage_tx));

	struct sockaddr_in true_socket_dst = *p_socket_dst;
//#define def_force_reply_to_same_port_where_you_listen
#ifdef def_force_reply_to_same_port_where_you_listen
//#warning trying the reply on the very same port where we listen
	true_socket_dst.sin_port = htons(def_port_number);
#endif

	unsigned int amessage_tx_size = 0;
	enum_build_ASACSOCKET_formatted_message r_build = build_ASACSOCKET_formatted_message(&amessage_tx, (char *)p, message_size, &amessage_tx_size);
	if (r_build == enum_build_ASACSOCKET_formatted_message_OK)
	{
		unsigned int slen=sizeof(true_socket_dst);
		//send the message
		if (sendto(socket_fd, (char*)&amessage_tx, amessage_tx_size , 0 , (struct sockaddr *) &true_socket_dst, slen)==-1)
		{
			my_log(LOG_ERR,"%s: unable to send a message using the sendto()", __func__);
		}
		else
		{
			is_OK = 1;
		}
	}
	return is_OK;
}

static unsigned int is_OK_check_protocol_request(type_ASAC_Zigbee_interface_request *p)
{
	if (p->h.p.major_protocol_id != def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MAJOR_VERSION)
	{
		my_log(LOG_ERR,"%s: invalid protocol id found: %u, expected: %u", __func__, p->h.p.major_protocol_id, def_ASACZ_ZIGBEE_NETWORK_COMMANDS_PROTOCOL_MAJOR_VERSION);
		return 0;
	}
	return 1;
}
// register a new cluster
int handle_ASACZ_request_input_cluster_register_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	enum_input_cluster_register_reply_retcode r = enum_input_cluster_register_reply_retcode_OK;
	type_ASAC_ZigBee_interface_network_input_cluster_register_req * p_body_request = &p->pzmessage_rx->req.input_cluster_register;
	my_log(LOG_INFO,"%s: end-point %u, cluster %u register request",__func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
	if (r == enum_input_cluster_register_reply_retcode_OK)
	{
		type_input_cluster_table_elem elem;
		memset(&elem,0,sizeof(elem));
		elem.cluster = *p_body_request;
		memcpy(&elem.si_other, &p->phs_in->si_other, p->phs_in->slen);

		enum_add_input_cluster_table_retcode retcode_input_cluster = add_input_cluster(&elem);
		switch(retcode_input_cluster)
		{
			case enum_add_input_cluster_table_retcode_OK :
			{
				my_log(LOG_INFO,"%s: end-point %u, cluster %u registered OK",__func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_add_input_cluster_table_retcode_OK_already_present:
			{
				my_log(LOG_WARNING,"%s: Input cluster already present: end-point %u, cluster %u",__func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_add_input_cluster_table_retcode_ERR_no_room:
			{
				r = enum_input_cluster_register_reply_retcode_ERR_no_room;
				my_log(LOG_ERR,"%s: NO ROOM to add end-point %u, cluster %u", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}

			default:
			{
				r = enum_add_input_cluster_table_retcode_ERR_unknwon;
				my_log(LOG_ERR,"%s: unknown return code %u adding end-point %u, cluster %u", __func__, (unsigned int)retcode_input_cluster, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
		}
// print the clusters?
//#define def_print_walk_ep_clustersid
#ifdef def_print_walk_ep_clustersid
		walk_endpoints_clusters_init();
		type_endpoint_cluster ec;
		while(is_OK_walk_endpoints_clusters_next(&ec))
		{
			int i;
			dbg_print(PRINT_LEVEL_INFO,"ENDPOINT %u", ec.endpoint);
			for (i = 0; i < ec.num_clusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO,"\tcluster %u", ec.clusters_id[i]);
			}
		}
#endif
	}

	{
		p->send_reply = 1;
		p->send_unknown = 0;

		uint32_t reply_max_command_version = def_input_cluster_register_req_command_version;
		init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),input_cluster_register);
		if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
		{
			type_ASAC_ZigBee_interface_network_input_cluster_register_reply * p_icr_reply = &p->pzmessage_tx->reply.input_cluster_register;
			p_icr_reply->endpoint = p_body_request->endpoint;
			p_icr_reply->input_cluster_id = p_body_request->input_cluster_id;
			p_icr_reply->retcode = r ;
		}
		else
		{
			p->send_reply = 0;
			p->send_unknown = 1;
			retcode = -1;
		}

	}
	return retcode;
}

typedef struct sockaddr_in type_struct_elem_socket_already_sent;
#define def_max_list_socket_already_sent 128
typedef struct _type_struct_list_socket_already_sent
{
	uint32_t numof;
	type_struct_elem_socket_already_sent list[def_max_list_socket_already_sent];
}type_struct_list_socket_already_sent;
static type_struct_list_socket_already_sent list_socket_already_sent;
void init_list_socket_already_sent(type_struct_list_socket_already_sent *p)
{
	memset(p, 0, sizeof(*p));
}
uint32_t add_if_not_exist_list_socket_already_sent(type_struct_list_socket_already_sent *p, type_struct_elem_socket_already_sent *p_new_elem)
{
	uint32_t found = 0;
	uint32_t i;
	for (i = 0; i < p->numof && !found; i++)
	{
		type_struct_elem_socket_already_sent *p_elem = &p->list[i];
		if (memcmp(p_elem, p_new_elem, sizeof(*p_elem)) == 0)
		{
			found = 1;
		}
	}
	if (!found)
	{
		int new_elem_idx = p->numof;
		if (new_elem_idx >= def_max_list_socket_already_sent)
		{
			new_elem_idx = 0;
		}
		else
		{
			p->numof++;
		}
		type_struct_elem_socket_already_sent *p_elem = &p->list[new_elem_idx];
		*p_elem = *p_new_elem;
	}
	return !found;
}

void notify_all_clients_device_list_changed(type_handle_server_socket *phss, uint32_t sequence_number)
{
	type_ASAC_Zigbee_interface_command_reply zmessage_tx;
	memset(&zmessage_tx, 0, sizeof(zmessage_tx));
	init_header(&zmessage_tx.h, def_device_list_changed_signal_command_version, enum_ASAC_ZigBee_interface_command_device_list_changed_signal, defReservedLinkId_notAReply);
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),device_list_changed);
	type_ASAC_ZigBee_interface_network_device_list_changed_signal * p_reply = &zmessage_tx.reply.device_list_changed;

	p_reply->sequence_number = sequence_number;
	walk_sockets_endpoints_clusters_init();
	init_list_socket_already_sent(&list_socket_already_sent);

	struct sockaddr_in si_other;
	while(is_OK_walk_sockets_endpoints_clusters_next(&si_other))
	{
		type_struct_elem_socket_already_sent si;
		si = si_other;
		if (add_if_not_exist_list_socket_already_sent(&list_socket_already_sent, &si))
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: sending notification @%s, port %u", __func__, inet_ntoa(si.sin_addr), (unsigned int)si.sin_port);
			if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size,phss->socket_fd, &si_other))
			{
			}
			else
			{
				my_log(LOG_ERR,"%s: unable to send the reply", __func__);
			}
		}
	}

}

int handle_ASACZ_request_input_cluster_unregister_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	enum_input_cluster_unregister_reply_retcode r = enum_input_cluster_unregister_reply_retcode_OK;
	type_ASAC_ZigBee_interface_network_input_cluster_unregister_req * p_body_request = &p->pzmessage_rx->req.input_cluster_unregister;
	if (r == enum_input_cluster_unregister_reply_retcode_OK)
	{
		type_ASAC_ZigBee_interface_network_input_cluster_register_req reg;
		reg.endpoint = p_body_request->endpoint;
		reg.input_cluster_id = p_body_request->input_cluster_id;
		type_input_cluster_table_elem elem;
		memset(&elem, 0, sizeof(elem));
		elem.cluster = reg;
		memcpy(&elem.si_other, &p->phs_in->si_other, p->phs_in->slen);

		enum_remove_input_cluster_table_retcode retcode_input_cluster = remove_input_cluster(&elem);
		switch(retcode_input_cluster)
		{
			case enum_remove_input_cluster_table_retcode_OK :
			{
				my_log(LOG_INFO,"%s: end-point %u, cluster %u un-registered OK", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_remove_input_cluster_table_retcode_ERR_not_found:
			default:
			{
				r = enum_input_cluster_unregister_reply_retcode_ERR_not_found;
				my_log(LOG_ERR,"%s: unable to find end-point %u, cluster %u to un-register", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
		}
//#define def_print_walk_ep_clustersid
#ifdef def_print_walk_ep_clustersid
		walk_endpoints_clusters_init();
		type_endpoint_cluster ec;
		while(is_OK_walk_endpoints_clusters_next(&ec))
		{
			int i;
			dbg_print(PRINT_LEVEL_INFO,"ENDPOINT %u", ec.endpoint);
			for (i = 0; i < ec.num_clusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO,"\tcluster %u", ec.clusters_id[i]);
			}
		}
#endif
	}

	{
		p->send_reply = 1;
		p->send_unknown = 0;

		uint32_t reply_max_command_version = def_input_cluster_unregister_req_command_version;
		init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);


		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),input_cluster_unregister);
		type_ASAC_ZigBee_interface_network_input_cluster_unregister_reply * p_icu_reply = &p->pzmessage_tx->reply.input_cluster_unregister;

		if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
		{
			p_icu_reply->endpoint = p_body_request->endpoint;
			p_icu_reply->input_cluster_id = p_body_request->input_cluster_id;
			p_icu_reply->retcode = r ;
		}
		else
		{
			p->send_reply = 0;
			p->send_unknown = 1;
			retcode = -1;
		}

	}
	return retcode;
}



int handle_ASACZ_request_echo_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;

	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_echo_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),echo);
	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		type_ASAC_ZigBee_interface_network_echo_reply * p_echo_reply = &p->pzmessage_tx->reply.echo;
		memcpy((char*)p_echo_reply->message_to_echo, p->pzmessage_rx->req.echo.message_to_echo, sizeof(p_echo_reply->message_to_echo));
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}

	return retcode;
}

int handle_ASACZ_request_device_list_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;

	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_device_list_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx), device_list);
		type_ASAC_ZigBee_interface_network_device_list_req * p_device_list_req = &p->pzmessage_rx->req.device_list;
		type_ASAC_ZigBee_interface_network_device_list_reply * p_device_list_reply = &p->pzmessage_tx->reply.device_list;
		p_device_list_reply->start_index 	= p_device_list_req->start_index;
	#define def_max_device_to_read (sizeof(p_device_list_reply->list_chunk) / sizeof(p_device_list_reply->list_chunk[0]))
		uint64_t my_IEEE_address = 0;
		uint32_t ui_is_valid_my_IEEE_address = is_OK_get_my_radio_IEEE_address(&my_IEEE_address);

		type_struct_device_list get_device_list;
		if (is_OK_get_device_IEEE_list(p_device_list_req->start_index, p_device_list_req->sequence, &get_device_list, def_max_device_to_read))
		{
			my_log(LOG_INFO,"%s: device list chunk read OK", __func__);
			p_device_list_reply->sequence_valid	= get_device_list.sequence_valid;
			p_device_list_reply->sequence		= get_device_list.sequence;
			p_device_list_reply->list_ends_here	= get_device_list.list_ends_here;
			unsigned int num_devices_copied = 0;
			unsigned int num_devices_skipped_OK = 0;
			unsigned int num_devices_read;
			for (num_devices_read = 0; (num_devices_read < get_device_list.num_devices_in_chunk) && (num_devices_read < def_max_device_to_read); num_devices_read++)
			{
				uint64_t cur_IEEE_address = get_device_list.IEEE_chunk[num_devices_read];
				if (ui_is_valid_my_IEEE_address && (cur_IEEE_address == my_IEEE_address))
				{
					num_devices_skipped_OK++;
				}
				else
				{
					p_device_list_reply->list_chunk[num_devices_copied++].IEEE_address = cur_IEEE_address;
				}
			}
			p_device_list_reply->num_devices_in_chunk = num_devices_copied;
			if (num_devices_copied + num_devices_skipped_OK < get_device_list.num_devices_in_chunk)
			{
				p_device_list_reply->list_ends_here	= 0;
			}
		}
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}


int handle_ASACZ_request_firmware_version_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_firmware_version_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx), firmware_version);
		type_ASAC_ZigBee_interface_network_firmware_version_reply * p_firmware_version_reply = &p->pzmessage_tx->reply.firmware_version;

		type_ASACZ_firmware_version firmware_version;
		get_ASACZ_firmware_version_whole_struct(&firmware_version);
		p_firmware_version_reply->major_number 	= firmware_version.major_number;
		p_firmware_version_reply->middle_number = firmware_version.middle_number;
		p_firmware_version_reply->minor_number 	= firmware_version.minor_number;
		p_firmware_version_reply->build_number 	= firmware_version.build_number;
		snprintf((char*)p_firmware_version_reply->date_and_time, sizeof(p_firmware_version_reply->date_and_time), "%s",firmware_version.date_and_time);
		snprintf((char*)p_firmware_version_reply->patch, sizeof(p_firmware_version_reply->patch), "%s",firmware_version.patch);
		snprintf((char*)p_firmware_version_reply->notes, sizeof(p_firmware_version_reply->notes), "%s",firmware_version.notes);
		snprintf((char*)p_firmware_version_reply->string, sizeof(p_firmware_version_reply->string), "%s",firmware_version.string);
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}

	return retcode;
}

int handle_ASACZ_request_my_IEEE_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_my_IEEE_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),my_IEEE);
		type_ASAC_ZigBee_interface_network_my_IEEE_reply * p_reply = &p->pzmessage_tx->reply.my_IEEE;
		if (is_OK_get_my_radio_IEEE_address(&p_reply->IEEE_address))
		{
			p_reply->is_valid_IEEE_address = 1;
		}
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;

}


int handle_ASACZ_request_signal_strength_req(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_signal_strength_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),signal_strength);
		type_ASAC_ZigBee_interface_network_signal_strength_reply * p_reply = &p->pzmessage_tx->reply.signal_strength;
		type_link_quality_body lqb;
		get_app_last_link_quality(&lqb);
		p_reply->level_min0_max4 = lqb.level;
		p_reply->v_0_255 = lqb.v;
		p_reply->v_dBm = lqb.v_dBm;
		int64_t ago = get_current_epoch_time_ms();
		ago -= lqb.t;
		if (ago < 0)
		{
			ago = 0;
		}
		p_reply->milliseconds_ago = ago;
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_request_outside_send_message(type_handle_ASACZ_request *p)
{
	int retcode = 0;

	p->send_reply = 1;
	p->send_unknown = 0;

	uint32_t reply_max_command_version = def_outside_send_message_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),outside_send_message);
		type_ASAC_ZigBee_interface_command_outside_send_message_req * p_body_request = &p->pzmessage_rx->req.outside_send_message;
		type_ASAC_ZigBee_interface_command_outside_send_message_reply * p_outside_send_message_reply = &p->pzmessage_tx->reply.outside_send_message;

		p_outside_send_message_reply->dst_id = p_body_request->dst_id;
		p_outside_send_message_reply->message_length = p_body_request->message_length;
		p_outside_send_message_reply->retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK;
		uint32_t id = get_invalid_id();

		// we send the message to the radio
		if (!is_OK_push_Tx_outside_message(p_body_request, &id))
		{
			p_outside_send_message_reply->retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_ERROR_unable_to_push_message;
			my_log(LOG_ERR,"%s: unable to push the outside message", __func__);
			retcode = -1;
		}
		// save the assigned message id
		p_outside_send_message_reply->id = id;
		#ifdef def_test_without_Zigbee
		{
			type_ASAC_ZigBee_interface_command_outside_send_message_req m;
			uint32_t id;
			// pop away the message
			is_OK_pop_outside_message(&m, &id);
		}
		#endif
	}
	else
	{
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}

	return retcode;
}


int handle_ASACZ_request(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	if (!is_OK_check_protocol_request(pzmessage_rx)	)
	{
		build_and_send_unknown_command(0, phs_in, phss);
		retcode = -1;
	}
	else
	{
		uint32_t command_code = pzmessage_rx->h.c.command_code;
		type_ASAC_Zigbee_interface_command_reply zmessage_tx;
		memset(&zmessage_tx, 0, sizeof(zmessage_tx));
		type_handle_ASACZ_request handle_ASACZ_request;
		memset(&handle_ASACZ_request, 0, sizeof(handle_ASACZ_request));

		handle_ASACZ_request.pzmessage_rx= pzmessage_rx;
		handle_ASACZ_request.pzmessage_tx= &zmessage_tx;
		handle_ASACZ_request.phs_in = phs_in;
		handle_ASACZ_request.phss = phss;

		switch(command_code)
		{
			case enum_ASAC_ZigBee_interface_command_network_input_cluster_register_req:
			{
				retcode = handle_ASACZ_request_input_cluster_register_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_input_cluster_unregister_req:
			{
				retcode = handle_ASACZ_request_input_cluster_unregister_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_echo_req:
			{
				retcode = handle_ASACZ_request_echo_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_outside_send_message:
			{
				retcode = handle_ASACZ_request_outside_send_message(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_device_list_req:
			{
				retcode = handle_ASACZ_request_device_list_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_firmware_version_req:
			{
				retcode = handle_ASACZ_request_firmware_version_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_my_IEEE_req:
			{
				retcode = handle_ASACZ_request_my_IEEE_req(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_signal_strength_req:
			{
				retcode = handle_ASACZ_request_signal_strength_req(&handle_ASACZ_request);
				break;
			}

			default:
			{
				handle_ASACZ_request.send_unknown = 1;
				retcode = -1;
			}
		}

		if (handle_ASACZ_request.send_reply)
		{
			if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, handle_ASACZ_request.zmessage_size, phss->socket_fd, &phs_in->si_other))
			{
				//my_log(LOG_INFO,"%s: reply sent OK", __func__);
			}
			else
			{
				my_log(LOG_ERR,"%s: unable to send the reply", __func__);
				retcode = -1;
			}
		}
		else if (handle_ASACZ_request.send_unknown)
		{
			my_log(LOG_ERR,"%s: unknown command %u, version %u", __func__, zmessage_tx.h.c.command_code, zmessage_tx.h.c.command_version);
			build_and_send_unknown_command(zmessage_tx.h.c.command_code, phs_in, phss);
		}

	}
	return retcode;
}

typedef enum
{
	enum_check_outside_messages_from_ZigBee_retcode_OK = 0,
	enum_check_outside_messages_from_ZigBee_retcode_unhandled_end_point_command,
	enum_check_outside_messages_from_ZigBee_retcode_unable_to_forward,
	enum_check_outside_messages_from_ZigBee_retcode_numof
}enum_check_outside_messages_from_ZigBee_retcode;

enum_check_outside_messages_from_ZigBee_retcode check_outside_messages_from_ZigBee(type_handle_server_socket *phss)
{
	enum_check_outside_messages_from_ZigBee_retcode r = enum_check_outside_messages_from_ZigBee_retcode_OK;
	type_ASAC_Zigbee_interface_command_reply zmessage_tx;
	init_header(&zmessage_tx.h, def_outside_received_message_callback_command_version, enum_ASAC_ZigBee_interface_command_outside_received_message, defReservedLinkId_notAReply);
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),received_message_callback);
	type_ASAC_ZigBee_interface_command_received_message_callback * p_icr_rx = &zmessage_tx.reply.received_message_callback;
	uint32_t id;
	// if a message has been received...
	if (is_OK_pop_Rx_outside_message(p_icr_rx, &id))
	{
		type_ASAC_ZigBee_src_id * psrcid = &p_icr_rx->src_id;
		// we must check if someone is listening to the end-point/command destination of the outside message
		struct sockaddr_in s_reply;
		dbg_print(PRINT_LEVEL_VERBOSE,"%s: popped message from ep:%i, cluster_id:%i", __func__,psrcid->destination_endpoint, psrcid->cluster_id);

		if (!p_find_socket_of_input_cluster_end_point_command(&s_reply, psrcid->destination_endpoint, psrcid->cluster_id))
		{
			dbg_print(PRINT_LEVEL_ERROR,"%s: ERROR NO SOCKET BOUND TO ep:%i, cluster_id:%i", __func__,psrcid->destination_endpoint, psrcid->cluster_id);
			my_log(LOG_ERR,"%s: unable to find end_point %u, command %u", __func__, (unsigned int )psrcid->destination_endpoint, (unsigned int)psrcid->cluster_id);
			r = enum_check_outside_messages_from_ZigBee_retcode_unhandled_end_point_command;
		}
		else
		{
			char *ip = inet_ntoa(s_reply.sin_addr);
			dbg_print(PRINT_LEVEL_VERBOSE, "%s: OK found socket bound to ep:%i, cluster_id:%i, ip:%s, port=%u", __func__,psrcid->destination_endpoint, psrcid->cluster_id, ip, (unsigned int)s_reply.sin_port);
			// send the message in callback to the destination registered application
			if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &s_reply))
			{
				dbg_print(PRINT_LEVEL_VERBOSE, "%s: message sent OK", __func__);
				my_log(LOG_INFO,"%s: reply sent OK", __func__);
			}
			else
			{
				dbg_print(PRINT_LEVEL_ERROR,"%s: error sending message", __func__);
				my_log(LOG_ERR,"%s: unable to forward the message", __func__);
				r = enum_check_outside_messages_from_ZigBee_retcode_unable_to_forward;
			}

		}
	}
	return r;
}


typedef enum
{
	enum_ASACZ_read_source_socket = 0,
	enum_ASACZ_read_source_ZigBee,
	enum_ASACZ_read_source_numof
}enum_ASACZ_read_source;
typedef enum
{
	enum_ASACZ_server_status_init = 0,
	enum_ASACZ_server_status_begin_loop,
	enum_ASACZ_server_status_read_socket,
	enum_ASACZ_server_status_read_ZigBee,
	enum_ASACZ_server_status_numof
}enum_ASACZ_server_status;

typedef struct _type_ASACZ_UDP_server_handle
{
	enum_ASACZ_read_source read_source;
	enum_ASACZ_server_status server_status;
	type_handle_server_socket *phss;
	type_handle_socket_in handle_socket_in;
	uint32_t device_list_sequence_number;
	int64_t last_time_checked_device_list_ms;
}type_ASACZ_UDP_server_handle;
static type_ASACZ_UDP_server_handle h;


void * ASACZ_UDP_server_thread(void *arg)
{
	memset(&h, 0, sizeof(h));
	h.phss = (type_handle_server_socket *)arg;
	if (!h.phss->port_number)
	{
		h.phss->port_number = def_port_number;
	}
	h.handle_socket_in.slen = sizeof(h.handle_socket_in.si_other);
	my_log(LOG_INFO, "%s: Server thread starts on port %i", __func__, (int)h.phss->port_number);

	// opens the socket
	h.phss->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (h.phss->socket_fd < 0)
	{
		my_log(LOG_ERR, "%s: Error opening the socket: %s", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	my_log(LOG_INFO, "%s: Socket created OK", __func__);

	// mark the socket as NON blocking
	{
		int flags = fcntl(h.phss->socket_fd, F_GETFL, 0);
		if (flags == -1)
		{
			my_log(LOG_ERR, "%s: Error reading the socket flags: %s",__func__, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fcntl(h.phss->socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			my_log(LOG_ERR, "%s: Error setting the socket flag O_NONBLOCK: %s",__func__, strerror(errno));
			exit(EXIT_FAILURE);
		}

	}
	my_log(LOG_INFO, "%s: Socket marked as non blocking OK",__func__);

	// do the bind
	{
		struct sockaddr_in * p = &h.phss->server_address;
		p->sin_family = AF_INET;
		p->sin_addr.s_addr = INADDR_ANY;
		uint16_t port_number = h.phss->port_number;
		p->sin_port = htons(port_number);
		if (bind(h.phss->socket_fd, (struct sockaddr *) p,sizeof(*p)) < 0)
		{
			my_log(LOG_ERR, "%s: Unable to bind the socket on port %u: %s",__func__,(unsigned int)port_number, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	my_log(LOG_INFO, "%s: Bind OK", __func__);

#if def_local_preformatted_test

	// loop handling incoming packets
    while (h.hss.status_accept_connection != enum_status_accept_connection_ended)
    {
    	// sleep some time
    	//usleep(def_sleep_time_between_cycles_us);
    	usleep(300000);
    	struct sockaddr_in si_other;
    	int recv_len;
    	unsigned int slen = sizeof(si_other);
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(h.hss.socket_fd, h.hss.buffer, sizeof(h.hss.buffer), 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			// printf("error while executing recvfrom:\n");
		}
		else
		{
	        //print details of the client/peer and the data received
			if (!is_OK_check_formatted_message(h.hss.buffer, sizeof(h.hss.buffer)))
			{
		        printf("ERROR ON Data: %*.*s\n" , recv_len,recv_len,h.hss.buffer);
		        exit(1);
			}
	        printf("Data: %*.*s\n" , recv_len,recv_len,h.hss.buffer);
	        //now reply the client with the same data
	        if (sendto(h.hss.socket_fd, h.hss.buffer,  recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
	        {
				printf("error while executing sendto:\n");
	        }
		}


     } /* end of while */
#elif def_local_ASACSOCKET_test

	// loop handling incoming packets
    while (h.phss->status_accept_connection != enum_status_accept_connection_ended)
    {
    	// sleep some time
    	usleep(def_sleep_time_between_cycles_us);
    	switch(h.server_status)
    	{
    		case enum_ASACZ_server_status_init:
    		default:
    		{
    			h.server_status = enum_ASACZ_server_status_begin_loop;
    			break;
    		}
    		case enum_ASACZ_server_status_begin_loop:
    		{
    			// every N milliseconds checks for device list changes
    			{
					#define def_period_check_device_list_ms 5000
					int64_t now = get_current_epoch_time_ms();
					if ((now < h.last_time_checked_device_list_ms) || (now > h.last_time_checked_device_list_ms + def_period_check_device_list_ms))
					{
						h.last_time_checked_device_list_ms = now;
						uint32_t new_device_list_sequence_number = get_device_list_sequence_number();
						if (new_device_list_sequence_number != h.device_list_sequence_number)
						{
							h.device_list_sequence_number = new_device_list_sequence_number;
							notify_all_clients_device_list_changed(h.phss, h.device_list_sequence_number);
						}
					}
    			}

    	    	switch (h.read_source)
    	    	{
    	    		case enum_ASACZ_read_source_socket:
    	    		default:
    	    		{
    	    			h.server_status = enum_ASACZ_server_status_read_socket;
    	    			h.read_source = enum_ASACZ_read_source_ZigBee;
    	    			break;
    	    		}
    	    		case enum_ASACZ_read_source_ZigBee:
    	    		{
    	    			h.server_status = enum_ASACZ_server_status_read_ZigBee;
    	    			h.read_source = enum_ASACZ_read_source_socket;
    	    			break;
    	    		}
    	    	}
    			break;
    		}
    		case enum_ASACZ_server_status_read_ZigBee:
    		{
    			h.server_status = enum_ASACZ_server_status_begin_loop;
    			enum_check_outside_messages_from_ZigBee_retcode r = check_outside_messages_from_ZigBee(h.phss);
    			switch(r)
    			{
    				case enum_check_outside_messages_from_ZigBee_retcode_OK:
    				{
    					break;
    				}
    				default:
    				{
    	    			my_log(LOG_ERR, "error %u checking outside messages", (unsigned int)r);
    					break;
    				}
    			}
    			break;
    		}

        	case enum_ASACZ_server_status_read_socket:
        	{
        		h.server_status = enum_ASACZ_server_status_begin_loop;
        		//try to receive some data, this is a NON-blocking call
                int n_received_bytes = recvfrom(h.phss->socket_fd, h.phss->buffer, sizeof(h.phss->buffer), 0, (struct sockaddr *) &h.handle_socket_in.si_other, &h.handle_socket_in.slen);
                // check if some data have been received
                if (n_received_bytes == -1)
                {
                	// no data available from the socket, this is normal, we'll try later
                	continue;
                }
        		//print details of the client/peer and the data received
        		//my_log(LOG_INFO,"%s: Received packet from %s, port %d", __func__, inet_ntoa(h.handle_socket_in.si_other.sin_addr), ntohs(h.handle_socket_in.si_other.sin_port));

        		// check if the packet received is a valid message or not
        		enum_check_ASACSOCKET_formatted_message retcode_check = check_ASACSOCKET_formatted_message(h.phss->buffer, n_received_bytes);
        		// was the check OK?
        		if (retcode_check != enum_check_ASACSOCKET_formatted_message_OK)
        		{
        			my_log(LOG_ERR,"%s: Check error %i / %s", __func__, (int)retcode_check , string_of_check_ASACSOCKET_return_code(retcode_check));
        			continue;
        		}
        		//my_log(LOG_INFO,"%s: The message is valid",__func__);

        		type_struct_ASACSOCKET_msg * p = (type_struct_ASACSOCKET_msg *)h.phss->buffer;
        		type_ASAC_Zigbee_interface_request *pzmessage_rx = (type_ASAC_Zigbee_interface_request *)&(p->body);
        		// handle the request
        		if (handle_ASACZ_request(pzmessage_rx, &h.handle_socket_in, h.phss) < 0)
        		{
        			my_log(LOG_ERR,"%s: request handled with error", __func__);
        		}
        		break;
        	}
    	}

     } /* end of while */

#endif

    // the thread ends here
    h.phss->is_running = 0;
    h.phss->is_terminated =1;

    return 0;

}
