/*
 * ASACZ_version.c
 *
 *  Created on: Nov 22, 2016
 *      Author: michele
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ASACZ_firmware_version.h"

#define def_ASACZ_firmware_version_BUILD_NUMBER 	18
#define def_ASACZ_firmware_version_MAJOR_NUMBER 	0
#define def_ASACZ_firmware_version_MIDDLE_NUMBER 	1
#define def_ASACZ_firmware_version_MINOR_NUMBER 	2
#define def_ASACZ_firmware_version_DATE_AND_TIME  	__DATE__" "__TIME__
#define def_ASACZ_firmware_version_PATCH 			""
#define def_ASACZ_firmware_version_NOTES 			"internal version"

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
