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

#include "ASACZ_app.h"
#include "ASACZ_ZAP.h"
#include "ASACZ_devices_list.h"
#include "ASACZ_firmware_version.h"
#include "ASACZ_conf.h"
#include <ASACZ_boot_check.h>
#include "dataSendRcv.h"
#include "input_cluster_table.h"
#include "server_thread.h"
#include "simple_server.h"
#include <CC2650_fw_update.h>

typedef struct _type_request_exit
{
	unsigned int enabled;
	unsigned int num_req;
	unsigned int num_ack;
}type_request_exit;
static type_request_exit my_request_exit;

void init_request_exit(void)
{
	memset(&my_request_exit, 0, sizeof(my_request_exit));
}
void enable_request_exit(unsigned int enable)
{
	my_request_exit.enabled = !!enable;
}
unsigned int is_enabled_request_exit(void)
{
	return my_request_exit.enabled;
}

void request_exit(void)
{
	if (is_enabled_request_exit())
	{
		my_request_exit.num_req++;
	}
}
unsigned int is_required_exit(void)
{
	unsigned int is_required = 0;
	unsigned int nr = my_request_exit.num_req;
	unsigned int na = my_request_exit.num_ack;
	if (nr != na)
	{
		my_request_exit.num_ack = nr;
		is_required = 1;
	}
	return is_required;
}

void *appTask(void *argument)
{
	while (1)
	{
		appProcess(NULL);
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
	enum_thread_id_autoupdate,
	enum_thread_id_numof
}enum_thread_id;
typedef struct _type_thread_id_created_info
{
	pthread_t t;
	uint32_t created_OK;
}type_thread_id_created_info;

type_thread_id_created_info threads_cancel_info[enum_thread_id_numof];

#include "ASACZ_update_yourself.h"
static type_ASACZ_update_yourself_inout ASACZ_update_yourself_inout;
void get_ASACZ_update_yourself_inout_current_reply(type_fwupd_ASACZ_status_update_reply_body *p_status_reply)
{
	pthread_mutex_lock(&ASACZ_update_yourself_inout.mtx_id);
		memcpy(p_status_reply, &ASACZ_update_yourself_inout.body_reply, sizeof (*p_status_reply));
	pthread_mutex_unlock(&ASACZ_update_yourself_inout.mtx_id);
}
void signal_thread_ASACZ_update_yourself_ends(void)
{
	threads_cancel_info[enum_thread_id_autoupdate].created_OK = 0;
}

unsigned int isOK_ASACZ_update_yourself(type_fwupd_ASACZ_do_update_req_body *p_body_req, type_fwupd_ASACZ_do_update_reply_body *p_body_reply)
{
	unsigned int canceled_OK = 1;
	if (threads_cancel_info[enum_thread_id_autoupdate].created_OK)
	{
		enum_thread_id e = enum_thread_id_autoupdate;
		threads_cancel_info[e].created_OK = 0;
		pthread_t t = threads_cancel_info[e].t;
#ifdef ANDROID
		int s;
		if ( (s = pthread_kill(t, SIGUSR1)) != 0)
		{
			canceled_OK = 0;
			printf("Error canceling thread %d, error = %d (%s)", (int)t, s, strerror(s));
		}
#else
		int s;
		s = pthread_cancel(t);
		if (s != 0)
		{
			canceled_OK = 0;
			syslog(LOG_ERR, "Unable to cancel thread id %i", (int)e);
		}
		else
		{
			void *res;
			/* Join with thread to see what its exit status was */
			s = pthread_join(t, &res);
			if (s != 0)
			{
				canceled_OK = 0;
				syslog(LOG_ERR, "Unable to join thread id %i", (int)e);
			}
			if (res == PTHREAD_CANCELED)
			{
				syslog(LOG_INFO, "Thread id %i canceled OK", (int)e);
			}
			else
			{
				canceled_OK = 0;
				syslog(LOG_ERR, "Error canceling thread id %i", (int)e);
			}
		}
	}
#endif

	if (canceled_OK)
	{
		memset(&ASACZ_update_yourself_inout, 0, sizeof(ASACZ_update_yourself_inout));

		pthread_mutex_init(&ASACZ_update_yourself_inout.mtx_id, NULL);
		{
			pthread_mutexattr_t mutexattr;

			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&ASACZ_update_yourself_inout.mtx_id, &mutexattr);
		}
		memcpy(&ASACZ_update_yourself_inout.body_req, p_body_req, sizeof(ASACZ_update_yourself_inout.body_req));

		int thread_create_retcode = pthread_create(&threads_cancel_info[enum_thread_id_autoupdate].t, NULL, thread_ASACZ_update_yourself, (void*)&ASACZ_update_yourself_inout);
		if (thread_create_retcode != 0)
		{
			p_body_reply->started_OK = 0;
		}
		else
		{
			p_body_reply->started_OK = 1;
			threads_cancel_info[enum_thread_id_autoupdate].created_OK = 1;
		}
	}
	else
	{
		p_body_reply->started_OK = 0;
	}
	return p_body_reply->started_OK;
}

