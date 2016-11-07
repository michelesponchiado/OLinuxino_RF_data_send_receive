/**************************************************************************************************
 * Filename:       dataSendRcv.c
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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>

#include "rpc.h"
#include "mtSys.h"
#include "mtSapi.h"
#include "mtZdo.h"
#include "mtAf.h"
#include "mtParser.h"
#include "rpcTransport.h"
#include "dbgPrint.h"
#include "hostConsole.h"
#include "ts_util.h"
#include "ZigBee_messages.h"
#include "my_queues.h"
#include "ASACZ_devices_list.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * TYPES
 */

/*********************************************************************
 * LOCAL VARIABLE
 */

uint8_t gSrcEndPoint = 1;
uint8_t gDstEndPoint = 1;

typedef enum
{
	enum_app_status_idle = 0,
	enum_app_status_init,
	enum_app_status_flush_messages_init,
	enum_app_status_flush_messages_do,
	enum_app_status_start_network,
	enum_app_status_get_IEEE_address,
	enum_app_status_set_TX_power,
	enum_app_status_display_devices_init,
	enum_app_status_display_devices_do,
	enum_app_status_display_devices_ends,
	enum_app_status_rx_msgs,
	enum_app_status_tx_msgs,
	enum_app_status_error,
	enum_app_status_numof
}enum_app_status;

typedef enum
{
	enum_app_error_code_none,
	enum_app_error_code_too_many_messages_flushed,
	enum_app_error_code_unable_to_start_network,
	enum_app_error_code_numof
}enum_app_error_code;

typedef struct _type_handle_app
{
	enum_app_status status;
	uint32_t num_unknown_status;
	uint32_t start_ack;
	uint32_t start_request;
	uint32_t num_msgs_flushed;
	devStates_t devState;
	enum_app_error_code error_code;
	DataRequestFormat_t DataRequest;
	uint32_t initDone;
	char message[def_size_my_elem_queue];
	unsigned int message_id;
	uint8_t transId;
	pthread_mutex_t mtx_id;
}type_handle_app;
static type_handle_app handle_app;

void init_handle_app(void)
{
	memset(&handle_app, 0, sizeof(handle_app));
	handle_app.devState = DEV_HOLD;
	pthread_mutex_init(&handle_app.mtx_id, NULL);
}
unsigned int get_app_new_trans_id(uint32_t message_id)
{
	pthread_mutex_lock(&handle_app.mtx_id);
	uint8_t trans_id = handle_app.transId++;
		// cleanup all of the similar identifiers in the id queue, set the new id with the trans_id passed
		message_history_set_trans_id(trans_id, message_id);
	pthread_mutex_unlock(&handle_app.mtx_id);
	return trans_id;
}
void start_handle_app(void)
{
	handle_app.start_request++;
}

static void handle_app_error(enum_app_error_code error_code)
{
	handle_app.error_code = error_code;
}



typedef struct _type_OKERR_stats
{
	unsigned int OK;
	unsigned int ERR;
}type_OKERR_stats;
typedef struct _type_device_list_stats
{
	type_OKERR_stats add_header;
	type_OKERR_stats add_end_points;
	type_OKERR_stats req_simple_desc;
	type_OKERR_stats add_single_end_point, clusters_output, clusters_input;
}type_device_list_stats;

typedef struct _type_datasendrcv_stats
{
	type_device_list_stats device_list;
}type_datasendrcv_stats;
static type_datasendrcv_stats stats;


/***********************************************************************/

/*********************************************************************
 * LOCAL FUNCTIONS
 */
//ZDO Callbacks
static uint8_t mtZdoStateChangeIndCb(uint8_t newDevState);
static uint8_t mtZdoSimpleDescRspCb(SimpleDescRspFormat_t *msg);
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);

//SYS Callbacks

static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg);
static uint8_t mtSysSetTxPowerSrspCb(SetTxPowerSrspFormat_t *msg);

//AF callbacks
static uint8_t mtAfDataConfirmCb(DataConfirmFormat_t *msg);
static uint8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *msg);

