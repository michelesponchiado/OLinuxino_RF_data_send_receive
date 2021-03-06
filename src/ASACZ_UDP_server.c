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
#include "ASACZ_diag_test.h"
#include "ASACZ_admin_device_info.h"
#include <ASACZ_update_yourself.h>

typedef struct _type_handle_socket_in
{
	struct sockaddr_in si_other;
	socklen_t slen;
}type_handle_socket_in;

typedef struct _type_handle_ASACZ_request
{
	type_ASAC_Zigbee_interface_request *pzmessage_rx;
	type_ASAC_Zigbee_interface_command_reply *pzmessage_tx;
	uint32_t zmessage_tx_size;
	type_handle_socket_in *phs_in;
	type_handle_server_socket *phss;
	uint32_t send_reply;
	uint32_t send_unknown;
}type_handle_ASACZ_request;

unsigned int is_OK_send_ASACSOCKET_formatted_message_ZigBee(type_ASAC_Zigbee_interface_command_reply *p, unsigned int message_size, int socket_fd, struct sockaddr_in *p_socket_dst);

#ifdef print_all_received_messages
#warning USING DEBUG PRINT MESSAGE!!!
//#define laser_printer_at_home
//#define print_console
#define ethernet_printer_fair


#ifdef ethernet_printer_fair
	#undef laser_printer_at_home
	typedef struct _type_ethernet_printer_socket
	{
		unsigned int is_initialized_OK;
		int sockfd;
		int printer_portno;
		char * printer_ip_address_string;
		struct sockaddr_in serv_addr;
	}type_ethernet_printer_socket;
	type_ethernet_printer_socket ethernet_printer_socket;

	static void init_ethernet_printer_socket(void)
	{
		memset(&ethernet_printer_socket, 0, sizeof(ethernet_printer_socket));
		ethernet_printer_socket.sockfd = -1;
	}
	static void close_ethernet_printer_socket(void)
	{
		if (ethernet_printer_socket.is_initialized_OK && ethernet_printer_socket.sockfd >= 0)
		{
			close(ethernet_printer_socket.sockfd);
		}
		init_ethernet_printer_socket();
	}
	static void open_ethernet_printer_socket(void)
	{
		close_ethernet_printer_socket();
		ethernet_printer_socket.sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (ethernet_printer_socket.sockfd < 0)
		{
			my_log(LOG_ERR,"%s: error opening socket", __func__);
			init_ethernet_printer_socket();
			return;
		}


	#if 0
			// mark the socket as NON blocking
			{
				int flags = fcntl(ethernet_printer_socket.sockfd, F_GETFL, 0);
				int status_fcntl = fcntl(ethernet_printer_socket.sockfd, F_SETFL, flags | O_NONBLOCK);
				if (status_fcntl < 0)
				{
					my_log(LOG_ERR,"%s: error setting socket flag O_NONBLOCK", __func__);
				}
			}
	#endif


			ethernet_printer_socket.printer_portno = 9100;
			ethernet_printer_socket.printer_ip_address_string = "192.168.1.231";
			bzero((char *) &ethernet_printer_socket.serv_addr, sizeof(ethernet_printer_socket.serv_addr));
			ethernet_printer_socket.serv_addr.sin_family = AF_INET;
			ethernet_printer_socket.serv_addr.sin_port = htons(ethernet_printer_socket.printer_portno);
			ethernet_printer_socket.serv_addr.sin_addr.s_addr = inet_addr(ethernet_printer_socket.printer_ip_address_string);
		    if (connect(ethernet_printer_socket.sockfd,(struct sockaddr *) &ethernet_printer_socket.serv_addr, sizeof(ethernet_printer_socket.serv_addr)) < 0)
		    {
				my_log(LOG_ERR,"%s: error connecting socket", __func__);
				init_ethernet_printer_socket();
				return;
		    }
			ethernet_printer_socket.is_initialized_OK = 1;
			my_log(LOG_INFO,"%s: printer socket open OK", __func__);
		}


	#define def_num_max_rows_ticket 32
	#define def_num_max_chars_ticket 128
	typedef struct _type_ticket_row
	{
		uint8_t valid;
		uint8_t len;
		char chars[def_num_max_chars_ticket + 1];
	}type_ticket_row;

	typedef struct _type_handle_ticket
	{
		uint32_t id_valid;
		uint32_t id;
		uint32_t num_rows;
		type_ticket_row rows[def_num_max_rows_ticket];
	}type_handle_ticket;
	type_handle_ticket handle_ticket;

	void send_ticket_to_printer(char *s, int len)
	{
		unsigned int message_printed_OK = 0;
		unsigned int loop_retry;
		for (loop_retry = 0; !message_printed_OK && (loop_retry < 3); loop_retry++)
		{
			if (!ethernet_printer_socket.is_initialized_OK)
			{
				if (loop_retry)
				{
					usleep(20000);
				}
				open_ethernet_printer_socket();
			}
#if 0 //this does not work under OLinuxino... if  shut down the printer I have no way to check if it ha sbeen rebooted...
			{
				char c;
			    ssize_t x = recv(ethernet_printer_socket.sockfd, &c, 1, MSG_PEEK | MSG_DONTWAIT);
			    if (x > 0) {
			        /* ...have data, leave it in socket buffer until B connects */
			    } else if (x == 0) {
			        // connection has been closed
			    	ethernet_printer_socket.is_initialized_OK = 0;
			    } else {
			    	int the_error= errno;
			    	if (the_error == EAGAIN || the_error== EWOULDBLOCK)
			    	{
			    		// no data available
			    	}
			    	else
			    	{
			    		// error reading from the socket
				    	ethernet_printer_socket.is_initialized_OK = 0;
			    	}
			    }
			}
#endif
			if (ethernet_printer_socket.is_initialized_OK)
			{

				int n = send(ethernet_printer_socket.sockfd, s, len, MSG_NOSIGNAL);
				if (n < 0 || n!= len)
				{
					my_log(LOG_ERR,"%s: error sending the command", __func__);
					ethernet_printer_socket.is_initialized_OK = 0;
				}
				else
				{
					my_log(LOG_INFO,"%s: command sent OK", __func__);
					message_printed_OK = 1;
				}
			}
			else
			{
				my_log(LOG_ERR,"%s: socket is not initialized", __func__);
			}
		}
	}

	void v_handle_ticket(char *s, unsigned int n)
	{
		typedef struct _type_ticket
		{
			uint16_t key;
			uint16_t id;
			uint16_t row_index;
			uint16_t row_numof;
			char row[1];
		}type_ticket;
#define def_header_ticket_size (sizeof(uint16_t) * 4)
		type_ticket *p = (type_ticket*)s;
		if (n < def_header_ticket_size || p->key != 0xA55A || p->row_index >= def_num_max_rows_ticket || p->row_numof <= 0)
		{
			return;
		}
		type_ticket_row *prow = &handle_ticket.rows[p->row_index];
		if (!handle_ticket.id_valid || p->id != handle_ticket.id)
		{
			handle_ticket.id_valid = 1;
			handle_ticket.id = p->id;
			handle_ticket.num_rows = p->row_numof;
			memset(&handle_ticket.rows[0], 0 , sizeof(handle_ticket.rows));
		}
		int len_row = n - def_header_ticket_size;
		if (handle_ticket.num_rows != p->row_numof || n < 0 || n > def_num_max_chars_ticket)
		{
			return;
		}
		prow->valid = 1;
		prow->len = len_row;
		memcpy(&prow->chars[0], &p->row[0], len_row);
		{
			int valid = 1;
			int idx;
			for (idx = 0; valid && idx < p->row_numof; idx++)
			{
				type_ticket_row *prow = &handle_ticket.rows[idx];
				if (!prow->valid)
				{
					valid = 0;
				}
			}
			if (valid)
			{
				close_ethernet_printer_socket();
				for (idx = 0; valid && idx < handle_ticket.num_rows; idx++)
				{
					type_ticket_row *prow = &handle_ticket.rows[idx];
					send_ticket_to_printer(prow->chars, prow->len);
				}
				handle_ticket.id_valid = 0;
				close_ethernet_printer_socket();
			}
		}
	}
