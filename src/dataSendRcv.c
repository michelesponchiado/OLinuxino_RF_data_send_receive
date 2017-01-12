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
#include <inttypes.h>

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
#include "ASACZ_app.h"
#include "ASAC_ZigBee_network_commands.h"
#include "ASACZ_devices_list.h"
#include "ASACZ_message_history.h"
#include "timeout_utils.h"
#include "dataSendRcv.h"
#include "input_cluster_table.h"

//#define def_test_txrx

#ifdef def_test_txrx
static unsigned int txrx_req;
static unsigned int txrx_ack;
static IncomingMsgFormat_t txrx_i;
#endif


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

#define def_timeout_wait_callbacks_do_ms 10000

static void set_link_quality_current_value(uint8_t v);


static const RegisterFormat_t default_RegisterFormat_t =
{
		.AppProfId = 0x0104,
		.AppDeviceId = 0x0100,
		.AppDevVer = 1,
		.LatencyReq = 0,
		.EndPoint = 0xf0,
		.AppNumInClusters = 1,
		.AppInClusterList[0] = 0x0001,
		.AppNumOutClusters = 1,
		.AppOutClusterList[0] = 0x0001,
};


typedef enum
{
	enum_app_error_code_none,
	enum_app_error_code_too_many_messages_flushed,
	enum_app_error_code_unable_to_start_network,
	enum_app_error_code_unable_to_sysGetExtAddr,
	enum_app_error_code_unable_to_sysOsalNvWrite,
	enum_app_error_code_unable_to_set_TX_power,
	enum_app_error_code_timeout_waiting_init_callbacks,
	enum_app_error_code_numof
}enum_app_error_code;

typedef struct _type_handle_app_Tx_power
{
	unsigned int valid;
	int power_dbM;
}type_handle_app_Tx_power;
static void invalidate_Tx_power(type_handle_app_Tx_power *p)
{
	memset(p, 0, sizeof(*p));
}
static void set_Tx_power(type_handle_app_Tx_power *p, int power_dbM)
{
	p->power_dbM = power_dbM;
	p->valid = 1;
}
static int get_Tx_power(type_handle_app_Tx_power *p)
{
	if (p->valid)
	{
		return p->power_dbM;
	}
	return -1000;
}
static unsigned int is_valid_Tx_power(type_handle_app_Tx_power *p)
{
	return p->valid;
}


typedef struct _type_handle_app_IEEE_address
{
	unsigned int valid;
	uint64_t address;
}type_handle_app_IEEE_address;

static void invalidate_IEEE_address(type_handle_app_IEEE_address *p)
{
	memset(p, 0, sizeof(*p));
}
static void set_IEEE_address(type_handle_app_IEEE_address *p, uint64_t address)
{
	p->address = address;
	p->valid = 1;
}
static uint64_t get_IEEE_address(type_handle_app_IEEE_address *p)
{
	if (p->valid)
	{
		return p->address;
	}
	return 0;
}
static unsigned int is_valid_IEEE_address(type_handle_app_IEEE_address *p)
{
	return p->valid;
}



static char * get_link_quality_string(enum_link_quality_level level)
{
	char *pc = "unknown";
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
	return pc;
}



typedef struct _type_link_quality
{
	pthread_mutex_t mtx_id;
	type_link_quality_body b; // the epoch time when the link quality was updated
}type_link_quality;

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
	type_handle_app_IEEE_address IEEE_address;
	type_handle_app_Tx_power Tx_power;
	type_my_timeout my_timeout;
	unsigned int channel_index; // channel index, between 11 and 26
	unsigned int network_restart_req;
	unsigned int network_restart_ack;
	type_link_quality link_quality;
}type_handle_app;
static type_handle_app handle_app;
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

void init_handle_app(void)
{
	memset(&handle_app, 0, sizeof(handle_app));
	handle_app.devState = DEV_HOLD;
	handle_app.channel_index = 11;
	pthread_mutex_init(&handle_app.mtx_id, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&handle_app.mtx_id, &mutexattr);
	}
	init_link_quality(&handle_app.link_quality);
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
static uint8_t mtZdoIEEEAddrRspCb(IeeeAddrRspFormat_t *msg);
static uint8_t mtZdoActiveEpRspCb(ActiveEpRspFormat_t *msg);
static uint8_t mtZdoEndDeviceAnnceIndCb(EndDeviceAnnceIndFormat_t *msg);

