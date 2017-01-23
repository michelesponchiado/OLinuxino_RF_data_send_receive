/*
 * ASACZ_ZAP_NV.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_NV_H_
#define ASACZ_ZAP_ASACZ_ZAP_NV_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

uint8_t setNVChanList(uint32_t chanList);
uint8_t setNVPanID(uint32_t panId);
uint8_t setNVStartup(uint8_t startupOption);

#endif /* ASACZ_ZAP_ASACZ_ZAP_NV_H_ */
