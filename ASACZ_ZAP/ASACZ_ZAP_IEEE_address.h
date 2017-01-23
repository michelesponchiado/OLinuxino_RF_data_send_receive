/*
 * ASACZ_ZAP_IEEE_address.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_IEEE_ADDRESS_H_
#define ASACZ_ZAP_ASACZ_ZAP_IEEE_ADDRESS_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

void invalidate_IEEE_address(type_handle_app_IEEE_address *p);
void set_IEEE_address(type_handle_app_IEEE_address *p, uint64_t address);
uint64_t get_IEEE_address(type_handle_app_IEEE_address *p);
unsigned int is_valid_IEEE_address(type_handle_app_IEEE_address *p);
uint32_t is_OK_get_my_radio_IEEE_address(uint64_t *p);

#endif /* ASACZ_ZAP_ASACZ_ZAP_IEEE_ADDRESS_H_ */
