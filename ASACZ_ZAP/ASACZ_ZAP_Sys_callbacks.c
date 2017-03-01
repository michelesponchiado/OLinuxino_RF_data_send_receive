/*
 * ASACZ_ZAP_Sys_callbacks.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#include <ASACZ_ZAP.h>
#include "ASACZ_ZAP_IEEE_address.h"
#include "ASACZ_ZAP_TX_power.h"

// callback from MT_SYS_GET_EXTADDR
uint8_t mtSysGetExtAddrSrsp(GetExtAddrSrspFormat_t *msg)
{
	set_IEEE_address(&handle_app.IEEE_address, msg->ExtAddr);
	my_log(LOG_INFO, "IEEE address: 0x%" PRIx64, get_IEEE_address(&handle_app.IEEE_address));
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

uint8_t mtSysVersionSrsp(VersionSrspFormat_t *msg)
{
	my_log(LOG_INFO, "RADIO CHIP FIRMWARE VERSION %u.%u.%u ; product is %u, transport version is %u",
		   	  (uint32_t)msg->MajorRel
			, (uint32_t)msg->MinorRel
			, (uint32_t)msg->MaintRel
			, (uint32_t)msg->Product
			, (uint32_t)msg->TransportRev
			);
	VersionSrspFormat_t *p_dst = &handle_app.CC2650_version.version;
	memcpy(p_dst, msg, sizeof(*p_dst));
	handle_app.CC2650_version.is_valid = 1;
	return SUCCESS;
}

void get_CC2650_firmware_version(type_fwupd_CC2650_read_version_reply_body *p_dst)
{
	VersionSrspFormat_t *p_src = &handle_app.CC2650_version.version;
	p_dst->is_valid = handle_app.CC2650_version.is_valid;
	p_dst->major 	= p_src->MajorRel;
	p_dst->middle 	= p_src->MinorRel;
	p_dst->minor 	= p_src->MaintRel;
	p_dst->product 	= p_src->Product;
	p_dst->transport= p_src->TransportRev;
}


// SYS callbacks
static mtSysCb_t mtSysCb =
	{ 		.pfnSysPingSrsp 		= NULL,
			.pfnSysGetExtAddrSrsp 	= mtSysGetExtAddrSrsp,
			.pfnSysRamReadSrsp 		= NULL,
			.pfnSysResetInd 		= mtSysResetIndCb,
			.pfnSysVersionSrsp 		= mtSysVersionSrsp,
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
