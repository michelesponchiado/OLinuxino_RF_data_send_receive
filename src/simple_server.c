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
#include "ts_util.h"
#include "server_thread.h"
#include "simple_server.h"

static void error(const char *msg)
{
	char message[256];
	int err_save = errno;
	snprintf(message,sizeof(message),"%s Error code is %i (%s)",msg, err_save, strerror(err_save));

    perror(message);
    exit(1);
}




static void increment_server_stat(type_handle_server_socket *p, enum_socket_server_stat e)
{
	if (e >= enum_socket_server_stat_numof)
	{
		e = enum_socket_server_stat_unknown;
	}
	p->stats[e]++;
}
static void increment_thread_exit_status_stat(type_handle_server_socket *p, enum_thread_exit_status e)
{
	if (e >= enum_thread_exit_status_numof)
	{
		e = enum_thread_exit_status_ERR_unknown;
	}
	p->thread_exit_status_stats[e]++;
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


void * simple_server_thread(void *arg)
{
	type_handle_server_socket *phss = arg;
	memset(phss,0,sizeof(*phss));

	// opens the socket
	phss->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
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
	// mark the socket as a socket which accepts incoming connections
	listen(phss->socket_fd,LISTEN_BACKLOG);
	// loop handling incoming connections
    while (phss->status_accept_connection != enum_status_accept_connection_ended)
    {
    	// sleep some time
    	usleep(def_sleep_time_between_cycles_us);
    	// handle the connection list
    	connection_housekeeping(phss);
    	if (phss->shutdown)
    	{
    		if (!phss->shutdown_ack)
    		{
    			phss->shutdown_ack = 1;
    			phss->status_accept_connection = enum_status_accept_connection_init_shutdown;
    		}
    	}
    	// status machine for the connection requests
    	switch(phss->status_accept_connection)
    	{
    		case enum_status_accept_connection_idle:
    		default:
    		{
    			phss->status_accept_connection = enum_status_accept_connection_running;
    			break;
    		}
    		case enum_status_accept_connection_init_shutdown:
    		{
    			phss->status_accept_connection = enum_status_accept_connection_wait_shutdown;

				if (clock_gettime(CLOCK_REALTIME, &phss->ts_waiting_shutdown) == -1)
				{
					error("ERROR getting current time while initializing shutdown!");
				}

				phss->ts_waiting_shutdown.tv_sec += def_max_ms_wait_server_shutdown_seconds;

    			unsigned int i;
    			for (i = 0; i < sizeof(phss->active_connections)/sizeof(phss->active_connections[0]); i++)
    			{
        			type_thread_info *p_thread_to_kill;
        			p_thread_to_kill = &phss->active_connections[i];
        			p_thread_to_kill->force_ends = 1;
    			}
    			break;
    		}
    		case enum_status_accept_connection_wait_shutdown:
    		{
    			if (phss->num_of_active_connections == 0)
    			{
    				phss->status_accept_connection = enum_status_accept_connection_ends;
    			}
    			struct timespec ts_now = {0};
    			if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1)
    			{
    				error("ERROR getting current time while waiting for a free thread from the pool!");
    			}
    			if (tsCompare(ts_now, phss->ts_waiting_shutdown) >0 )
    			{
        			unsigned int i;
        			for (i = 0; i < sizeof(phss->active_connections)/sizeof(phss->active_connections[0]); i++)
        			{
            			type_thread_info *p_thread_to_kill;
            			p_thread_to_kill = &phss->active_connections[i];
            			if (p_thread_to_kill->started)
            			{
            				p_thread_to_kill->started = 0;
            				close(p_thread_to_kill->socket_fd);
            			}
        			}
    				phss->status_accept_connection = enum_status_accept_connection_ends;
    				break;
    			}
    			break;
    		}
    		case enum_status_accept_connection_ends:
    		{
    		    // close the socket
    		    close(phss->socket_fd);
    		    phss->socket_fd = -1;
    		    phss->status_accept_connection = enum_status_accept_connection_ended;
    			break;
    		}
    		case enum_status_accept_connection_ended:
    		{
    			break;
    		}
    		case enum_status_accept_connection_running:
    		{
    			struct pollfd pfd = {0};
				pfd.fd = phss->socket_fd;
				pfd.events = POLLIN | POLLPRI | POLLRDHUP;
				pfd.revents = 0;
				// if there are incoming requests but no free handles...
    			if ( (poll(&pfd, 1, 1) > 0) && (pfd.revents & (POLLIN | POLLPRI)) && !is_available_free_connection(phss))
    			{
    				phss->status_accept_connection = enum_status_accept_connection_init_wait_free;
    			}
    			else
    			{
    				int newsockfd;
    				// initialize client socket length variable
    				phss->client_socket_length = sizeof(phss->client_address);
    				// NON-blocking accept incoming connection
    				newsockfd = accept(phss->socket_fd,(struct sockaddr *) &phss->client_address, &phss->client_socket_length);
    				// check if error
    		        if (newsockfd >= 0)
    		        {
    		        	phss->accepted_socket_fd = newsockfd;
    		        	phss->status_accept_connection = enum_status_accept_connection_new;
    		        }
    		        else
    		        {
    		        	unsigned int very_bad_error = 1;
    		        	switch(errno)
    		        	{
    		        		// maybe just there are no incoming connections
    		        		case EAGAIN:
    		        		//case EWOULDBLOCK:
    		        		{
    		        			very_bad_error = 0;
    		        			increment_server_stat(phss,enum_socket_server_stat_no_incoming_conn);
    		        			break;
    		        		}
    		        		case ECONNABORTED:
    		        		{
    		        			very_bad_error = 0;
    		        			increment_server_stat(phss,enum_socket_server_stat_aborted_conn);
    		        			break;
    		        		}
    		        		// descriptor invalid???
    		        		case EBADF:
    		        		{
    		        			break;
    		        		}
    		        		// system interrupted before valid connection established?
    		        		case EINTR:
    		        		{
    		        			very_bad_error = 0;
    		        			increment_server_stat(phss,enum_socket_server_stat_interrupted);
    		        			break;
    		        		}
    		        		// Socket is not listening for connections, or addrlen is invalid (e.g., is negative).
    		        		case EINVAL:
    		        		{
    		        			break;
    		        		}
    		        		case EMFILE: // 	The per-process limit of open file descriptors has been reached.
    		        		{
    		        			break;
    		        		}
    		        		case ENFILE: // 	The system limit on the total number of open files has been reached.
    		        		{
    		        			break;
    		        		}
    		        		default:
    		        		{
    	    					break;
    		        		}
    		        	}
    		        	if (very_bad_error)
    		        	{
        					error("ERROR on accept!");
    		        	}
    		        }
    			}
    			break;
    		}
    		case enum_status_accept_connection_new:
    		{
    			// back to the running state
				phss->status_accept_connection = enum_status_accept_connection_running;
				type_thread_info * p = p_get_new_thread_info(phss);
				if (!p)
				{
#ifdef def_try_bailout_on_cannot_create_thread
					type_thread_info bailout_thread ={0};
        			increment_server_stat(phss,enum_socket_server_stat_bailout_thread);
        			bailout_thread.num_of_thread_starts = 1;
        			bailout_thread.socket_fd = phss->accepted_socket_fd;
					p = &bailout_thread;
					thread_start(p);
#else
					error("ERROR unexpected no free threads!");
#endif
				}
				else
				{
					// initialize the thread info structure
					initialize_thread_info(p, phss->accepted_socket_fd);
					int thread_create_retcode = pthread_create(&p->thread_id, NULL,&thread_start, p);
					// check the return code
					if (thread_create_retcode != 0)
					{
						error("ERROR on pthread_create!");
					}
					else
					{
						// mark thread as started
						mark_as_started_thread_info(p);
						// increase the number of active connections
						phss->num_of_active_connections ++;
#ifdef def_enable_debug
			printf("# of active connections: %i\n",phss->num_of_active_connections);
#endif
					}
				}
    			break;
    		}
    		case enum_status_accept_connection_init_wait_free:
    		{
    			phss->status_accept_connection = enum_status_accept_connection_wait_free;
        		if (clock_gettime(CLOCK_REALTIME, &phss->ts_waiting_free_thread) == -1)
        		{
        			error("ERROR initializing waiting free thread time!");
        		}
    			break;
    		}
    		case enum_status_accept_connection_wait_free:
    		{
    			if (!is_available_free_connection(phss))
    			{
    	    		struct timespec ts_now = {0};
    	    		if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1)
    	    		{
    	    			error("ERROR getting current time while waiting for a free thread from the pool!");
    	    		}
    	    		// timeout elapsed???
            		if (tsCompare(ts_now, phss->ts_waiting_free_thread) >0 )
            		{
            			phss->status_accept_connection = enum_status_accept_connection_timeout_wait_free;
        			}
            		else
            		{
            			usleep(def_sleep_time_waiting_thread_pool_us);
            		}
    			}
    			else
    			{
        			phss->status_accept_connection = enum_status_accept_connection_running;
    			}
    			break;
    		}
    		case enum_status_accept_connection_timeout_wait_free:
    		{
#ifdef def_enable_debug
			printf("killing a connection because there is no room for a new thread\n");
#endif
			// no free thread from the pool? A thread must be killed!
				type_thread_info *p_thread_to_kill;
				p_thread_to_kill = &phss->active_connections[0];
				unsigned int idx_connection;
				for (idx_connection = 1; idx_connection < sizeof(phss->active_connections)/sizeof(phss->active_connections[0]); idx_connection++)
				{
					if (phss->active_connections[idx_connection].progressive_id < p_thread_to_kill->progressive_id)
					{
						p_thread_to_kill = &phss->active_connections[idx_connection];
					}
				}
#ifdef def_enable_debug
			printf("killing the connection %i (id %i)\n",(int)(p_thread_to_kill-&phss->active_connections[0]),p_thread_to_kill->progressive_id);
#endif
    			p_thread_to_kill->force_ends = 1;
    			increment_server_stat(phss,enum_socket_server_stat_force_kill);
    			unsigned int list_free = 0;
    			unsigned int num_loop_wait_kill;
    			for (num_loop_wait_kill = 0; !list_free && num_loop_wait_kill < def_max_ms_wait_thread_forced_ends; num_loop_wait_kill++)
    			{
    				usleep(1000);
    		    	// handle the connection list
    		    	connection_housekeeping(phss);
    		    	// is a thread finally free?
    		    	if (is_available_free_connection(phss))
    		    	{
#ifdef def_enable_debug
			printf("connection killed OK\n");
#endif
    		    		list_free = 1;
    		    		phss->status_accept_connection = enum_status_accept_connection_running;
    		    	}
    			}
    			if (!list_free)
    			{
#ifdef def_enable_debug
			printf("connection kill ERROR\n");
#endif
    				error("ERROR cannot free the pool!");
    			}
    			break;
    		}

    	}

     } /* end of while */

    // the thread ends here
    phss->is_running = 0;
    phss->is_terminated =1;

    return NULL;

}
