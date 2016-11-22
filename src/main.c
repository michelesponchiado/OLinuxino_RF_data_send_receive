/**************************************************************************************************
 * Filename:       main.c
 * Description:    This file contains the main for the gnu platform.
 *
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include "rpc.h"
#include "dataSendRcv.h"

#include "dbgPrint.h"

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
#include "ASACZ_devices_list.h"
#include "ASACZ_firmware_version.h"
#include "input_cluster_table.h"

void *rpcTask(void *argument)
{
	while (1)
	{
		rpcProcess();
	}

	dbg_print(PRINT_LEVEL_WARNING, "rpcTask exited!\n");
}

void *appTask(void *argument)
{
	while (1)
	{
		appProcess(NULL);
	}
}

void *appInMessageTask(void *argument)
{
	while (1)
	{
		//usleep(1000);
		appMsgProcess(NULL);
	}
}
static void open_syslog(void)
{
// this goes straight to /var/log/syslog file
//	Nov  3 12:16:53 localhost ASAC_Zlog[13149]: Log just started
//	Nov  3 12:16:53 localhost ASAC_Zlog[13149]: The application starts

	openlog("ASAC_Zlog", LOG_PID|LOG_CONS, LOG_DAEMON);
	syslog(LOG_INFO, "Log just started");
}

typedef enum
{
	enum_thread_id_rpc,
	enum_thread_id_app,
	enum_thread_id_inMessage,
	enum_thread_id_socket,
	enum_thread_id_numof
}enum_thread_id;
typedef struct _type_thread_id_created_info
{
	pthread_t t;
	uint32_t created_OK;
}type_thread_id_created_info;

type_thread_id_created_info threads_cancel_info[enum_thread_id_numof];

#ifdef ANDROID
void thread_exit_handler(int sig)
{
printf("this signal is %d \n", sig);
pthread_exit(0);
} 
#endif

static void my_at_exit(void)
{
	enum_thread_id e;
	for (e = (enum_thread_id)0; e < enum_thread_id_numof; e++)
	{
		if (!threads_cancel_info[e].created_OK)
		{
			syslog(LOG_INFO, "Skipping uninitialized thread id %i", (int)e);
		}
		else
		{
			syslog(LOG_INFO, "Trying to cancel initialized thread id %i", (int)e);
			threads_cancel_info[e].created_OK = 0;
			pthread_t t = threads_cancel_info[e].t;
#ifdef ANDROID
			int s;
			if ( (s = pthread_kill(t, SIGUSR1)) != 0)
			{
				printf("Error cancelling thread %d, error = %d (%s)", (int)t, s, strerror(s));
			}
#else
			int s;
			s = pthread_cancel(t);
			if (s != 0)
			{
				syslog(LOG_ERR, "Unable to cancel thread id %i", (int)e);
			}
			else
			{
				void *res;
				/* Join with thread to see what its exit status was */
				s = pthread_join(t, &res);
				if (s != 0)
				{
					syslog(LOG_ERR, "Unable to join thread id %i", (int)e);
				}
				if (res == PTHREAD_CANCELED)
				{
					syslog(LOG_INFO, "Thread id %i canceled OK", (int)e);
				}
				else
				{
					syslog(LOG_ERR, "Error canceling thread id %i", (int)e);
				}
			}
#endif
		}
	}
	syslog(LOG_INFO, "The application closes");
	// at exit, close system log
	closelog();
}

