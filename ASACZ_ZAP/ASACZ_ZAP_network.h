/*
 * ASACZ_ZAP_network.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_NETWORK_H_
#define ASACZ_ZAP_ASACZ_ZAP_NETWORK_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

int32_t ZAP_startNetwork(unsigned int channel_index);
int32_t ZAP_restartNetwork(void);
unsigned int ZAP_is_required_network_restart(void);
void ZAP_require_network_restart(void);

#endif /* ASACZ_ZAP_ASACZ_ZAP_NETWORK_H_ */
