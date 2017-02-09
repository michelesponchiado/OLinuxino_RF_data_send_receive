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

typedef enum
{
	enum_start_network_type_resume = 0,
	enum_start_network_type_from_scratch,

	enum_start_network_type_numof
}enum_start_network_type;

int32_t ZAP_startNetwork(unsigned int register_user_end_point, enum_start_network_type start_network_type);

unsigned int ZAP_is_required_network_restart_from_scratch(void);
void ZAP_require_network_restart_from_scratch(void);

#endif /* ASACZ_ZAP_ASACZ_ZAP_NETWORK_H_ */
