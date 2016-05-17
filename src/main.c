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
		appMsgProcess(NULL);
	}
}

int main(int argc, char* argv[])
{

#ifndef def_test_without_Zigbee
	char * selected_serial_port;
	pthread_t rpcThread, appThread, inMThread;

	dbg_print(PRINT_LEVEL_INFO, "%s -- %s %s\n", argv[0], __DATE__, __TIME__);

	// accept only 1
	if (argc < 2)
	{
#ifdef OLINUXINO
		dbg_print(PRINT_LEVEL_INFO, "attempting to use /dev/ttyS1\n\n");
		selected_serial_port = "/dev/ttyS1";
#else
		dbg_print(PRINT_LEVEL_INFO, "attempting to use /dev/USB2\n\n");
		selected_serial_port = "/dev/ttyUSB2";
#endif
	}
	else
	{
		selected_serial_port = argv[1];
	}

	int serialPortFd = rpcOpen(selected_serial_port, 0);
	if (serialPortFd == -1)
	{
		dbg_print(PRINT_LEVEL_ERROR, "could not open serial port\n");
		exit(-1);
	}

	rpcInitMq();

	//init the application thread to register the callbacks
	appInit();

	//Start the Rx thread
	dbg_print(PRINT_LEVEL_INFO, "creating RPC thread\n");
	pthread_create(&rpcThread, NULL, rpcTask, (void *) &serialPortFd);

	//Start the example thread
	dbg_print(PRINT_LEVEL_INFO, "creating example thread\n");
	pthread_create(&appThread, NULL, appTask, NULL);
	pthread_create(&inMThread, NULL, appInMessageTask, NULL);
#endif

// STARTING THE SOCKET SERVER

	printf("Server starting...\n");
	type_handle_server_socket handle_server_socket = {0};
	int thread_create_retcode = pthread_create(&handle_server_socket.thread_id, NULL,&simple_server_thread, &handle_server_socket);
	// check the return code
	if (thread_create_retcode != 0)
	{
		perror("ERROR on main thread_create!");
	}
	printf("Server started OK\n");
	while(!handle_server_socket.is_terminated)
	{
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
