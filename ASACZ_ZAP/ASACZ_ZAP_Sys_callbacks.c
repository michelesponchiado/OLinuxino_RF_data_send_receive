/*
 * ASACZ_ZAP_Sys_callbacks.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_IEEE_address.h"
#include "../ASACZ_ZAP/ASACZ_ZAP_TX_power.h"

// callback from MT_SYS_GET_EXTADDR
uint8_t mtSysGetExtAddrSrsp(GetExtAddrSrspFormat_t *msg)
{
	set_IEEE_address(&handle_app.IEEE_address, msg->ExtAddr);
	my_log(LOG_INFO, "IEEE address: 0x%lX", (long int)get_IEEE_address(&handle_app.IEEE_address));
	return 0;
}

uint8_t mtSysSetTxPowerSrspCb(SetTxPowerSrspFormat_t *msg)
{
	set_Tx_power(&handle_app.Tx_power, (int)msg->TxPower);
	my_log(LOG_INFO, "Radio power callback: set to %i dBm", get_Tx_power(&handle_app.Tx_power));
	return 0;
}

uint8_t mtSysResetIndCb(ResetIndFormat_t *msg)
{

	dbg_print(PRINT_LEVEL_INFO, "ZNP Version: %d.%d.%d", msg->MajorRel, msg->MinorRel, msg->HwRev);
	return 0;
}


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

void ASACZ_ZAP_Sys_callback_register(void)
{
	sysRegisterCallbacks(mtSysCb);
}
