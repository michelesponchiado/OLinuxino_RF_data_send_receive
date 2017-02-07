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

#define def_fix_channel 11
#ifdef def_fix_channel
#warning always using fixed channel
#endif

void ZAP_require_network_restart(void)
{
	handle_app.network_restart_req++;
}
unsigned int ZAP_is_required_network_restart(void)
{
	unsigned int is_required =0;
	if (handle_app.network_restart_req != handle_app.network_restart_ack)
	{
		handle_app.network_restart_ack = handle_app.network_restart_req;
		is_required = 1;
	}
	return is_required;
}

int32_t ZAP_restartNetwork(void)
{
#ifndef CC26xx
	char cDevType;
	uint8_t devType;
#endif
	int32_t status;
	handle_app.initDone = 0;
#ifdef ANDROID
	{
		extern int radio_asac_barebone_off();
		radio_asac_barebone_off();
		usleep(300*1000);
		extern int radio_asac_barebone_on();
		radio_asac_barebone_on();
		usleep(300*1000);
	}
#endif
	rpcWaitMqClientMsg(2000);
// enable full restart cleaning previous settings after commit amend
#define def_full_restart_asacz
#ifndef def_full_restart_asacz
	my_log(LOG_WARNING, "%s: full restart disabled", __func__);
	status = setNVStartup(0);
#else
	my_log(LOG_WARNING, "%s: full restart ENABLED", __func__);
	status = setNVStartup(ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
#endif

	if (status != MT_RPC_SUCCESS)
	{
		my_log(LOG_WARNING, "%s: network startup failed", __func__);
		return -1;
	}
	ResetReqFormat_t resReq;
	resReq.Type = 1;
	sysResetReq(&resReq);
	//flush the rsp
	rpcWaitMqClientMsg(2000);

#ifdef def_full_restart_asacz
	my_log(LOG_INFO, "Setting the PAN ID");
	status = setNVPanID(0xFFFF);
	if (status != MT_RPC_SUCCESS)
	{
		my_log(LOG_WARNING, "setNVPanID failed");
		return -1;
	}
	// this must be between 11 and 26
	my_log(LOG_INFO, "Setting the Channel %i", (int)def_fix_channel);
	status = setNVChanList(1 << def_fix_channel);
	if (status != MT_RPC_SUCCESS)
	{
		my_log(LOG_WARNING, "setNVChanList failed on channel %i", def_fix_channel);
		return -1;
	}
#endif

	my_log(LOG_INFO, "channel set OK");
	my_log(LOG_INFO, "about to call registerAf_default");
	// register the default end point
	registerAf_default();
	my_log(LOG_INFO, "default end point registered OK");
	my_log(LOG_INFO, "about to call register_user_end_points");
	// register the user end points
	register_user_end_points();

	my_log(LOG_INFO, "about to call zdo init");
	status = zdoInit();
	if (status == NEW_NETWORK)
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit NEW_NETWORK");
		my_log(LOG_WARNING, "%s: zdoInit NEW_NETWORK", __func__);
		status = MT_RPC_SUCCESS;
	}
	else if (status == RESTORED_NETWORK)
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit RESTORED_NETWORK");
		my_log(LOG_WARNING, "%s: zdoInit RESTORED_NETWORK", __func__);
		status = MT_RPC_SUCCESS;
	}
	else
	{
		my_log(LOG_WARNING, "%s: zdoInit failed", __func__);
		status = -1;
	}

	//flush AREQ ZDO State Change messages
	while (status != -1)
	{
		status = rpcWaitMqClientMsg(2000);
#ifndef CC26xx
		if (((devType == DEVICETYPE_COORDINATOR) && (devState == DEV_ZB_COORD))
		        || ((devType == DEVICETYPE_ROUTER) && (devState == DEV_ROUTER))
		        || ((devType == DEVICETYPE_ENDDEVICE)
		                && (devState == DEV_END_DEVICE)))
		{
			break;
		}
#endif
	}


	{
		PermitJoiningReqFormat_t my_join_permit;
		memset(&my_join_permit, 0, sizeof(my_join_permit));
		my_join_permit.Destination = 0x00;
		my_join_permit.Timeout = 255;
		uint8_t retcode;
		retcode = zbPermitJoiningReq(&my_join_permit);
		if ( retcode )
		{
			my_log(LOG_INFO, "%s: permit join req ko", __func__);
		}
		else
		{
			my_log(LOG_INFO, "%s: permit join req OK", __func__);
		}
	}

	//set startup option back to keep configuration in case of reset
	status = setNVStartup(0);
	if (handle_app.devState < DEV_END_DEVICE)
	{
		my_log(LOG_WARNING, "%s: network restart failed", __func__);
		//start network failed
		return -1;
	}

	my_log(LOG_INFO, "%s: network restart OK", __func__);

	return 0;
}


int32_t ZAP_startNetwork(unsigned int channel_index)
{
#ifndef CC26xx
	char cDevType;
	uint8_t devType;
#endif
	int32_t status;
	uint32_t idxloop;
	uint32_t networkOK = 0;
	for (idxloop = 0; ! networkOK && idxloop < 2; idxloop++)
	{
		uint8_t newNwk = 0;
		if (idxloop >= 1)
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
			my_log(LOG_INFO, "Setting the PAN ID");
			status = setNVPanID(0xFFFF);
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_WARNING, "setNVPanID failed");
				continue;
			}
	#ifdef def_interactive_chan
			dbg_print(PRINT_LEVEL_INFO, "Enter channel 11-26:\n");
			consoleGetLine(sCh, 128);
			status = setNVChanList(1 << atoi(sCh));
	#else
			// this must be between 11 and 26
			my_log(LOG_INFO, "Setting the Channel %i", def_fix_channel);
			status = setNVChanList(1 << def_fix_channel);
	#endif
			if (status != MT_RPC_SUCCESS)
			{
				my_log(LOG_WARNING, "setNVChanList failed on channel %i", def_fix_channel);
				continue;
			}

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
