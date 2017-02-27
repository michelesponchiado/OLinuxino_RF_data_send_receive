/*
 * ASACZ_ZAP_network.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */
#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_network.h"

#include "ASACZ_ZAP_AF_register.h"
#include "ASACZ_ZAP_end_points_update_list.h"
#include "ASACZ_ZAP_NV.h"
#include "ASACZ_conf.h"

//#define def_fix_channel 11
#ifdef def_fix_channel
#warning always using fixed channel
#endif

static void request_firmware_version()
{
#define def_timeout_wait_fw_version_ms 5000
	uint8_t status = sysVersion(def_timeout_wait_fw_version_ms);
	if (status != SUCCESS)
	{
		my_log(LOG_ERR,"%s: error requesting radio chip firmware version", __func__);
	}
	else
	{
		my_log(LOG_INFO,"%s: requesting radio chip firmware version", __func__);
	}
}

void ZAP_require_network_restart_from_scratch(void)
{
	handle_app.network_restart_from_scratch_req++;
}
unsigned int ZAP_is_required_network_restart_from_scratch(void)
{
	unsigned int is_required =0;
	if (handle_app.network_restart_from_scratch_req != handle_app.network_restart_from_scratch_ack)
	{
		my_log(LOG_INFO,"%s: restart from scratch has been requested", __func__);
		handle_app.network_restart_from_scratch_ack = handle_app.network_restart_from_scratch_req;
		is_required = 1;
	}
	return is_required;
}

int32_t ZAP_startNetwork(unsigned int register_user_endpoints, enum_start_network_type start_network_type)
{
#ifndef CC26xx
	char cDevType;
	uint8_t devType;
#endif
	int32_t status;
	uint32_t idxloop;
	uint32_t networkOK = 0;
	{
		uint32_t from_conf_restart_from_scratch = ASACZ_get_conf_param_restart_from_scratch();
		if (from_conf_restart_from_scratch)
		{
			start_network_type = enum_start_network_type_from_scratch;
			ASACZ_reset_conf_param_restart_from_scratch();
		}
	}
	for (idxloop = 0; ! networkOK && idxloop < 3; idxloop++)
	{
		handle_app.devState = DEV_HOLD;
		uint8_t newNwk = 0;
		if (start_network_type == enum_start_network_type_from_scratch)
		{
			newNwk = 1;
#ifdef ANDROID
			{
				extern int radio_asac_barebone_on();
				extern int radio_asac_barebone_off();
				radio_asac_barebone_off();
				usleep(50000);
				radio_asac_barebone_on();
				usleep(200000);
			}
#else
			usleep(50000);
#endif
		}
		if (newNwk)
		{
			my_log(LOG_INFO, "starting a new network");
			status = setNVStartup(ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
		}
		else
		{
			my_log(LOG_INFO, "trying to resume a previous network");
			status = setNVStartup(0);
		}
		if (status != MT_RPC_SUCCESS)
		{
			my_log(LOG_WARNING, "network startup failed");
			continue;
		}
		my_log(LOG_INFO, "Resetting ZNP");
		ResetReqFormat_t resReq;
		resReq.Type = 1;
		sysResetReq(&resReq);
		//flush the rsp
		my_log(LOG_INFO, "Flushing rcp");
		rpcWaitMqClientMsg(2000);

		if (newNwk)
		{
			//Select random PAN ID for Coord and join any PAN for RTR/ED
			uint32_t PAN_id = ASACZ_get_conf_param_PAN_id();
			my_log(LOG_INFO, "Setting the PAN ID 0x%X", PAN_id);
			status = setNVPanID(PAN_id);
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_WARNING, "setNVPanID failed on 0x%X", PAN_id);
				continue;
			}
			my_log(LOG_INFO, "setNVPanID 0x%X: OK!", PAN_id);
	#ifdef def_interactive_chan
			dbg_print(PRINT_LEVEL_INFO, "Enter channel 11-26:\n");
			consoleGetLine(sCh, 128);
			status = setNVChanList(1 << atoi(sCh));
	#else
		#ifdef def_fix_channel
			// this must be between 11 and 26
			my_log(LOG_INFO, "Setting the Channel %i", (int)def_fix_channel);
			status = setNVChanList(1 << def_fix_channel);
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_WARNING, "setNVChanList failed on channel %i", def_fix_channel);
				continue;
			}
		#else
			uint32_t channels_mask = ASACZ_get_conf_param_channel_mask();
			my_log(LOG_INFO, "using as channel mask 0x%X", channels_mask);
			status = setNVChanList(channels_mask);
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_WARNING, "setNVChanList failed on channel mask 0x%X", channels_mask);
				continue;
			}
		#endif
	#endif

		}

		my_log(LOG_INFO, "registering the default end point/command");
		registerAf_default();

		if (register_user_endpoints)
		{
			my_log(LOG_INFO, "registering the user end points");
			register_user_end_points();
		}
		else
		{
			my_log(LOG_INFO, "no user end points register needed");
		}

		my_log(LOG_INFO, "zdo init");
		status = zdoInit();
		if (status == NEW_NETWORK)
		{
			my_log(LOG_INFO, "zdoInit NEW_NETWORK");
			status = MT_RPC_SUCCESS;
		}
		else if (status == RESTORED_NETWORK)
		{
			my_log(LOG_INFO, "zdoInit RESTORED_NETWORK");
			status = MT_RPC_SUCCESS;
		}
		else
		{
			my_log(LOG_WARNING, "zdoInit failed");
			status = -1;
		}

		my_log(LOG_INFO, "process zdoStatechange callbacks");

		//flush AREQ ZDO State Change messages
		type_my_timeout timeout_wait_zdo_settled;
		initialize_my_timeout(&timeout_wait_zdo_settled);
