/*
 * ASACZ_ZAP_network.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */
#include <ASACZ_ZAP.h>
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

void ZAP_require_network_restart_from_scratch(void)
{
	handle_app.network_restart_from_scratch_req++;
}
unsigned int ZAP_is_required_network_restart_from_scratch(void)
{
	unsigned int is_required =0;
	if (handle_app.network_restart_from_scratch_req != handle_app.network_restart_from_scratch_ack)
	{
		handle_app.network_restart_from_scratch_ack = handle_app.network_restart_from_scratch_req;
		is_required = 1;
	}
	return is_required;
}

int32_t ZAP_startNetwork(enum_start_network_type start_network_type)
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
		uint8_t newNwk = 0;
		if (start_network_type == enum_start_network_type_from_scratch)
		{
			newNwk = 1;
			usleep(50000);
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
		while (status != -1)
		{
			status = rpcWaitMqClientMsg(2000);
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
