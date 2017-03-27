/**************************************************************************************************
f * Filename:       dataSendRcv.c
 * Description:    This file contains dataSendRcv application.
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

/*********************************************************************
 * INCLUDES
 */

#include <ASACZ_ZAP.h>
#include "ASACZ_ZAP_AF_callbacks.h"
#include "ASACZ_ZAP_AF_register.h"
#include "ASACZ_ZAP_end_points_update_list.h"
#include "ASACZ_ZAP_IEEE_address.h"
#include "ASACZ_ZAP_network.h"
#include "ASACZ_ZAP_NV.h"
#include "ASACZ_ZAP_Sapi_callbacks.h"
#include "ASACZ_ZAP_Sys_callbacks.h"
#include "ASACZ_ZAP_TX_power.h"
#include "ASACZ_ZAP_Zdo_callbacks.h"
#include "ASACZ_diag_test.h"
#include <CC2650_fw_update.h>
/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPES
 */

/*********************************************************************
 * LOCAL VARIABLE
 */

static char * get_link_quality_string(enum_link_quality_level level)
{
	const char *pc = "unknown";
	switch(level)
	{
		case enum_link_quality_level_unknown:
		default:
		{
			break;
		}
		case enum_link_quality_level_low:
		{
			pc = "low";
			break;
		}
		case enum_link_quality_level_normal:
		{
			pc = "normal";
			break;
		}
		case enum_link_quality_level_good:
		{
			pc = "good";
			break;
		}
		case enum_link_quality_level_excellent:
		{
			pc = "excellent";
			break;
		}
	}
	return (char*)pc;
}

void init_link_quality(type_link_quality *p)
{
	memset(p, 0, sizeof(*p));
	pthread_mutex_init(&p->mtx_id, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&p->mtx_id, &mutexattr);
	}
}

char * get_app_current_link_quality_string(void)
{
	return get_link_quality_string(handle_app.link_quality.b.level);
}
uint8_t get_app_current_link_quality_value_energy_detected(void)
{
	return handle_app.link_quality.b.v;
}
int32_t get_app_current_link_quality_value_dBm(void)
{
	return handle_app.link_quality.b.v_dBm;
}
void get_app_last_link_quality(type_link_quality_body *p_dst)
{
	type_link_quality *p = &handle_app.link_quality;
	pthread_mutex_lock(&p->mtx_id);
		*p_dst = p->b;
	pthread_mutex_unlock(&p->mtx_id);
}
type_handle_app handle_app;



void force_zigbee_shutdown(void)
{
	handle_app.force_shutdown = 1;
}

static unsigned int is_required_shutdown(void)
{
	return handle_app.force_shutdown;
}

static void leave_network(void)
{
	MgmtLeaveReqFormat_t leave_req;
	memset(&leave_req, 0, sizeof(leave_req));
	// send the message to the coordinator
	leave_req.DstAddr = 0;
	// set my own IEEE address as device address to remove myself from the network
	memcpy(&leave_req.DeviceAddr[0], &handle_app.IEEE_address.address, sizeof(leave_req.DeviceAddr));
	// do not remove children etc
	leave_req.RemoveChildre_Rejoin = 0;
	// signal that I want to leave the network
	handle_app.leave.num_req++;
	// send the request
	uint8_t retcode = zdoMgmtLeaveReq(&leave_req);
	switch(retcode)
	{
		case MT_RPC_SUCCESS:
		{
			dbg_print(PRINT_LEVEL_INFO,"leave request sent OK!");
			break;
		}
		default:
		{
			dbg_print(PRINT_LEVEL_ERROR,"unable to leave the network!");
			break;
		}
	}
	// wait for the call back reply
#define def_total_timeout_wait_leave_callback_ms 8000
#define def_base_timeout_wait_leave_callback_ms 20
#define def_num_loop_wait_leave_callback_ms (def_total_timeout_wait_leave_callback_ms / def_base_timeout_wait_leave_callback_ms)
	int max_loops = 1 + def_num_loop_wait_leave_callback_ms;
	unsigned int continue_loop = 1;
	while(continue_loop )
	{
		if (handle_app.leave.num_req == handle_app.leave.num_ack)
		{
			continue_loop = 0;
			if (handle_app.leave.isOK)
			{
				dbg_print(PRINT_LEVEL_INFO,"%s: OK received from the leave callback", __func__);
				break;
			}
			else
			{
				dbg_print(PRINT_LEVEL_ERROR,"%s: error from the leave callback", __func__);
			}
		}
		else if (max_loops > 0)
		{
			max_loops--;
			usleep(def_base_timeout_wait_leave_callback_ms * 1000);
		}
		else
		{
			continue_loop = 0;
			dbg_print(PRINT_LEVEL_ERROR,"%s: timeout waiting for the leave callback", __func__);
		}
	}
}


unsigned int is_required_CC2650_firmware_update(void)
{
	unsigned int is_required = 0;
	type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
	if (p_CC2650fwupd->num_req != p_CC2650fwupd->num_ack)
	{
		pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
			if (p_CC2650fwupd->num_req != p_CC2650fwupd->num_ack && p_CC2650fwupd->key_a55a0336 == 0xa55a0336)
			{
				p_CC2650fwupd->key_a55a0336 = 0;
				p_CC2650fwupd->num_ack = p_CC2650fwupd->num_req;
				is_required = 1;
			}
		pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
	}
	return is_required;
}