#endif

	void init_print_message(void)
	{
	#ifdef ethernet_printer_fair
		init_ethernet_printer_socket();
	#endif
	}
	void close_print_message(void)
	{
	#ifdef ethernet_printer_fair
		close_ethernet_printer_socket();
	#endif
	}
static void print_message(char *s, int len)
{
#ifdef laser_printer_at_home
	char string_to_print[256];
	snprintf(string_to_print, sizeof(string_to_print),"%*s",len, s);
	if (strchr(string_to_print, '"'))
	{
		my_log(LOG_ERR,"%s: invalid characters "" into the string %s", __func__,string_to_print);
	}
	else
#else
#ifdef print_console
		printf(LOG_INFO, "%s", __func__);
#endif
#endif
	{
		my_log(LOG_INFO,"%s: sending to the printer the command %3.3s...", __func__,s);
#ifdef laser_printer_at_home
		FILE *printer = popen("lpr", "w");
		if (printer)
		{
			fprintf(printer, "%s", string_to_print);
			pclose(printer);
		}
#endif
#ifdef ethernet_printer_fair
		v_handle_ticket(s, len);
#endif
	}
}
#endif
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
				r = enum_input_cluster_register_reply_retcode_ERR_unknown;
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

		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),input_cluster_register);
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


		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),input_cluster_unregister);
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

	p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),echo);
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
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx), device_list);
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
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx), firmware_version);
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
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),my_IEEE);
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
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),signal_strength);
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

