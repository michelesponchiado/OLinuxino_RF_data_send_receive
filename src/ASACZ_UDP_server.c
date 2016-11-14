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
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>

#include "ts_util.h"
#include "server_thread.h"
#include "simple_server.h"
#include "dbgPrint.h"
#include "ASACSOCKET_check.h"
#include "ASAC_ZigBee_network_commands.h"
#include "input_cluster_table.h"
#include "ZigBee_messages.h"

typedef struct _type_handle_socket_in
{
	struct sockaddr_in si_other;
	unsigned int slen;
}type_handle_socket_in;

static void error(const char *msg)
{
	char message[256];
	int err_save = errno;
	snprintf(message,sizeof(message),"%s Error code is %i (%s)",msg, err_save, strerror(err_save));

    perror(message);
    exit(1);
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
	int i;
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
	struct timespec ts_timeout = {0};
	if (clock_gettime(CLOCK_REALTIME, &ts_timeout) == -1)
	{
		error("ERROR getting current time while waiting for a free thread from the pool!");
	}
	// adds 2 seconds margin waiting for the thread to stop
	ts_timeout.tv_sec += def_max_ms_wait_server_shutdown_seconds + 2;
	while(!p->is_terminated)
	{
		usleep(2000);
		struct timespec ts_now = {0};
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
			syslog(LOG_ERR,"%s: unable to send a message using the sendto()", __func__);
		}
		else
		{
			is_OK = 1;
		}
	}
	return is_OK;
}

