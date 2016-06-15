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


void * simple_server_thread(void *arg)
{
	type_handle_server_socket hss;
	type_handle_server_socket *phss = &hss;
	memset(phss,0,sizeof(*phss));

	// opens the socket
	phss->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (phss->socket_fd < 0)
	{
		error("ERROR opening socket!");
	}
	// mark the socket as NON blocking
	{
		int flags = fcntl(phss->socket_fd, F_GETFL, 0);
		fcntl(phss->socket_fd, F_SETFL, flags | O_NONBLOCK);
	}
	// do the bind
	{
		struct sockaddr_in * p = &phss->server_address;
		p->sin_family = AF_INET;
		p->sin_addr.s_addr = INADDR_ANY;
		p->sin_port = htons(def_port_number);
		if (bind(phss->socket_fd, (struct sockaddr *) p,sizeof(*p)) < 0)
		{
			error("ERROR on bind!");
		}
	}
	printf("server started\n");

#if def_local_preformatted_test

	// loop handling incoming packets
    while (phss->status_accept_connection != enum_status_accept_connection_ended)
    {
    	// sleep some time
    	//usleep(def_sleep_time_between_cycles_us);
    	usleep(300000);
    	struct sockaddr_in si_other;
    	int recv_len;
    	unsigned int slen = sizeof(si_other);
		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(phss->socket_fd, phss->buffer, sizeof(phss->buffer), 0, (struct sockaddr *) &si_other, &slen)) == -1)
		{
			// printf("error while executing recvfrom:\n");
		}
		else
		{
	        //print details of the client/peer and the data received
	        printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			if (!is_OK_check_formatted_message(phss->buffer, sizeof(phss->buffer)))
			{
		        printf("ERROR ON Data: %*.*s\n" , recv_len,recv_len,phss->buffer);
		        exit(1);
			}
	        printf("Data: %*.*s\n" , recv_len,recv_len,phss->buffer);
	        //now reply the client with the same data
	        if (sendto(phss->socket_fd, phss->buffer,  recv_len, 0, (struct sockaddr*) &si_other, slen) == -1)
	        {
				printf("error while executing sendto:\n");
	        }
		}


     } /* end of while */
#elif def_local_ASACSOCKET_test
	// loop handling incoming packets
    while (phss->status_accept_connection != enum_status_accept_connection_ended)
    {
    	// sleep some time
    	usleep(def_sleep_time_between_cycles_us);
    	struct sockaddr_in si_other;
    	unsigned int slen = sizeof(si_other);
        //try to receive some data, this is a NON-blocking call
        int n_received_bytes = recvfrom(phss->socket_fd, phss->buffer, sizeof(phss->buffer), 0, (struct sockaddr *) &si_other, &slen);
        if (n_received_bytes == -1)
        {
        	//printf("error on recvfrom()");
        }
		else
		{
	        //print details of the client/peer and the data received
			dbg_print(PRINT_LEVEL_INFO,"Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
			enum_check_ASACSOCKET_formatted_message retcode_check = check_ASACSOCKET_formatted_message(phss->buffer, n_received_bytes);
			if ( retcode_check != enum_check_ASACSOCKET_formatted_message_OK)
			{
		        dbg_print(PRINT_LEVEL_ERROR,"ERROR %i ON Data: %*.*s\n", (int)retcode_check , n_received_bytes,n_received_bytes,phss->buffer);
		        exit(1);
			}
	        type_struct_ASACSOCKET_msg * p = (type_struct_ASACSOCKET_msg *)phss->buffer;
        	{
	        	type_ASAC_Zigbee_interface_request *pzmessage_rx = (type_ASAC_Zigbee_interface_request *)&(p->body);
        		switch(pzmessage_rx->code)
        		{
        			case enum_ASAC_ZigBee_interface_command_network_echo_req:
        			{
                		type_ASAC_Zigbee_interface_request zmessage_tx = {0};
                		zmessage_tx.code = enum_ASAC_ZigBee_interface_command_network_echo_req;
                		unsigned int zmessage_size = def_size_ASAC_Zigbee_interface_req((&zmessage_tx),echo);
                		type_ASAC_ZigBee_interface_network_echo_req * p_echo_req = &zmessage_tx.req.echo;
                		snprintf((char*)p_echo_req->message_to_echo,sizeof(p_echo_req->message_to_echo),"%s", pzmessage_rx->req.echo.message_to_echo);

                		type_struct_ASACSOCKET_msg amessage_tx;
                		memset(&amessage_tx,0,sizeof(amessage_tx));

                		unsigned int amessage_tx_size = 0;
                		enum_build_ASACSOCKET_formatted_message r_build = build_ASACSOCKET_formatted_message(&amessage_tx, (char *)&zmessage_tx, zmessage_size, &amessage_tx_size);
                		if (r_build == enum_build_ASACSOCKET_formatted_message_OK)
                		{
            				unsigned int slen=sizeof(si_other);
            				//send the message
            				if (sendto(phss->socket_fd, (char*)&amessage_tx, amessage_tx_size , 0 , (struct sockaddr *) &si_other, slen)==-1)
            				{
            					dbg_print(PRINT_LEVEL_ERROR,"error on sendto()");
            				}
            				else
            				{
            					dbg_print(PRINT_LEVEL_INFO,"message echo sent OK: %s\n", p_echo_req->message_to_echo);
            				}
                		}
        				break;
        			}
        			default:
        			{
        				dbg_print(PRINT_LEVEL_ERROR,"unknown message code received: %u\n", pzmessage_rx->code);
        			}
        		}
        	}
		}


     } /* end of while */

#endif

    // the thread ends here
    phss->is_running = 0;
    phss->is_terminated =1;

    return 0;

}