#ifdef ANDROID
void thread_exit_handler(int sig)
{
printf("this signal is %d \n", sig);
pthread_exit(0);
} 
#endif

static void do_shutdown(void)
{
	syslog(LOG_INFO,"%s + doing shutdown...", __func__);
#ifdef print_all_received_messages
	void close_print_message(void);
	close_print_message();
#endif

	{
		dbg_print(PRINT_LEVEL_INFO, "%s: + network shutdown", __func__);
		force_zigbee_shutdown();
#define def_max_wait_shutdown_ms 8000
#define def_base_wait_shutdown_ms 100
#define max_loop_wait_shutdown (1 + def_max_wait_shutdown_ms / def_base_wait_shutdown_ms)
		unsigned int continue_loop = 1;
		int nloop = max_loop_wait_shutdown;
		while(continue_loop)
		{
			enum_app_status s = get_app_status();
			if (s == enum_app_status_shutdown)
			{
				dbg_print(PRINT_LEVEL_INFO, "%s: device has shutdown properly", __func__);
				continue_loop = 0;
			}
			else if (nloop <= 0)
			{
				dbg_print(PRINT_LEVEL_ERROR, "%s: timeout waiting for shutdown", __func__);
				continue_loop = 0;
			}
			else
			{
				nloop--;
				usleep(def_base_wait_shutdown_ms * 1000);
			}
		}
		dbg_print(PRINT_LEVEL_INFO, "%s: - network shutdown", __func__);
	}
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
				printf("Error canceling thread %d, error = %d (%s)", (int)t, s, strerror(s));
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
	syslog(LOG_INFO, "%s the application ends here", __func__);
	syslog(LOG_INFO,"%s - doing shutdown...", __func__);
	// at exit, close system log
	closelog();
}

#include <signal.h>


// Define the function to be called when ctrl-c (SIGINT) signal is sent to process
void signal_shutdown_callback_handler(int signum)
{
	static volatile sig_atomic_t fatal_error_in_progress = 0;
	// Since this handler is established for more than one kind of signal, it might still get invoked recursively by delivery of some other kind
	// of signal.  Use a static variable to keep track of that.
	if (fatal_error_in_progress)
	{
		raise (signum);
	}
	fatal_error_in_progress = 1;

	syslog(LOG_WARNING, "Caught signal %d\n",signum);
	// Cleanup and close up stuff here
	do_shutdown();

	// Now re-raise the signal.  We reactivate the signal’s default handling, which is to terminate the process.
	// We could just call exit or abort, but re-raising the signal sets the return status from the process correctly.
	signal (signum, SIG_DFL);
	raise (signum);
}

static void init_sig_handlers(void)
{
   // Register signal and signal handler
   signal(SIGTERM, signal_shutdown_callback_handler);
   signal(SIGINT, signal_shutdown_callback_handler);
   signal(SIGQUIT, signal_shutdown_callback_handler);
   signal(SIGABRT, signal_shutdown_callback_handler);
}