int handle_ASACZ_request_restart_network_from_scratch(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: restart from scratch received", __func__);

	uint32_t reply_max_command_version = def_restart_network_from_scratch_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	if (p->pzmessage_tx->h.c.command_version == reply_max_command_version)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),restart_network_from_scratch_reply);
		type_ASAC_ZigBee_interface_restart_network_from_scratch_reply * p_reply = &p->pzmessage_tx->reply.restart_network_from_scratch_reply;
		my_log(LOG_INFO,"%s: requesting restart from scratch", __func__);
		ZAP_require_network_restart_from_scratch();
		p_reply->restart_required_OK = 1;
	}
	else
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_command_administrator_UTC(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_UTC received", __func__);

	uint32_t reply_max_command_version = def_administrator_UTC_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),UTC_reply);
		type_ASAC_ZigBee_interface_administrator_UTC_req * p_req = &p->pzmessage_rx->req.UTC_req;
		type_ASAC_ZigBee_interface_administrator_UTC_reply * p_reply = &p->pzmessage_tx->reply.UTC_reply;
		memset(p_reply, 0, sizeof(*p_reply));
		p_reply->op.uint_op = p_req->op.uint_op;
		p_reply->return_code.enum_r = enum_UTC_op_retcode_OK;
		snprintf((char *)p_reply->return_ascii, sizeof(p_reply->return_ascii), "%s", "OK");
		time_t t = time(NULL);
		unsigned int get_errno_code = 0;
		int my_err = 0;
		switch(p_req->op.enum_op)
		{
			case enum_UTC_op_get:
			{
				if (t == ((time_t)-1))
				{
					my_err = errno;
					get_errno_code = 1;
					p_reply->return_code.enum_r = enum_UTC_op_retcode_ERROR_get;
				}
				break;
			}
			case enum_UTC_op_set:
			{
				time_t timetoset = p_req->body.time_to_set;
				if (stime(&timetoset) < 0)
				{
					my_err = errno;
					get_errno_code = 1;
					p_reply->return_code.enum_r = enum_UTC_op_retcode_ERROR_set;
				}
				// read again the system time
				t = time(NULL);
				break;
			}
			default:
			{
				snprintf((char *)p_reply->return_ascii, sizeof(p_reply->return_ascii), "Unknown operation requested: %u", p_req->op.uint_op);
				p_reply->return_code.enum_r = enum_UTC_op_retcode_ERROR_unknown_op;
				break;
			}
		}
		if (get_errno_code)
		{
			snprintf((char *)p_reply->return_ascii, sizeof(p_reply->return_ascii), "%s", strerror(my_err));
			get_errno_code = 0;
		}
		p_reply->current_system_epoch_time = t;
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}