//helper functions
static uint8_t setNVStartup(uint8_t startupOption);
static uint8_t setNVChanList(uint32_t chanList);
static uint8_t setNVPanID(uint32_t panId);
#ifndef CC26xx
static uint8_t setNVDevType(uint8_t devType);
#endif
static int32_t startNetwork(void);
static int32_t registerAf(void);

unsigned long get_system_time_ms(void)
{
	unsigned long            ms; // Milliseconds
	time_t          s;  // Seconds
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);

	s  = (spec.tv_sec);
	ms = s*1000+(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
	return ms;
}

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

// SYS callbacks
static mtSysCb_t mtSysCb =
	{ 		NULL, NULL, NULL,
			mtSysResetIndCb, NULL, NULL,
			NULL, NULL, NULL,
			NULL, NULL, NULL,
			NULL, mtSysSetTxPowerSrspCb
	};

static mtZdoCb_t mtZdoCb =
	{ NULL,       // MT_ZDO_NWK_ADDR_RSP
	        NULL,      // MT_ZDO_IEEE_ADDR_RSP
	        NULL,      // MT_ZDO_NODE_DESC_RSP
	        NULL,     // MT_ZDO_POWER_DESC_RSP
	        mtZdoSimpleDescRspCb,    // MT_ZDO_SIMPLE_DESC_RSP
	        mtZdoActiveEpRspCb,      // MT_ZDO_ACTIVE_EP_RSP
	        NULL,     // MT_ZDO_MATCH_DESC_RSP
	        NULL,   // MT_ZDO_COMPLEX_DESC_RSP
	        NULL,      // MT_ZDO_USER_DESC_RSP
	        NULL,     // MT_ZDO_USER_DESC_CONF
	        NULL,    // MT_ZDO_SERVER_DISC_RSP
	        NULL, // MT_ZDO_END_DEVICE_BIND_RSP
	        NULL,          // MT_ZDO_BIND_RSP
	        NULL,        // MT_ZDO_UNBIND_RSP
	        NULL,   // MT_ZDO_MGMT_NWK_DISC_RSP
	        mtZdoMgmtLqiRspCb,       // MT_ZDO_MGMT_LQI_RSP
	        NULL,       // MT_ZDO_MGMT_RTG_RSP
	        NULL,      // MT_ZDO_MGMT_BIND_RSP
	        NULL,     // MT_ZDO_MGMT_LEAVE_RSP
	        NULL,     // MT_ZDO_MGMT_DIRECT_JOIN_RSP
	        NULL,     // MT_ZDO_MGMT_PERMIT_JOIN_RSP
	        mtZdoStateChangeIndCb,   // MT_ZDO_STATE_CHANGE_IND
	        mtZdoEndDeviceAnnceIndCb,   // MT_ZDO_END_DEVICE_ANNCE_IND
	        NULL,        // MT_ZDO_SRC_RTG_IND
	        NULL,	 //MT_ZDO_BEACON_NOTIFY_IND
	        NULL,			 //MT_ZDO_JOIN_CNF
	        NULL,	 //MT_ZDO_NWK_DISCOVERY_CNF
	        NULL,                    // MT_ZDO_CONCENTRATOR_IND_CB
	        NULL,         // MT_ZDO_LEAVE_IND
	        NULL,   //MT_ZDO_STATUS_ERROR_RSP
	        NULL,  //MT_ZDO_MATCH_DESC_RSP_SENT
	        NULL, NULL };

static mtAfCb_t mtAfCb =
	{ mtAfDataConfirmCb,				//MT_AF_DATA_CONFIRM
	        mtAfIncomingMsgCb,				//MT_AF_INCOMING_MSG
	        NULL,				//MT_AF_INCOMING_MSG_EXT
	        NULL,			//MT_AF_DATA_RETRIEVE
	        NULL,			    //MT_AF_REFLECT_ERROR
	    };
typedef struct
{
	uint16_t ChildAddr;
	uint8_t Type;

} ChildNode_t;

typedef struct
{
	uint16_t NodeAddr;
	uint8_t Type;
	uint8_t ChildCount;
	ChildNode_t childs[256];
} Node_t;

Node_t nodeList[64];
uint8_t nodeCount = 0;

/********************************************************************
 * START OF SYS CALL BACK FUNCTIONS
 */

