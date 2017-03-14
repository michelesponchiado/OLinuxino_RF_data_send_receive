/*
 * ASACZ_diag_test.c
 *
 *  Created on: Mar 9, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "ASACZ_ZAP_AF_register.h"
#include "ASACZ_diag_test.h"

//#define def_simulate_diag_test

#ifdef def_simulate_diag_test
static int64_t start_time_ms;
static int64_t prev_time_ms;
static int running;

#define flow_tx_OK_per_second 100
#define flow_rx_OK_per_second 90
#define flow_tx_ERR_per_thousand 23
#define flow_rx_ERR_per_thousand 55

#define delta_tx_OK_per_second 10
#define delta_rx_OK_per_second 8
type_admin_diag_test_status_standard_format_body private_default_body;
static int random_int_min_max(int min, int max)
{
   return min + rand() % (max+1 - min);
}
#endif

#define def_min_diag_msg_length 20
#define def_max_diag_msg_length 120


typedef enum
{
	enum_diag_test_status_idle = 0,
	enum_diag_test_status_init,
	enum_diag_test_status_run,
	enum_diag_test_status_run_wait_after_tx,
	enum_diag_test_status_ends,
	enum_diag_test_status_stop,

	enum_diag_test_status_numof
}enum_diag_test_status;

typedef struct _type_handle_diag_test_stats
{
	uint64_t num_msg_tx_OK;
	uint64_t num_msg_tx_ERR;
	uint64_t num_bytes_tx_OK;
	uint64_t num_msg_rx_OK;
	uint64_t num_msg_rx_ERR;
	uint64_t num_bytes_rx_OK;
	uint32_t elapsed_ms;
}type_handle_diag_test_stats;

typedef enum
{
	enum_diag_msg_type_send_req = 0,
	enum_diag_msg_type_send_ack,
	enum_diag_msg_type_numof
}enum_diag_msg_type;

typedef struct _type_diag_message
{
	uint32_t progressive_id;
	uint16_t diag_msg_type; // must be of type enum_diag_msg_type
	uint8_t body[256];
}__attribute__((__packed__)) type_diag_message;

#define def_mark_free_sent_transId 0xffffffff
typedef struct _type_handle_diag_sent_msg_info
{
	uint32_t trans_id;
	uint32_t message_length;
}type_handle_diag_sent_msg_info;

#define def_max_diag_batches 64
#define def_max_send_times 64
typedef struct _type_handle_diag_test_run
{
	volatile unsigned int is_running;
	uint16_t message_length;
	uint32_t progressive_id;
	type_diag_message message;
	type_handle_diag_sent_msg_info sent_msg_info[16];
	uint32_t num_wait_transId_free;
	int64_t batch_start_time;
	unsigned int idx_next_batch;
	type_handle_diag_test_stats batches[def_max_diag_batches];
	type_handle_diag_test_stats stats;

	unsigned int send_times_idx;
	int64_t send_times[def_max_send_times];

}type_handle_diag_test_run;

typedef struct _type_diag_averages
{
	float average_num_msg_per_second_tx;
	float average_num_msg_per_second_rx;
	float average_num_bytes_per_second_tx;
	float average_num_bytes_per_second_rx;
}type_diag_averages;


typedef enum
{
	enum_diag_mutex_stats = 0,
	enum_diag_mutex_numof
}enum_diag_mutex;

typedef struct _type_handle_diag_test
{
	enum_diag_test_status status;
	type_admin_diag_test_req_start_body req_start;
	DataRequestFormat_t DataRequest;
	type_handle_diag_test_stats stats;
	type_handle_diag_test_run run;
	pthread_mutex_t mtx_id[enum_diag_mutex_numof];
	type_diag_averages averages;
	uint16_t server_network_short_address;
	volatile uint32_t idle_req;
	volatile uint32_t idle_ack;
	volatile uint32_t start_req;
	volatile uint32_t start_ack;
}type_handle_diag_test;

static type_handle_diag_test handle_diag_test;

unsigned int is_OK_AF_callback_diag_transId(uint8_t transId)
{
	if (handle_diag_test.run.is_running == 0)
	{
		return 0;
	}
	unsigned int is_it = 0;
	type_handle_diag_test_run *p_run = &handle_diag_test.run;
	unsigned int i;
	for (i = 0; !is_it && i < sizeof(p_run->sent_msg_info) / sizeof(p_run->sent_msg_info[0]); i++)
	{
		if (p_run->sent_msg_info[i].trans_id == transId)
		{
			type_handle_diag_test_stats *p_stats = &handle_diag_test.run.stats;
			pthread_mutex_lock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
				p_stats->num_msg_tx_OK++;
				p_stats->num_bytes_tx_OK += p_run->sent_msg_info[i].message_length;
			pthread_mutex_unlock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
			p_run->sent_msg_info[i].trans_id = def_mark_free_sent_transId;
			is_it = 1;
		}
	}
	return is_it;
}


static void init_transId(void)
{
	type_handle_diag_test_run *p_run = &handle_diag_test.run;
	unsigned int i;
	for (i = 0; i < sizeof(p_run->sent_msg_info) / sizeof(p_run->sent_msg_info[0]); i++)
	{
		p_run->sent_msg_info[i].trans_id = def_mark_free_sent_transId;
	}
}

static unsigned int is_OK_get_free_transId_index(unsigned int *p_index)
{
	unsigned int is_OK = 0;
	type_handle_diag_test_run *p_run = &handle_diag_test.run;
	unsigned int i;
	for (i = 0; !is_OK && i < sizeof(p_run->sent_msg_info) / sizeof(p_run->sent_msg_info[0]); i++)
	{
		if (p_run->sent_msg_info[i].trans_id == def_mark_free_sent_transId)
		{
			*p_index = i;
			is_OK = 1;
		}
	}
	return is_OK;
}

void diag_rx_callback(IncomingMsgFormat_t *msg)
{
	type_handle_diag_test_run *p_run = &handle_diag_test.run;

	if (msg->Len < sizeof(p_run->message.progressive_id) + sizeof(p_run->message.diag_msg_type))
	{
		my_log(LOG_ERR, "Invalid diagnostic message length %u", (uint32_t)msg->Len);
		return;
	}

	type_diag_message * p_msg_rx = (type_diag_message *) msg->Data;
	// if this is the acknowledgment of a previously sent message, update the stats
	if (p_msg_rx->diag_msg_type == enum_diag_msg_type_send_ack)
	{
		type_handle_diag_test_stats *p_stats = &handle_diag_test.run.stats;
		pthread_mutex_lock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
			p_stats->num_msg_rx_OK++;
			p_stats->num_bytes_rx_OK += msg->Len;
		pthread_mutex_unlock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
	}
	// if this is a diagnostic message request, send the reply
	else if (p_msg_rx->diag_msg_type == enum_diag_msg_type_send_req)
	{
		DataRequestFormat_t dr;
		dr.Len         = msg->Len;
		// store the message body
		memcpy(dr.Data, (char*) &p_run->message, dr.Len);
		type_diag_message * p_msg_tx = (type_diag_message *) dr.Data;
		// this is the acknowledgment of the request
		p_msg_tx->diag_msg_type = enum_diag_msg_type_send_ack;
		// set the destination address
		dr.DstAddr     = msg->SrcAddr;
		dr.DstEndpoint = msg->SrcEndpoint;
		dr.ClusterID   = msg->ClusterId;
		dr.SrcEndpoint = msg->DstEndpoint;
		// set the transaction Id to 0
		dr.TransID     = 0;
		// send the message, ignore the return code
		afDataRequest(&dr);

	}

}

unsigned int is_request_start_diag_test(void)
{
	if (handle_diag_test.start_req != handle_diag_test.start_ack)
	{
		handle_diag_test.start_ack = handle_diag_test.start_req;
		handle_diag_test.status = enum_diag_test_status_init;
		return 1;
	}
	return 0;
}
unsigned int is_running_diag_test(void)
{

	return handle_diag_test.run.is_running;
}
unsigned int is_OK_handle_diag_test(void)
{
	if (handle_diag_test.idle_req != handle_diag_test.idle_ack)
	{
		handle_diag_test.idle_ack = handle_diag_test.idle_req;
		handle_diag_test.start_ack = handle_diag_test.start_req;
		handle_diag_test.status = enum_diag_test_status_idle;
	}
	unsigned int is_OK = 1;
	switch(handle_diag_test.status)
	{
		case enum_diag_test_status_idle:
		default:
		{
			handle_diag_test.run.is_running = 0;
			is_OK = 0;
			break;
		}
		case enum_diag_test_status_stop:
		{
			handle_diag_test.status = enum_diag_test_status_ends;
			break;
		}
		case enum_diag_test_status_init:
		{
			handle_diag_test.status = enum_diag_test_status_run;

			memset(&handle_diag_test.stats, 0, sizeof(handle_diag_test.stats));
			memset(&handle_diag_test.run, 0, sizeof(handle_diag_test.run));
			init_transId();
			handle_diag_test.run.batch_start_time = get_current_epoch_time_ms();
			handle_diag_test.run.is_running = 1;

			uint16_t message_length = def_min_diag_msg_length;
			switch(handle_diag_test.req_start.length_type.enum_type)
			{
				case enum_admin_diag_test_message_length_type_default:
				case enum_admin_diag_test_message_length_type_medium:
				default:
				{
					message_length = 70;
					break;
				}
				case enum_admin_diag_test_message_length_type_very_short:
				{
					message_length = def_min_diag_msg_length;
					break;
				}
				case enum_admin_diag_test_message_length_type_short:
				{
					message_length = 45;
					break;
				}
				case enum_admin_diag_test_message_length_type_large:
				{
					message_length = 95;
					break;
				}
				case enum_admin_diag_test_message_length_type_very_large:
				{
					message_length = def_max_diag_msg_length;
					break;
				}
			}
			handle_diag_test.run.message_length = message_length;
			break;
		}
		case enum_diag_test_status_run:
		{
			handle_diag_test.status = enum_diag_test_status_run_wait_after_tx;
			type_handle_diag_test_stats *p_stats = &handle_diag_test.run.stats;
			type_handle_diag_test_run *p_run = &handle_diag_test.run;

			unsigned int index_sent_trans_id = 0;
			if (!is_OK_get_free_transId_index(&index_sent_trans_id))
			{
				if (++p_run->num_wait_transId_free < 128)
				{
					break;
				}
				init_transId();
			}


			DataRequestFormat_t * pdr = &handle_diag_test.DataRequest;
			// store the message length
			p_run->message.progressive_id = ++p_run->progressive_id;
			p_run->message.diag_msg_type = enum_diag_msg_type_send_req;
			int residual_length = p_run->message_length;
			residual_length -= sizeof(p_run->message.progressive_id);
			if (residual_length < def_min_diag_msg_length)
			{
				residual_length = def_min_diag_msg_length;
			}
			else if (residual_length > def_max_diag_msg_length)
			{
				residual_length = def_max_diag_msg_length;
			}
			snprintf((char*)p_run->message.body, sizeof(p_run->message.body), "%-*.*s", residual_length, residual_length, "ASACZ diagnostic test message");
			uint16_t total_length = sizeof(p_run->message.progressive_id) + residual_length;
			if (total_length > 128)
			{
				total_length = 128;
			}
			pdr->Len         = total_length;
			// store the message body
			memcpy(pdr->Data, (char*) &p_run->message, pdr->Len);
			// set the destination address
			pdr->DstAddr     = handle_diag_test.server_network_short_address;
			pdr->DstEndpoint = ASACZ_reserved_endpoint;
			pdr->ClusterID   = ASACZ_reserved_endpoint_cluster_diag;
			pdr->SrcEndpoint = ASACZ_reserved_endpoint;
			// retrieve a new transaction ID
			pdr->TransID     = get_app_new_trans_id(handle_app.message_id);
			p_run->sent_msg_info[index_sent_trans_id].trans_id = pdr->TransID;
			p_run->sent_msg_info[index_sent_trans_id].message_length = pdr->Len;
			{
				unsigned int idx_time = p_run->send_times_idx;
				if (idx_time >= def_max_send_times)
				{
					idx_time = 0;
				}
				p_run->send_times[idx_time] = get_current_epoch_time_ms();
				if (++idx_time >= def_max_send_times)
				{
					idx_time = 0;
				}
				p_run->send_times_idx = idx_time;
			}
			p_run->num_wait_transId_free = 0;

			// send the message
			int32_t status = afDataRequest(pdr);
			// check the return code
			if (status != MT_RPC_SUCCESS)
			{
				pthread_mutex_lock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
					p_stats->num_msg_tx_ERR++;
				pthread_mutex_unlock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
			}

			break;
		}
		case enum_diag_test_status_run_wait_after_tx:
		{
			handle_diag_test.status = enum_diag_test_status_run;
			//usleep(4000);
			int64_t now = get_current_epoch_time_ms();
			int64_t elapsed = now - handle_diag_test.run.batch_start_time;
			if (elapsed >= handle_diag_test.req_start.batch_acquire_period_ms)
			{
				type_handle_diag_test_stats *p_run_stats = &handle_diag_test.run.stats;
				type_handle_diag_test_stats *p_global_stats = &handle_diag_test.stats;
				pthread_mutex_lock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
				uint32_t idx = handle_diag_test.run.idx_next_batch;
					if (idx >= sizeof(handle_diag_test.run.batches) / sizeof(handle_diag_test.run.batches[0]))
					{
						 idx = 0;
					}
					type_handle_diag_test_stats *p_batch = &handle_diag_test.run.batches[idx];
					*p_batch = *p_run_stats;
					p_batch->elapsed_ms = elapsed;

					if (++idx >= sizeof(handle_diag_test.run.batches) / sizeof(handle_diag_test.run.batches[0]))
					{
						 idx = 0;
					}
					if (idx >= handle_diag_test.req_start.num_batch_samples_for_average)
					{
						idx = 0;
					}
					handle_diag_test.run.idx_next_batch = idx;

					p_global_stats->num_msg_rx_OK 	+= p_run_stats->num_msg_rx_OK;
					p_global_stats->num_msg_rx_ERR 	+= p_run_stats->num_msg_rx_ERR;
					p_global_stats->num_bytes_rx_OK += p_run_stats->num_bytes_rx_OK;
					p_global_stats->num_msg_tx_OK 	+= p_run_stats->num_msg_tx_OK;
					p_global_stats->num_msg_tx_ERR 	+= p_run_stats->num_msg_tx_ERR;
					p_global_stats->num_bytes_tx_OK += p_run_stats->num_bytes_tx_OK;

					type_diag_averages avg;
					memset(&avg, 0, sizeof(avg));
					uint32_t tot_elapsed_ms = 0;
					unsigned int i;
					for (i = 0; i < sizeof(handle_diag_test.run.batches) / sizeof(handle_diag_test.run.batches[0]) && i < handle_diag_test.req_start.num_batch_samples_for_average; i++)
					{
						type_handle_diag_test_stats *pcur = &handle_diag_test.run.batches[i];
						avg.average_num_bytes_per_second_rx += pcur->num_bytes_rx_OK;
						avg.average_num_bytes_per_second_tx += pcur->num_bytes_tx_OK;
						avg.average_num_msg_per_second_rx 	+= pcur->num_msg_rx_OK;
						avg.average_num_msg_per_second_tx 	+= pcur->num_msg_tx_OK;
						tot_elapsed_ms += pcur->elapsed_ms;
					}
					float inv_elapsed = 1000.0 / tot_elapsed_ms;
					type_diag_averages *p_global_avg = &handle_diag_test.averages;
					p_global_avg->average_num_bytes_per_second_rx = avg.average_num_bytes_per_second_rx * inv_elapsed;
					p_global_avg->average_num_bytes_per_second_tx = avg.average_num_bytes_per_second_tx * inv_elapsed;
					p_global_avg->average_num_msg_per_second_rx = avg.average_num_msg_per_second_rx * inv_elapsed;
					p_global_avg->average_num_msg_per_second_tx = avg.average_num_msg_per_second_tx * inv_elapsed;
					memset(p_run_stats, 0, sizeof(*p_run_stats));
					handle_diag_test.run.batch_start_time = get_current_epoch_time_ms();;

				pthread_mutex_unlock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
			}
			break;
		}

		case enum_diag_test_status_ends:
		{
			handle_diag_test.run.is_running = 0;
			handle_diag_test.status = enum_diag_test_status_idle;
			break;
		}
	}

	return is_OK;
}



enum_admin_diag_test_start_retcode start_CC2650_diag_test(type_admin_diag_test_req_start_body *p_req)
{
	enum_admin_diag_test_start_retcode retcode = enum_admin_diag_test_start_retcode_OK;
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (handle_diag_test.run.is_running)
		{
			handle_diag_test.idle_req++;
			type_my_timeout my_timeout;
			initialize_my_timeout(&my_timeout);
			while(handle_diag_test.run.is_running)
			{
				usleep(1000);
				if (is_my_timeout_elapsed_ms(&my_timeout, 100))
				{
					break;
				}
			}
			if (handle_diag_test.run.is_running)
			{
				retcode = enum_admin_diag_test_start_retcode_unable_to_reset_diag_thread;
			}
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (!handle_app.initDone)
		{
			retcode = enum_admin_diag_test_start_retcode_radio_stack_not_running;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->average_type.enum_type >= enum_admin_diag_test_average_type_numof)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_average_type;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
	#define def_max_batch_acquire_period_ms 10000
		if (p_req->batch_acquire_period_ms > def_max_batch_acquire_period_ms)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_batch_acquire_period_ms;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->custom_message_length_type >= def_max_diag_msg_length)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_custom_message_length_type;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->length_type.enum_type >= enum_admin_diag_test_message_length_type_numof)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_length_type;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->message_body_type.enum_type >= enum_admin_diag_test_message_body_type_numof)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_message_body_type;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->num_batch_samples_for_average >= def_max_diag_batches)
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_num_batch_samples_for_average;
		}
	}
	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		if (p_req->server_IEEE_address == 0)
		{
			handle_diag_test.server_network_short_address = 0;
		}
		else if (!is_OK_get_network_short_address_from_IEEE(p_req->server_IEEE_address, &handle_diag_test.server_network_short_address))
		{
			retcode = enum_admin_diag_test_start_retcode_invalid_server_IEEE_address;
		}
	}

	if (retcode == enum_admin_diag_test_start_retcode_OK)
	{
		// copy the start request
		memcpy(&handle_diag_test.req_start, p_req, sizeof(handle_diag_test.req_start));

		handle_diag_test.start_req++;

	#ifdef def_simulate_diag_test
		memset(&private_default_body, 0, sizeof(private_default_body));
		start_time_ms = get_current_epoch_time_ms();
		prev_time_ms = start_time_ms;
		running = 1;
	#endif
	}
	return retcode;
}

typedef struct _type_start_diag_retcode_msg_table
{
	enum_admin_diag_test_start_retcode r;
	const char *msg;
}type_start_diag_retcode_msg_table;
const type_start_diag_retcode_msg_table start_diag_retcode_msg_table[]=
{
	{.r = enum_admin_diag_test_start_retcode_OK, 									.msg = "OK"										},
	{.r = enum_admin_diag_test_start_retcode_radio_stack_not_running , 				.msg = "radio_stack_not_running"				},
	{.r = enum_admin_diag_test_start_retcode_invalid_batch_acquire_period_ms, 		.msg = "invalid_batch_acquire_period_ms"		},
	{.r = enum_admin_diag_test_start_retcode_invalid_average_type, 					.msg = "invalid_average_type"					},
	{.r = enum_admin_diag_test_start_retcode_invalid_custom_message_length_type,	.msg = "invalid_custom_message_length_type"		},
	{.r = enum_admin_diag_test_start_retcode_invalid_length_type, 					.msg = "invalid_length_type"					},
	{.r = enum_admin_diag_test_start_retcode_invalid_message_body_type, 			.msg = "invalid_message_body_type"				},
	{.r = enum_admin_diag_test_start_retcode_invalid_num_batch_samples_for_average, .msg = "invalid_num_batch_samples_for_average"	},
	{.r = enum_admin_diag_test_start_retcode_invalid_server_IEEE_address, 			.msg = "invalid_server_IEEE_address"			},
	{.r = enum_admin_diag_test_start_retcode_unable_to_reset_diag_thread, 			.msg = "unable_to_reset_diagnostic_thread"		},
};

void get_CC2650_start_diag_test_retcode_string(enum_admin_diag_test_start_retcode r, char *s, int size_s)
{
	unsigned int found = 0;
	unsigned int i;
	for (i = 0; !found && i < sizeof(start_diag_retcode_msg_table)/sizeof(start_diag_retcode_msg_table[0]); i++)
	{
		if (start_diag_retcode_msg_table[i].r == r)
		{
			snprintf(s, size_s, "%s", start_diag_retcode_msg_table[i].msg);
			found = 1;
		}
	}
	if (!found)
	{
		snprintf(s, size_s, "Unknown start return code");
	}
}

enum_admin_diag_test_status_req_format get_CC2650_diag_test_status(enum_admin_diag_test_status_req_format f, char* p_dst)
{
	switch(f)
	{
		case enum_admin_diag_test_status_req_format_default:
		default:
		{
			f = enum_admin_diag_test_status_req_format_default;
			type_admin_diag_test_status_standard_format_body *p_body = (type_admin_diag_test_status_standard_format_body *)p_dst;
			type_handle_diag_test_stats my_stats;
			type_diag_averages averages;
			pthread_mutex_lock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
				my_stats = handle_diag_test.stats;
				averages = handle_diag_test.averages;
			pthread_mutex_unlock(&handle_diag_test.mtx_id[enum_diag_mutex_stats]);
			p_body->average_num_bytes_per_second_rx = averages.average_num_bytes_per_second_rx;
			p_body->average_num_bytes_per_second_tx = averages.average_num_bytes_per_second_tx;
			p_body->average_num_msg_per_second_rx 	= averages.average_num_msg_per_second_rx;
			p_body->average_num_msg_per_second_tx 	= averages.average_num_msg_per_second_tx;
			p_body->nbytes_rx_OK 	= my_stats.num_bytes_rx_OK;
			p_body->nbytes_tx_OK 	= my_stats.num_bytes_tx_OK;
			p_body->nmsg_rx_ERR 	= my_stats.num_msg_rx_ERR;
			p_body->nmsg_rx_OK 		= my_stats.num_msg_rx_OK;
			p_body->nmsg_tx_ERR 	= my_stats.num_msg_tx_ERR;
			p_body->nmsg_tx_OK 		= my_stats.num_msg_tx_OK;
#ifdef def_simulate_diag_test
#define def_msg_size 80
			type_admin_diag_test_status_standard_format_body *p_body = (type_admin_diag_test_status_standard_format_body *)p_dst;
			if (running)
			{
				int64_t now_ms = get_current_epoch_time_ms();
				int elapsed_ms = now_ms - prev_time_ms;
				if (elapsed_ms <= 0)
				{
					elapsed_ms = 10;
				}
				int nmsg_rx_OK = flow_rx_OK_per_second * elapsed_ms * 0.001 + random_int_min_max(-delta_rx_OK_per_second, delta_rx_OK_per_second);
				if (nmsg_rx_OK < 0)
				{
					nmsg_rx_OK = 0;
				}
				private_default_body.nmsg_rx_OK += nmsg_rx_OK;
				int nbytes_rx_OK = nmsg_rx_OK * def_msg_size;
				private_default_body.nbytes_rx_OK += nbytes_rx_OK;
				int nmsg_rx_ERR =  (random_int_min_max(0, 1000) <= flow_rx_ERR_per_thousand) ? 1 : 0;
				private_default_body.nmsg_rx_ERR += nmsg_rx_ERR;
				int nmsg_tx_OK = flow_tx_OK_per_second * elapsed_ms * 0.001 + random_int_min_max(-delta_tx_OK_per_second, delta_tx_OK_per_second);
				if (nmsg_tx_OK < 0)
				{
					nmsg_tx_OK = 0;
				}
				private_default_body.nmsg_tx_OK += nmsg_tx_OK;
				int nbytes_tx_OK = nmsg_tx_OK * def_msg_size;
				private_default_body.nbytes_tx_OK += nmsg_tx_OK * def_msg_size;
				int nmsg_tx_ERR = (random_int_min_max(0, 1000) <= flow_tx_ERR_per_thousand) ? 1 : 0;
				private_default_body.nmsg_tx_ERR += nmsg_tx_ERR;

				private_default_body.average_num_msg_per_second_rx= nmsg_rx_OK * 1000 / elapsed_ms;
				private_default_body.average_num_msg_per_second_tx= nmsg_tx_OK * 1000 / elapsed_ms;
				private_default_body.average_num_bytes_per_second_rx = nbytes_rx_OK* 1000 / elapsed_ms;
				private_default_body.average_num_bytes_per_second_tx = nbytes_tx_OK * 1000 / elapsed_ms;
			}
			*p_body = private_default_body;
#endif
			break;
		}
	}
	return f;
}
enum_admin_diag_test_stop_retcode stop_CC2650_diag_test(void)
{
	enum_admin_diag_test_stop_retcode r = enum_admin_diag_test_stop_retcode_OK;
	handle_diag_test.status = enum_diag_test_status_stop;
#ifdef def_simulate_diag_test
	running = 0;
#endif
	return r;
}


typedef struct _type_stop_diag_retcode_msg_table
{
	enum_admin_diag_test_stop_retcode r;
	const char *msg;
}type_stop_diag_retcode_msg_table;
const type_stop_diag_retcode_msg_table stop_diag_retcode_msg_table[]=
{
	{.r = enum_admin_diag_test_stop_retcode_OK, 									.msg = "OK"										},
};

void get_CC2650_stop_diag_test_retcode_string(enum_admin_diag_test_stop_retcode r, char *s, int size_s)
{
	unsigned int found = 0;
	unsigned int i;
	for (i = 0; !found && i < sizeof(stop_diag_retcode_msg_table)/sizeof(stop_diag_retcode_msg_table[0]); i++)
	{
		if (stop_diag_retcode_msg_table[i].r == r)
		{
			snprintf(s, size_s, "%s", stop_diag_retcode_msg_table[i].msg);
			found = 1;
		}
	}
	if (!found)
	{
		snprintf(s, size_s, "Unknown stop return code");
	}

}

void init_diag_test(void)
{
	enum_diag_mutex e;
	for (e =(enum_diag_mutex)0; e < enum_diag_mutex_numof; e++)
	{
		pthread_mutex_init(&handle_diag_test.mtx_id[e], NULL);
		{
			pthread_mutexattr_t mutexattr;

			pthread_mutexattr_init(&mutexattr);
			pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&handle_diag_test.mtx_id[e], &mutexattr);
		}
	}
}