enum_do_CC2650_fw_update_retcode sem_CC2650_fw_operation(enum_CC2650_fw_operation op, const char *path_binary_file, type_ASACZ_CC2650_fw_update_header *p_dst_header)
{
	pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_update);
		char complete_path[1096];
		int needed = snprintf((char*)complete_path, sizeof(complete_path), "%s%s", default_CC2650_firmware_file_directory, path_binary_file);
		enum_do_CC2650_fw_update_retcode r = enum_do_CC2650_fw_update_retcode_ERR_unable_to_open_the_firmware_file;
		if (needed <= (int) sizeof(complete_path))
		{
			r = do_CC2650_fw_operation(op, complete_path, p_dst_header);
		}
	pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_update);
	return r;
}


void CC2650_query_firmware_file(char* CC2650_fw_query_filename , type_fwupd_CC2650_query_firmware_file_reply *p_reply_body)
{
	if (p_reply_body)
	{
		memset(p_reply_body, 0, sizeof(*p_reply_body));
	}
	type_ASACZ_CC2650_fw_update_header hw_header;
	// here the magic is done!
	enum_do_CC2650_fw_update_retcode fw_upd_return_code = sem_CC2650_fw_operation(enum_CC2650_fw_operation_get_file_header_info, CC2650_fw_query_filename, &hw_header);
	if (p_reply_body)
	{
		p_reply_body->retcode= fw_upd_return_code;
		snprintf((char*)p_reply_body->query_result_string, sizeof(p_reply_body->query_result_string), "%s", get_msg_from_CC2650_fw_update_retcode(fw_upd_return_code));
		if (fw_upd_return_code == enum_do_CC2650_fw_update_retcode_OK)
		{
			snprintf((char*)p_reply_body->CC2650_fw_query_filename	, sizeof(p_reply_body->CC2650_fw_query_filename)	, "%s", CC2650_fw_query_filename);
			snprintf((char*)p_reply_body->ascii_fw_type			, sizeof(p_reply_body->ascii_fw_type)				, "%s", hw_header.ascii_fw_type);
			snprintf((char*)p_reply_body->ascii_version_number		, sizeof(p_reply_body->ascii_version_number)		, "%s", hw_header.ascii_version_number);
			snprintf((char*)p_reply_body->date						, sizeof(p_reply_body->date)						, "%s", hw_header.date);
			snprintf((char*)p_reply_body->magic_name				, sizeof(p_reply_body->magic_name)					, "%s", hw_header.magic_name);
			p_reply_body->header_CRC32_CC2650			= hw_header.header_CRC32_CC2650;
			p_reply_body->firmware_body_CRC32_CC2650 	= hw_header.firmware_body_CRC32_CC2650;
			p_reply_body->firmware_body_size			= hw_header.firmware_body_size;
			p_reply_body->fw_type						= hw_header.fw_type;
			p_reply_body->fw_version_major				= hw_header.fw_version_major;
			p_reply_body->fw_version_middle				= hw_header.fw_version_middle;
			p_reply_body->fw_version_minor				= hw_header.fw_version_minor;
		}
	}
}

void get_CC2650_firmware_update_status(type_fwupd_CC2650_query_update_status_reply_body *p)
{
	type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
	pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
		p->ends_ERR = p_CC2650fwupd->ends_ERR;
		p->ends_OK = p_CC2650fwupd->ends_OK;
		p->num_ack = p_CC2650fwupd->num_ack;
		p->num_request = p_CC2650fwupd->num_req;
		p->flash_write_percentage = get_CC2650_fw_update_progress();
		p->fw_update_result_code = p_CC2650fwupd->CC2650_fw_update_retcode;
		p->fw_update_result_code_is_valid = p_CC2650fwupd->CC2650_fw_update_retcode_is_valid;
		snprintf((char*)p->fw_update_result_string, sizeof(p->fw_update_result_string), "%s", p_CC2650fwupd->result);
		p->status = p_CC2650fwupd->status;
	pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
}

enum_request_CC2650_firmware_update_retcode request_CC2650_firmware_update(char *firmware_file_path, type_fwupd_CC2650_start_update_reply_body *p_dst)
{
	enum_request_CC2650_firmware_update_retcode r = enum_request_CC2650_firmware_update_retcode_OK;
	type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
	pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
		int needed = snprintf((char*)p_CC2650fwupd->CC2650_fw_signed_filename, sizeof(p_CC2650fwupd->CC2650_fw_signed_filename), "%s", firmware_file_path);
		if (p_CC2650fwupd->is_running)
		{
			snprintf((char*)p_dst->result_message, sizeof(p_dst->result_message), "ERR already running");
			r = enum_request_CC2650_firmware_update_retcode_ERR_already_running;
		}
		else if (needed >= (int)sizeof(p_CC2650fwupd->CC2650_fw_signed_filename))
		{
			snprintf((char*)p_dst->result_message, sizeof(p_dst->result_message), "ERR too long filename");
			r = enum_request_CC2650_firmware_update_retcode_ERR_too_long_filename;
		}
		else
		{
			p_CC2650fwupd->key_a55a0336 = 0xa55a0336;
			p_CC2650fwupd->num_req++;
			if (p_dst)
			{
				snprintf((char*)p_dst->result_message, sizeof(p_dst->result_message), "OK");
				p_dst->num_request = p_CC2650fwupd->num_req;
			}
		}
	pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
	if (p_dst)
	{
		p_dst->result_code = r;
		p_dst->is_OK = (r == enum_request_CC2650_firmware_update_retcode_OK);
	}

	return r;
}