static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg)
{

	consolePrint("ZNP Version: %d.%d.%d\n", msg->MajorRel, msg->MinorRel,
	        msg->HwRev);
	return 0;
}

static uint8_t mtSysSetTxPowerSrspCb(SetTxPowerSrspFormat_t *msg)
{
	consolePrint("Radio power set to %i dBm\n", (int)msg->TxPower);
	return 0;
}

/********************************************************************
 * START OF ZDO CALL BACK FUNCTIONS
 */

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
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Initialized - not started automatically\n");
		break;
	case DEV_INIT:
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Initialized - not connected to anything\n");
		break;
	case DEV_NWK_DISC:
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Discovering PAN's to join\n");
		consolePrint("Network Discovering\n");
		break;
	case DEV_NWK_JOINING:
		dbg_print(PRINT_LEVEL_INFO, "mtZdoStateChangeIndCb: Joining a PAN\n");
		consolePrint("Network Joining\n");
		break;
	case DEV_NWK_REJOIN:
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: ReJoining a PAN, only for end devices\n");
		consolePrint("Network Rejoining\n");
		break;
	case DEV_END_DEVICE_UNAUTH:
		consolePrint("Network Authenticating\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Joined but not yet authenticated by trust center\n");
		break;
	case DEV_END_DEVICE:
		consolePrint("Network Joined\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Started as device after authentication\n");
		break;
	case DEV_ROUTER:
		consolePrint("Network Joined\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Device joined, authenticated and is a router\n");
		break;
	case DEV_COORD_STARTING:
		consolePrint("Network Starting\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
		break;
	case DEV_ZB_COORD:
		consolePrint("Network Started\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
		break;
	case DEV_NWK_ORPHAN:
		consolePrint("Network Orphaned\n");
		dbg_print(PRINT_LEVEL_INFO,
		        "mtZdoStateChangeIndCb: Device has lost information about its parent\n");
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
			consolePrint("\tEndpoint: 0x%02X\n", msg->Endpoint);
			consolePrint("\tProfileID: 0x%04X\n", msg->ProfileID);
			consolePrint("\tDeviceID: 0x%04X\n", msg->DeviceID);
			consolePrint("\tDeviceVersion: 0x%02X\n", msg->DeviceVersion);
			consolePrint("\tNumInClusters: %d\n", msg->NumInClusters);
			uint32_t i;
			for (i = 0; i < msg->NumInClusters; i++)
			{
				consolePrint("\t\tInClusterList[%d]: 0x%04X\n", i,
				        msg->InClusterList[i]);
			}
			consolePrint("\tNumOutClusters: %d\n", msg->NumOutClusters);
			for (i = 0; i < msg->NumOutClusters; i++)
			{
				consolePrint("\t\tOutClusterList[%d]: 0x%04X\n", i,
				        msg->OutClusterList[i]);
			}
			consolePrint("\n");
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
					stats.device_list.clusters_input.ERR++;
				}
				else
				{
					stats.device_list.clusters_input.OK++;
				}
				e.num_of_clusters_input	= n;
				int i;
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
					stats.device_list.clusters_output.ERR++;
				}
				else
				{
					stats.device_list.clusters_output.OK++;
				}
				e.num_of_clusters_output	= n;
				int i;
				for (i = 0; i < n; i++)
				{
					e.list_clusters_output[i] = msg->OutClusterList[i];
				}
			}
			e.profile_ID= msg->ProfileID;
			enum_add_ASACZ_device_list_single_end_point_retcode r = add_ASACZ_device_list_single_end_point(msg->NwkAddr, &e);
			if (r == enum_add_ASACZ_device_list_single_end_point_retcode_OK)
			{
				stats.device_list.add_single_end_point.OK++;
			}
			else
			{
				stats.device_list.add_single_end_point.ERR++;
			}
		}

	}
	else
	{
		consolePrint("SimpleDescRsp Status: FAIL 0x%02X\n", msg->Status);
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
		nodeList[nodeCount].NodeAddr = msg->SrcAddr;
		nodeList[nodeCount].Type = (msg->SrcAddr == 0 ?
		DEVICETYPE_COORDINATOR :
		                                                DEVICETYPE_ROUTER);
		nodeList[nodeCount].ChildCount = 0;
		uint32_t i;
		for (i = 0; i < msg->NeighborLqiListCount; i++)
		{
			devType = msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat & 3;
			devRelation = ((msg->NeighborLqiList[i].DevTyp_RxOnWhenIdle_Relat
			        >> 4) & 7);
			if (devRelation == 1 || devRelation == 3)
			{
				uint8_t cCount = nodeList[nodeCount].ChildCount;
				nodeList[nodeCount].childs[cCount].ChildAddr =
				        msg->NeighborLqiList[i].NetworkAddress;
				nodeList[nodeCount].childs[cCount].Type = devType;
				nodeList[nodeCount].ChildCount++;
				if (devType == DEVICETYPE_ROUTER)
				{
					req.DstAddr = msg->NeighborLqiList[i].NetworkAddress;
					req.StartIndex = 0;
					zdoMgmtLqiReq(&req);
				}
			}
		}
		nodeCount++;

	}
	else
	{
		consolePrint("MgmtLqiRsp Status: FAIL 0x%02X\n", msg->Status);
	}

	return msg->Status;
}

