/*
 * ASACZ_version.c
 *
 *  Created on: Nov 22, 2016
 *      Author: michele
 */

#include "ASACZ_firmware_version.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// the version main numbers
#define def_ASACZ_firmware_version_MAJOR_NUMBER 	0
#define def_ASACZ_firmware_version_MIDDLE_NUMBER 	1
#define def_ASACZ_firmware_version_MINOR_NUMBER 	3

// the version build number
#define def_ASACZ_firmware_version_BUILD_NUMBER 	8

#define def_ASACZ_firmware_version_DATE_AND_TIME  	__DATE__" "__TIME__
#define def_ASACZ_firmware_version_PATCH 			""
#define def_ASACZ_firmware_version_NOTES 			"internal version"
#ifdef print_all_received_messages
	#undef def_ASACZ_firmware_version_NOTES
	#define def_ASACZ_firmware_version_NOTES 			"prints received messages"
#endif

// 0.1.3 build 8
// * removing the enable to the debug code to test the firmware update under Android: it works after some bugs have been removed from the TI library
// 0.1.3 build 7
// * added the option --version to print on the console the program version
// * added the option --help to print on the console the arguments syntax
// * the boot check changes are volatile ("uci commit" is not executed after the setting have been done)
// 0.1.3 build 6
// * adding the firmware update procedure, experimenting, but it seems to work OK
// 0.1.3 build 5
// * now the channel mask parameter works OK;
//   - when the coordinator starts, it sets the channel to 0; the first device entering the network sets the channel
// * added the get_device_info API, when the network is formed, all of the device informations are printed in the log, e.g.:
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: dev state: DEV_END_DEVICE
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: IEEE address: 0x124B0006E30188
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: short address: 0xD5BB
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: parent short address: 0x0
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: parent IEEE address: 0x124b0007ae7981
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: channel: 11
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: PANID: 0x11C2
//		Feb 22 16:45:38 localhost ASAC_Zlog[32600]: getDeviceInfoSrspCb: EXT PANID: 0x124b0007ae7981
//
// WORKS OK SETTING A PANID DIFFERENT FROM 0XFFFF TOO
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: dev state: DEV_END_DEVICE
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: IEEE address: 0x124B0006E30188
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: short address: 0xD5BB
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: parent short address: 0x0
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: parent IEEE address: 0x124b0007ae7981
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: channel: 11
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: PANID: 0x1330
//		Feb 22 17:11:10 localhost ASAC_Zlog[1074]: getDeviceInfoSrspCb: EXT PANID: 0x124b0007ae7981
//
// 0.1.3 build 4
// added the restart network from scratch administration command
//
// 0.1.3 build 3
// added the ASACZ conf module
//
// 0.1.3 build 2
// CHANGED THE PROTOCOL HEADER
//  added the print all messages define
//
// 0.1.3 build 1
//   - added the delete end point command
//   - when an end point is added, the network is NOT restarted, but the end point list is updated instead
//   - when closing the server, the leave command is sent to the coordinator
// 0.1.2 build 26
//   start and restart network always use new network; when a new network is created, it is better to start from scratch
// 0.1.2 build 25
//   start and restart network always use existing network if available
// 0.1.2 build 24
//   adding diagnostic to check network restart
// 0.1.2 build 23
//   adding Android log
// 0.1.2 build 22
//   checking BUG: when entering the network, the devices not always communow the channel mask parameter works OK;nicate
// 0.1.2 build 21
//   corrects BUG: on some reply structures the packed attribute was missing, so the reply wass ent in the incorrect format
// 0.1.2 build 20
//   adding routers too to the list
// 0.1.2 build 19
//   solved bug in the device list query, the ends flag was never set
// 0.1.2 build 18
//   added the link quality message
// 0.1.2 build 17
//   adding the link quality information

static type_ASACZ_firmware_version ASACZ_firmware_version;

void get_ASACZ_firmware_version_string(char *p_string, uint32_t max_char_to_copy)
{
	if (p_string)
	{
		snprintf(p_string, max_char_to_copy, "%s", ASACZ_firmware_version.string);
	}
}

void get_ASACZ_firmware_version_whole_struct(type_ASACZ_firmware_version *p)
{
	if (p)
	{
		memcpy(p, &ASACZ_firmware_version, sizeof(*p));
	}
}
void init_ASACZ_firmware_version(void)
{
	ASACZ_firmware_version.build_number = def_ASACZ_firmware_version_BUILD_NUMBER;
	ASACZ_firmware_version.major_number = def_ASACZ_firmware_version_MAJOR_NUMBER;
	ASACZ_firmware_version.middle_number= def_ASACZ_firmware_version_MIDDLE_NUMBER;
	ASACZ_firmware_version.minor_number = def_ASACZ_firmware_version_MINOR_NUMBER;
	snprintf(ASACZ_firmware_version.date_and_time, sizeof(ASACZ_firmware_version.date_and_time), "%s", def_ASACZ_firmware_version_DATE_AND_TIME);
	snprintf(ASACZ_firmware_version.patch, sizeof(ASACZ_firmware_version.patch), "%s", def_ASACZ_firmware_version_PATCH);
	snprintf(ASACZ_firmware_version.notes, sizeof(ASACZ_firmware_version.notes), "%s", def_ASACZ_firmware_version_NOTES);
	snprintf(ASACZ_firmware_version.string, sizeof(ASACZ_firmware_version.string), "ASACZ server ver. %u.%u.%u build %u, %s, patch=%s, notes=%s"
			,ASACZ_firmware_version.major_number
			,ASACZ_firmware_version.middle_number
			,ASACZ_firmware_version.minor_number
			,ASACZ_firmware_version.build_number
			,ASACZ_firmware_version.date_and_time
			,ASACZ_firmware_version.patch
			,ASACZ_firmware_version.notes
			);
}