void init_handle_app(void)
{
	memset(&handle_app, 0, sizeof(handle_app));
	handle_app.devState = DEV_HOLD;
	pthread_mutex_init(&handle_app.mtx_id, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_app.mtx_id, &mutexattr);
	}

	pthread_mutex_init(&handle_app.CC2650_fw_update_handle.mtx_id, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_app.CC2650_fw_update_handle.mtx_id, &mutexattr);
	}
	pthread_mutex_init(&handle_app.CC2650_fw_update_handle.mtx_update, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_app.CC2650_fw_update_handle.mtx_update, &mutexattr);
	}

	init_link_quality(&handle_app.link_quality);
	init_diag_test();
}
unsigned int get_app_new_trans_id(uint32_t message_id)
{
	pthread_mutex_lock(&handle_app.mtx_id);
		uint8_t trans_id = handle_app.transId++;
		// cleanup all of the similar identifiers in the id queue, set the new id with the trans_id passed
		message_history_tx_set_trans_id(message_id, trans_id);
	pthread_mutex_unlock(&handle_app.mtx_id);
	return trans_id;
}
void start_handle_app(void)
{
	handle_app.status = enum_app_status_idle;
	handle_app.start_request++;
}
unsigned int is_in_error_app(void)
{
	return (handle_app.status == enum_app_status_error) ? 1: 0;
}

enum_app_status get_app_status(void)
{
	return handle_app.status;
}

void restart_handle_app(void)
{
	my_log(LOG_WARNING, "Restart handle app requested");
	start_handle_app();
}

static void handle_app_error(enum_app_error_code error_code)
{
	handle_app.error_code = error_code;
	handle_app.status = enum_app_status_error;
}

/***********************************************************************/

/*********************************************************************
 * LOCAL FUNCTIONS
 */



/*********************************************************************
 * CALLBACK FUNCTIONS
 */


/********************************************************************
 * START OF SYS CALL BACK FUNCTIONS
 */



/********************************************************************
 * START OF ZDO CALL BACK FUNCTIONS
 */


static void displayDevices(void)
{
	ActiveEpReqFormat_t actReq;
	int32_t status;

	MgmtLqiReqFormat_t req;

	req.DstAddr = 0;
	req.StartIndex = 0;
	handle_app.handle_node_list.nodeCount = 0;
	zdoMgmtLqiReq(&req);
	do
	{
		status = rpcWaitMqClientMsg(1000);
	} while (status != -1);

	dbg_print(PRINT_LEVEL_INFO, "Available devices:");
	uint8_t i;
	for (i = 0; i < handle_app.handle_node_list.nodeCount; i++)
	{
		Node_t * pnode_t = &handle_app.handle_node_list.nodeList[i];
		char *devtype =
		        (pnode_t->Type == DEVICETYPE_ROUTER ? (char*)"ROUTER" : (char*)"COORDINATOR");
		// asks info about the node, if coordinator, so we can add it to the device list
		//if (nodeList[i].Type == DEVICETYPE_COORDINATOR)
		{
			IeeeAddrReqFormat_t IEEEreq;
			IEEEreq.ShortAddr = pnode_t->NodeAddr;
			IEEEreq.ReqType = 0; // single device response
			IEEEreq.StartIndex = 0; // start from first
			zdoIeeeAddrReq(&IEEEreq);
			rpcWaitMqClientMsg(200);
		}

		dbg_print(PRINT_LEVEL_INFO, "Type: %s", devtype);
		actReq.DstAddr = pnode_t->NodeAddr;
		actReq.NwkAddrOfInterest = pnode_t->NodeAddr;
		zdoActiveEpReq(&actReq);
//		rpcGetMqClientMsg();
		rpcWaitMqClientMsg(15000); 
		do
		{
			status = rpcWaitMqClientMsg(1000);
		} while (status != -1);
		uint8_t cI;
		for (cI = 0; cI < pnode_t->ChildCount; cI++)
		{
			uint8_t type = pnode_t->childs[cI].Type;
			if (type == DEVICETYPE_ENDDEVICE)
			{
				dbg_print(PRINT_LEVEL_INFO, "Type: END DEVICE");

				// asks info about the node, so we can add it to the device list
				{
					IeeeAddrReqFormat_t IEEEreq;
					IEEEreq.ShortAddr = pnode_t->childs[cI].ChildAddr;
					IEEEreq.ReqType = 0; // single device response
					IEEEreq.StartIndex = 0; // start from first
					zdoIeeeAddrReq(&IEEEreq);
					rpcWaitMqClientMsg(200);
					while (status != -1)
					{
						status = rpcWaitMqClientMsg(200);
					}
				}

				actReq.DstAddr = pnode_t->childs[cI].ChildAddr;
				actReq.NwkAddrOfInterest = pnode_t->childs[cI].ChildAddr;
				zdoActiveEpReq(&actReq);
				status = 0;
				//rpcGetMqClientMsg();
				rpcWaitMqClientMsg(15000); 
				while (status != -1)
				{
					status = rpcWaitMqClientMsg(1000);
				}
			}

		}

	}
}