/**
 * Active end point list callback
 */
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg)
{

	//SimpleDescReqFormat_t simReq;
	consolePrint("NwkAddr: 0x%04X\n", msg->NwkAddr);
	if (msg->Status == MT_RPC_SUCCESS)
	{
		// print on console the end-points list
		{
			consolePrint("Number of end-points: %d\nActive end-points: ", msg->ActiveEPCount);
			uint32_t i;
			for (i = 0; i < msg->ActiveEPCount; i++)
			{
				consolePrint("0x%02X\t", msg->ActiveEPList[i]);

			}
			consolePrint("\n");
		}
		// add the end-point list
		enum_add_ASACZ_device_list_end_points_retcode r = add_ASACZ_device_list_end_points(msg->NwkAddr, msg->ActiveEPCount, msg->ActiveEPList);
		if (r == enum_add_ASACZ_device_list_end_points_retcode_OK)
		{
			stats.device_list.add_end_points.OK++;
		}
		else
		{
			stats.device_list.add_end_points.ERR++;
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
					stats.device_list.req_simple_desc.OK++;
				}
				else
				{
					stats.device_list.req_simple_desc.ERR++;
				}
			}
		}

	}
	else
	{
		consolePrint("ActiveEpRsp Status: FAIL 0x%02X\n", msg->Status);
		stats.device_list.add_end_points.ERR++;
	}

	return msg->Status;
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
		enum_add_ASACZ_device_list_header_retcode r = add_ASACZ_device_list_header(&device_header);
		if (r != enum_add_ASACZ_device_list_header_retcode_OK)
		{
			stats.device_list.add_header.ERR++;
		}
		else
		{
			stats.device_list.add_header.OK++;
		}
	}

	consolePrint("\nNew device joined network.\n");
	// requests active end-point configuration
	zdoActiveEpReq(&actReq);
	return 0;
}

/********************************************************************
 * AF DATA REQUEST CALL BACK FUNCTION
 */

static uint8_t mtAfDataConfirmCb(DataConfirmFormat_t *msg)
{

	if (msg->Status == MT_RPC_SUCCESS)
	{
		//consolePrint("[%lu]Message %i transmitted successfully!\n", get_system_time_ms(), (int)msg->TransId);
	}
	else
	{
		consolePrint("[%lu]Message %i failed to transmit\n", get_system_time_ms(), (int)msg->TransId);
	}
	return msg->Status;
}
static unsigned int txrx_req;
static unsigned int txrx_ack;
static IncomingMsgFormat_t txrx_i;
static uint8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *msg)
{

	msg->Data[msg->Len] = '\0';
	if (strncmp((char*)msg->Data,"TXRX",4) == 0 )
	{
		txrx_i = *msg;
		txrx_req ++;
	}
	else
	{
		consolePrint("%s\n", (char*) msg->Data);
		consolePrint(
		        "\nIncoming Message from Endpoint 0x%02X and Address 0x%04X:\n",
		        msg->SrcEndpoint, msg->SrcAddr);
		consolePrint(
		        "\nEnter message to send or type CHANGE to change the destination \nor QUIT to exit:\n");
	}

	return 0;
}

