/*
 * ASACZ_firmware_version.h
 *
 *  Created on: Nov 22, 2016
 *      Author: michele
 */

#ifndef INC_ASACZ_FIRMWARE_VERSION_H_
#define INC_ASACZ_FIRMWARE_VERSION_H_
#include <inttypes.h>

#ifndef ANDROID
#define print_all_received_messages
#endif


typedef struct _type_ASACZ_firmware_version
{
	uint32_t major_number;			//!< the version major number
	uint32_t middle_number;			//!< the version middle number
	uint32_t minor_number;			//!< the version minor number
	uint32_t build_number;			//!< the build number
	char	date_and_time[64];		//!< the version date and time
	char	patch[64];				//!< the version patch
	char	notes[64];				//!< the version notes
	char	string[256];			//!< the version string
}type_ASACZ_firmware_version;

void init_ASACZ_firmware_version(void);
void get_ASACZ_firmware_version_string(char *p_string, uint32_t max_char_to_copy);
void get_ASACZ_firmware_version_whole_struct(type_ASACZ_firmware_version *p);

#endif /* INC_ASACZ_FIRMWARE_VERSION_H_ */