static void flush_rpc_queue(void)
{
	my_log(LOG_INFO, "Flushing rpc queue");

#define def_purge_queue_timeout_ms 5000
	type_my_timeout my_timeout;
	initialize_my_timeout(&my_timeout);

	uint32_t num_msg_flushed = 0;
	unsigned int too_long_waiting_for_flush = 0;
	unsigned int queue_flushed = 0;
	while (!queue_flushed && !too_long_waiting_for_flush)
	{
		int32_t status = rpcWaitMqClientMsg(10);
		if (status == -1)
		{
			queue_flushed = 1;
		}
		else
		{
			num_msg_flushed++;
			if (is_my_timeout_elapsed_ms(&my_timeout, def_purge_queue_timeout_ms))
			{
				too_long_waiting_for_flush = 1;
			}
		}
	}
	if (!too_long_waiting_for_flush)
	{
		my_log(LOG_ERR, "Timeout waiting for rpc queue to flush [max %i ms]", def_purge_queue_timeout_ms);
	}
	else if (!queue_flushed)
	{
		my_log(LOG_ERR, "Error waiting for rpc queue to flush");
	}
	else
	{
		my_log(LOG_INFO, "Rpc queue flushed OK, %i messages flushed", num_msg_flushed);
	}
}

/*********************************************************************
 * INTERFACE FUNCTIONS
 */
uint32_t appInit(void)
{

	//Flush all messages from the queue
	flush_rpc_queue();

	//Register Callbacks MT system callbacks
	ASACZ_ZAP_Sys_callback_register();
	ASACZ_ZAP_Zdo_callback_register();
	ASACZ_ZAP_AF_callback_register();
	ASACZ_ZAP_Sapi_callback_register();

	init_handle_app();
	// initialize the handle app stati machine
	start_handle_app();

	return 0;
}

static void check_send_message(void)
{
	DataRequestFormat_t *pdr = &handle_app.DataRequest;
	type_ASAC_ZigBee_interface_command_outside_send_message_req m;
	if (is_OK_pop_Tx_outside_message(&m, &handle_app.message_id))
	{
		if (m.message_length > sizeof(pdr->Data))
		{
			// mark the data request has been sent OK
			message_history_tx_set_error(handle_app.message_id, enum_message_history_error_too_big_outside_message_on_pop);
			my_log(LOG_ERR, "Too big message with id = %u, length %u, available %u", handle_app.message_id, m.message_length, (unsigned int)sizeof(pdr->Data));
		}
		else
		{
			uint16_t DstAddr = 0;
			if (!is_OK_get_network_short_address_from_IEEE(m.dst_id.IEEE_destination_address, &DstAddr))
			{
				dbg_print(PRINT_LEVEL_INFO, "%s: ERROR UNKNOWN DST @ IEEE Address 0x%"PRIx64, __func__, m.dst_id.IEEE_destination_address);
				message_history_tx_set_error(handle_app.message_id, enum_message_history_error_unknown_IEEE_address);
				my_log(LOG_ERR, "Unable to find device with IEEE address: %" PRIx64 "", m.dst_id.IEEE_destination_address);
			}
			else
			{
				dbg_print(PRINT_LEVEL_INFO, "%s: sending DST @ IEEE Address 0x%" PRIx64 "", __func__, m.dst_id.IEEE_destination_address);
				dbg_print(PRINT_LEVEL_INFO, "%s: msg length: %u", __func__, (unsigned int)m.message_length);
				{
					char the_bytes[256];
					memset(&the_bytes[0], 0, sizeof(the_bytes));
					int residual_bytes = 255;
					int nbytes = m.message_length;
					if (nbytes > 20)
					{
						nbytes = 20;
					}
					char *pc = the_bytes;
					int ns = snprintf(pc, residual_bytes, "%s msg bytes: ", __func__);
					if (ns > 0)
					{
						residual_bytes -= ns;
						pc += ns;
						int idx_byte;
						idx_byte = 0;
						while(residual_bytes > 0 && idx_byte < nbytes)
						{
							ns = snprintf(pc, residual_bytes, "%02X ", (unsigned int)m.message[idx_byte]);
							idx_byte ++;
							if (ns < 0)
							{
								break;
							}
							residual_bytes -= ns;
							pc += ns;
						}
						dbg_print(PRINT_LEVEL_INFO, the_bytes);
					}
				}
				// store the message length
				pdr->Len         = m.message_length;
				// store the message body
				memcpy(pdr->Data, m.message, m.message_length);
				// set the destination address
				pdr->DstAddr     = DstAddr;
				pdr->DstEndpoint = m.dst_id.destination_endpoint;
				pdr->ClusterID   = m.dst_id.cluster_id;
				pdr->SrcEndpoint = m.dst_id.source_endpoint;
				// retrieve a new transaction ID
				pdr->TransID     = get_app_new_trans_id(handle_app.message_id);
				// send the message
				int32_t status = afDataRequest(pdr);
				// check the return code
				if (status != MT_RPC_SUCCESS)
				{
					dbg_print(PRINT_LEVEL_INFO, "%s: ERROR sending DST @ IEEE Address 0x%" PRIx64 "", __func__, m.dst_id.IEEE_destination_address);
					// mark the data request has been sent OK
					message_history_tx_set_id_status(handle_app.message_id, enum_message_history_status_datareq_sent);
					my_log(LOG_ERR, "Unable to send message with id = %u", handle_app.message_id);
				}
				else
				{
					dbg_print(PRINT_LEVEL_INFO, "%s: sent OK @ IEEE Address 0x%" PRIx64 "", __func__, m.dst_id.IEEE_destination_address);
					my_log(LOG_INFO, "Message with id = %u sent OK", handle_app.message_id);
				}
			}
		}
	}
	else
	{
		// the queue is empty, no messages to pop
	}
}


