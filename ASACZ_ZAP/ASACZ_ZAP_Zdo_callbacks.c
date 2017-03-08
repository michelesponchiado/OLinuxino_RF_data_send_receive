/*
 * ASACZ_ZAP_Zdo_callbacks.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */
#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_Zdo_callbacks.h"


/**
 * Node description
 */
static uint8_t mtZdoIEEEAddrRspCb(IeeeAddrRspFormat_t *msg)
{
	if (msg->Status == MT_RPC_SUCCESS)
	{
		type_struct_ASACZ_device_header device_header;
		memset(&device_header, 0, sizeof(device_header));
		device_header.IEEE_address = msg->IEEEAddr;
		device_header.network_short_address = msg->NwkAddr;
//		my_log(LOG_INFO, "Found device @ IEEE address 0x%lX", device_header.IEEE_address);
		my_log(LOG_INFO, "Found device @ IEEE address 0x%" PRIx64, device_header.IEEE_address);
		enum_add_ASACZ_device_list_header_retcode r = add_ASACZ_device_list_header(&device_header);
		if (r != enum_add_ASACZ_device_list_header_retcode_OK)
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: ERROR ADDING DEVICE @ IEEE Address 0x%"  PRIx64 " / network address 0x%X", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			handle_app.stats.device_list.add_header.ERR++;
		}
		else
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: *** Device @ IEEE Address 0x%"  PRIx64 " / network address 0x%X added OK", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			handle_app.stats.device_list.add_header.OK++;
		}
	}
	return msg->Status;
}

static uint8_t mtZdoMgmtLeaveRspCb(MgmtLeaveRspFormat_t *msg)
{
	handle_app.leave.isOK = (msg->Status == 0) ? 1 : 0;
	handle_app.leave.num_ack = handle_app.leave.num_req;
	return 0;
}

static uint8_t mtZdoLeaveIndCb(LeaveIndFormat_t *msg)
{
	dbg_print(PRINT_LEVEL_INFO, "Device with IEEE address 0x%" PRIx64 " has left the network", msg->ExtAddr);
	// if it's me leaving the network, the leave procedure is complete
	handle_app.leave.isOK = (msg->ExtAddr == handle_app.IEEE_address.address) ? 1 : 0;
	// remove the device from the list
	enum_remove_ASACZ_device_list_device_IEEE r = remove_ASACZ_device_list_IEEE(msg->ExtAddr);
	switch(r)
	{
		case enum_remove_ASACZ_device_list_device_IEEE_OK:
		{
			dbg_print(PRINT_LEVEL_INFO, "Device with IEEE address 0x%" PRIx64 " has been removed OK from the device list", msg->ExtAddr);
			break;
		}
		case enum_remove_ASACZ_device_list_device_IEEE_not_found:
		{
			dbg_print(PRINT_LEVEL_INFO, "Device with IEEE address 0x%" PRIx64 " not found in the device list", msg->ExtAddr);
			break;
		}
		default:
		{
			dbg_print(PRINT_LEVEL_ERROR, "Unable to remove from the device list the device with IEEE address 0x%" PRIx64 " , error code %u", msg->ExtAddr, (unsigned int)r );
			break;
		}
	}
	// if it was me leaving the network, set the acknowledgment of the leave request
	if (handle_app.leave.isOK)
	{
		handle_app.leave.num_ack = handle_app.leave.num_req;
	}
	return 0;
}

/**
 * End device announce callback
 */