/********************************************************************
 * HELPER FUNCTIONS
 */
// helper functions for building and sending the NV messages
static uint8_t setNVStartup(uint8_t startupOption)
{
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;

	// sending startup option
	nvWrite.Id = ZCD_NV_STARTUP_OPTION;
	nvWrite.Offset = 0;
	nvWrite.Len = 1;
	nvWrite.Value[0] = startupOption;
	status = sysOsalNvWrite(&nvWrite);

	dbg_print(PRINT_LEVEL_INFO, "\n");

	dbg_print(PRINT_LEVEL_INFO, "NV Write Startup Option cmd sent[%d]...\n",
	        status);

	return status;
}

#ifndef CC26xx

static uint8_t setNVDevType(uint8_t devType)
{
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;

	// setting dev type
	nvWrite.Id = ZCD_NV_LOGICAL_TYPE;
	nvWrite.Offset = 0;
	nvWrite.Len = 1;
	nvWrite.Value[0] = devType;
	status = sysOsalNvWrite(&nvWrite);

	dbg_print(PRINT_LEVEL_INFO, "\n");
	dbg_print(PRINT_LEVEL_INFO, "NV Write Device Type cmd sent... [%d]\n",
	        status);

	return status;
}
#endif


static uint8_t setNVPanID(uint32_t panId)
{
	uint8_t status;
	OsalNvWriteFormat_t nvWrite;

	dbg_print(PRINT_LEVEL_INFO, "\n");
	dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sending...\n");

	nvWrite.Id = ZCD_NV_PANID;
	nvWrite.Offset = 0;
	nvWrite.Len = 2;
	nvWrite.Value[0] = LO_UINT16(panId);
	nvWrite.Value[1] = HI_UINT16(panId);
	status = sysOsalNvWrite(&nvWrite);

	dbg_print(PRINT_LEVEL_INFO, "\n");
	dbg_print(PRINT_LEVEL_INFO, "NV Write PAN ID cmd sent...[%d]\n", status);

	return status;
}

static uint8_t setNVChanList(uint32_t chanList)
{
	OsalNvWriteFormat_t nvWrite;
	uint8_t status;
	// setting chanList
	nvWrite.Id = ZCD_NV_CHANLIST;
	nvWrite.Offset = 0;
	nvWrite.Len = 4;
	nvWrite.Value[0] = BREAK_UINT32(chanList, 0);
	nvWrite.Value[1] = BREAK_UINT32(chanList, 1);
	nvWrite.Value[2] = BREAK_UINT32(chanList, 2);
	nvWrite.Value[3] = BREAK_UINT32(chanList, 3);
	status = sysOsalNvWrite(&nvWrite);

	dbg_print(PRINT_LEVEL_INFO, "\n");
	dbg_print(PRINT_LEVEL_INFO, "NV Write Channel List cmd sent...[%d]\n",
	        status);

	return status;
}