int handle_ASACZ_command_administrator_restart_me(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_restart_me received", __func__);

	uint32_t reply_max_command_version = def_administrator_restart_me_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),restart_me_reply);
		type_ASAC_ZigBee_interface_administrator_restart_me_req * p_req = &p->pzmessage_rx->req.restart_me_req;
		type_ASAC_ZigBee_interface_administrator_restart_me_reply * p_reply = &p->pzmessage_tx->reply.restart_me_reply;
		memset(p_reply, 0, sizeof(*p_reply));
		p_reply->restart_req_id = p_req->restart_req_id;
		memcpy(p_reply->restart_req_message, p_req->restart_req_message, sizeof(p_reply->restart_req_message));
		my_log(LOG_INFO,"%s: administrator_restart_me id: %u, msg: %s", __func__, p_req->restart_req_id, p_req->restart_req_message);
		// issuing the exit request, the respawn service will restart the application in few seconds
		request_exit();
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}


int handle_ASACZ_command_administrator_system_reboot(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_system_reboot received", __func__);

	uint32_t reply_max_command_version = def_administrator_system_reboot_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),system_reboot_reply);
		type_ASAC_ZigBee_interface_administrator_system_reboot_req * p_req = &p->pzmessage_rx->req.system_reboot_req;
		type_ASAC_ZigBee_interface_administrator_system_reboot_reply * p_reply = &p->pzmessage_tx->reply.system_reboot_reply;
		memset(p_reply, 0, sizeof(*p_reply));
		p_reply->reboot_req_id = p_req->reboot_req_id;
		memcpy(p_reply->reboot_req_message, p_req->reboot_req_message, sizeof(p_reply->reboot_req_message));
		my_log(LOG_INFO,"%s: administrator_system_reboot id: %u, msg: %s", __func__, p_req->reboot_req_id, p_req->reboot_req_message);
		// issuing the system reboot request
		system("reboot");
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_command_administrator_device_info(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_device_info received", __func__);

	uint32_t reply_max_command_version = def_administrator_device_info_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),device_info_reply);
		type_ASAC_ZigBee_interface_administrator_device_info_req * p_req = &p->pzmessage_rx->req.device_info_req;
		type_ASAC_ZigBee_interface_administrator_device_info_reply * p_reply = &p->pzmessage_tx->reply.device_info_reply;
		my_log(LOG_INFO,"%s: handling administrator_device_info", __func__);
		memset(p_reply, 0, sizeof(*p_reply));
		p_reply->op.enum_op = p_req->op.enum_op;
		switch(p_req->op.enum_op)
		{
			case enum_administrator_device_info_op_get_serial_number:
			{
				type_administrator_device_info_op_get_serial_number_reply *p_get_reply = &p_reply->body.get_serial_number;
				get_ASACZ_admin_device_sn(p_get_reply);
				break;
			}
			case enum_administrator_device_info_op_set_serial_number:
			{
				type_administrator_device_info_op_set_serial_number_reply *p_set_reply = &p_reply->body.set_serial_number;
				set_ASACZ_admin_device_sn(&p_req->body.set_serial_number, p_set_reply);
				break;
			}
			default:
			{
				is_format_OK = 0;
				break;
			}
		}
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_command_network_probe(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: network probe received", __func__);

	uint32_t reply_max_command_version = def_ASAC_ZigBee_network_probe_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),network_probe_reply);
		type_ASAC_ZigBee_interface_network_probe_request * p_req = &p->pzmessage_rx->req.network_probe_request;
		type_ASAC_ZigBee_interface_network_probe_reply * p_reply = &p->pzmessage_tx->reply.network_probe_reply;
		my_log(LOG_INFO,"%s: handling network probe request", __func__);
		memset(p_reply, 0, sizeof(*p_reply));
		p_reply->op.enum_op = p_req->op.enum_op;
		switch(p_req->op.enum_op)
		{
			case enum_network_probe_op_discovery:
			{
				{
					#include <sys/sysinfo.h>
					struct sysinfo info;
					sysinfo(&info);
					p_reply->body.discovery.uptime_seconds = info.uptime;
				}
				type_ASAC_ZigBee_interface_network_probe_reply_discovery *p_discovery_reply = &p_reply->body.discovery;
				if (is_OK_get_my_radio_IEEE_address(&p_discovery_reply->IEEE_address_info.IEEE_address))
				{
					p_discovery_reply->IEEE_address_info.is_valid_IEEE_address = 1;
				}
				type_ASACZ_firmware_version firmware_version;
				get_ASACZ_firmware_version_whole_struct(&firmware_version);
				type_ASAC_ZigBee_interface_network_firmware_version_reply *pv = &p_discovery_reply->ASACZ_version;
				pv->major_number 	= firmware_version.major_number;
				pv->middle_number = firmware_version.middle_number;
				pv->minor_number 	= firmware_version.minor_number;
				pv->build_number 	= firmware_version.build_number;
				snprintf((char*)pv->date_and_time, sizeof(pv->date_and_time), "%s",firmware_version.date_and_time);
				snprintf((char*)pv->patch, sizeof(pv->patch), "%s",firmware_version.patch);
				snprintf((char*)pv->notes, sizeof(pv->notes), "%s",firmware_version.notes);
				snprintf((char*)pv->string, sizeof(pv->string), "%s",firmware_version.string);
				{
					type_administrator_device_info_op_get_serial_number_reply g;
					memset(&g, 0, sizeof(g));
					get_ASACZ_admin_device_sn(&g);
					p_discovery_reply->serial_number = g.serial_number;
				}
				if (is_OK_get_my_radio_IEEE_address(&p_discovery_reply->IEEE_address_info.IEEE_address))
				{
					p_discovery_reply->IEEE_address_info.is_valid_IEEE_address = 1;
				}

				get_CC2650_firmware_version(&p_discovery_reply->CC2650_version);
				get_OpenWrt_version(p_discovery_reply->OpenWrt_release, sizeof(p_discovery_reply->OpenWrt_release));

				break;
			}
			default:
			{
				is_format_OK = 0;
				break;
			}
		}
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}

