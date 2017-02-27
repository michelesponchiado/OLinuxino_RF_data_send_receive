/*
 * ASACZ_ZAP_Sapi_callbacks.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>



typedef enum
{
	enum_api_sapi_request_device_info = 0,
	enum_api_sapi_numof
}enum_api_sapi;

typedef struct _type_sapi_ackreq
{
	volatile uint32_t num_req;
	volatile uint32_t num_ack;
}type_sapi_ackreq;
typedef struct _type_sapi_handle
{
	type_sapi_ackreq ack_req[enum_api_sapi_numof];
	pthread_mutex_t mtx_struct;
	type_sapi_device_info_struct s;
}type_sapi_handle;

static type_sapi_handle sapi_handle;


uint8_t request_device_info(enum_ZB_INFO e, uint32_t wait_reply_ms, type_sapi_device_info_struct *p_dst)
{
	type_sapi_ackreq *p_ackreq = &sapi_handle.ack_req[enum_api_sapi_request_device_info];
	uint32_t num_ack = p_ackreq->num_ack;
	p_ackreq->num_req++;
	GetDeviceInfoFormat_t req;
	req.Param = e;
	uint8_t status = zbGetDeviceInfo(&req);
	if (status >= 0 && wait_reply_ms)
	{
		if (wait_reply_ms > 10000)
		{
			wait_reply_ms = 10000;
		}
		type_my_timeout timeout_wait;
		initialize_my_timeout(&timeout_wait);
		while(1)
		{
			if (is_my_timeout_elapsed_ms(&timeout_wait, wait_reply_ms))
			{
				status = -1;
				break;
			}
			if (p_ackreq->num_ack != num_ack)
			{
				if (p_dst)
				{
					memcpy(p_dst, &sapi_handle.s, sizeof(*p_dst));
				}
				break;
			}
		}
	}

	return status;
}


uint8_t request_all_device_info(void)
{
	uint8_t status = 0;
	enum_ZB_INFO e;
	for (e = (enum_ZB_INFO)0; e < enum_ZB_INFO_numof; e++)
	{
		if (request_device_info(e, 1000, NULL) < 0)
		{
			status = -1;
		}
	}
	return status;
}


static const char *dev_state_names[]=
{
	"DEV_HOLD", // Initialized - not started automatically
	"DEV_INIT", // Initialized - not connected to anything
	"DEV_NWK_DISC", // Discovering PAN's to join
	"DEV_NWK_JOINING", // Joining a PAN
	"DEV_NWK_REJOIN", // ReJoining a PAN, only for end devices
	"DEV_END_DEVICE_UNAUTH", // Joined but not yet authenticated by trust center
	"DEV_END_DEVICE", // Started as device after authentication
	"DEV_ROUTER", // Device joined, authenticated and is a router
	"DEV_COORD_STARTING", // Started as Zigbee Coordinator
	"DEV_ZB_COORD", // Started as Zigbee Coordinator
	"DEV_NWK_ORPHAN" // Device has lost information about its parent..
};

const char *get_dev_state_string(devStates_t e)
{
	if (e >= sizeof(dev_state_names) / sizeof(dev_state_names[0]))
	{
		return "UNKNOWN";
	}
	return dev_state_names[e];
}


static uint8_t getDeviceInfoSrspCb (GetDeviceInfoSrspFormat_t *msg)
{
	uint8_t ret = SUCCESS;
	pthread_mutex_lock(&sapi_handle.mtx_struct);
		type_sapi_device_info_struct *ps = &sapi_handle.s;
		if (msg->Param >= enum_ZB_INFO_DEV_STATE && msg->Param < enum_ZB_INFO_UNKNOWN)
		{
			ps->e = msg->Param;
			memcpy(&ps->u, msg->Value, sizeof(ps->u));
		}
		else
		{
			memset(&ps->u, 0, sizeof(ps->u));
			ps->e = enum_ZB_INFO_UNKNOWN;
		}
		switch(ps->e)
		{
			case enum_ZB_INFO_DEV_STATE:
			{
				my_log(LOG_INFO, "%s: dev state: %s", __func__, get_dev_state_string(ps->u.dev_state));
				break;
			}
			case enum_ZB_INFO_IEEE_ADDR:
			{
				my_log(LOG_INFO, "%s: IEEE address: 0x%"PRIX64, __func__, ps->u.IEEEaddress);
				break;
			}
			case enum_ZB_INFO_SHORT_ADDR:
			{
				my_log(LOG_INFO, "%s: short address: 0x%X", __func__, (uint32_t)ps->u.shortAddress);
				break;
			}
			case enum_ZB_INFO_PARENT_SHORT_ADDR:
			{
				my_log(LOG_INFO, "%s: parent short address: 0x%X", __func__, (uint32_t)ps->u.parent_shortAddress);
				break;
			}
			case enum_ZB_INFO_PARENT_IEEE_ADDR:
			{
				my_log(LOG_INFO, "%s: parent IEEE address: 0x%"PRIx64, __func__, ps->u.parent_IEEEaddress);
				break;
			}
			case enum_ZB_INFO_CHANNEL:
			{
				my_log(LOG_INFO, "%s: channel: %u", __func__, (uint32_t)ps->u.channel);
				break;
			}
			case enum_ZB_INFO_PAN_ID:
			{
				my_log(LOG_INFO, "%s: PANID: 0x%X", __func__, (uint32_t)ps->u.PANID);
				break;
			}
			case enum_ZB_INFO_EXT_PAN_ID:
			{
				my_log(LOG_INFO, "%s: EXT PANID: 0x%"PRIx64, __func__, ps->u.extPANID);
				break;
			}
			case enum_ZB_INFO_UNKNOWN:
			default:
			{
				my_log(LOG_ERR, "%s: UNKNOWN info %u", (uint32_t)ps->e);
				break;
			}

		}
		type_sapi_ackreq *p_ackreq = &sapi_handle.ack_req[enum_api_sapi_request_device_info];

		p_ackreq->num_ack = p_ackreq->num_req;
	pthread_mutex_unlock(&sapi_handle.mtx_struct);
	return ret;
}

static mtSapiCb_t mtSapiCb=
{
	.pfnSapiReadConfigurationSrsp 	= NULL, //MT_SAPI_READ_CONFIGURATION
	.pfnSapiGetDeviceInfoSrsp 		= getDeviceInfoSrspCb,//MT_SAPI_GET_DEVICE_INFO
	.pfnSapiFindDeviceCnf 			= NULL,//MT_SAPI_FIND_DEVICE_CNF
	.pfnSapiSendDataCnf 			= NULL,//MT_SAPI_SEND_DATA_CNF
	.pfnSapiReceiveDataInd 			= NULL,//MT_SAPI_RECEIVE_DATA_IND
	.pfnSapiAllowBindCnf 			= NULL,//MT_SAPI_ALLOW_BIND_CNF
	.pfnSapiBindCnf 				= NULL,//MT_SAPI_BIND_CNF
	.pfnSapiStartCnf 				= NULL,//MT_SAPI_START_CNF

};

void ASACZ_ZAP_Sapi_callback_register(void)
{
	memset(&sapi_handle, 0, sizeof(sapi_handle));
	pthread_mutex_init(&sapi_handle.mtx_struct, NULL);
	{
		pthread_mutexattr_t mutexattr;

		pthread_mutexattr_init(&mutexattr);
		pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&sapi_handle.mtx_struct, &mutexattr);
	}

	sapiRegisterCallbacks(mtSapiCb);
}