static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg)
{

	ActiveEpReqFormat_t actReq;
	actReq.DstAddr = msg->NwkAddr;
	actReq.NwkAddrOfInterest = msg->NwkAddr;
	{
		type_struct_ASACZ_device_header device_header;
		memset(&device_header, 0, sizeof(device_header));
		device_header.IEEE_address = msg->IEEEAddr;
		device_header.network_short_address = msg->NwkAddr;
		{
			type_union_ASACZ_MAC_capabilities u;
			u.u8 = msg->Capabilities;
			device_header.MAC_capabilities = u.s;
		}
		my_log(LOG_INFO, "Device 0x%" PRIx64 " just joined the network", device_header.IEEE_address);
		enum_add_ASACZ_device_list_header_retcode r = add_ASACZ_device_list_header(&device_header);
		if (r != enum_add_ASACZ_device_list_header_retcode_OK)
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: ERROR ADDING DEVICE @ IEEE Address 0x%"  PRIx64 " / network address 0x%X", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			handle_app.stats.device_list.add_header.ERR++;
		}
		else
		{
			dbg_print(PRINT_LEVEL_INFO, "%s: *** Device @ IEEE Address 0x%"  PRIx64 " / network address 0x%X added OK", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			handle_app.stats.device_list.add_header.OK++;
		}
	}

	dbg_print(PRINT_LEVEL_INFO, "New device joined network; IEEE address is 0x%" PRIx64 "; short address is 0x%X ", msg->IEEEAddr, (unsigned int)msg->NwkAddr);
	// requests active end-point configuration
	zdoActiveEpReq(&actReq);
	return 0;
}

/********************************************************************
 * @fn     Callback function for ZDO State Change Indication

 * @brief  receives the AREQ status and specifies the change ZDO state
 *
 * @param  uint8 zdoState
 *
 * @return SUCCESS or FAILURE
 */

static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState)
{

	switch (newDevState)
	{
	case DEV_HOLD:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Initialized - not started automatically");
		break;
	case DEV_INIT:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Initialized - not connected to anything");
		break;
	case DEV_NWK_DISC:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Discovering PAN's to join");
		dbg_print(PRINT_LEVEL_INFO, "Network Discovering");
		break;
	case DEV_NWK_JOINING:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Joining a PAN");
		dbg_print(PRINT_LEVEL_INFO, "Network Joining");
		break;
	case DEV_NWK_REJOIN:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: ReJoining a PAN, only for end devices");
		dbg_print(PRINT_LEVEL_INFO, "Network Rejoining");
		break;
	case DEV_END_DEVICE_UNAUTH:
		dbg_print(PRINT_LEVEL_INFO, "Network Authenticating");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Joined but not yet authenticated by trust center");
		break;
	case DEV_END_DEVICE:
		dbg_print(PRINT_LEVEL_INFO, "Network Joined");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Started as device after authentication");
		break;
	case DEV_ROUTER:
		dbg_print(PRINT_LEVEL_INFO, "Network Joined");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Device joined, authenticated and is a router");
		break;
	case DEV_COORD_STARTING:
		dbg_print(PRINT_LEVEL_INFO, "Network Starting");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Starting as Zigbee Coordinator");
		break;
	case DEV_ZB_COORD:
		dbg_print(PRINT_LEVEL_INFO, "Network Started");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Started as Zigbee Coordinator");
		break;
	case DEV_NWK_ORPHAN:
		dbg_print(PRINT_LEVEL_INFO, "Network Orphaned");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Device has lost information about its parent");
		break;
	default:
		dbg_print(PRINT_LEVEL_INFO, "mtZdoStateChangeIndCb: unknown state");
		break;
	}

	handle_app.devState = (devStates_t) newDevState;

	return SUCCESS;
}


/**
 * End point description
 */