#ifdef OLINUXINO
int is_OK_do_CC2650_reset(unsigned int enable_boot_mode)
{
	int is_OK = 1;
#define gpio_base_path "/sys/class/gpio"
#define gpio_export_path gpio_base_path"/export"
#define gpio_value_path gpio_base_path"/value"

	syslog(LOG_INFO, "+%s\n", __func__);
	int fd_reset_low = -1;
	int fd_boot_enable_high = -1;
	char fname_reset[128];
	char fname_boot_enable[128];
	if (snprintf(fname_boot_enable, sizeof(fname_boot_enable), "%s/gpio0/value", gpio_base_path) < 0)
	{
		syslog(LOG_ERR, "ERROR doing snprintf %s\n", fname_boot_enable);
		is_OK = 0;
	}
	else if (snprintf(fname_reset, sizeof(fname_reset), "%s/gpio12/value", gpio_base_path) < 0)
	{
		syslog(LOG_ERR, "ERROR doing snprintf reset %s\n", fname_reset);
		is_OK = 0;
	}
	else
	{
		fd_reset_low = open((char *)fname_reset, O_WRONLY);
		fd_boot_enable_high = open((char *)fname_boot_enable, O_WRONLY);
		if (fd_reset_low < 0)
		{
			syslog(LOG_ERR, "ERROR doing open reset gpio %s", fname_reset);
			is_OK = 0;
		}
		else if (fd_boot_enable_high < 0)
		{
			syslog(LOG_ERR, "ERROR doing open boot enable gpio %s", fname_boot_enable);
			is_OK = 0;
		}
		else
		{
			typedef struct _type_cycle_op
			{
				unsigned int gpio_reset12_boot_0;
				unsigned int value;
				unsigned int delay_ms;
			}type_cycle_op;
			type_cycle_op cycle_op[3];

			// reset low (Active) then wait 100ms
			cycle_op[0].gpio_reset12_boot_0 = 12;
			cycle_op[0].value = 0;
			cycle_op[0].delay_ms = 100;

			// boot enable high (Active)/ low (NOT active) then wait 200ms
			cycle_op[1].gpio_reset12_boot_0 = 0;
			cycle_op[1].value = enable_boot_mode ? 1 : 0;
			cycle_op[1].delay_ms = 200;

			// reset high (NOT Active) then wait 10ms
			cycle_op[2].gpio_reset12_boot_0 = 12;
			cycle_op[2].value = 1;
			cycle_op[2].delay_ms = 10;
#if 0
			// boot enable low (NOT Active) then wait 10ms
			cycle_op[3].gpio_reset12_boot_0 = 0;
			cycle_op[3].value = 0;
			cycle_op[3].delay_ms = 10;
#endif
			unsigned int idx_loop;
			for (idx_loop = 0; is_OK && idx_loop < sizeof(cycle_op)/ sizeof(cycle_op[0]); idx_loop++)
			{
				type_cycle_op * p_cyc = &cycle_op[idx_loop];
				char val[256];
				int n= snprintf(val, sizeof(val), "%u", p_cyc->value);
				if (n > 0)
				{
					int val_len = strlen(val);
					int fd = p_cyc->gpio_reset12_boot_0 == 0 ? fd_boot_enable_high : fd_reset_low;
					int n = write(fd, val, val_len);
					if (n != val_len)
					{
						is_OK = 0;
						syslog(LOG_ERR, "ERROR doing write value %u gpio %u OK", p_cyc->value, p_cyc->gpio_reset12_boot_0);
					}
					else
					{
						syslog(LOG_INFO, "set value %u gpio %u OK", p_cyc->value, p_cyc->gpio_reset12_boot_0);
					}
				}
				else
				{
					is_OK = 0;
					syslog(LOG_ERR, "ERR doing snprintf value %u gpio %u OK", p_cyc->value, p_cyc->gpio_reset12_boot_0);
				}
			}
		}
	}

	if (fd_reset_low >= 0)
	{
		close(fd_reset_low);
	}
	if (fd_boot_enable_high >= 0)
	{
		close(fd_boot_enable_high);
	}

	syslog(LOG_INFO, "-%s returns is_OK = %u\n", __func__, is_OK);
	return	is_OK;
}
#endif

//#define def_test_fw_upd
#undef def_test_fw_upd

#include <libgen.h>
static void print_syntax(char *full_name)
{
	char copy_path[512];
	snprintf(copy_path, sizeof(copy_path), "%s", full_name);
	char *bname = basename(copy_path);
	if (!bname)
	{
		bname = "ASACZ";
	}
	printf("Syntax:\n");
	printf("%s --help\n", bname);
		printf("\tshows this help\n");
	printf("%s --version\n", bname);
		printf("\tshows the program version info\n");
	printf("%s --respawn\n", bname);
		printf("\tused by the system when respawning the server\n");
	printf("%s --CC2650fwupdate=<CC2650_firmware_file_pathname>\n", bname);
		printf("\tupdates the CC2650 firmware using the signed binary <CC2650_firmware_file_pathname>\n");
	printf("%s [udpport=<UDP port number> [serialport=<serial port name>]]\n", bname);
		printf("\tsets the UDP port number (default %u) and optionally the serial port name (default %s)\n", def_port_number, def_selected_serial_port);
	printf("\n");
	printf("Examples of valid calls:\n");
	printf("%s\n", bname);
		printf("\tuses the default UDP port number %u and the default serial port name %s\n", def_port_number, def_selected_serial_port);
	printf("%s udpport=3118\n", bname);
		printf("\tuses the UDP port number 3118 and the default serial port name %s\n", def_selected_serial_port);
	printf("%s udpport=3119 serialport=/dev/ttys3\n", bname);
		printf("\tuses the UDP port number 3118 and the serial port name /dev/ttys3\n");
	printf("%s --CC2650fwupdate=/usr/ASACZ_CC2650fw_COORDINATOR.2_6_5\n", bname);
		printf("\tupdates the firmware using the file /usr/ASACZ_CC2650fw_COORDINATOR.2_6_5\n");
}


