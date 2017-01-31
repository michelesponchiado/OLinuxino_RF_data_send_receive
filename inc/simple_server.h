/*
 * simple_server.h
 *
 *  Created on: Apr 20, 2016
 *      Author: michele
 */

#ifndef INC_SIMPLE_SERVER_H_
#define INC_SIMPLE_SERVER_H_

#include "simple_server_common.h"


typedef enum
{
	enum_status_accept_connection_idle = 0,
	enum_status_accept_connection_running,
	enum_status_accept_connection_new,
	enum_status_accept_connection_init_wait_free,
	enum_status_accept_connection_wait_free,
	enum_status_accept_connection_timeout_wait_free,
	enum_status_accept_connection_init_shutdown,
	enum_status_accept_connection_wait_shutdown,
	enum_status_accept_connection_ends,
	enum_status_accept_connection_ended,
	enum_status_accept_connection_numof
}enum_status_accept_connection;

typedef enum
{
	enum_socket_server_stat_accepted_conn = 0,
	enum_socket_server_stat_no_incoming_conn ,
	enum_socket_server_stat_aborted_conn,
	enum_socket_server_stat_interrupted,
	enum_socket_server_stat_bailout_thread,
	enum_socket_server_stat_force_kill,

	// this better to be the last
	enum_socket_server_stat_unknown,
	enum_socket_server_stat_numof
}enum_socket_server_stat;


// the structure we use to handle our socket
typedef struct _type_handle_server_socket
{
	//pthread_t thread_id;  					// ID returned by pthread_create()
	unsigned int shutdown;
	unsigned int shutdown_ack;
	int socket_fd;
	int accepted_socket_fd;
	int port_number;
	unsigned int is_running;
	volatile unsigned int is_terminated;
    struct sockaddr_in server_address;
    socklen_t client_socket_length;
    struct sockaddr_in client_address;
	struct timespec ts_waiting_free_thread;
	struct timespec ts_waiting_shutdown;
    unsigned int num_of_active_connections;
    type_thread_info active_connections[LISTEN_BACKLOG];
    enum_status_accept_connection status_accept_connection;
    unsigned int stats[enum_socket_server_stat_numof];
    unsigned int thread_exit_status_stats[enum_thread_exit_status_numof];
    char buffer[SOCKET_MESSAGE_SIZE];
}type_handle_server_socket;
// argument should be a type_handle_server_socket pointer
void * ASACZ_UDP_server_thread(void *arg);
unsigned int is_OK_shutdown_server(type_handle_server_socket *p);


#endif /* INC_SIMPLE_SERVER_H_ */