// this function just receives the messages from the CC chip when the ZigBee network is initialized
void* appMsgProcess(void *argument)
{

	if (handle_app.initDone )
	{
		rpcWaitMqClientMsg(10);
	}
	else
	{
		usleep(10000);
	}
	return NULL;
}

//#define test_firmware_update
#ifdef test_firmware_update
#warning test_firmware_update !!!!!!!!!!!!!!!
int fw_test_done = 0;
#endif	

// processes the statuses of the ZigBee network handling
void* appProcess(void *argument)
{
	// give some breath to the system
	usleep(2000);

	switch(handle_app.status)
	{
		default:
		{
			handle_app.num_unknown_status++;
			handle_app.status = enum_app_status_idle;
			break;
		}
		// if in error, only a restart can change status
		case enum_app_status_error:
		{
#ifdef test_firmware_update
	if (!fw_test_done)
	{
			my_log(LOG_INFO, "test_firmware_update: CC2650 FIRMWARE UPDATE TEST STARTS!");
		fw_test_done = 1;
		handle_app.status = enum_app_status_firmware_update_init;
		break;
	}
#endif
			if (is_required_CC2650_firmware_update())
			{
				handle_app.status = enum_app_status_firmware_update_init;
				break;
			}
			if (ZAP_is_required_network_restart_from_scratch())
			{
				handle_app.status = enum_app_status_restart_network_from_scratch;
				break;
			}
			break;
		}
		case enum_app_status_idle:
		{
			if (handle_app.start_ack != handle_app.start_request)
			{
				handle_app.start_ack = handle_app.start_request;
				handle_app.status = enum_app_status_init;
			}
			break;
		}
		case enum_app_status_init:
		{
			start_queue_message_Tx();
			start_queue_message_Rx();
			handle_app.devState = DEV_HOLD;
			handle_app.status = enum_app_status_flush_messages_init;
			// reset my own IEEE address
			invalidate_IEEE_address(&handle_app.IEEE_address);
			// send an IEEE address request
			sysGetExtAddr();
			// request the CC2650 firmware version without waiting
			sysVersion(0);

			break;
		}
		case enum_app_status_flush_messages_init:
		{
			my_log(LOG_INFO, "Network Initialization");
			handle_app.num_msgs_flushed = 0;
			handle_app.status = enum_app_status_flush_messages_do;
			break;
		}
		case enum_app_status_flush_messages_do:
		{
#define def_max_messages_flushed 25
			int32_t status = rpcWaitMqClientMsg(200);
			if (status == -1)
			{
				handle_app.status = enum_app_status_start_network;
				if (is_valid_IEEE_address(&handle_app.IEEE_address))
				{
					dbg_print(PRINT_LEVEL_INFO, "My IEEE Address is: 0x%" PRIx64 "", handle_app.IEEE_address.address);
				}
			}
			else if (++handle_app.num_msgs_flushed >= def_max_messages_flushed)
			{
				handle_app_error(enum_app_error_code_too_many_messages_flushed);
			}
			break;
		}
		case enum_app_status_restart_network_from_scratch:
		{
			handle_app.devState = DEV_HOLD;

			int32_t status = ZAP_startNetwork(1, enum_start_network_type_from_scratch);
			if (status != -1)
			{
				my_log(LOG_INFO, "Network up");
				handle_app.status = enum_app_status_get_IEEE_address;
			}
			else
			{
				my_log(LOG_ERR, "Unable to restart Network");
				handle_app_error(enum_app_error_code_unable_to_start_network);
			}
			break;
		}
		case enum_app_status_start_network:
		{
			handle_app.devState = DEV_HOLD;
// starts always from scratch at powerup			
#ifdef OLINUXINO
			int32_t status = ZAP_startNetwork(0, enum_start_network_type_from_scratch);
#else
			int32_t status = ZAP_startNetwork(0, enum_start_network_type_resume);
#endif
			if (status != -1)
			{
				my_log(LOG_INFO, "Network up");
				handle_app.status = enum_app_status_get_IEEE_address;
			}
			else
			{
				my_log(LOG_ERR, "Unable to start Network");
				handle_app_error(enum_app_error_code_unable_to_start_network);
			}
			break;
		}
		case enum_app_status_get_IEEE_address:
		{
			if (!is_valid_IEEE_address(&handle_app.IEEE_address))
			{
				// reset my own IEEE address
				invalidate_IEEE_address(&handle_app.IEEE_address);
				// send an IEEE address request
				int32_t status = sysGetExtAddr();
				if (status != MT_RPC_SUCCESS)
				{
					my_log(LOG_ERR, "Unable to execute sysGetExtAddr");
					handle_app_error(enum_app_error_code_unable_to_sysGetExtAddr);
				}
				else
				{
					my_log(LOG_INFO, "IEEE address request sent OK");
					handle_app.status = enum_app_status_sysOsalNvWrite;
				}
			}
			else
			{
				my_log(LOG_INFO, "IEEE address already valid");
				handle_app.status = enum_app_status_sysOsalNvWrite;
			}
			break;
		}
		case enum_app_status_sysOsalNvWrite:
		{
			// TODO: what is this?
			OsalNvWriteFormat_t nvWrite;
			nvWrite.Id = ZCD_NV_ZDO_DIRECT_CB;
			nvWrite.Offset = 0;
			nvWrite.Len = 1;
			nvWrite.Value[0] = 1;
			int32_t status = sysOsalNvWrite(&nvWrite);
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_ERR, "Unable to execute sysOsalNvWrite");
				handle_app_error(enum_app_error_code_unable_to_sysOsalNvWrite);
			}
			else
			{
				my_log(LOG_INFO, "sysOsalNvWrite request sent OK");
				handle_app.status = enum_app_status_set_TX_power;
			}
			break;
		}
		case enum_app_status_set_TX_power:
		{
			invalidate_Tx_power(&handle_app.Tx_power);
			int32_t status = set_TX_power();
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_ERR, "Unable to execute set_TX_power");
				handle_app_error(enum_app_error_code_unable_to_set_TX_power);
			}
			else
			{
				handle_app.status = enum_app_status_wait_callbacks_init;
			}
			break;
		}
		case enum_app_status_wait_callbacks_init:
		{
			initialize_my_timeout(&handle_app.my_timeout);
			handle_app.status = enum_app_status_wait_callbacks_do;
			break;
		}
		case enum_app_status_wait_callbacks_do:
		{
			#define def_timeout_wait_callbacks_do_ms 10000
			if (is_valid_IEEE_address(&handle_app.IEEE_address) && is_valid_Tx_power(&handle_app.Tx_power))
			{
				dbg_print(PRINT_LEVEL_INFO, "My IEEE Address is: 0x%" PRIx64 "", handle_app.IEEE_address.address);

				my_log(LOG_INFO, "Callback OK, going to device init");
				handle_app.status = enum_app_status_display_devices_init;
			}
			else if (is_my_timeout_elapsed_ms(&handle_app.my_timeout, def_timeout_wait_callbacks_do_ms))
			{
				my_log(LOG_ERR, "Timeout waiting init callbacks");
				handle_app_error(enum_app_error_code_timeout_waiting_init_callbacks);
			}
			break;
		}
		case enum_app_status_display_devices_init:
		{
			handle_app.handle_node_list.nodeCount = 0;
			handle_app.initDone = 0;
			handle_app.status = enum_app_status_display_devices_do;
			break;
		}
		case enum_app_status_display_devices_do:
		{
			displayDevices();
			handle_app.status = enum_app_status_display_devices_ends;
			break;
		}
		case enum_app_status_display_devices_ends:
		{
			//refresh my end point list
			refresh_my_endpoint_list();
			dbg_print(PRINT_LEVEL_INFO, "My IEEE Address is: 0x%" PRIx64 "", handle_app.IEEE_address.address);
			my_log(LOG_INFO, "Display device OK, going to rx/tx mode");
// wait until a device shows up?
//#warning better to remove this default end point / destination address
			handle_app.DataRequest.DstAddr 		= (uint16_t) 0xfa24;
			handle_app.DataRequest.DstEndpoint 	= get_default_dst_endpoint();
			handle_app.DataRequest.SrcEndpoint 	= get_default_src_endpoint();
			handle_app.DataRequest.ClusterID 	= get_default_clusterID();
			handle_app.DataRequest.TransID 		= 0;
			handle_app.DataRequest.Options 		= 0;
			handle_app.DataRequest.Radius 		= 0xEE;
			handle_app.initDone = 1;
			handle_app.status = enum_app_status_rx_msgs;
			break;
		}
		case enum_app_status_rx_msgs:
		{
#ifdef test_firmware_update
	if (!fw_test_done)
	{
			my_log(LOG_INFO, "test_firmware_update: CC2650 FIRMWARE UPDATE TEST STARTS!");
		fw_test_done = 1;
		handle_app.status = enum_app_status_firmware_update_init;
		break;
	}
#endif			
			handle_app.status = enum_app_status_tx_msgs;
			break;
		}
		case enum_app_status_update_end_points:
		{
			handle_end_points_update_list(&handle_app.handle_end_point_update);
			handle_app.status = enum_app_status_tx_msgs;
			break;
		}
		case enum_app_status_firmware_update_init:
		{
			my_log(LOG_INFO,"%s: firmware update init", __func__);
			type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;

			pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
				p_CC2650fwupd->status = enum_CC2650_fw_update_status_init;
				p_CC2650fwupd->CC2650_fw_update_retcode_is_valid = 0;
				p_CC2650fwupd->CC2650_fw_update_retcode = 0;
				p_CC2650fwupd->ends_ERR = 0;
				p_CC2650fwupd->ends_OK = 0;
				p_CC2650fwupd->is_running = 1;
			pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
			handle_app.num_loop_wait_suspend_rx_thread = 0;
			handle_app.suspend_rx_tasks = ++handle_app.suspend_rx_tasks_idx;
			// sends a ping with no Rx so we can unlock the receiving threads
			sysPing_noRx();
			
			handle_app.status = enum_app_status_firmware_update_init_wait;
			initialize_my_timeout(&handle_app.timeout_init_fw_update);

			break;
		}
		case enum_app_status_firmware_update_init_wait:
		{
			type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
			pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
				p_CC2650fwupd->status = enum_CC2650_fw_update_status_init_wait;
				p_CC2650fwupd->CC2650_fw_update_retcode_is_valid = 0;
				p_CC2650fwupd->CC2650_fw_update_retcode= 0;
			pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
			if (handle_app.InMessageTaskSuspended == handle_app.suspend_rx_tasks && handle_app.rpcTaskSuspended == handle_app.suspend_rx_tasks)
			{
				handle_app.status = enum_app_status_firmware_update_do;
				my_log(LOG_INFO,"%s: receive tasks suspended OK, closing the serial port", __func__);
				// closes the serial port
				rpcTransportClose();
			}
			else if (is_my_timeout_elapsed_ms(&handle_app.timeout_init_fw_update, 1000))
			{
				initialize_my_timeout(&handle_app.timeout_init_fw_update);
				if (++handle_app.num_loop_wait_suspend_rx_thread >= 3)
				{
					handle_app.num_loop_wait_suspend_rx_thread = 0;
					handle_app.status = enum_app_status_firmware_update_ends;
#if 0					
					my_log(LOG_ERR,"%s: timeout waiting firmware, abandon", __func__);
					pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
						p_CC2650fwupd->status = enum_CC2650_fw_update_status_ends;
						p_CC2650fwupd->ends_ERR= 1;
						snprintf((char*)p_CC2650fwupd->result, sizeof(p_CC2650fwupd->result), "TIMEOUT STOPPING RX THREAD");
					pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
					break;
#else
					my_log(LOG_ERR,"%s: timeout waiting firmware update start, trying anyway the firmware update", __func__);
				handle_app.status = enum_app_status_firmware_update_do;
				// closes the serial port
				rpcTransportClose();
#endif
				}
				else
				{
					my_log(LOG_INFO,"%s: trying again to unlock the receiving threads", __func__);
					extern void rpc_no_blocking(void);
					rpc_no_blocking();
				}
			}
			break;
		}
		case enum_app_status_firmware_update_do:
		{
			my_log(LOG_INFO,"%s: firmware update do", __func__);
			handle_app.status = enum_app_status_firmware_update_ends;
			type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
			char fw_file_path[sizeof(p_CC2650fwupd->CC2650_fw_signed_filename)];
			pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
				p_CC2650fwupd->status = enum_CC2650_fw_update_status_do;
			pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
#ifdef test_firmware_update
			int n_needed = snprintf(fw_file_path, sizeof(fw_file_path), "/data/system/ASACZ_CC2650fw_ROUTER.2_6_5");
			my_log(LOG_INFO, "test_firmware_update: using as update the file %s", fw_file_path);
#else
			int n_needed = snprintf(fw_file_path, sizeof(fw_file_path), "%s", p_CC2650fwupd->CC2650_fw_signed_filename);
#endif			
			
			{
				if (n_needed >= (int)sizeof(fw_file_path))
				{
					pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
						p_CC2650fwupd->status = enum_CC2650_fw_update_status_ends;
						p_CC2650fwupd->ends_ERR = 1;
						p_CC2650fwupd->ends_OK = 0;
						snprintf((char*)p_CC2650fwupd->result, sizeof(p_CC2650fwupd->result), "TOO LONG INPUT FILENAME");
					pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
					my_log(LOG_ERR,"%s: too long filename %i bytes needed, %u bytes available: %s", __func__, n_needed, sizeof(fw_file_path) - 1, p_CC2650fwupd->CC2650_fw_signed_filename);
					break;
				}
			}
			//uint32_t fw_upd_return_code = do_CC2650_fw_update("/usr/ASACZ_CC2650fw_COORDINATOR.2_6_5");
			my_log(LOG_INFO,"%s: firmware update starts with filename = %s", __func__, fw_file_path);

			type_ASACZ_CC2650_fw_update_header hw_header;
			uint32_t fw_upd_return_code = enum_do_CC2650_fw_update_retcode_OK;
			unsigned int idx_retry;
			#define def_max_retry_upd_fw_CC2650 3
			for (idx_retry = 0; idx_retry < def_max_retry_upd_fw_CC2650; idx_retry ++)
			{
				// here the magic is done!
				fw_upd_return_code = sem_CC2650_fw_operation(enum_CC2650_fw_operation_update_firmware, fw_file_path, &hw_header);
				if (fw_upd_return_code == enum_do_CC2650_fw_update_retcode_OK)
				{
					break;
				}
				else
				{
					my_log(LOG_INFO,"%s: firmware update ends with error %u (%s)", __func__, fw_upd_return_code, get_msg_from_CC2650_fw_update_retcode(fw_upd_return_code));
				}
			}

			pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
				p_CC2650fwupd->CC2650_fw_update_retcode = fw_upd_return_code;
				p_CC2650fwupd->status = enum_CC2650_fw_update_status_ends;
				const char *p_msg = get_msg_from_CC2650_fw_update_retcode(fw_upd_return_code);
				snprintf((char*)p_CC2650fwupd->result, sizeof(p_CC2650fwupd->result), "%s", p_msg);
				if (fw_upd_return_code == enum_do_CC2650_fw_update_retcode_OK)
				{
					p_CC2650fwupd->num_upd_OK++;
					p_CC2650fwupd->ends_ERR = 0;
					p_CC2650fwupd->ends_OK = 1;
				}
				else
				{
					p_CC2650fwupd->num_upd_ERR++;
					p_CC2650fwupd->ends_OK = 0;
					p_CC2650fwupd->ends_ERR = 1;
				}
				p_CC2650fwupd->CC2650_fw_update_retcode_is_valid = 1;
			pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
			if (fw_upd_return_code == enum_do_CC2650_fw_update_retcode_OK)
			{
				my_log(LOG_INFO,"%s: firmware update finished OK", __func__);
			}
			else
			{
				my_log(LOG_INFO,"%s: firmware update ends with error %u (%s)", __func__, fw_upd_return_code, p_msg);
			}
			break;
		}
		case enum_app_status_firmware_update_ends:
		{
			type_CC2650_fw_update_handle *p_CC2650fwupd = &handle_app.CC2650_fw_update_handle;
			pthread_mutex_lock(&handle_app.CC2650_fw_update_handle.mtx_id);
				p_CC2650fwupd->status = enum_CC2650_fw_update_status_ends;
				p_CC2650fwupd->is_running = 0;
			pthread_mutex_unlock(&handle_app.CC2650_fw_update_handle.mtx_id);
			my_log(LOG_INFO,"%s: reopening the serial port", __func__);
			int fd = rpcOpen(NULL, 0);
			if (fd == -1)
			{
				dbg_print(PRINT_LEVEL_ERROR, "could not reopen serial port\n");
				exit(-1);
			}
			handle_app.suspend_rx_tasks = 0;
			my_log(LOG_INFO,"%s: starting again the network", __func__);
			handle_app.status = enum_app_status_restart_network_from_scratch;

			break;
		}

		case enum_app_status_diag_test:
		{
			if (!is_OK_handle_diag_test())
			{
				handle_app.status = enum_app_status_rx_msgs;
			}
			break;
		}

		case enum_app_status_shutdown:
		{
			break;
		}
		case enum_app_status_tx_msgs:
		{
			if (is_request_start_diag_test() || is_running_diag_test())
			{
				handle_app.status = enum_app_status_diag_test;
				break;
			}
			if (is_required_CC2650_firmware_update())
			{
				handle_app.status = enum_app_status_firmware_update_init;
				break;
			}
			if (is_required_shutdown())
			{
				// leave the network, then shutdown
				leave_network();
				handle_app.status = enum_app_status_shutdown;
				break;
			}
			if (ZAP_is_required_network_restart_from_scratch())
			{
				my_log(LOG_INFO,"%s: restart the network from scratch", __func__);
				// leave the network
				leave_network();
				// restart the network from scratch
				handle_app.status = enum_app_status_restart_network_from_scratch;
				break;
			}
			if (is_needed_end_point_update_list(&handle_app.handle_end_point_update))
			{
				handle_app.status = enum_app_status_update_end_points;
				break;
			}
			handle_app.status = enum_app_status_rx_msgs;
			check_send_message();
			break;
		}
	}

	return NULL;
}


unsigned int is_suspended_rx_task(void)
{
	return handle_app.suspend_rx_tasks;
}

void *rpcTask(void *argument)
{
	while (1)
	{
		if (!handle_app.suspend_rx_tasks)
		{
			handle_app.rpcTaskSuspended = 0;
			if (rpcProcess() < 0)
			{
				usleep(1000);
			}
		}
		else
		{
			handle_app.rpcTaskSuspended = handle_app.suspend_rx_tasks;
		}
		usleep(2000);
	}

	dbg_print(PRINT_LEVEL_WARNING, "rpcTask exited!\n");
}
void *appInMessageTask(void *argument)
{
	while (1)
	{
		usleep(2000);
		if (!handle_app.suspend_rx_tasks)
		{
			handle_app.InMessageTaskSuspended = 0;
			appMsgProcess(NULL);
		}
		else
		{
			handle_app.InMessageTaskSuspended = handle_app.suspend_rx_tasks;
		}
	}
}