int main(int argc, char* argv[])
{
	find_my_own_name(argv[0]);
#define def_filename_signal_ASACZ_started_once "/tmp/ASACZ_started_once"
	//unsigned int respawn = 0;
	volatile unsigned int do_CC2650_fw_update = 0;
	char CC2650_fw_path[1024];
	memset(CC2650_fw_path, 0, sizeof(CC2650_fw_path));

	init_request_exit();

    if ((argc >= 2) && (strncasecmp(argv[1],"--help",6)==0))
    {
        print_syntax(argv[0]);
		return 0;
    }
    if ((argc >= 2) && (strncasecmp(argv[1],"--version",9)==0))
    {
        init_ASACZ_firmware_version();
        char v[128];
    	get_ASACZ_firmware_version_string(v, sizeof(v));
        printf("version:\n\t%s\n", v);
		return 0;
    }
    if ((argc >= 2) && (strncasecmp(argv[1],"--respawn",9)==0))
    {
    	//respawn = 1;
    }
    if ((argc >= 2) && (strncasecmp(argv[1],"--CC2650fwupdate=",17)==0))
    {
		printf("CC2650 firmware update has been requested\n");
    	int n = snprintf(CC2650_fw_path, sizeof(CC2650_fw_path), "%s", &argv[1][17]);
    	if (n >= (int)sizeof(CC2650_fw_path))
    	{
    		printf("too long input filename (max %u chars); firmware update will be NOT executed\n", sizeof(CC2650_fw_path) - 1);
    	}
    	else
    	{
    		printf("CC2650 firmware update will start using file %s\n", CC2650_fw_path);
        	do_CC2650_fw_update = 1;
    	}
    }
	init_sig_handlers();
	
#if 0
	{
		void ASACZ_conf_module_test(void);
		ASACZ_conf_module_test();
	}
#endif

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
	unsigned int asacz_never_started_before = 0;
#ifdef OLINUXINO
	{
		// if this file NOT exists, the application was never started at least once before power up
		if( access( def_filename_signal_ASACZ_started_once, F_OK ) == -1 )
		{
			asacz_never_started_before = 1;
		}
	}
#endif
	if (asacz_never_started_before)
	{
		syslog(LOG_WARNING, "The system is booting up! Waiting some seconds while it settles.");
		// execute a quite long sleep to give the system the needed time to properly setup, if booting
		usleep( 15 * 1000 * 1000);
		syslog(LOG_WARNING, "Long delay after system boot-up settlement ends here.");
	}
	memset(&threads_cancel_info, 0, sizeof(threads_cancel_info));

	if (do_CC2650_fw_update)
	{
		type_ASACZ_CC2650_fw_update_header my_h;
		printf("CC2650 firmware update starts right NOW!\n");
		printf("\t please wait, up to one minute could be needed...\n");
		enum_do_CC2650_fw_update_retcode r = do_CC2650_fw_operation(enum_CC2650_fw_operation_update_firmware, CC2650_fw_path, &my_h);
		if (enum_do_CC2650_fw_update_retcode_OK == r)
		{
			printf("\nOK CC2650 firmware updated OK ! :) :) :) \n");
			printf("\t the updated firmware has version %s, type %s\n", my_h.ascii_version_number, my_h.ascii_fw_type);
		}
		else
		{
			printf("\n****\nBAD NEWS! ERROR updating CC2650 firmware: %s\n\n", get_msg_from_CC2650_fw_update_retcode(r));
		}
		return 0;
	}
	enable_request_exit(1);

#ifdef OLINUXINO
	// reset chip and disables boot mode
	{
		unsigned int is_OK = 0;
		unsigned int idx_try_do_CC2650_reset;
#define def_total_timeout_try_do_CC2650_reset_ms 20000
#define def_pause_try_do_CC2650_reset_ms 1000
#define def_num_try_do_CC2650_reset (1 + def_total_timeout_try_do_CC2650_reset_ms / def_pause_try_do_CC2650_reset_ms)
		for (idx_try_do_CC2650_reset = 0; !is_OK && idx_try_do_CC2650_reset < def_num_try_do_CC2650_reset ; idx_try_do_CC2650_reset++)
		{
			is_OK = is_OK_do_CC2650_reset(0);
			if (!is_OK)
			{
				if (idx_try_do_CC2650_reset + 1 < def_num_try_do_CC2650_reset)
				{
			        my_log(LOG_WARNING, "%s trying again to reset CC2650 in %u ms, try %u of %u", __func__, def_pause_try_do_CC2650_reset_ms, idx_try_do_CC2650_reset + 1 , def_num_try_do_CC2650_reset);
					usleep(def_pause_try_do_CC2650_reset_ms * 1000);
				}
			}
		}
		if (!is_OK)
		{
	        my_log(LOG_ERR, "%s unable to reset CC2650!", __func__);
		}
	}

	if (asacz_never_started_before)
	{

        my_log(LOG_INFO, "%s +checking boot", __func__);
		unsigned int is_OK = 0;
		unsigned int idx_try_boot_check;
#define def_total_timeout_try_boot_check_ms 20000
#define def_pause_try_boot_check_ms 2000
#define def_num_try_boot_check (1 + def_total_timeout_try_boot_check_ms / def_pause_try_boot_check_ms)

		for (idx_try_boot_check = 0; !is_OK && idx_try_boot_check < def_num_try_boot_check ; idx_try_boot_check++)
		{
			enum_boot_check_retcode r = boot_check();
			if (r == enum_boot_check_retcode_OK)
			{
				is_OK = 1;
				my_log(LOG_INFO, "%s boot check is executed OK", __func__);
			}
			else
			{
				my_log(LOG_ERR, "%s boot check returns ERROR, try %u of %u", __func__, idx_try_boot_check + 1, def_num_try_boot_check);
				// wait 1 second before checking again!
				if (idx_try_boot_check + 1 < def_num_try_boot_check)
				{
					usleep(1000 * 1000);
				}
			}
		}
		if (!is_OK)
		{
	        my_log(LOG_ERR, "%s unable to do to boot check!", __func__);
		}
        my_log(LOG_INFO, "%s -checking boot", __func__);
	}
	else
	{
        my_log(LOG_INFO, "%s skipping the boot check because we are not booting", __func__);
	}

	// write down an information stating that we booted up and already executed the boot check
	if (asacz_never_started_before)
	{
		FILE *f = fopen(def_filename_signal_ASACZ_started_once, "wb");
		if (f)
		{
			fprintf(f, "ASACZ started once\n");
			fclose(f);
		}
	}

#endif
	//atexit(my_at_exit);

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
    }
	init_ASACZ_device_list();
	init_input_cluster_table();

