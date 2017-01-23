/*
 * TX_power.c
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */


#include <ASACZ_ZAP.h>
#include "../ASACZ_ZAP/ASACZ_ZAP_TX_power.h"


void invalidate_Tx_power(type_handle_app_Tx_power *p)
{
	memset(p, 0, sizeof(*p));
}
void set_Tx_power(type_handle_app_Tx_power *p, int power_dbM)
{
	p->power_dbM = power_dbM;
	p->valid = 1;
}
int get_Tx_power(type_handle_app_Tx_power *p)
{
	if (p->valid)
	{
		return p->power_dbM;
	}
	return -1000;
}
unsigned int is_valid_Tx_power(type_handle_app_Tx_power *p)
{
	return p->valid;
}

uint8_t set_TX_power(void)
{
	uint8_t retcode = MT_RPC_SUCCESS;
	int8_t pwr_required_dbm = 2;
	SetTxPowerFormat_t req = {0};
	*(int8_t*)&req.TxPower = pwr_required_dbm;
	my_log(LOG_INFO, "setting the TX power to %i dBm", (int)pwr_required_dbm);
	retcode = sysSetTxPower(&req);
	// removed double set TX power
	switch(retcode)
	{
		case MT_RPC_SUCCESS:
		{
			my_log(LOG_INFO, "TX power sent result: %s", "OK");
			break;
		}
		default:
		{
			my_log(LOG_ERR, "TX power sent result: %s", "**ERROR**");
			break;
		}
	}
	return retcode;
}