// register a new cluster
int handle_ASACZ_request_input_cluster_register_req(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	enum_input_cluster_register_reply_retcode r = enum_input_cluster_register_reply_retcode_OK;
	type_ASAC_ZigBee_interface_network_input_cluster_register_req * p_body_request = &pzmessage_rx->req.input_cluster_register;
	if (r == enum_input_cluster_register_reply_retcode_OK)
	{
		type_input_cluster_table_elem elem;
		memset(&elem,0,sizeof(elem));
		elem.cluster = *p_body_request;
		memcpy(&elem.si_other, &phs_in->si_other, phs_in->slen);

		enum_add_input_cluster_table_retcode retcode_input_cluster = add_input_cluster(&elem);
		switch(retcode_input_cluster)
		{
			case enum_add_input_cluster_table_retcode_OK :
			{
				syslog(LOG_INFO,"%s: end-point %u, cluster %u registered OK\n",__func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_add_input_cluster_table_retcode_OK_already_present:
			{
				syslog(LOG_WARNING,"%s: Input cluster already present: end-point %u, cluster %u\n",__func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_add_input_cluster_table_retcode_ERR_no_room:
			{
				r = enum_input_cluster_register_reply_retcode_ERR_no_room;
				syslog(LOG_ERR,"%s: NO ROOM to add end-point %u, cluster %u\n", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}

			default:
			{
				r = enum_add_input_cluster_table_retcode_ERR_unknwon;
				syslog(LOG_ERR,"%s: unknown return code %u adding end-point %u, cluster %u\n", __func__, (unsigned int)retcode_input_cluster, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
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
			dbg_print(PRINT_LEVEL_INFO,"ENDPOINT %u\n", ec.endpoint);
			for (i = 0; i < ec.num_clusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO,"\tcluster %u\n", ec.clusters_id[i]);
			}
		}
#endif
	}

	{
		type_ASAC_Zigbee_interface_command_reply zmessage_tx = {0};
		zmessage_tx.code = enum_ASAC_ZigBee_interface_command_network_input_cluster_register_req;
		unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),input_cluster_register);
		type_ASAC_ZigBee_interface_network_input_cluster_register_reply * p_icr_reply = &zmessage_tx.reply.input_cluster_register;

		p_icr_reply->endpoint = p_body_request->endpoint;
		p_icr_reply->input_cluster_id = p_body_request->input_cluster_id;
		p_icr_reply->retcode = r ;

		if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &phs_in->si_other))
		{
			syslog(LOG_INFO,"%s: reply sent OK", __func__);
		}
		else
		{
			syslog(LOG_ERR,"%s: unable to send the reply", __func__);
			retcode = -1;
		}
	}
	return retcode;
}

int handle_ASACZ_request_input_cluster_unregister_req(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	enum_input_cluster_unregister_reply_retcode r = enum_input_cluster_unregister_reply_retcode_OK;
	type_ASAC_ZigBee_interface_network_input_cluster_unregister_req * p_body_request = &pzmessage_rx->req.input_cluster_unregister;
	if (r == enum_input_cluster_unregister_reply_retcode_OK)
	{
		type_ASAC_ZigBee_interface_network_input_cluster_register_req reg;
		reg.endpoint = p_body_request->endpoint;
		reg.input_cluster_id = p_body_request->input_cluster_id;
		type_input_cluster_table_elem elem;
		memset(&elem, 0, sizeof(elem));
		elem.cluster = reg;
		memcpy(&elem.si_other, &phs_in->si_other, phs_in->slen);

		enum_remove_input_cluster_table_retcode retcode_input_cluster = remove_input_cluster(&elem);
		switch(retcode_input_cluster)
		{
			case enum_remove_input_cluster_table_retcode_OK :
			{
				syslog(LOG_INFO,"%s: end-point %u, cluster %u un-registered OK\n", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
				break;
			}
			case enum_remove_input_cluster_table_retcode_ERR_not_found:
			default:
			{
				r = enum_input_cluster_unregister_reply_retcode_ERR_not_found;
				syslog(LOG_ERR,"%s: unable to find end-point %u, cluster %u to un-register\n", __func__, (unsigned int )p_body_request->endpoint, (unsigned int )p_body_request->input_cluster_id);
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
			dbg_print(PRINT_LEVEL_INFO,"ENDPOINT %u\n", ec.endpoint);
			for (i = 0; i < ec.num_clusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO,"\tcluster %u\n", ec.clusters_id[i]);
			}
		}
#endif
	}

	{
		type_ASAC_Zigbee_interface_command_reply zmessage_tx = {0};
		zmessage_tx.code = enum_ASAC_ZigBee_interface_command_network_input_cluster_unregister_req;
		unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),input_cluster_unregister);
		type_ASAC_ZigBee_interface_network_input_cluster_unregister_reply * p_icr_reply = &zmessage_tx.reply.input_cluster_unregister;

		p_icr_reply->endpoint = p_body_request->endpoint;
		p_icr_reply->input_cluster_id = p_body_request->input_cluster_id;
		p_icr_reply->retcode = r ;

		if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size,phss->socket_fd, &phs_in->si_other))
		{
			syslog(LOG_INFO,"%s: reply sent OK", __func__);
		}
		else
		{
			syslog(LOG_ERR,"%s: unable to send the reply", __func__);
			retcode = -1;
		}
	}
	return retcode;
}

int handle_ASACZ_request_echo_req(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	type_ASAC_Zigbee_interface_command_reply zmessage_tx = {0};
	zmessage_tx.code = enum_ASAC_ZigBee_interface_command_network_echo_req;
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),echo);
	type_ASAC_ZigBee_interface_network_echo_reply * p_echo_reply = &zmessage_tx.reply.echo;
	// use a memcpy and not a snprintf to achieve a real echo!
	memcpy((char*)p_echo_reply->message_to_echo, pzmessage_rx->req.echo.message_to_echo, sizeof(p_echo_reply->message_to_echo));

	if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &phs_in->si_other))
	{
		syslog(LOG_INFO,"%s: reply sent OK", __func__);
	}
	else
	{
		syslog(LOG_ERR,"%s: unable to send the reply", __func__);
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_request_outside_send_message(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	type_ASAC_ZigBee_interface_command_outside_send_message_req * p_body_request = &pzmessage_rx->req.outside_send_message;
	type_ASAC_Zigbee_interface_command_reply zmessage_tx;
	zmessage_tx.code = enum_ASAC_ZigBee_interface_command_outside_send_message;
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),outside_send_message);
	type_ASAC_ZigBee_interface_command_outside_send_message_reply * p_outside_send_message_reply = &zmessage_tx.reply.outside_send_message;

	p_outside_send_message_reply->dst_id = p_body_request->dst_id;
	p_outside_send_message_reply->message_length = p_body_request->message_length;
	p_outside_send_message_reply->retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK;
	uint32_t id = get_invalid_id();

	// we send the message to the radio
	if (!is_OK_push_Tx_outside_message(p_body_request, &id))
	{
		p_outside_send_message_reply->retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_ERROR_unable_to_push_message;
		syslog(LOG_ERR,"%s: unable to push the outside message", __func__);
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

	if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &phs_in->si_other))
	{
		syslog(LOG_INFO,"%s: reply sent OK", __func__);
	}
	else
	{
		syslog(LOG_ERR,"%s: unable to send the reply", __func__);
		retcode = -1;
	}
	return retcode;
}


