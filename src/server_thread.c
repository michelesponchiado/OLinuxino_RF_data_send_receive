/*
 * server_thread.c
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
#include "ZigBee_messages.h"
#include "ASAC_ZigBee_network_commands.h"


static unsigned int has_reply_decode_incoming_message_from_socket(char *message, unsigned int message_length, char *message_reply, unsigned int max_message_reply_length, unsigned int *pui_message_reply_length)
{
	unsigned int has_reply = 1;
	type_ASAC_Zigbee_interface_request *p = (type_ASAC_Zigbee_interface_request *)message;
	type_ASAC_Zigbee_interface_command_reply *p_reply =(type_ASAC_Zigbee_interface_command_reply *)message_reply;
	// if there is no room for the code, error
	if (max_message_reply_length < def_size_ASAC_Zigbee_interface_code(p_reply))
	{
		has_reply = 0;
	}
	if (has_reply)
	{
		// set the output code the same as the input
		p_reply->code = p->code;
		switch( p->code )
		{
			case enum_ASAC_ZigBee_interface_command_outside_send_message:
			{
				if (max_message_reply_length < def_size_ASAC_Zigbee_interface_reply(p_reply, outside_send_message))
				{
					has_reply = 0;
				}
				if (has_reply)
				{
					// set the output message length
					*pui_message_reply_length = def_size_ASAC_Zigbee_interface_reply(p_reply, outside_send_message);
					// initialize the return code to OK
					p_reply->reply.outside_send_message.retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_OK;
					p_reply->reply.outside_send_message.id = get_invalid_id();
#ifdef def_enable_debug
				printf("sending user message: %s\n", p->req.outside_send_message.message);
#endif
					uint32_t id;

					// we send the message to the radio
					if (!is_OK_push_Tx_outside_message(&p->req.outside_send_message, &id))
					{
						p_reply->reply.outside_send_message.retcode = enum_ASAC_ZigBee_interface_command_outside_send_message_reply_retcode_ERROR_unable_to_push_message;
					}
					// save the assigned message id
					else
					{
						p_reply->reply.outside_send_message.id = id;
					}
#ifdef def_test_without_Zigbee
					{
						type_ASAC_ZigBee_interface_command_outside_send_message_req m;
						uint32_t id;
						// pop away the message
						is_OK_pop_outside_message(&m, &id);
					}
#endif
				}
				break;
			}
			default:
			{
				if (max_message_reply_length < def_size_ASAC_Zigbee_interface_reply(p_reply, unknown))
				{
					has_reply = 0;
				}
				if (has_reply)
				{
					// set the output message length
					*pui_message_reply_length = def_size_ASAC_Zigbee_interface_reply(p_reply, unknown);
					p_reply->reply.unknown.the_unknown_code = p->code;
					p_reply->code = enum_ASAC_ZigBee_interface_command_unknown;
				}
				break;
			}
		}
	}
	return has_reply;

}



const char *get_thread_exit_status_string(enum_thread_exit_status status)
{
	const char *s ="UNKNOWN";
	switch(status)
	{
		default:
		{
			break;
		}
		case enum_thread_exit_status_uninitialized:
		{
			s = "uninitialized";
			break;
		}
		case enum_thread_exit_status_OK:
		{
			s = "OK";
			break;
		}
		case enum_thread_exit_status_OK_num_loop_limit_reached:
		{
			s = "OK_num_loop_limit_reached";
			break;
		}
		case enum_thread_exit_status_OK_lost_socket:
		{
			s = "OK_lost_socket";
			break;
		}
		case enum_thread_exit_status_forced_stop:
		{
			s = "forced_stop";
			break;
		}
		case enum_thread_exit_status_ERR_clock_api:
		{
			s = "ERR_clock_api";
			break;
		}
		case enum_thread_exit_status_ERR_thread_timeout:
		{
			s = "ERR_thread_timeout";
			break;
		}
		case enum_thread_exit_status_ERR_single_message_timeout:
		{
			s = "ERR_single_message_timeout";
			break;
		}
		case enum_thread_exit_status_ERR_reading:
		{
			s = "ERR_reading";
			break;
		}
		case enum_thread_exit_status_ERR_writing:
		{
			s = "ERR_writing";
			break;
		}
		case enum_thread_exit_status_ERR_unknown:
		{
			s = "ERR_unknown";
			break;
		}
	}
	return s;
}

void initialize_thread_info(type_thread_info * p, int fd)
{
	// reset the structure
	memset(p,0,sizeof(*p));
	// set the socket file descriptor
	p->socket_fd = fd;
	// set the max seconds waiting for a message
	p->message_timeout_seconds = def_max_seconds_waiting_incoming_messages;
	// set the maximum thread duration
	p->thread_timeout_seconds = def_max_seconds_thread_incoming_messages;
	// no limits on the number of messages we can receive
	p->max_loop_read_messages = 0;
	// increase the number of thread starts (i.e. it is set to 1)
	p->num_of_thread_starts ++;
	{
		static unsigned int progressive_id;
		p->progressive_id = ++progressive_id;
	}
	// initialize the thread start time...
	clock_gettime(CLOCK_REALTIME, &p->ts_start_time);
}

void mark_as_started_thread_info(type_thread_info * p)
{
	// the thread is started OK
	p->started = 1;
}
void unmark_as_started_thread_info(type_thread_info * p)
{
	// the thread is no more started OK
	p->started = 0;
}

unsigned int is_ended_thread_info(type_thread_info * p)
{
	if (p->started && (p->num_of_thread_starts == p->num_of_thread_ends))
	{
		return 1;
	}
	return 0;
}



void * thread_start(void *arg)
{
    type_thread_info *tinfo = arg;
    typedef enum
    {
    	enum_thread_server_status_init = 0,
    	enum_thread_server_status_read,
    	enum_thread_server_status_write,
    	enum_thread_server_status_ends_clock_api_error,
		enum_thread_server_status_ends_single_message_timeout,
		enum_thread_server_status_ends_thread_timeout,
    	enum_thread_server_status_ends,
    	enum_thread_server_status_ended,
    	enum_thread_server_status_numof
    }enum_thread_server_status;
    enum_thread_server_status thread_server_status = enum_thread_server_status_init;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t stacksize;
    pthread_attr_getstacksize(&attr, &stacksize);
    printf("Thread stack size = %d bytes \n", (int)stacksize);

    // set global thread timeout clock, if needed
	struct timespec ts_timeout_thread = {0};
	if (tinfo->thread_timeout_seconds)
	{
		if (clock_gettime(CLOCK_REALTIME, &ts_timeout_thread) == -1)
		{
			thread_server_status = enum_thread_server_status_ends_clock_api_error;
		}
		else
		{
			ts_timeout_thread.tv_sec += tinfo->thread_timeout_seconds;
		}
	}
	struct timespec ts_timeout_message = {0};

    while (thread_server_status != enum_thread_server_status_ended)
    {
    	usleep(def_thread_sleep_time_between_cycles_us);
    	if (tinfo->force_ends)
    	{
    		tinfo->force_ends_acknowledged = 1;
    		tinfo->thread_exit_status = enum_thread_exit_status_forced_stop;
    		thread_server_status = enum_thread_server_status_ends;
    	}
    	if (tinfo->thread_timeout_seconds)
    	{
    		struct timespec ts_now = {0};
    		if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1)
    		{
    			thread_server_status = enum_thread_server_status_ends_clock_api_error;
    		}
    		else
    		{
        		if (tsCompare(ts_now, ts_timeout_thread) >0 )
        		{
    				thread_server_status = enum_thread_server_status_ends_thread_timeout;
    			}
    		}
    	}
    	switch(thread_server_status)
    	{
    		case enum_thread_server_status_init:
    		default:
    		{
	    		thread_server_status = enum_thread_server_status_read;
	    		if (tinfo->message_timeout_seconds)
	    		{
	    			if (clock_gettime(CLOCK_REALTIME, &ts_timeout_message) == -1)
	    			{
	    				thread_server_status = enum_thread_server_status_ends_clock_api_error;
	    			}
	    			else
	    			{
	    				ts_timeout_message.tv_sec += tinfo->message_timeout_seconds;
	    			}
	    		}

    			break;
    		}
    		case enum_thread_server_status_read:
    		{

    			int socket_error = 0;
    			socklen_t len_socket_error = sizeof (socket_error);
    			int retval_getsockopt = getsockopt (tinfo->socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &len_socket_error);
    			if ((retval_getsockopt != 0) || (socket_error != 0))
    			{
        			tinfo->thread_exit_status = enum_thread_exit_status_OK_lost_socket;
    	    		thread_server_status = enum_thread_server_status_ends;
    	    		break;
    			}

    			struct pollfd pfd = {0};
				pfd.fd = tinfo->socket_fd;
				pfd.events = POLLIN | POLLHUP | POLLRDNORM | POLLRDHUP;
				pfd.revents = 0;
    			if (poll(&pfd, 1, 1) > 0)
    			{
        	    	int n = read(tinfo->socket_fd,tinfo->rx_buffer,sizeof(tinfo->rx_buffer));
        	    	if (n <= 0)
        			{
        	    		if (pfd.revents & POLLRDHUP)
        	    		{
                			tinfo->thread_exit_status = enum_thread_exit_status_OK;
        	    		}
        	    		else
        	    		{
                			tinfo->thread_exit_status = enum_thread_exit_status_ERR_reading;
        	    		}
        	    		thread_server_status = enum_thread_server_status_ends;
        			}
        	    	else if (n > 0)
        	    	{
        	        	//printf("Here is the message: %s\n",tinfo->rx_buffer);
        	    		if (has_reply_decode_incoming_message_from_socket(tinfo->rx_buffer , n, tinfo->tx_buffer,sizeof(tinfo->tx_buffer), &tinfo->tx_buffer_chars_written))
        	    		{
            	    		thread_server_status = enum_thread_server_status_write;
        	    		}
        	    		else
        	    		{
            	    		thread_server_status = enum_thread_server_status_init;
        	    		}
        	    	}
    			}
    			else
    			{
    		    	if (tinfo->message_timeout_seconds)
    		    	{
    		    		struct timespec ts_now = {0};
    		    		if (clock_gettime(CLOCK_REALTIME, &ts_now) == -1)
    		    		{
    		    			thread_server_status = enum_thread_server_status_ends_clock_api_error;
    		    		}
    		    		else
    		    		{
    		        		if (tsCompare(ts_now, ts_timeout_message) >0 )
    		        		{
    		    				thread_server_status = enum_thread_server_status_ends_single_message_timeout;
    		    			}
    		    		}
    		    	}
    			}
    			break;
    		}
    		case enum_thread_server_status_write:
    		{
            	int n = write(tinfo->socket_fd,tinfo->tx_buffer,tinfo->tx_buffer_chars_written);
            	if (n != tinfo->tx_buffer_chars_written)
        		{
        			tinfo->thread_exit_status = enum_thread_exit_status_ERR_writing;
    	    		thread_server_status = enum_thread_server_status_ends;
        		}
    	    	else
    	    	{
    	    		tinfo->num_loop_read_messages_executed++;
    	    		if (tinfo->max_loop_read_messages)
    	    		{
    	    			if (tinfo->num_loop_read_messages_executed >= tinfo->max_loop_read_messages)
    	    			{
    	    				tinfo->thread_exit_status = enum_thread_exit_status_OK_num_loop_limit_reached;
    	    				thread_server_status = enum_thread_server_status_ends;
    	    			}
    	    		}
    	    		thread_server_status = enum_thread_server_status_init;
    	    	}
    			break;
    		}
    		case enum_thread_server_status_ends_single_message_timeout:
    		{
    			tinfo->thread_exit_status = enum_thread_exit_status_ERR_single_message_timeout;
	    		thread_server_status = enum_thread_server_status_ends;
    			break;
    		}
    		case enum_thread_server_status_ends_thread_timeout:
    		{
    			tinfo->thread_exit_status = enum_thread_exit_status_ERR_thread_timeout;
	    		thread_server_status = enum_thread_server_status_ends;
	    		break;
    		}
    		case enum_thread_server_status_ends_clock_api_error:
    		{
    			tinfo->thread_exit_status = enum_thread_exit_status_ERR_clock_api;
	    		thread_server_status = enum_thread_server_status_ends;
	    		break;
    		}
    		case enum_thread_server_status_ends:
    		{
	    		thread_server_status = enum_thread_server_status_ended;
    			close(tinfo->socket_fd);
    			break;
    		}
    		case enum_thread_server_status_ended:
    		{
    			break;
    		}
    	}
    }

	tinfo->num_of_thread_ends = tinfo->num_of_thread_starts;
	return NULL;
}
