/*
 * simple_server_common.h
 *
 *  Created on: Apr 20, 2016
 *      Author: michele
 */

#ifndef SIMPLE_SERVER_COMMON_H_
#define SIMPLE_SERVER_COMMON_H_

// use this to enable debug printouts
#define def_enable_debug


// the port number we use
#define def_port_number 3117
// the maximum number of pending connections we can tolerate
#define LISTEN_BACKLOG 50
// max number of chars in the thread receive buffer
#define def_max_char_thread_rx_buffer 4096
// max number of chars in the thread transmit buffer
#define def_max_char_thread_tx_buffer 4096
// max time waiting for incoming messages: set to 0 to disable the timeout
#define def_max_seconds_waiting_incoming_messages 60
// max duration of the receiving thread: set to 0 to disable the timeout
#define def_max_seconds_thread_incoming_messages 240
// sleep time between cycles of the connection loop [microseconds]
#define def_sleep_time_between_cycles_us 1000
// thread sleep time between cycles of the connection loop [microseconds]
#define def_thread_sleep_time_between_cycles_us 1000
// sleep time while waiting for available threads from the pool [microseconds]
#define def_sleep_time_waiting_thread_pool_us 10000
// max wait for a thread to terminate after the force has been sent [ms]
#define def_max_ms_wait_thread_forced_ends 1000
// max wait time waiting for the shutdown of the server [seconds]
#define def_max_ms_wait_server_shutdown_seconds 10


#endif /* SIMPLE_SERVER_COMMON_H_ */