uint8_t dType;
static int32_t startNetwork(void)
{
#ifndef CC26xx	
	char cDevType;
	uint8_t devType;
#endif
	int32_t status;
	uint8_t newNwk = 0;
//#define def_interactive
#ifdef def_interactive
	char sCh[128];

	do
	{
		consolePrint("Do you wish to start/join a new network? (y/n)\n");
		consoleGetLine(sCh, 128);
		if (sCh[0] == 'n' || sCh[0] == 'N')
		{
			status = setNVStartup(0);
		}
		else if (sCh[0] == 'y' || sCh[0] == 'Y')
		{
			status = setNVStartup(
			ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
			newNwk = 1;

		}
		else
		{
			consolePrint("Incorrect input please type y or n\n");
		}
	} while (sCh[0] != 'y' && sCh[0] != 'Y' && sCh[0] != 'n' && sCh[0] != 'N');
#else
#define def_new_network
#ifdef def_new_network
	status = setNVStartup(ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
	newNwk = 1;
#else
	status = setNVStartup(0);
#endif
#endif
	if (status != MT_RPC_SUCCESS)
	{
		dbg_print(PRINT_LEVEL_WARNING, "network start failed\n");
		return -1;
	}
	consolePrint("Resetting ZNP\n");
	ResetReqFormat_t resReq;
	resReq.Type = 1;
	sysResetReq(&resReq);
	//flush the rsp
	rpcWaitMqClientMsg(5000);

	if (newNwk)
	{
#ifndef CC26xx
		consolePrint(
		        "Enter device type c: Coordinator, r: Router, e: End Device:\n");
		consoleGetLine(sCh, 128);
		cDevType = sCh[0];

		switch (cDevType)
		{
		case 'c':
		case 'C':
			devType = DEVICETYPE_COORDINATOR;
			break;
		case 'r':
		case 'R':
			devType = DEVICETYPE_ROUTER;
			break;
		case 'e':
		case 'E':
		default:
			devType = DEVICETYPE_ENDDEVICE;
			break;
		}
		status = setNVDevType(devType);

		if (status != MT_RPC_SUCCESS)
		{
			dbg_print(PRINT_LEVEL_WARNING, "setNVDevType failed\n");
			return 0;
		}
#endif //CC26xx
		//Select random PAN ID for Coord and join any PAN for RTR/ED
		status = setNVPanID(0xFFFF);
		if (status != MT_RPC_SUCCESS)
		{
			dbg_print(PRINT_LEVEL_WARNING, "setNVPanID failed\n");
			return -1;
		}
		consolePrint("Enter channel 11-26:\n");
#ifdef def_interactive_chan
		consoleGetLine(sCh, 128);
		status = setNVChanList(1 << atoi(sCh));
#else
		status = setNVChanList(1 << 11);
#endif
		if (status != MT_RPC_SUCCESS)
		{
			dbg_print(PRINT_LEVEL_INFO, "setNVPanID failed\n");
			return -1;
		}

	}

	registerAf();
	consolePrint("EndPoint: 1\n");

	status = zdoInit();
	if (status == NEW_NETWORK)
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit NEW_NETWORK\n");
		status = MT_RPC_SUCCESS;
	}
	else if (status == RESTORED_NETWORK)
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit RESTORED_NETWORK\n");
		status = MT_RPC_SUCCESS;
	}
	else
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit failed\n");
		status = -1;
	}

	dbg_print(PRINT_LEVEL_INFO, "process zdoStatechange callbacks\n");

	//flush AREQ ZDO State Change messages
	while (status != -1)
	{
		status = rpcWaitMqClientMsg(5000);
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
	if (1)
	{	
		PermitJoiningReqFormat_t my_join_permit = {0};
		my_join_permit.Destination = 0x00;
		my_join_permit.Timeout = 255;
		uint8_t retcode;
		retcode = zbPermitJoiningReq(&my_join_permit);
		if ( retcode )
		{
			dbg_print(PRINT_LEVEL_WARNING, " *** permit join req ERROR: %i\n", (int)retcode);
		}
		else
		{
			dbg_print(PRINT_LEVEL_WARNING, "permit join req OK\n");
		}
	}
#endif
	//set startup option back to keep configuration in case of reset
	status = setNVStartup(0);
	if (handle_app.devState < DEV_END_DEVICE)
	{
		//start network failed
		return -1;
	}

	return 0;
}
static int32_t registerAf(void)
{
	int32_t status = 0;
	RegisterFormat_t reg;

	reg.EndPoint = 1;
	reg.AppProfId = 0x0104;
	reg.AppDeviceId = 0x0100;
	reg.AppDevVer = 1;
	reg.LatencyReq = 0;
	reg.AppNumInClusters = 1;
	reg.AppInClusterList[0] = 0x0006;
	reg.AppNumOutClusters = 0;

	status = afRegister(&reg);
	return status;
}