int handle_ASACZ_request_administrator_diagnostic_test(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_diagnostic_test received", __func__);

	uint32_t reply_max_command_version = def_ASAC_ZigBee_admin_diag_test_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),diag_test_reply);
		type_ASAC_admin_diag_test_req * p_req = &p->pzmessage_rx->req.diag_test_req;
		type_ASAC_admin_diag_test_reply * p_reply = &p->pzmessage_tx->reply.diag_test_reply;
		my_log(LOG_INFO,"%s: handling diagnostic test request", __func__);
		p_reply->op.enum_op = p_req->op.enum_op;
		switch(p_req->op.enum_op)
		{
			case enum_admin_diag_test_op_start:
			{
				type_admin_diag_test_req_start_body *p_start_req = &p_req->body.start;
				type_admin_diag_test_reply_start_body *p_start_reply = &p_reply->body.start;
				p_start_reply->retcode.enum_retcode = start_CC2650_diag_test(p_start_req);
				get_CC2650_start_diag_test_retcode_string(p_start_reply->retcode.enum_retcode, (char*)p_start_reply->retcode_ascii, sizeof(p_start_reply->retcode_ascii));
				break;
			}
			case enum_admin_diag_test_op_read_status:
			{
				type_admin_diag_test_req_status_body *p_status_req = &p_req->body.status;
				type_admin_diag_test_reply_status_body *p_status_reply = &p_reply->body.status;
				p_status_reply->format.enum_format = get_CC2650_diag_test_status(p_status_req->format.enum_format, (char*)&p_status_reply->body);
				break;
			}
			case enum_admin_diag_test_op_stop:
			{
				type_admin_diag_test_reply_stop_body *p_stop_reply = &p_reply->body.stop;
				p_stop_reply->retcode.enum_retcode = stop_CC2650_diag_test();
				get_CC2650_stop_diag_test_retcode_string(p_stop_reply->retcode.enum_retcode, (char*)p_stop_reply->retcode_ascii, sizeof(p_stop_reply->retcode_ascii));
				break;
			}
			default:
			{
				is_format_OK = 0;
				break;
			}
		}
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
		p->send_reply = 0;
		p->send_unknown = 1;
		retcode = -1;
	}
	return retcode;
}



