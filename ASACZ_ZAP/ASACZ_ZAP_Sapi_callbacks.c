/*
 * ASACZ_ZAP_Sapi_callbacks.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */


#include <ASACZ_ZAP.h>
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

void ASACZ_ZAP_Sapi_callback_register(void)
{
	sapiRegisterCallbacks(mtSapiCb);
}
