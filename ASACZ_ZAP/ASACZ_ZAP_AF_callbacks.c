/*
 * AF_callbacks.c
 *
 *  Created on: Jan 18, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "ASACZ_diag_test.h"
#include "ASACZ_ZAP_AF_register.h"
#include "ASACZ_ZAP_AF_callbacks.h"


static const char *get_af_error_code_string(uint8_t code)
{
	const char *s = "unknown";
	switch(code)
	{
		case 0x0: 	{ s = "SUCCESS"; 		break;}
		case 0x1: 	{ s = "FAILED"; 		break;}
		case 0x2: 	{ s = "INVALID_PARAM"; 	break;}
		case 0x10: 	{ s = "MEM FAIL"; 		break;}
		case 0xCD: 	{ s = "NO ROUTE"; 		break;}
		default: 	{ 						break;}
	}
	return s;
}



static void set_link_quality_current_value(uint8_t v)
{
#define def_max_dBm 10
#define def_min_dBm -97

	//	High quality: 90% ~= -55db
	//	Medium quality: 50% ~= -75db
	//  Low quality: 30% ~= -85db
	//  Unusable quality: 8% ~= -96db

	int32_t v_dBm = v;
	v_dBm = def_min_dBm + (v_dBm * (def_max_dBm - def_min_dBm)) / 256;

	enum_link_quality_level level = enum_link_quality_level_low;
	if (v_dBm >= -55)
	{
		level = enum_link_quality_level_excellent;
	}
	else if (v_dBm >= -65)
	{
		level = enum_link_quality_level_good;
	}
	else if (v_dBm >= -75)
	{
		level = enum_link_quality_level_normal;
	}
	else
	{
		level = enum_link_quality_level_low;
	}
	// set all of the body properties
	type_link_quality_body lq_body;
	lq_body.level = level;
	lq_body.v = v;
	lq_body.v_dBm = v_dBm;
	lq_body.t = get_current_epoch_time_ms();
	// save the properties
	type_link_quality *p = &handle_app.link_quality;
	pthread_mutex_lock(&p->mtx_id);
		p->b = lq_body;
	pthread_mutex_unlock(&p->mtx_id);
}


static uint8_t mtAfDataConfirmCb(DataConfirmFormat_t *msg)
{
	// was it a callback of a diagnostic message
	if (is_OK_AF_callback_diag_transId(msg->TransId))
	{
	}
	else
	{
		if (msg->Status == MT_RPC_SUCCESS)
		{
			//my_log(LOG_INFO, "Data confirm cb OK: end-point: %u, trans-id: %u", (uint32_t)msg->Endpoint, (uint32_t)msg->TransId);
			message_history_tx_set_trans_id_status(msg->TransId, enum_message_history_status_dataconfirm_received);
		}
		else
		{
			//my_log(LOG_ERR, "Data confirm cb ERR: end-point: %u, trans-id: %u", (uint32_t)msg->Endpoint, (uint32_t)msg->TransId);
			message_history_tx_set_trans_id_error(msg->TransId, enum_message_history_error_DATACONFIRM);
		}
	}
	return msg->Status;
}

static uint8_t mtAfDataDeleteSrspCb(DataDeleteSrspFormat_t *msg)
{
	uint8_t retcode = SUCCESS;
	switch(msg->return_value)
	{
		case SUCCESS:
		{
			dbg_print(PRINT_LEVEL_VERBOSE,"The end point delete has given OK code %u: %s", msg->return_value, get_af_error_code_string(msg->return_value));
			break;
		}
		default:
		{
			retcode = FAILURE;
			dbg_print(PRINT_LEVEL_ERROR,"The end point delete has given error %u: %s", msg->return_value, get_af_error_code_string(msg->return_value));
			break;
		}
	}
	type_handle_end_point_update * p = &handle_app.handle_end_point_update;
	p->call_back_return_code = msg->return_value;
	p->call_back_ack = p->call_back_req;
	return retcode;
}

/********************************************************************
 * AF DATA REQUEST CALL BACK FUNCTION
 */


static uint8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *msg)
{
	uint8_t retcode = SUCCESS;
	if (msg->SrcEndpoint == ASACZ_reserved_endpoint && msg->ClusterId == ASACZ_reserved_endpoint_cluster_diag)
	{
		diag_rx_callback(msg);
	}
	else
	{
		// incoming message received, we put it in the received messages queue
		uint64_t IEEE_address;
		dbg_print(PRINT_LEVEL_INFO, "%s: message from device @ Short Address 0x%X", __func__,(unsigned int)msg->SrcAddr);
		set_link_quality_current_value(msg->LinkQuality);
		dbg_print(PRINT_LEVEL_INFO, "%s: link quality: %s (%u / %i dBm)", __func__
				, get_app_current_link_quality_string()
				, (unsigned int )get_app_current_link_quality_value_energy_detected()
				, (int )get_app_current_link_quality_value_dBm()
				);

		if (!is_OK_get_IEEE_from_network_short_address(msg->SrcAddr, &IEEE_address, enum_device_lifecycle_action_do_refresh_rx))
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: UNKNOWN DEVICE @ Short Address 0x%X", __func__,(unsigned int)msg->SrcAddr);
			my_log(LOG_ERR, "Unknown short address 0x%X on incoming message", (uint32_t)msg->SrcAddr);
			retcode = FAILURE;
		}
		else
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: the device is @ IEEE 0x%"  PRIx64 ", Short Address 0x%X", __func__,IEEE_address, (unsigned int)msg->SrcAddr);
			uint32_t id;
			type_ASAC_ZigBee_interface_command_received_message_callback m;
			memset(&m, 0, sizeof(m));
			{
				type_ASAC_ZigBee_src_id *ph = &m.src_id;

				ph->IEEE_source_address		= IEEE_address;
				ph->destination_endpoint 	= msg->DstEndpoint;
				ph->cluster_id 				= msg->ClusterId;
				ph->source_endpoint 		= msg->SrcEndpoint;
				ph->transaction_id 			= msg->TransSeqNum;
			}
			m.message_length = msg->Len;
			memcpy(m.message, msg->Data, m.message_length);
			if (!is_OK_push_Rx_outside_message(&m, &id))
			{
				dbg_print(PRINT_LEVEL_INFO, "%s: ERROR PUTTING message in outside queue", __func__);
				my_log(LOG_ERR, "Unable to put received message in rx queue");
				retcode = FAILURE;
			}
			else
			{
				dbg_print(PRINT_LEVEL_INFO, "%s: message put OK in outside queue", __func__);
			}
		}
	}
	return retcode;
}



static mtAfCb_t mtAfCb =
	{
		.pfnAfDataConfirm 		= mtAfDataConfirmCb,	// MT_AF_DATA_CONFIRM
		.pfnAfIncomingMsg 		= mtAfIncomingMsgCb,	// MT_AF_INCOMING_MSG
		.pfnAfDataDeleteSrsp 	= mtAfDataDeleteSrspCb,	// callback of the synchronous reply to the data delete request
	};
void ASACZ_ZAP_AF_callback_register(void)
{
	afRegisterCallbacks(mtAfCb);
}
