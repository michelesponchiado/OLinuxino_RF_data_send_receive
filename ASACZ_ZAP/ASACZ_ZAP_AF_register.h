/*
 * ASACZ_ZAP_AF_register.h
 *
 *  Created on: Jan 20, 2017
 *      Author: michele
 */

#ifndef ASACZ_ZAP_ASACZ_ZAP_AF_REGISTER_H_
#define ASACZ_ZAP_ASACZ_ZAP_AF_REGISTER_H_
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#define ASACZ_reserved_endpoint 0xf0
#define ASACZ_reserved_endpoint_clusters_numof 5
#define ASACZ_reserved_endpoint_cluster_random 1
#define ASACZ_reserved_endpoint_cluster_diag 2
#define ASACZ_reserved_endpoint_cluster_stream 3

int32_t registerAf_default(void);
unsigned int is_OK_registerAf_user(type_Af_user *p);
uint8_t get_default_dst_endpoint(void);
uint8_t get_default_src_endpoint(void);
uint16_t get_default_clusterID (void);

#endif /* ASACZ_ZAP_ASACZ_ZAP_AF_REGISTER_H_ */
