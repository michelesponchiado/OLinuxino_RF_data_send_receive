/*
 * ASACZ_ZAP.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_H_
#define ASACZ_ZAP_H_

#include <ASAC_ZigBee_network_commands.h>
#include <pthread.h>
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
#include "timeout_utils.h"

#include "ASACZ_app.h"
#include "ASACZ_devices_list.h"
#include "ASACZ_message_history.h"
#include "ASACZ_fw_update.h"
#include "dataSendRcv.h"
#include "input_cluster_table.h"
#include "my_queues.h"
#include "ZigBee_messages.h"


typedef enum
{
	// Device Info Constants
	enum_ZB_INFO_THE_VERY_FIRST = 0			,
	enum_ZB_INFO_DEV_STATE  = enum_ZB_INFO_THE_VERY_FIRST               ,
	enum_ZB_INFO_IEEE_ADDR                 ,
	enum_ZB_INFO_SHORT_ADDR                ,
	enum_ZB_INFO_PARENT_SHORT_ADDR         ,
	enum_ZB_INFO_PARENT_IEEE_ADDR          ,
	enum_ZB_INFO_CHANNEL                   ,
	enum_ZB_INFO_PAN_ID                    ,
	enum_ZB_INFO_EXT_PAN_ID                ,
	enum_ZB_INFO_UNKNOWN 					,
	enum_ZB_INFO_numof						,
}enum_ZB_INFO;

typedef struct _type_device_info_struct
{
	enum_ZB_INFO e;
	union
	{
		uint8_t dev_state;
		uint64_t IEEEaddress;
		uint16_t shortAddress;
		uint16_t parent_shortAddress;
		uint64_t parent_IEEEaddress;
		uint8_t channel;
		uint16_t PANID;
		uint64_t extPANID;
	}u;
}type_sapi_device_info_struct;

typedef struct _type_all_sapi_device_info_struct
{
	type_sapi_device_info_struct i[enum_ZB_INFO_numof];
}type_all_sapi_device_info_struct;


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

typedef struct _type_handle_app_IEEE_address
{
	unsigned int valid;
	uint64_t address;
}type_handle_app_IEEE_address;


typedef struct _type_link_quality
{
	pthread_mutex_t mtx_id;
	type_link_quality_body b; // the epoch time when the link quality was updated
}type_link_quality;

typedef struct _type_handle_app_Tx_power
{
	unsigned int valid;
	int power_dbM;
}type_handle_app_Tx_power;

typedef struct _type_handle_end_point_update
{
	int64_t last_update_epoch_ms;
	unsigned int call_back_req;
	unsigned int call_back_ack;
	unsigned int call_back_return_code;
}type_handle_end_point_update;


typedef struct _type_leave
{
	unsigned int isOK;
	unsigned int num_ack, num_req;
}type_leave;


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

typedef struct _type_handle_node_list
{
	Node_t nodeList[64];
	uint8_t nodeCount;
}type_handle_node_list;

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

typedef struct _type_ZAP_stats
{
	type_device_list_stats device_list;
}type_ZAP_stats;

typedef struct _type_CC2650_version
{
	uint32_t is_valid;
	VersionSrspFormat_t version;
}type_CC2650_version;

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
	unsigned int network_restart_from_scratch_req;
	unsigned int network_restart_from_scratch_ack;
	type_link_quality link_quality;
	type_handle_end_point_update handle_end_point_update;
	type_leave leave;
	type_handle_node_list handle_node_list;
	unsigned int force_shutdown;
	type_ZAP_stats stats;
	uint32_t suspend_rx_tasks_idx;
	uint32_t suspend_rx_tasks;
	uint32_t InMessageTaskSuspended;
	uint32_t rpcTaskSuspended;
	uint32_t num_loop_wait_suspend_rx_thread;
	type_my_timeout timeout_init_fw_update;
	type_all_sapi_device_info_struct all_sapi_device_info_struct;
	type_CC2650_version CC2650_version;
	type_CC2650_fw_update_handle CC2650_fw_update_handle;
}type_handle_app;

extern type_handle_app handle_app;



uint8_t request_all_device_info(type_all_sapi_device_info_struct *p_all_sapi_device_info_struct);
uint8_t request_device_info(enum_ZB_INFO e, uint32_t wait_reply_ms, type_sapi_device_info_struct *p_dst);
const char *get_dev_state_string(devStates_t e);


void *rpcTask(void *argument);
void *appInMessageTask(void *argument);


#endif /* ASACZ_ZAP_H_ */