static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg)
{

	if (msg->Status == MT_RPC_SUCCESS)
	{
		{
			dbg_print(PRINT_LEVEL_INFO, "End point: 0x%02X", msg->Endpoint);
			dbg_print(PRINT_LEVEL_INFO, "ProfileID: 0x%04X", msg->ProfileID);
			dbg_print(PRINT_LEVEL_INFO, "DeviceID: 0x%04X", msg->DeviceID);
			dbg_print(PRINT_LEVEL_INFO, "DeviceVersion: 0x%02X", msg->DeviceVersion);
			dbg_print(PRINT_LEVEL_INFO, "NumInClusters: %d", msg->NumInClusters);
			uint32_t i;
			for (i = 0; i < msg->NumInClusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO, "    InClusterList[%d]: 0x%04X", i,
				        msg->InClusterList[i]);
			}
			dbg_print(PRINT_LEVEL_INFO, "NumOutClusters: %d", msg->NumOutClusters);
			for (i = 0; i < msg->NumOutClusters; i++)
			{
				dbg_print(PRINT_LEVEL_INFO, "    OutClusterList[%d]: 0x%04X", i,
				        msg->OutClusterList[i]);
			}
		}
		{
			type_struct_ASACZ_endpoint_list_element e;
			e.device_ID 		= msg->DeviceID;


			e.device_version	= msg->DeviceVersion;
			e.end_point			= msg->Endpoint;
			{
				unsigned int n = msg->NumInClusters;
				if (n > def_ASACZ_max_clusters_input)
				{
					n = def_ASACZ_max_clusters_input;
					handle_app.stats.device_list.clusters_input.ERR++;
				}
				else
				{
					handle_app.stats.device_list.clusters_input.OK++;
				}
				e.num_of_clusters_input	= n;
				unsigned int i;
				for (i = 0; i < n; i++)
				{
					e.list_clusters_input[i] = msg->InClusterList[i];
				}
			}
			{
				unsigned int n = msg->NumOutClusters;
				if (n > def_ASACZ_max_clusters_output)
				{
					n = def_ASACZ_max_clusters_output;
					handle_app.stats.device_list.clusters_output.ERR++;
				}
				else
				{
					handle_app.stats.device_list.clusters_output.OK++;
				}
				e.num_of_clusters_output	= n;
				unsigned int i;
				for (i = 0; i < n; i++)
				{
					e.list_clusters_output[i] = msg->OutClusterList[i];
				}
			}
			e.profile_ID= msg->ProfileID;
			enum_add_ASACZ_device_list_single_end_point_retcode r = add_ASACZ_device_list_single_end_point(msg->NwkAddr, &e);
			if (r == enum_add_ASACZ_device_list_single_end_point_retcode_OK)
			{
				handle_app.stats.device_list.add_single_end_point.OK++;
			}
			else
			{
				handle_app.stats.device_list.add_single_end_point.ERR++;
			}
		}

	}
	else
	{
		dbg_print(PRINT_LEVEL_INFO, "SimpleDescRsp Status: FAIL 0x%02X", msg->Status);
	}

	return msg->Status;
}

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg)
{
	uint8_t devType = 0;
	uint8_t devRelation = 0;
	MgmtLqiReqFormat_t req;
	if (msg->Status == MT_RPC_SUCCESS)
	{
		type_handle_node_list *pnode_list = &handle_app.handle_node_list;
		if (pnode_list->nodeCount >= sizeof(pnode_list->nodeList) / sizeof(pnode_list->nodeList[0]))
		{
			dbg_print(PRINT_LEVEL_ERROR, "No room to put another node in list!");
		}
		else
		{

			Node_t *pNode_t = &	pnode_list->nodeList[pnode_list->nodeCount];
			pNode_t->NodeAddr = msg->SrcAddr;
			pNode_t->Type = (msg->SrcAddr == 0 ? DEVICETYPE_COORDINATOR : DEVICETYPE_ROUTER);
			pNode_t->ChildCount = 0;
			uint32_t i;
			for (i = 0; i < msg->NeighborLqiListCount; i++)
			{
				devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
				devRelation = ((msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat
						>> 4) & 7);
				if (devRelation == 1 || devRelation == 3)
				{
					uint8_t cCount = pNode_t->ChildCount;
					pNode_t->childs[cCount].ChildAddr =
							msg->NeighborLqiList[i].NetworkAddress;
					pNode_t->childs[cCount].Type = devType;
					pNode_t->ChildCount++;
					if (devType == DEVICETYPE_ROUTER)
					{
						req.DstAddr = msg->NeighborLqiList[i].NetworkAddress;
						req.StartIndex = 0;
						zdoMgmtLqiReq(&req);
					}
				}
			}
			pnode_list->nodeCount++;
		}
	}
	else
	{
		dbg_print(PRINT_LEVEL_INFO, "MgmtLqiRsp Status: FAIL 0x%02X", msg->Status);
	}

	return msg->Status;
}