int handle_ASACZ_request_administrator_firmware_update(type_handle_ASACZ_request *p)
{
	int retcode = 0;
	p->send_reply = 1;
	p->send_unknown = 0;
	my_log(LOG_INFO,"%s: administrator_firmware_update received", __func__);

	uint32_t reply_max_command_version = def_ASAC_ZigBee_fwupd_req_command_version;
	init_header_reply(&p->pzmessage_tx->h, p->pzmessage_rx, reply_max_command_version);

	uint32_t is_format_OK = 1;
	if (is_format_OK)
	{
		if (p->pzmessage_tx->h.c.command_version != reply_max_command_version)
		{
			is_format_OK = 0;
		}
	}
	if (is_format_OK)
	{
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),fwupd_reply);
		type_ASAC_ZigBee_interface_command_fwupd_req * p_req = &p->pzmessage_rx->req.fwupd_req;
		type_ASAC_ZigBee_interface_command_fwupd_reply * p_reply = &p->pzmessage_tx->reply.fwupd_reply;
		my_log(LOG_INFO,"%s: handling firmware update", __func__);
		switch(p_req->dst.enum_dst)
		{
			case enum_ASAC_ZigBee_fwupd_destination_CC2650:
			{
				p_reply->dst.enum_dst = enum_ASAC_ZigBee_fwupd_destination_CC2650;
				p_reply->ops.CC2650 = p_req->ops.CC2650;
				switch(p_req->ops.CC2650)
				{
					case enum_ASAC_ZigBee_fwupd_CC2650_op_read_version:
					{
						type_fwupd_CC2650_read_version_reply_body * p_reply_body = &p_reply->body.CC2650_read_firmware_version;
						get_CC2650_firmware_version(p_reply_body);
						break;
					}
					case enum_ASAC_ZigBee_fwupd_CC2650_op_start_update:
					{
						type_fwupd_CC2650_start_update_reply_body * p_reply_body = &p_reply->body.CC2650_start_firmware_update;
						request_CC2650_firmware_update((char*)&p_req->body.CC2650_start_firmware_update.CC2650_fw_signed_filename[0], p_reply_body);
						break;
					}
					case enum_ASAC_ZigBee_fwupd_CC2650_op_query_update_status:
					{
						type_fwupd_CC2650_query_update_status_reply_body * p_reply_body = &p_reply->body.CC2650_query_firmware_update_status;
						get_CC2650_firmware_update_status(p_reply_body);
						break;
					}
					case enum_ASAC_ZigBee_fwupd_CC2650_op_query_firmware_file:
					{
						type_fwupd_CC2650_query_firmware_file_reply * p_reply_body = &p_reply->body.CC2650_query_firmware_file_reply;
						CC2650_query_firmware_file((char*)p_req->body.CC2650_query_firmware_file_req.CC2650_fw_query_filename, p_reply_body);
						break;
					}
					default:
					{
						is_format_OK = 0;
						break;
					}
				}
				break;
			}
			case enum_ASAC_ZigBee_fwupd_destination_ASACZ:
			{
				p_reply->dst.enum_dst = enum_ASAC_ZigBee_fwupd_destination_ASACZ;
				p_reply->ops.ASACZ = p_req->ops.ASACZ;
				switch(p_req->ops.ASACZ)
				{
					case enum_ASAC_fwupd_ASACZ_op_start_update:
					{
						type_fwupd_ASACZ_do_update_req_body *p_body_req = &p_req->body.ASACZ_do_update_req_body;
						type_fwupd_ASACZ_do_update_reply_body *p_body_reply = &p_reply->body.ASACZ_do_update_reply_body;
						extern unsigned int isOK_ASACZ_update_yourself(type_fwupd_ASACZ_do_update_req_body *p_body_req, type_fwupd_ASACZ_do_update_reply_body *p_body_reply);
						if (isOK_ASACZ_update_yourself(p_body_req, p_body_reply))
						{
							// exits and automagically restarts ?? No! Wait an explicit command to do it
							// request_exit();
						}
						break;
					}
					case enum_ASAC_fwupd_ASACZ_op_status_update:
					{
						type_fwupd_ASACZ_status_update_reply_body *p_body_reply = &p_reply->body.ASACZ_status_update_reply_body;
						void get_ASACZ_update_yourself_inout_current_reply(type_fwupd_ASACZ_status_update_reply_body *p_status_reply);
						get_ASACZ_update_yourself_inout_current_reply(p_body_reply);
						break;
					}
					default:
					{
						is_format_OK = 0;
						break;
					}
				}
				break;
			}
			default:
			{
				is_format_OK = 0;
				break;
			}
		}
	}
	if (!is_format_OK)
	{
		my_log(LOG_ERR,"%s: error in the format", __func__);
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
		p->zmessage_tx_size = def_size_ASAC_Zigbee_interface_reply((p->pzmessage_tx),outside_send_message);
		type_ASAC_ZigBee_interface_command_outside_send_message_req * p_body_request = &p->pzmessage_rx->req.outside_send_message;
		type_ASAC_ZigBee_interface_command_outside_send_message_reply * p_outside_send_message_reply = &p->pzmessage_tx->reply.outside_send_message;

		p_outside_send_message_reply->dst_id = p_body_request->dst_id;
		p_outside_send_message_reply->message_length = p_body_request->message_length;
		p_outside_send_message_reply->retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK;
		uint32_t id = get_invalid_id();
#ifdef print_all_received_messages
		{
			uint64_t my_IEEE_address = 0;
			uint32_t ui_is_valid_my_IEEE_address = is_OK_get_my_radio_IEEE_address(&my_IEEE_address);
			uint16_t DstAddr = 0;
			if (ui_is_valid_my_IEEE_address && is_OK_get_network_short_address_from_IEEE(my_IEEE_address, &DstAddr))
			{
				// if the destination is the device with address 0 and I have short address 0, this is a message for me!
				if (p_body_request->dst_id.IEEE_destination_address == 0 && DstAddr == 0)
				{
					my_log(LOG_ERR,"%s: a message for me will be printed", __func__);
					print_message((char *)p_body_request->message, p_body_request->message_length);
					return retcode;
				}
			}
		}
#endif

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

			case enum_ASAC_ZigBee_interface_command_administrator_restart_network_from_scratch:
			{
				retcode = handle_ASACZ_request_restart_network_from_scratch(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_administrator_firmware_update:
			{
				retcode = handle_ASACZ_request_administrator_firmware_update(&handle_ASACZ_request);
				break;
			}


			case enum_ASAC_ZigBee_interface_command_administrator_diagnostic_test:
			{
				retcode = handle_ASACZ_request_administrator_diagnostic_test(&handle_ASACZ_request);
				break;
			}

			case enum_ASAC_ZigBee_interface_command_network_probe:
			{
				retcode = handle_ASACZ_command_network_probe(&handle_ASACZ_request);
				break;
			}
			case enum_ASAC_ZigBee_interface_command_administrator_device_info:
			{
				retcode = handle_ASACZ_command_administrator_device_info(&handle_ASACZ_request);
				break;
			}
			case enum_ASAC_ZigBee_interface_command_administrator_system_reboot:
			{
				retcode = handle_ASACZ_command_administrator_system_reboot(&handle_ASACZ_request);
				break;
			}
			case enum_ASAC_ZigBee_interface_command_administrator_UTC:
			{
				retcode = handle_ASACZ_command_administrator_UTC(&handle_ASACZ_request);
				break;
			}
			case enum_ASAC_ZigBee_interface_command_administrator_restart_me:
			{
				retcode = handle_ASACZ_command_administrator_restart_me(&handle_ASACZ_request);
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
			if (is_OK_send_ASACSOCKET_formatted_message_ZigBee(&zmessage_tx, handle_ASACZ_request.zmessage_tx_size, phss->socket_fd, &phs_in->si_other))
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

#ifdef print_all_received_messages
		{
			print_message((char *)p_icr_rx->message, p_icr_rx->message_length);
			return r;
		}
#endif
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