static uint8_t mtZdoMgmtLqiRspCb(MgmtLqiRspFormat_t *msg);

//SYS Callbacks

static uint8_t mtSysResetIndCb(ResetIndFormat_t *msg);
static uint8_t mtSysSetTxPowerSrspCb(SetTxPowerSrspFormat_t *msg);
static uint8_t mtSysGetExtAddrSrsp(GetExtAddrSrspFormat_t *msg);

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
static int32_t startNetwork(unsigned int channel_index);
static int32_t registerAf_default(void);

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
	{ 		.pfnSysPingSrsp 		= NULL,
			.pfnSysGetExtAddrSrsp 	= mtSysGetExtAddrSrsp,
			.pfnSysRamReadSrsp 		= NULL,
			.pfnSysResetInd 		= mtSysResetIndCb,
			.pfnSysVersionSrsp 		= NULL,
			.pfnSysOsalNvReadSrsp 	= NULL,
			.pfnSysOsalNvLengthSrsp = NULL,
			.pfnSysOsalTimerExpired = NULL,
			.pfnSysStackTuneSrsp 	= NULL,
			.pfnSysAdcReadSrsp 		= NULL,
			.pfnSysGpioSrsp 		= NULL,
			.pfnSysRandomSrsp 		= NULL,
			.pfnSysGetTimeSrsp 		= NULL,
			.pfnSysSetTxPowerSrsp 	= mtSysSetTxPowerSrspCb,
	};