int main(int argc, char* argv[])
{

	type_handle_server_socket handle_server_socket;
	memset(&handle_server_socket, 0, sizeof(handle_server_socket));
#ifdef ANDROID
	{
		struct sigaction actions;
		memset(&actions, 0, sizeof(actions));
		sigemptyset(&actions.sa_mask);
		actions.sa_flags = 0;
		actions.sa_handler = thread_exit_handler;
		sigaction(SIGUSR1,&actions,NULL);
	}
#endif

	// open the system log
	open_syslog();
	syslog(LOG_INFO, "The application starts");
	memset(&threads_cancel_info, 0, sizeof(threads_cancel_info));
	atexit(my_at_exit);

	handle_server_socket.port_number = def_port_number;
    if ((argc >= 2) && (strncasecmp(argv[1],"udpport=",8)==0))
    {
		handle_server_socket.port_number = atoi(&argv[1][8]);
        syslog(LOG_INFO, "Forcing UDP port %u", (unsigned int )handle_server_socket.port_number);
    }
    syslog(LOG_INFO, "Using UDP port %u", (unsigned int )handle_server_socket.port_number);

// call the initialization procedures
    {
        init_ASACZ_firmware_version();
        char v[128];
    	get_ASACZ_firmware_version_string(v, sizeof(v));
        syslog(LOG_INFO, "Firmware version: %s", v);
        printf("%s: firmware version: %s\n", __func__, v);
    }
	init_ASACZ_device_list();
	init_input_cluster_table();


#ifndef def_test_without_Zigbee
	char * selected_serial_port;
#ifdef ANDROID
	selected_serial_port = "/dev/ttymxc2";
#endif

	dbg_print(PRINT_LEVEL_INFO, "%s -- %s %s\n", argv[0], __DATE__, __TIME__);

	// accept only 1
	if (argc < 3)
	{
#ifdef ANDROID
	{
		extern int radio_asac_barebone_on();
		radio_asac_barebone_on();
	}
	selected_serial_port = "/dev/ttymxc2";
	printf("Opening serial port %s\n", selected_serial_port);
#else
	#ifdef OLINUXINO
			dbg_print(PRINT_LEVEL_INFO, "attempting to use /dev/ttyS1\n\n");
			selected_serial_port = "/dev/ttyS1";
	#else
			dbg_print(PRINT_LEVEL_INFO, "attempting to use /dev/USB1\n\n");
			selected_serial_port = "/dev/ttyUSB1";
	#endif
#endif
	}
	else if ((argc >= 3) && (strncasecmp(argv[2],"serialport=",11)==0))
	{
		selected_serial_port = &argv[2][11];
	    syslog(LOG_INFO, "Forcing serial port %s", selected_serial_port);
	}
    syslog(LOG_INFO, "Using serial port %s", selected_serial_port);

	int serialPortFd = rpcOpen(selected_serial_port, 0);
	if (serialPortFd == -1)
	{
		dbg_print(PRINT_LEVEL_ERROR, "could not open serial port\n");
		exit(-1);
	}

	{
		rpcInitMq();

		//init the application thread to register the callbacks
		appInit();

		int thread_create_retcode = 0;

		//Start the Rx thread
		dbg_print(PRINT_LEVEL_INFO, "creating RPC thread\n");
		thread_create_retcode = pthread_create(&threads_cancel_info[enum_thread_id_rpc].t, NULL, rpcTask, (void *) &serialPortFd);
		if (thread_create_retcode != 0)
		{
			syslog(LOG_ERR, "Unable to create the rpc thread, return code %i, message: %s", thread_create_retcode, strerror(errno));
			exit(EXIT_FAILURE);
		}
		threads_cancel_info[enum_thread_id_rpc].created_OK = 1;

		//Start the example thread
		dbg_print(PRINT_LEVEL_INFO, "creating example thread\n");
		thread_create_retcode = pthread_create(&threads_cancel_info[enum_thread_id_app].t, NULL, appTask, NULL);
		if (thread_create_retcode != 0)
		{
			syslog(LOG_ERR, "Unable to create the app thread, return code %i, message: %s", thread_create_retcode, strerror(errno));
			exit(EXIT_FAILURE);
		}
		threads_cancel_info[enum_thread_id_app].created_OK = 1;

		// start the in message thread
		thread_create_retcode = pthread_create(&threads_cancel_info[enum_thread_id_inMessage].t, NULL, appInMessageTask, NULL);
		if (thread_create_retcode != 0)
		{
			syslog(LOG_ERR, "Unable to create the inMessage thread, return code %i, message: %s", thread_create_retcode, strerror(errno));
			exit(EXIT_FAILURE);
		}
		threads_cancel_info[enum_thread_id_inMessage].created_OK = 1;
	}
#endif

// STARTING THE SOCKET SERVER

	{
		syslog(LOG_INFO, "Creating the main thread");
		int thread_create_retcode = pthread_create(&threads_cancel_info[enum_thread_id_socket].t, NULL,&ASACZ_UDP_server_thread, &handle_server_socket);
		// check the return code
		if (thread_create_retcode != 0)
		{
			syslog(LOG_ERR, "Unable to create the main thread, return code %i, message: %s", thread_create_retcode, strerror(errno));
			exit(EXIT_FAILURE);
		}
		threads_cancel_info[enum_thread_id_socket].created_OK = 1;
		syslog(LOG_INFO, "Main thread created OK");
	}

	while(!handle_server_socket.is_terminated)
	{
		// sleep some time and do basically nothing
		usleep(1000000);
//#define def_test_shutdown
#ifdef def_test_shutdown
		{
			static int i_test_shutdown;
			if (++i_test_shutdown == 36)
			{
				printf("Server shutdown starts\n");
				if (is_OK_shutdown_server(&handle_server_socket))
				{
					printf("Server shutdown OK\n");
				}
				else
				{
					printf("Server shutdown ERROR\n");
				}
			}
		}
#endif
	}
	printf("Server ends here\n");
	return 0; /* we never get here */


}