int handle_ASACZ_request(type_ASAC_Zigbee_interface_request *pzmessage_rx, type_handle_socket_in *phs_in, type_handle_server_socket *phss)
{
	int retcode = 0;
	switch(pzmessage_rx->code)
	{
		case enum_ASAC_ZigBee_interface_command_network_input_cluster_register_req:
		{
			retcode = handle_ASACZ_request_input_cluster_register_req(pzmessage_rx, phs_in, phss);
			break;
		}

		case enum_ASAC_ZigBee_interface_command_network_input_cluster_unregister_req:
		{
			retcode = handle_ASACZ_request_input_cluster_unregister_req(pzmessage_rx, phs_in, phss);
			break;
		}

		case enum_ASAC_ZigBee_interface_command_network_echo_req:
		{
			retcode = handle_ASACZ_request_echo_req(pzmessage_rx, phs_in, phss);
			break;
		}

		case enum_ASAC_ZigBee_interface_command_outside_send_message:
		{
			retcode = handle_ASACZ_request_outside_send_message(pzmessage_rx, phs_in, phss);
			break;
		}

		default:
		{
			syslog(LOG_WARNING,"%s: unknown message code received: 0x%X\n",__func__, pzmessage_rx->code);
			retcode = -1;
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
	zmessage_tx.code = enum_ASAC_ZigBee_interface_command_outside_received_message;
	unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_reply((&zmessage_tx),received_message_callback);
	type_ASAC_ZigBee_interface_command_received_message_callback * p_icr_rx = &zmessage_tx.reply.received_message_callback;
	uint32_t id;
	// if a message has been received...
	if (is_OK_pop_Rx_outside_message(p_icr_rx, &id))
	{
		type_ASAC_ZigBee_src_id * psrcid = &p_icr_rx->src_id;
		// we must check if someone is listening to the end-point/command destination of the outside message
		struct sockaddr_in s_reply;
		if (!p_find_socket_of_input_cluster_end_point_command(&s_reply, psrcid->destination_endpoint, psrcid->cluster_id))
		{
			syslog(LOG_ERR,"%s: unable to find end_point %u, command %u", __func__, (unsigned int )psrcid->destination_endpoint, (unsigned int)psrcid->cluster_id);
			r = enum_check_outside_messages_from_ZigBee_retcode_unhandled_end_point_command;
		}
		else
		{
			// send the message in callback to the destination registered application
			if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, zmessage_size, phss->socket_fd, &s_reply))
			{
				syslog(LOG_INFO,"%s: reply sent OK", __func__);
			}
			else
			{
				syslog(LOG_ERR,"%s: unable to forward the message", __func__);
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
	type_handle_server_socket hss;
	type_handle_socket_in handle_socket_in;
}type_ASACZ_UDP_server_handle;
static type_ASACZ_UDP_server_handle h;


void * ASACZ_UDP_server_thread(void *arg)
{
	//type_thread_info *tinfo = arg;
	memset(&h, 0, sizeof(h));
	h.handle_socket_in.slen = sizeof(h.handle_socket_in.si_other);
	syslog(LOG_INFO, "%s: Server thread starts", __func__);

	// opens the socket
	h.hss.socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (h.hss.socket_fd < 0)
	{
		syslog(LOG_ERR, "%s: Error opening the socket: %s", __func__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	syslog(LOG_INFO, "%s: Socket created OK", __func__);

	// mark the socket as NON blocking
	{
		int flags = fcntl(h.hss.socket_fd, F_GETFL, 0);
		if (flags == -1)
		{
			syslog(LOG_ERR, "%s: Error reading the socket flags: %s",__func__, strerror(errno));
			exit(EXIT_FAILURE);
		}
		if (fcntl(h.hss.socket_fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			syslog(LOG_ERR, "%s: Error setting the socket flag O_NONBLOCK: %s",__func__, strerror(errno));
			exit(EXIT_FAILURE);
		}

	}
	syslog(LOG_INFO, "%s: Socket marked as non blocking OK",__func__);

	// do the bind
	{
		struct sockaddr_in * p = &h.hss.server_address;
		p->sin_family = AF_INET;
		p->sin_addr.s_addr = INADDR_ANY;
		p->sin_port = htons(def_port_number);
		if (bind(h.hss.socket_fd, (struct sockaddr *) p,sizeof(*p)) < 0)
		{
			syslog(LOG_ERR, "%s: Unable to bind the socket on port %i: %s",__func__,def_port_number, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	syslog(LOG_INFO, "%s: Bind OK", __func__);

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
	        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
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
    while (h.hss.status_accept_connection != enum_status_accept_connection_ended)
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
    			enum_check_outside_messages_from_ZigBee_retcode r = check_outside_messages_from_ZigBee(&h.hss);
    			switch(r)
    			{
    				case enum_check_outside_messages_from_ZigBee_retcode_OK:
    				{
    					break;
    				}
    				default:
    				{
    	    			syslog(LOG_ERR, "error %u checking outside messages", (unsigned int)r);
    					break;
    				}
    			}
    			break;
    		}

        	case enum_ASACZ_server_status_read_socket:
        	{
        		h.server_status = enum_ASACZ_server_status_begin_loop;
        		//try to receive some data, this is a NON-blocking call
                int n_received_bytes = recvfrom(h.hss.socket_fd, h.hss.buffer, sizeof(h.hss.buffer), 0, (struct sockaddr *) &h.handle_socket_in.si_other, &h.handle_socket_in.slen);
                // check if some data have been received
                if (n_received_bytes == -1)
                {
                	// no data available from the socket, this is normal, we'll try later
                	continue;
                }
        		//print details of the client/peer and the data received
        		syslog(LOG_INFO,"%s: Received packet from %s, port %d", __func__, inet_ntoa(h.handle_socket_in.si_other.sin_addr), ntohs(h.handle_socket_in.si_other.sin_port));

        		// check if the packet received is a valid message or not
        		enum_check_ASACSOCKET_formatted_message retcode_check = check_ASACSOCKET_formatted_message(h.hss.buffer, n_received_bytes);
        		// was the check OK?
        		if (retcode_check != enum_check_ASACSOCKET_formatted_message_OK)
        		{
        			syslog(LOG_ERR,"%s: Check error %i / %s", __func__, (int)retcode_check , string_of_check_ASACSOCKET_return_code(retcode_check));
        			continue;
        		}
        		syslog(LOG_INFO,"%s: The message is valid",__func__);

        		type_struct_ASACSOCKET_msg * p = (type_struct_ASACSOCKET_msg *)h.hss.buffer;
        		type_ASAC_Zigbee_interface_request *pzmessage_rx = (type_ASAC_Zigbee_interface_request *)&(p->body);
        		// handle the request
        		if (handle_ASACZ_request(pzmessage_rx, &h.handle_socket_in, &h.hss) < 0)
        		{
        			syslog(LOG_ERR,"%s: request handled with error", __func__);
        		}
        		break;
        	}
    	}

     } /* end of while */

#endif

    // the thread ends here
    h.hss.is_running = 0;
    h.hss.is_terminated =1;

    return 0;

}