static mtZdoCb_t mtZdoCb =
	{ NULL,       // MT_ZDO_NWK_ADDR_RSP
			mtZdoIEEEAddrRspCb,      // MT_ZDO_IEEE_ADDR_RSP
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

static mtSapiCb_t mtSapiCb=
{
	.pfnSapiReadConfigurationSrsp 	= NULL, //MT_SAPI_READ_CONFIGURATION
	.pfnSapiGetDeviceInfoSrsp 		= NULL,//MT_SAPI_GET_DEVICE_INFO
	.pfnSapiFindDeviceCnf 			= NULL,//MT_SAPI_FIND_DEVICE_CNF
	.pfnSapiSendDataCnf 			= NULL,//MT_SAPI_SEND_DATA_CNF
	.pfnSapiReceiveDataInd 			= NULL,//MT_SAPI_RECEIVE_DATA_IND
	.pfnSapiAllowBindCnf 			= NULL,//MT_SAPI_ALLOW_BIND_CNF
	.pfnSapiBindCnf 				= NULL,//MT_SAPI_BIND_CNF
	.pfnSapiStartCnf 				= NULL,//MT_SAPI_START_CNF

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


// callback from MT_SYS_GET_EXTADDR
static uint8_t mtSysGetExtAddrSrsp(GetExtAddrSrspFormat_t *msg)
{
	set_IEEE_address(&handle_app.IEEE_address, msg->ExtAddr);
	my_log(LOG_INFO, "IEEE address: 0x%lX\n", (long int)get_IEEE_address(&handle_app.IEEE_address));
	return 0;
}

static uint8_t mtSysSetTxPowerSrspCb(SetTxPowerSrspFormat_t *msg)
{
	set_Tx_power(&handle_app.Tx_power, (int)msg->TxPower);
	my_log(LOG_INFO, "Radio power callback: set to %i dBm\n", get_Tx_power(&handle_app.Tx_power));
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
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Initialized - not started automatically\n");
		break;
	case DEV_INIT:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Initialized - not connected to anything\n");
		break;
	case DEV_NWK_DISC:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Discovering PAN's to join\n");
		consolePrint("Network Discovering\n");
		break;
	case DEV_NWK_JOINING:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Joining a PAN\n");
		consolePrint("Network Joining\n");
		break;
	case DEV_NWK_REJOIN:
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: ReJoining a PAN, only for end devices\n");
		consolePrint("Network Rejoining\n");
		break;
	case DEV_END_DEVICE_UNAUTH:
		consolePrint("Network Authenticating\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Joined but not yet authenticated by trust center\n");
		break;
	case DEV_END_DEVICE:
		consolePrint("Network Joined\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Started as device after authentication\n");
		break;
	case DEV_ROUTER:
		consolePrint("Network Joined\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Device joined, authenticated and is a router\n");
		break;
	case DEV_COORD_STARTING:
		consolePrint("Network Starting\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
		break;
	case DEV_ZB_COORD:
		consolePrint("Network Started\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Started as Zigbee Coordinator\n");
		break;
	case DEV_NWK_ORPHAN:
		consolePrint("Network Orphaned\n");
		my_log(LOG_INFO,"mtZdoStateChangeIndCb: Device has lost information about its parent\n");
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
					stats.device_list.clusters_output.ERR++;
				}
				else
				{
					stats.device_list.clusters_output.OK++;
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
			consolePrint("%s: ERROR ADDING DEVICE @ IEEE Address 0x%"  PRIx64 " / network address 0x%X\n", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			stats.device_list.add_header.ERR++;
		}
		else
		{
			consolePrint("%s: *** Device @ IEEE Address 0x%"  PRIx64 " / network address 0x%X added OK\n", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			stats.device_list.add_header.OK++;
		}
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
		my_log(LOG_INFO, "Device 0x%" PRIx64 " just joined the network", device_header.IEEE_address);
		enum_add_ASACZ_device_list_header_retcode r = add_ASACZ_device_list_header(&device_header);
		if (r != enum_add_ASACZ_device_list_header_retcode_OK)
		{
			consolePrint("%s: ERROR ADDING DEVICE @ IEEE Address 0x%"  PRIx64 " / network address 0x%X\n", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
			stats.device_list.add_header.ERR++;
		}
		else
		{
			consolePrint("%s: *** Device @ IEEE Address 0x%"  PRIx64 " / network address 0x%X added OK\n", __func__, device_header.IEEE_address, (unsigned int)device_header.network_short_address);
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
		my_log(LOG_INFO, "Data confirm cb OK: end-point: %u, trans-id: %u", (uint32_t)msg->Endpoint, (uint32_t)msg->TransId);
		message_history_tx_set_trans_id_status(msg->TransId, enum_message_history_status_dataconfirm_received);
	}
	else
	{
		my_log(LOG_ERR, "Data confirm cb ERR: end-point: %u, trans-id: %u", (uint32_t)msg->Endpoint, (uint32_t)msg->TransId);
		message_history_tx_set_trans_id_error(msg->TransId, enum_message_history_error_DATACONFIRM);
	}
	return msg->Status;
}


static uint8_t mtAfIncomingMsgCb(IncomingMsgFormat_t *msg)
{
	uint8_t retcode = SUCCESS;
	// incoming message received, we put it in the received messages queue
#if 1
	uint64_t IEEE_address;
	consolePrint("%s: message from device @ Short Address 0x%X\n", __func__,(unsigned int)msg->SrcAddr);
	set_link_quality_current_value(msg->LinkQuality);
	consolePrint("%s: link quality: %s (%u / %i dBm)\n", __func__
			, get_app_current_link_quality_string()
			, (unsigned int )get_app_current_link_quality_value_energy_detected()
			, (int )get_app_current_link_quality_value_dBm()
			);

	if (!is_OK_get_IEEE_from_network_short_address(msg->SrcAddr, &IEEE_address, enum_device_lifecycle_action_do_refresh_rx))
	{
		consolePrint("%s: UNKNOWN DEVICE @ Short Address 0x%X\n", __func__,(unsigned int)msg->SrcAddr);
		my_log(LOG_ERR, "Unknown short address 0x%X on incoming message", (uint32_t)msg->SrcAddr);
		retcode = FAILURE;
	}
	else
	{
		consolePrint("%s: the device is @ IEEE 0x%"  PRIx64 ", Short Address 0x%X\n", __func__,IEEE_address, (unsigned int)msg->SrcAddr);
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
			consolePrint("%s: ERROR PUTTING message in outside queue\n", __func__);
			my_log(LOG_ERR, "Unable to put received message in rx queue");
			retcode = FAILURE;
		}
		else
		{
			consolePrint("%s: message put OK in outside queue\n", __func__);
		}
	}
#else

	msg->Data[msg->Len] = '\0';
#ifdef def_test_txrx
	if (strncmp((char*)msg->Data,"TXRX",4) == 0 )
	{
		txrx_i = *msg;
		txrx_req ++;
	}
	else
#endif
	{
		consolePrint("%s\n", (char*) msg->Data);
		consolePrint(
		        "\nIncoming Message from Endpoint 0x%02X and Address 0x%04X:\n",
		        msg->SrcEndpoint, msg->SrcAddr);
		consolePrint(
		        "\nEnter message to send or type CHANGE to change the destination \nor QUIT to exit:\n");
	}
#endif
	return retcode;
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
static void refresh_my_endpoint_list(void)
{
	uint16_t my_short_address;
	my_log(1,"%s: +\n", __func__);
	if (!is_OK_get_network_short_address_from_IEEE(handle_app.IEEE_address.address, &my_short_address))
	{
		printf("%s: ERROR unable to refresh my end point list\n", __func__);
		my_log(1,"%s: ERROR unable to refresh my end point list\n", __func__);
	}
	else
	{
		printf("%s: refreshing my end point list...\n", __func__);
		my_log(1,"%s: refreshing my end point list...\n", __func__);
		ActiveEpReqFormat_t actReq;
		actReq.DstAddr = my_short_address;
		actReq.NwkAddrOfInterest = my_short_address;
		zdoActiveEpReq(&actReq);
		rpcWaitMqClientMsg(15000); //rpcGetMqClientMsg();
		int32_t status;
		int nloop = 0;
		do
		{
			status = rpcWaitMqClientMsg(1000);
			my_log(1,"%s: status = %i, nloop = %i\n", __func__, status, nloop);
		} while (status != -1 && (++nloop < 30));
	}
	my_log(1,"%s: -\n", __func__);
}


uint8_t dType;

void register_user_end_points(void)
{
	uint8_t prev_end_point = 0;
	uint32_t nloop = 0;
	printf("%s: + registering end_point\n", __func__);
	my_log(LOG_INFO, "%s: + registering end_point\n", __func__);
	while(++nloop < 256)
	{
		type_Af_user u;

		enum_get_next_end_point_command_list_retcode r = get_next_end_point_command_list(prev_end_point, &u);
		if (r != enum_get_next_end_point_command_list_retcode_OK)
		{
			break;
		}
		prev_end_point = u.EndPoint;
		printf("%s: registering end_point %u (%u commands)\n", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
		my_log(LOG_INFO,"%s: registering end_point %u (%u commands)\n", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
		if (!is_OK_registerAf_user(&u))
		{
			printf("%s: ERROR registering end_point %u (%u commands)\n", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
			my_log(LOG_INFO,"%s: ERROR registering end_point %u (%u commands)\n", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);
		}
		else
		{
			my_log(LOG_INFO,"%s: OK registered end_point %u (%u commands)\n", __func__, (unsigned int)prev_end_point, (unsigned int )u.AppNumInClusters);

		}

	}
	printf("%s: refrshing the end point list\n", __func__);
	printf("%s: - registering end_point\n", __func__);
	my_log(LOG_INFO,"%s: - registering end_point\n", __func__);
}


static int32_t restartNetwork(void)
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
		printf("%s: ERROR network startup failed\n", __func__);
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
#define def_fix_channel 11
	my_log(LOG_INFO, "Setting the Channel %i", def_fix_channel);
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
		dbg_print(PRINT_LEVEL_INFO, "zdoInit NEW_NETWORK\n");
		my_log(LOG_WARNING, "%s: zdoInit NEW_NETWORK", __func__);
		status = MT_RPC_SUCCESS;
	}
	else if (status == RESTORED_NETWORK)
	{
		dbg_print(PRINT_LEVEL_INFO, "zdoInit RESTORED_NETWORK\n");
		my_log(LOG_WARNING, "%s: zdoInit RESTORED_NETWORK", __func__);
		status = MT_RPC_SUCCESS;
	}
	else
	{
		my_log(LOG_WARNING, "%s: zdoInit failed", __func__);
		printf("%s: ERROR zdoInit failed\n", __func__);
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
			printf(" *** permit join req ERROR: %i\n", (int)retcode);
			my_log(LOG_INFO, "%s: permit join req ko", __func__);
		}
		else
		{
			printf("permit join req OK\n");
			my_log(LOG_INFO, "%s: permit join req OK", __func__);
		}
	}

	//set startup option back to keep configuration in case of reset
	status = setNVStartup(0);
	if (handle_app.devState < DEV_END_DEVICE)
	{
		my_log(LOG_WARNING, "%s: network restart failed", __func__);
		printf("%s: ERROR network restart failed\n", __func__);
		//start network failed
		return -1;
	}

	my_log(LOG_INFO, "%s: network restart OK", __func__);
	printf("%s: network restart OK\n", __func__);

	return 0;
}


static int32_t startNetwork(unsigned int channel_index)
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
	my_log(LOG_INFO, "starting a new network");
	status = setNVStartup(ZCD_STARTOPT_CLEAR_STATE | ZCD_STARTOPT_CLEAR_CONFIG);
	newNwk = 1;
#else
	status = setNVStartup(0);
#endif
#endif
	if (status != MT_RPC_SUCCESS)
	{
		my_log(LOG_WARNING, "network startup failed");
		return -1;
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
		my_log(LOG_INFO, "Setting the PAN ID");
		status = setNVPanID(0xFFFF);
		if (status != MT_RPC_SUCCESS)
		{
			my_log(LOG_WARNING, "setNVPanID failed");
			return -1;
		}
#ifdef def_interactive_chan
		consolePrint("Enter channel 11-26:\n");
		consoleGetLine(sCh, 128);
		status = setNVChanList(1 << atoi(sCh));
#else
		// this must be between 11 and 26
#define def_fix_channel 11
#warning always using fixed channel
		my_log(LOG_INFO, "Setting the Channel %i", def_fix_channel);
		status = setNVChanList(1 << def_fix_channel);
#endif
		if (status != MT_RPC_SUCCESS)
		{
			my_log(LOG_WARNING, "setNVChanList failed on channel %i", def_fix_channel);
			return -1;
		}

	}

	my_log(LOG_INFO, "registering the default end point/command");
	registerAf_default();

	my_log(LOG_INFO, "zdo init");
	status = zdoInit();
	if (status == NEW_NETWORK)
	{
		my_log(LOG_INFO, "zdoInit NEW_NETWORK\n");
		status = MT_RPC_SUCCESS;
	}
	else if (status == RESTORED_NETWORK)
	{
		my_log(LOG_INFO, "zdoInit RESTORED_NETWORK\n");
		status = MT_RPC_SUCCESS;
	}
	else
	{
		my_log(LOG_WARNING, "zdoInit failed\n");
		status = -1;
	}

	my_log(LOG_INFO, "process zdoStatechange callbacks\n");

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
	if (1)
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
	if (handle_app.devState < DEV_END_DEVICE)
	{
		my_log(LOG_WARNING, "setNVStartup failed with devState: %i", (int)handle_app.devState);
		//start network failed
		return -1;
	}

	my_log(LOG_INFO, "startNetwork ends OK");
	return 0;
}



static int32_t registerAf_default(void)
{
	int32_t status = 0;
	RegisterFormat_t reg;
	memcpy(&reg, &default_RegisterFormat_t, sizeof(reg));

	status = afRegister(&reg);
	return status;
}


void require_network_restart(void)
{
	handle_app.network_restart_req++;
}
unsigned int is_required_network_restart(void)
{
	unsigned int is_required =0;
	if (handle_app.network_restart_req != handle_app.network_restart_ack)
	{
		handle_app.network_restart_ack = handle_app.network_restart_req;
		is_required = 1;
	}
	return is_required;
}
unsigned int is_OK_registerAf_user(type_Af_user *p)
{
	unsigned int is_OK = 1;
	if (is_OK)
	{
		if (p->AppNumInClusters > sizeof(p->AppInClusterList)/sizeof(p->AppInClusterList[0]))
		{
			is_OK = 0;
		}
	}
	if (is_OK)
	{
		if (p->AppNumOutClusters > sizeof(p->AppOutClusterList)/sizeof(p->AppOutClusterList[0]))
		{
			is_OK = 0;
		}
	}
	if (is_OK)
	{

		printf("%s: refreshing my point list @endpoint %u...\n", __func__, (unsigned int)p->EndPoint);
		my_log(1,"%s: refreshing my point list @endpoint %u...\n", __func__, (unsigned int)p->EndPoint);
		RegisterFormat_t reg;
		memcpy(&reg, &default_RegisterFormat_t, sizeof(reg));
		reg.EndPoint = p->EndPoint;
		{
			reg.AppNumInClusters = p->AppNumInClusters;
			int i;
			for (i =0; i < reg.AppNumInClusters; i++)
			{
				reg.AppInClusterList[i] = p->AppInClusterList[i];
			}
		}
		{
			reg.AppNumOutClusters = p->AppNumOutClusters;
			int i;
			for (i =0; i < reg.AppNumOutClusters; i++)
			{
				reg.AppOutClusterList[i] = p->AppOutClusterList[i];
			}
		}

		{
			int32_t status = 0;
			my_log(1,"%s: about to call register af user\n", __func__, (unsigned int)p->EndPoint);
			status = afRegister(&reg);
			if (status != SUCCESS)
			{
				my_log(1,"%s: error afregister\n", __func__);
				is_OK = 0;
			}
			else
				my_log(1,"%s: OK afregister\n", __func__);
		}

	}
	{
		if (is_OK)
		{
			printf("%s: ends OK\n", __func__);
			my_log(1,"%s: ends OK\n", __func__);
		}
		else
		{
			printf("%s: ERROR, ends non OK\n", __func__);
			my_log(1,"%s: ERROR, ends non OK\n", __func__);
		}

	}
	return is_OK;
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
		// asks info about the node, if coordinator, so we can add it to the device list
		//if (nodeList[i].Type == DEVICETYPE_COORDINATOR)
		{
			IeeeAddrReqFormat_t IEEEreq;
			IEEEreq.ShortAddr = nodeList[i].NodeAddr;
			IEEEreq.ReqType = 0; // signle device response
			IEEEreq.StartIndex = 0; // start from first
			zdoIeeeAddrReq(&IEEEreq);
			rpcWaitMqClientMsg(200);
		}

		consolePrint("Type: %s\n", devtype);
		actReq.DstAddr = nodeList[i].NodeAddr;
		actReq.NwkAddrOfInterest = nodeList[i].NodeAddr;
		zdoActiveEpReq(&actReq);
//		rpcGetMqClientMsg();
		rpcWaitMqClientMsg(15000); 
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

				// asks info about the node, so we can add it to the device list
				{
					IeeeAddrReqFormat_t IEEEreq;
					IEEEreq.ShortAddr = nodeList[i].childs[cI].ChildAddr;
					IEEEreq.ReqType = 0; // signle device response
					IEEEreq.StartIndex = 0; // start from first
					zdoIeeeAddrReq(&IEEEreq);
					rpcWaitMqClientMsg(200);
					while (status != -1)
					{
						status = rpcWaitMqClientMsg(200);
					}
				}

				actReq.DstAddr = nodeList[i].childs[cI].ChildAddr;
				actReq.NwkAddrOfInterest = nodeList[i].childs[cI].ChildAddr;
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
		consolePrint("\n");

	}
}

uint8_t set_TX_power(void)
{
	uint8_t retcode = MT_RPC_SUCCESS;
	int8_t pwr_required_dbm = 2;
	SetTxPowerFormat_t req = {0};
	*(int8_t*)&req.TxPower = pwr_required_dbm;
	my_log(LOG_INFO, "setting the TX power to %i dBm\n", (int)pwr_required_dbm);
	retcode = sysSetTxPower(&req);
	switch(sysSetTxPower(&req))
	{
		case MT_RPC_SUCCESS:
		{
			my_log(LOG_INFO, "TX power sent result: %s\n", "OK");
			break;
		}
		default:
		{
			my_log(LOG_ERR, "TX power sent result: %s\n", "**ERROR**");
			break;
		}
	}
	return retcode;
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
	sysRegisterCallbacks(mtSysCb);
	zdoRegisterCallbacks(mtZdoCb);
	afRegisterCallbacks(mtAfCb);
	sapiRegisterCallbacks(mtSapiCb);

	init_handle_app();
	// initialize the handle app stati machine
	start_handle_app();


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

uint32_t is_OK_get_my_radio_IEEE_address(uint64_t *p)
{
	volatile type_handle_app_IEEE_address ieee_1;
	volatile type_handle_app_IEEE_address ieee_2;
	unsigned int i;
	unsigned int found = 0;
	for (i = 0; i < 20; i++)
	{
		ieee_1 = handle_app.IEEE_address;
		ieee_2 = handle_app.IEEE_address;
		if(memcmp((const void *)&ieee_1, (const void *)&ieee_2, sizeof(ieee_1)) == 0)
		{
			found = 1;
			break;
		}
	}
	if (!found || !is_valid_IEEE_address((type_handle_app_IEEE_address *)&ieee_1))
	{
		return 0;
	}
	*p = ieee_1.address;
	return 1;
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



void* appProcess(void *argument)
{
	usleep(1000);
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
			if (is_required_network_restart())
			{
				handle_app.status = enum_app_status_restart_network;
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
			handle_app.devState = DEV_HOLD;
			handle_app.status = enum_app_status_flush_messages_init;
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
		case enum_app_status_restart_network:
		{
			handle_app.devState = DEV_HOLD;

			int32_t status = restartNetwork();
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

			int32_t status = startNetwork(handle_app.channel_index);
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
			if (is_valid_IEEE_address(&handle_app.IEEE_address) && is_valid_Tx_power(&handle_app.Tx_power))
			{
				consolePrint("My IEEE Address is: 0x%" PRIx64 "\n", handle_app.IEEE_address.address);
				//refresh my end point list
				refresh_my_endpoint_list();

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
			consolePrint("My IEEE Address is: 0x%" PRIx64 "\n", handle_app.IEEE_address.address);
			my_log(LOG_INFO, "Display device OK, going to rx/tx mode");
// wait until a device shows up?
#warning better to remove this default end point / destination address
			handle_app.DataRequest.DstAddr 		= (uint16_t) 0xfa24;
			handle_app.DataRequest.DstEndpoint 	= (uint8_t) default_RegisterFormat_t.EndPoint;
			handle_app.DataRequest.SrcEndpoint 	= default_RegisterFormat_t.EndPoint;
			handle_app.DataRequest.ClusterID 	= default_RegisterFormat_t.AppInClusterList[0];
			handle_app.DataRequest.TransID 		= 0;
			handle_app.DataRequest.Options 		= 0;
			handle_app.DataRequest.Radius 		= 0xEE;
			handle_app.initDone = 1;
			handle_app.status = enum_app_status_rx_msgs;
			break;
		}
		case enum_app_status_rx_msgs:
		{
			handle_app.status = enum_app_status_tx_msgs;
			break;
		}
		case enum_app_status_tx_msgs:
		{
			if (is_required_network_restart())
			{
				handle_app.status = enum_app_status_restart_network;
				break;
			}
			handle_app.status = enum_app_status_rx_msgs;
#define def_use_only_zigbee_queue_messages
#ifdef def_use_only_zigbee_queue_messages
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
						consolePrint("%s: ERROR UNKNOWN DST @ IEEE Address 0x%" PRIx64 "\n", __func__, m.dst_id.IEEE_destination_address);
						message_history_tx_set_error(handle_app.message_id, enum_message_history_error_unknown_IEEE_address);
						my_log(LOG_ERR, "Unable to find device with IEEE address: %" PRIx64 "", m.dst_id.IEEE_destination_address);
					}
					else
					{
						consolePrint("%s: sending DST @ IEEE Address 0x%" PRIx64 "\n", __func__, m.dst_id.IEEE_destination_address);
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
							consolePrint("%s: ERROR sending DST @ IEEE Address 0x%" PRIx64 "\n", __func__, m.dst_id.IEEE_destination_address);
							// mark the data request has been sent OK
							message_history_tx_set_id_status(handle_app.message_id, enum_message_history_status_datareq_sent);
							my_log(LOG_ERR, "Unable to send message with id = %u", handle_app.message_id);
						}
						else
						{
							consolePrint("%s: sent OK @ IEEE Address 0x%" PRIx64 "\n", __func__, m.dst_id.IEEE_destination_address);
							my_log(LOG_INFO, "Message with id = %u sent OK", handle_app.message_id);
						}
					}
				}
			}
			else
			{
				//my_log(LOG_ERR, "Unable to pop a message from the Zigbee queue");
			}

#else
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