#ifdef print_all_received_messages
	void init_print_message(void);
	init_print_message();
#endif
	{
		enum_load_conf_retcode rload = ASACZ_load_conf();
		if (rload != enum_load_conf_retcode_OK)
		{
			my_log(LOG_ERR,"%s: unable to load the ASACZ conf", __func__);
			enum_save_conf_retcode rsave = ASACZ_save_conf();
			if (rsave != enum_save_conf_retcode_OK)
			{
				my_log(LOG_ERR,"%s: unable to save the ASACZ conf", __func__);
			}
			else
			{
				my_log(LOG_INFO,"%s: default ASACZ conf saved OK", __func__);
			}
		}
		else
		{
			my_log(LOG_INFO,"%s: ASACZ conf loaded OK", __func__);
		}
	}



#ifndef def_test_without_Zigbee
	char * selected_serial_port;

	dbg_print(PRINT_LEVEL_INFO, "%s -- %s %s\n", argv[0], __DATE__, __TIME__);

	selected_serial_port = def_selected_serial_port;
	// accept only 1
	if (argc < 3)
	{
		//printf("Opening serial port %s\n", selected_serial_port);
#ifdef ANDROID
	{
		extern int radio_asac_barebone_on();
		radio_asac_barebone_on();
	}
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
		dbg_print(PRINT_LEVEL_INFO, "creating app thread\n");
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
		// issuing the exit request
		if (is_required_exit())
		{
			syslog(LOG_INFO, "%s: exiting from the main loop as per exit request", __func__);
			// send a SIGTERM to exit gracefully
			kill(getpid(), SIGTERM);
		}

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
	dbg_print(PRINT_LEVEL_INFO,"Server ends here");
	return 0;
}