/**
 * Active end point list callback
 */
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg)
{

	//SimpleDescReqFormat_t simReq;
	dbg_print(PRINT_LEVEL_INFO, "NwkAddr: 0x%04X", msg->NwkAddr);
	if (msg->Status == MT_RPC_SUCCESS)
	{
		// print on console the end-points list
		{
			dbg_print(PRINT_LEVEL_INFO, "Number of end-points: %d", (unsigned int) msg->ActiveEPCount);
			dbg_print(PRINT_LEVEL_INFO, "Active end-points: ");
			uint32_t i;
			for (i = 0; i < msg->ActiveEPCount; i++)
			{
				dbg_print(PRINT_LEVEL_INFO, "    0x%02X", (unsigned int)msg->ActiveEPList[i]);

			}
		}
		// add the end-point list
		enum_add_ASACZ_device_list_end_points_retcode r = add_ASACZ_device_list_end_points(msg->NwkAddr, msg->ActiveEPCount, msg->ActiveEPList);
		if (r == enum_add_ASACZ_device_list_end_points_retcode_OK)
		{
			handle_app.stats.device_list.add_end_points.OK++;
		}
		else
		{
			handle_app.stats.device_list.add_end_points.ERR++;
		}
		{
			// requests the end-point configuration for all of the end-points
			int i;
			SimpleDescReqFormat_t simReq;
			simReq.DstAddr = msg->SrcAddr;
			simReq.NwkAddrOfInterest= msg->NwkAddr;
			for (i = 0; i < msg->ActiveEPCount; i++)
			{
				simReq.Endpoint =  msg->ActiveEPList[i];
				uint8_t st_simple_req = zdoSimpleDescReq(&simReq);
				if (st_simple_req == SUCCESS)
				{
					handle_app.stats.device_list.req_simple_desc.OK++;
				}
				else
				{
					handle_app.stats.device_list.req_simple_desc.ERR++;
				}
			}
		}

	}
	else
	{
		dbg_print(PRINT_LEVEL_INFO, "ActiveEpRsp Status: FAIL 0x%02X", msg->Status);
		handle_app.stats.device_list.add_end_points.ERR++;
	}

	return msg->Status;
}

static mtZdoCb_t mtZdoCb =
{
	.pfnZdoIeeeAddrRsp = mtZdoIEEEAddrRspCb,      // MT_ZDO_IEEE_ADDR_RSP
	.pfnZdoSimpleDescRsp = mtZdoSimpleDescRspCb,    // MT_ZDO_SIMPLE_DESC_RSP
	.pfnZdoActiveEpRsp = mtZdoActiveEpRspCb,      // MT_ZDO_ACTIVE_EP_RSP
	.pfnZdoMgmtLqiRsp = mtZdoMgmtLqiRspCb,       // MT_ZDO_MGMT_LQI_RSP
	.pfnmtZdoStateChangeInd = mtZdoStateChangeIndCb,   // MT_ZDO_STATE_CHANGE_IND
	.pfnZdoEndDeviceAnnceInd = mtZdoEndDeviceAnnceIndCb,   // MT_ZDO_END_DEVICE_ANNCE_IND
	.pfnZdoMgmtLeaveRsp = mtZdoMgmtLeaveRspCb,
	.pfnZdoLeaveInd = mtZdoLeaveIndCb,
};

void ASACZ_ZAP_Zdo_callback_register(void)
{
	zdoRegisterCallbacks(mtZdoCb);
}