static void displayDevices(void)
{
	ActiveEpReqFormat_t actReq;
	int32_t status;

	MgmtLqiReqFormat_t req;

	req.DstAddr = 0;
	req.StartIndex = 0;
	nodeCount = 0;
	zdoMgmtLqiReq(&req);
	do
	{
		status = rpcWaitMqClientMsg(1000);
	} while (status != -1);

	consolePrint("\nAvailable devices:\n");
	uint8_t i;
	for (i = 0; i < nodeCount; i++)
	{
		char *devtype =
		        (nodeList[i].Type == DEVICETYPE_ROUTER ?
		                "ROUTER" : "COORDINATOR");

		consolePrint("Type: %s\n", devtype);
		actReq.DstAddr = nodeList[i].NodeAddr;
		actReq.NwkAddrOfInterest = nodeList[i].NodeAddr;
		zdoActiveEpReq(&actReq);
		rpcGetMqClientMsg();
		do
		{
			status = rpcWaitMqClientMsg(1000);
		} while (status != -1);
		uint8_t cI;
		for (cI = 0; cI < nodeList[i].ChildCount; cI++)
		{
			uint8_t type = nodeList[i].childs[cI].Type;
			if (type == DEVICETYPE_ENDDEVICE)
			{
				consolePrint("Type: END DEVICE\n");
				actReq.DstAddr = nodeList[i].childs[cI].ChildAddr;
				actReq.NwkAddrOfInterest = nodeList[i].childs[cI].ChildAddr;
				zdoActiveEpReq(&actReq);
				status = 0;
				rpcGetMqClientMsg();
				while (status != -1)
				{
					status = rpcWaitMqClientMsg(1000);
				}
			}

		}
		consolePrint("\n");

	}
}

void set_TX_power(void)
{
	int8_t pwr_required_dbm = 2;
	SetTxPowerFormat_t req = {0};
	*(int8_t*)&req.TxPower = pwr_required_dbm;
	dbg_print(PRINT_LEVEL_WARNING, "setting the TX power to %i dBm\n", (int)pwr_required_dbm);
	unsigned int success = 0;
	switch(sysSetTxPower(&req))
	{
		case MT_RPC_SUCCESS:
		{
			success = 1;
			break;
		}
		default:
		{
			break;
		}
	}
	dbg_print(PRINT_LEVEL_WARNING, "TX power sent %s\n", success?"OK":"**ERROR**");
}
/*********************************************************************
 * INTERFACE FUNCTIONS
 */
uint32_t appInit(void)
{
	int32_t status = 0;
	uint32_t msgCnt = 0;

	//Flush all messages from the que
	while (status != -1)
	{
		status = rpcWaitMqClientMsg(10);
		if (status != -1)
		{
			msgCnt++;
		}
	}

	dbg_print(PRINT_LEVEL_INFO, "flushed %d message from msg queue\n", msgCnt);

	//Register Callbacks MT system callbacks
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	afRegisterCallbacks(mtAfCb);

	return 0;
}

unsigned int suspend_rx_ack;
unsigned int suspend_rx_req;
unsigned int end_suspend_rx_ack;
unsigned int end_suspend_rx_req;
unsigned int do_suspend_rx;


void* appMsgProcess(void *argument)
{

	if (handle_app.initDone )
	{
		if (do_suspend_rx)
		{
			usleep(1000);
		}
		else
		{
			rpcWaitMqClientMsg(10);
		}
	}
	if (suspend_rx_req != suspend_rx_ack)
	{
		suspend_rx_ack = suspend_rx_req;
		do_suspend_rx = 1;
	}
	else if (end_suspend_rx_req != end_suspend_rx_ack)
	{
		end_suspend_rx_ack = end_suspend_rx_req;
		do_suspend_rx = 0;
	}

	return 0;
}