#define def_timeout_wait_zdo_settled_ms 30000
#define def_timeout_msg_rx_discovery_ms 10000
		int is_timeout_global = 0;
		int is_timeout_msg_rx = 0;
		int is_configured_OK = 0;
		type_my_timeout timeout_no_msgs_rx;
		initialize_my_timeout(&timeout_no_msgs_rx);
		while (!is_configured_OK && !is_timeout_global &&!is_timeout_msg_rx)
		{
			status = rpcWaitMqClientMsg(5000);
			if (status < 0)
			{
				if (is_my_timeout_elapsed_ms(&timeout_no_msgs_rx, def_timeout_msg_rx_discovery_ms))
				{
					is_timeout_msg_rx = 1;
				}
			}
			else
			{
				initialize_my_timeout(&timeout_no_msgs_rx);
			}
			if (is_my_timeout_elapsed_ms(&timeout_wait_zdo_settled, def_timeout_wait_zdo_settled_ms))
			{
				is_timeout_global = 1;
			}
			switch(handle_app.devState)
			{
				case DEV_END_DEVICE: // Started as device after authentication
				case DEV_ROUTER: // Device joined, authenticated and is a router
				case DEV_COORD_STARTING: // Started as Zigbee Coordinator
				case DEV_ZB_COORD: // Started as Zigbee Coordinator
				{
					is_configured_OK = 1;
					break;
				}
				default:
				{
					break;
				}
			}

	#ifndef CC26xx
			if (((devType == DEVICETYPE_COORDINATOR) && (handle_app.devState == DEV_ZB_COORD))
			        || ((devType == DEVICETYPE_ROUTER) && (handle_app.devState == DEV_ROUTER))
			        || ((devType == DEVICETYPE_ENDDEVICE)
			                && (handle_app.devState == DEV_END_DEVICE)))
			{
				break;
			}
	#endif
		}
	#define def_always_enabled_join_req
	#ifdef def_always_enabled_join_req
		{
			PermitJoiningReqFormat_t my_join_permit;
			memset(&my_join_permit, 0, sizeof(my_join_permit));
			my_join_permit.Destination = 0x00;
			my_join_permit.Timeout = 255;
			uint8_t retcode;
			retcode = zbPermitJoiningReq(&my_join_permit);
			if ( retcode )
			{
				my_log(LOG_WARNING, " *** permit join req ERROR: %i", (int)retcode);
			}
			else
			{
				my_log(LOG_INFO, "permit join req OK");
			}
		}
	#endif
		my_log(LOG_INFO, "setNVStartup");
		//set startup option back to keep configuration in case of reset
		status = setNVStartup(0);
		if (handle_app.devState >= DEV_END_DEVICE)
		{
			networkOK = 1;
		}
		else
		{
			my_log(LOG_WARNING, "setNVStartup failed with devState: %i", (int)handle_app.devState);
		}
		// request all of the device infos...
		{
			request_all_device_info(&handle_app.all_sapi_device_info_struct);
		}
		{
			request_firmware_version();
		}
	}
	if (handle_app.devState < DEV_END_DEVICE)
	{
		my_log(LOG_WARNING, "setNVStartup failed with devState: %i", (int)handle_app.devState);
		//start network failed
		return -1;
	}

	my_log(LOG_INFO, "startNetwork ends OK");
	return 0;
}
