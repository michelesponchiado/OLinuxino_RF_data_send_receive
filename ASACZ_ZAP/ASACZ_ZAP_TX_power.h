/*
 * ASACZ_ZAP_TX_power.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_TX_POWER_H_
#define ASACZ_ZAP_ASACZ_ZAP_TX_POWER_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

void invalidate_Tx_power(type_handle_app_Tx_power *p);
void set_Tx_power(type_handle_app_Tx_power *p, int power_dbM);
int get_Tx_power(type_handle_app_Tx_power *p);
unsigned int is_valid_Tx_power(type_handle_app_Tx_power *p);
uint8_t set_TX_power(void);

#endif /* ASACZ_ZAP_ASACZ_ZAP_TX_POWER_H_ */
