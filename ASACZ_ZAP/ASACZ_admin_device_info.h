/*
 * AZACZ_admin_device_info.h
 *
 *  Created on: Mar 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ADMIN_DEVICE_INFO_H_
#define ASACZ_ADMIN_DEVICE_INFO_H_
#include <ASACZ_ZAP.h>

void get_ASACZ_admin_device_sn(type_administrator_device_info_op_get_serial_number_reply *p_get_reply);
void set_ASACZ_admin_device_sn(type_administrator_device_info_op_set_serial_number *p_req, type_administrator_device_info_op_set_serial_number_reply *p_set_reply);

#endif /* ASACZ_ADMIN_DEVICE_INFO_H_ */
