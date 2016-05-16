/*
 * server_thread.h
 *
 *  Created on: Apr 20, 2016
 *      Author: michele
 */

#ifndef SERVER_THREAD_H_
#define SERVER_THREAD_H_

#include "simple_server_common.h"

typedef enum
{
	enum_thread_exit_status_uninitialized = 0,
	enum_thread_exit_status_OK,
	enum_thread_exit_status_OK_num_loop_limit_reached,
	enum_thread_exit_status_OK_lost_socket,
	enum_thread_exit_status_forced_stop,
	enum_thread_exit_status_ERR_clock_api,
	enum_thread_exit_status_ERR_thread_timeout,
	enum_thread_exit_status_ERR_single_message_timeout,
	enum_thread_exit_status_ERR_reading,
	enum_thread_exit_status_ERR_writing,
	enum_thread_exit_status_ERR_unknown,
	enum_thread_exit_status_numof
}enum_thread_exit_status;

const char *get_thread_exit_status_string(enum_thread_exit_status status);


typedef struct _type_thread_info // Used as argument to thread_start()
{
	pthread_t thread_id;  					// ID returned by pthread_create()
	int socket_fd;							// the socket file descriptor
	unsigned int progressive_id;
	unsigned int num_of_thread_starts;     // number of thread starts
	unsigned int num_of_thread_ends;       // number of thread ends
	unsigned int started;					// is the thread started?
	unsigned int force_ends;				// is the thread end forced from an external force?
	unsigned int force_ends_acknowledged; 	// has the thread end forcing been acknowledged?
	struct timespec ts_start_time;			// the thread start time
	char rx_buffer[def_max_char_thread_rx_buffer];	// the tx and rx buffers for the thread
	char tx_buffer[def_max_char_thread_rx_buffer];
	unsigned int thread_timeout_seconds; 			// the max thread duration [seconds]; 0 means no timeout
	unsigned int message_timeout_seconds; 	// the maximum time to wait for an incoming message [seconds]; 0 means no timeout
	unsigned int max_loop_read_messages; 			// the maximum number of messages we can read; 0 means no limits on the number of loop to execute
	unsigned int num_loop_read_messages_executed; 	// the number of message loop executed by the thread; 0 means no limits on the number of loop to execute
	enum_thread_exit_status thread_exit_status;		// the thread exit status
}type_thread_info;

void initialize_thread_info(type_thread_info * p, int fd);
void mark_as_started_thread_info(type_thread_info * p);
void * thread_start(void *arg);
unsigned int is_ended_thread_info(type_thread_info * p);
void unmark_as_started_thread_info(type_thread_info * p);

#endif /* SERVER_THREAD_H_ */