void* appProcess(void *argument)
{
	switch(handle_app.status)
	{
		default:
		{
			handle_app.num_unknown_status++;
			handle_app.status = enum_app_status_idle;
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
			start_queue_message_to_ZigBee();
			handle_app.devState = DEV_HOLD;
			handle_app.status = enum_app_status_flush_messages_init;
			break;
		}
		case enum_app_status_flush_messages_init:
		{
			handle_app.num_msgs_flushed = 0;
			handle_app.status = enum_app_status_flush_messages_do;
			break;
		}
		case enum_app_status_flush_messages_do:
		{
#define def_max_messages_flushed 100
			int32_t status = rpcWaitMqClientMsg(50);
			if (status == -1)
			{
				handle_app.status = enum_app_status_start_network;
			}
			else if (++handle_app.num_msgs_flushed >= def_max_messages_flushed)
			{
				handle_app_error(enum_app_error_code_too_many_messages_flushed);
			}
			break;
		}
		case enum_app_status_start_network:
		{
			handle_app.devState = DEV_HOLD;

			int32_t status = startNetwork();
			if (status != -1)
			{
				syslog(LOG_INFO, "Network up");
				handle_app.status = enum_app_status_get_IEEE_address;
			}
			else
			{
				syslog(LOG_ERR, "Unable to start Network");
				handle_app_error(enum_app_error_code_unable_to_start_network);
			}
			break;
		}
		case enum_app_status_get_IEEE_address:
		{
			sysGetExtAddr();
			// TODO: what is this?
			OsalNvWriteFormat_t nvWrite;
			nvWrite.Id = ZCD_NV_ZDO_DIRECT_CB;
			nvWrite.Offset = 0;
			nvWrite.Len = 1;
			nvWrite.Value[0] = 1;
			int32_t status = sysOsalNvWrite(&nvWrite);
			handle_app.status = enum_app_status_set_TX_power;
			break;
		}
		case enum_app_status_set_TX_power:
		{
			set_TX_power();
			handle_app.status = enum_app_status_display_devices_init;
			break;
		}
		case enum_app_status_display_devices_init:
		{
			nodeCount = 0;
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
			handle_app.DataRequest.DstAddr =  (uint16_t) 0xfa24;
			handle_app.DataRequest.DstEndpoint = (uint8_t) 1;
			handle_app.DataRequest.SrcEndpoint = 1;
			handle_app.DataRequest.ClusterID = 6;
			handle_app.DataRequest.TransID = 5;
			handle_app.DataRequest.Options = 0;
			handle_app.DataRequest.Radius = 0xEE;
			handle_app.initDone = 1;
			handle_app.status = enum_app_status_rx_msgs;
			break;
		}
		case enum_app_status_rx_msgs:
		{
			rpcWaitMqClientMsg(10);
			handle_app.status = enum_app_status_tx_msgs;
			break;
		}
		case enum_app_status_tx_msgs:
		{
			handle_app.status = enum_app_status_rx_msgs;
#define def_use_only_zigbee_queue_messages
#ifdef def_use_only_zigbee_queue_messages
			unsigned int is_element_in_queue = 0;
			unsigned int elem_popped_size;
			DataRequestFormat_t *pdr = &handle_app.DataRequest;
			if (is_OK_pop_message_to_Zigbee(pdr->Data, &elem_popped_size, sizeof(pdr->Data), &handle_app.message_id))
			{

				pdr->TransID = get_app_new_trans_id(handle_app.message_id);
				pdr->Len = elem_popped_size;

				int32_t status = afDataRequest(pdr);
				if (status == MT_RPC_SUCCESS)
				{
					syslog(LOG_ERR, "Unable to send message with id = %u, length = %u", handle_app.message_id, elem_popped_size);
				}
				else
				{

				}
				// TODO: is a rpcWaitMqClientMsg really needed?

			}

#else
	#define def_test_txrx
	#ifndef def_test_txrx
				consolePrint(
						"Enter message to send or type CHANGE to change the destination\n");
				consolePrint("or QUIT to exit\n");
				consoleGetLine(cmd, 128);
				//handle_app.initDone = 1;
				if (strcmp(cmd, "CHANGE") == 0)
				{
					break;
				}
				else if (strcmp(cmd, "QUIT") == 0)
				{
					quit = 1;
					break;
				}
	#endif
	#ifdef def_test_txrx
				if (txrx_req != txrx_ack)
				{

					DataRequest.DstAddr = txrx_i.SrcAddr;
					unsigned int len = snprintf((char*)DataRequest.Data, sizeof(DataRequest.Data), "%s",txrx_i.Data);
					DataRequest.Len = len;
					txrx_ack = txrx_req;
					handle_app.initDone = 0;
					afDataRequest(&DataRequest);
					handle_app.initDone = 1;
					end_suspend_rx_req ++;
				}
				usleep(1000);
				continue;
	#endif
#endif

			break;
		}
	}

	return 0;
}

