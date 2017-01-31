/*
 * ASACZ_ZAP_end_points_update_list.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_END_POINTS_UPDATE_LIST_H_
#define ASACZ_ZAP_ASACZ_ZAP_END_POINTS_UPDATE_LIST_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

void reset_time_end_point_update_list(type_handle_end_point_update * p);
unsigned int is_needed_end_point_update_list(type_handle_end_point_update * p);
void handle_end_points_update_list(type_handle_end_point_update * p);
void refresh_my_endpoint_list(void);
void register_user_end_points(void);

#endif /* ASACZ_ZAP_ASACZ_ZAP_END_POINTS_UPDATE_LIST_H_ */
