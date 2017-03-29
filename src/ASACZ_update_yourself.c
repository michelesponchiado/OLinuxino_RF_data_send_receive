/*
 * ASACZ_update_yourself.c
 *
 *  Created on: Mar 29, 2017
 *      Author: michele
 */


#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <ASACZ_ZAP.h>
#include <ASACZ_update_yourself.h>

#define def_magic_name_OLinuxino_ASACZ_header "OLinuxino_ASACZ_header"
typedef struct _type_OLinuxino_ASACZ_fw_update_header
{
	uint8_t magic_name[32];				// must be set to "OLinuxino_ASACZ_header", and filled up with 0x00 in the remaining bytes
	uint32_t major_number;				//!< the version major number
	uint32_t middle_number;				//!< the version middle number
	uint32_t minor_number;				//!< the version minor number
	uint32_t build_number;				//!< the build number
	uint8_t	date_and_time[64];			//!< the version date and time
	uint8_t	patch[64];					//!< the version patch
	uint8_t	notes[64];					//!< the version notes
	uint8_t	string[256];				//!< the version string
	uint32_t firmware_body_size;		// the expected number of bytes in the firmware body, most of the times it should be 131072, i.e. 128 kBytes
	uint32_t firmware_body_CRC32_CC2650;// the CRC32 of the firmware body calculated as CC2650 does it, please see the calcCrcLikeChip routine
	uint32_t header_CRC32_CC2650;		// the CRC32 of the header (this field excluded), calculated as CC2650 does it, please see the calcCrcLikeChip routine

}__attribute__((__packed__)) type_OLinuxino_ASACZ_fw_update_header;


typedef struct _type_enum_asacz_package_retcode_table
{
	enum_asacz_package_retcode e;
	const char *s;
}type_enum_asacz_package_retcode_table;

static const type_enum_asacz_package_retcode_table asacz_package_retcode_table[]=
{
	{.e = enum_asacz_package_retcode_OK, .s ="OK"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_start_procedure, .s = "ERR_unable_to_start_procedure"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_open_file, .s ="ERR_unable_to_open_file"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_seek_end_input_file,.s ="ERR_unable_to_seek_end_input_file"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_get_input_file_size,.s ="ERR_unable_to_get_input_file_size"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_seek_begin_input_file,.s ="ERR_unable_to_seek_begin_input_file"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_alloc_input_file_body_buffer,.s ="ERR_unable_to_alloc_input_file_body_buffer"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_read_input_file_body,.s ="ERR_unable_to_read_input_file_body"},
	{.e = enum_asacz_package_retcode_ERR_close_input_file,.s ="ERR_close_input_file"},
	{.e = enum_asacz_package_retcode_ERR_magic_name_too_long,.s ="ERR_magic_name_too_long"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_sprint_output_filename,.s ="ERR_unable_to_sprint_output_filename"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_open_output_file,.s ="ERR_unable_to_open_output_file"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_write_header,.s ="ERR_unable_to_write_header"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_write_body,.s ="ERR_unable_to_write_body"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_close_output_file,.s ="ERR_unable_to_close_output_file"},
	{.e = enum_asacz_package_retcode_ERR_filelength_no_header, .s = "ERR_filelength_no_header"},
	{.e = enum_asacz_package_retcode_ERR_filelength_invalid_body, .s = "ERR_filelength_invalid_body"},
	{.e = enum_asacz_package_retcode_ERR_header_CRC, .s ="ERR_header_CRC"},
	{.e = enum_asacz_package_retcode_ERR_body_CRC, .s = "ERR_body_CRC"},
	{.e = enum_asacz_package_retcode_ERR_magic_name, .s = "ERR_magic_name"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_get_exe_name, .s = "ERR_unable_to_get_exe_name"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_delete_old_exe, .s = "ERR_unable_to_delete_old_exe"},
	{.e = enum_asacz_package_retcode_ERR_unable_to_change_permissions, .s = "ERR_unable_to_change_permissions"},
	{.e = enum_asacz_package_retcode_ERR_too_long_input_filename, .s = "ERR_too_long_input_filename"},
};

const char *get_enum_asacz_package_retcode_string(enum_asacz_package_retcode e)
{
	const char *p_return_string = "unknown return code";
	unsigned int found = 0;
	unsigned int i;
	for (i = 0; !found && i < sizeof(asacz_package_retcode_table)/sizeof(asacz_package_retcode_table[0]); i++)
	{
		const type_enum_asacz_package_retcode_table *p = &asacz_package_retcode_table[i];
		if (p->e == e)
		{
			p_return_string = p->s;
		}
	}

	return p_return_string;
}


void *thread_ASACZ_update_yourself(void *arg)
{
	type_ASACZ_update_yourself_inout *p = (type_ASACZ_update_yourself_inout *)arg;
	type_fwupd_ASACZ_do_update_req_body *p_body_req = &p->body_req;
	type_fwupd_ASACZ_status_update_reply_body *p_body_reply = &p->body_reply;
	enum_asacz_package_retcode r  = enum_asacz_package_retcode_OK;

	pthread_mutex_lock(&p->mtx_id);
		p_body_reply->running = 1;
		p->thread_completed = 0;
		p->thread_completed_OK = 0;
		p->thread_completed_ERROR = 0;
	pthread_mutex_unlock(&p->mtx_id);

	char filename_in[512];
	memset(filename_in, 0, sizeof(filename_in));

	int n_needed = snprintf(filename_in, sizeof(filename_in), "%s%s", default_ASACZ_firmware_file_directory, (char *)p_body_req->ASACZ_fw_signed_filename);

	if (n_needed > sizeof(filename_in))
	{
		r = enum_asacz_package_retcode_ERR_too_long_input_filename;
	}
	FILE *fin = NULL;
	FILE *fout = NULL;

	if (r  == enum_asacz_package_retcode_OK)
	{
		fin = fopen(filename_in, "rb");
		if (!fin)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_open_file;
		}
	}
	if (r  == enum_asacz_package_retcode_OK)
	{
		int result = fseek(fin, 0L, SEEK_END);
		if (result)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_seek_end_input_file;
		}
	}
	long l_filesize = 0;
	if (r == enum_asacz_package_retcode_OK)
	{
		l_filesize = ftell(fin);
		if (l_filesize < 0)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_get_input_file_size;
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		int result = fseek(fin, 0L, SEEK_SET);
		if (result)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_seek_begin_input_file;
		}
	}
	uint8_t * p_bin_file_whole_malloced = NULL;
	if (r == enum_asacz_package_retcode_OK)
	{
		p_bin_file_whole_malloced = (uint8_t *)malloc(l_filesize);
		if (!p_bin_file_whole_malloced)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_alloc_input_file_body_buffer;
		}
		else
		{
			memset(p_bin_file_whole_malloced, 0, l_filesize);
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		size_t n_read = fread(p_bin_file_whole_malloced, l_filesize, 1, fin);
		if (n_read != 1)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_read_input_file_body;
		}
	}
	if (fin)
	{
		if (fclose(fin))
		{
			if (r == enum_asacz_package_retcode_OK)
			{
				r = enum_asacz_package_retcode_ERR_close_input_file;
			}
		}
	}
	type_OLinuxino_ASACZ_fw_update_header OLinuxino_ASACZ_fw_update_header;
	memset(&OLinuxino_ASACZ_fw_update_header, 0, sizeof(OLinuxino_ASACZ_fw_update_header));

	uint8_t * p_bin_file_body = NULL;

	if (r == enum_asacz_package_retcode_OK)
	{
		if (l_filesize < sizeof(OLinuxino_ASACZ_fw_update_header))
		{
			r = enum_asacz_package_retcode_ERR_filelength_no_header;
		}
		else
		{
			memcpy(&OLinuxino_ASACZ_fw_update_header, p_bin_file_whole_malloced, sizeof(OLinuxino_ASACZ_fw_update_header));
			p_bin_file_body = p_bin_file_whole_malloced +  sizeof(type_OLinuxino_ASACZ_fw_update_header);

			int header_CRC_size = sizeof(OLinuxino_ASACZ_fw_update_header) - sizeof(OLinuxino_ASACZ_fw_update_header.header_CRC32_CC2650);
			uint32_t calculated_header_CRC32_CC2650 = calcCrcLikeChip((const unsigned char *)&OLinuxino_ASACZ_fw_update_header, header_CRC_size);
			if (calculated_header_CRC32_CC2650 != OLinuxino_ASACZ_fw_update_header.header_CRC32_CC2650)
			{
				r = enum_asacz_package_retcode_ERR_header_CRC;
			}
			else if (strcmp((char*)OLinuxino_ASACZ_fw_update_header.magic_name, def_magic_name_OLinuxino_ASACZ_header))
			{
				r = enum_asacz_package_retcode_ERR_magic_name;
			}
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		uint32_t calculated_body_CRC32_CC2650 = calcCrcLikeChip((const unsigned char *)p_bin_file_body, OLinuxino_ASACZ_fw_update_header.firmware_body_size);
		if (calculated_body_CRC32_CC2650 != OLinuxino_ASACZ_fw_update_header.firmware_body_CRC32_CC2650)
		{
			r = enum_asacz_package_retcode_ERR_body_CRC;
		}
	}
	char outfile_name[512];
	memset(outfile_name, 0, sizeof(outfile_name));
	if (r == enum_asacz_package_retcode_OK)
	{
		if (readlink ("/proc/self/exe", outfile_name, sizeof(outfile_name) - 1) < 0)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_get_exe_name;
		}
	}
	unsigned int delete_old_file = 0;
	if (r == enum_asacz_package_retcode_OK)
	{
		if( access( outfile_name, F_OK ) != -1 )
		{
			delete_old_file = 1;
			// file exists
		} else
		{
			delete_old_file = 0;
			// file doesn't exist
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		if (delete_old_file && unlink(outfile_name) < 0)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_delete_old_exe;
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		fout = fopen(outfile_name, "wb");
		if (!fout)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_open_output_file;
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{
		size_t n = fwrite(p_bin_file_body, OLinuxino_ASACZ_fw_update_header.firmware_body_size, 1, fout);
		if (n != 1)
		{
			r = enum_asacz_package_retcode_ERR_unable_to_write_body;
		}
	}
	if (fout)
	{
		if (fclose(fout))
		{
			if (r == enum_asacz_package_retcode_OK)
			{
				r = enum_asacz_package_retcode_ERR_unable_to_close_output_file;
			}
		}
	}
	if (r == enum_asacz_package_retcode_OK)
	{

		char mode[] = "0777";
	    int permissions = strtol(mode, 0, 8);
	    if (chmod(outfile_name, permissions) < 0)
	    {
			r = enum_asacz_package_retcode_ERR_unable_to_change_permissions;
	    }
	}

	if (p_bin_file_whole_malloced)
	{
		free(p_bin_file_whole_malloced);
	}
	pthread_mutex_lock(&p->mtx_id);
		p_body_reply->running = 0;
		p_body_reply->ends_OK = (r == enum_asacz_package_retcode_OK) ? 1 : 0;
		p_body_reply->ends_ERROR = !p_body_reply->ends_OK;
		p_body_reply->result_code = r;
		snprintf((char*)p_body_reply->result_message, sizeof(p_body_reply->result_message), "%s", get_enum_asacz_package_retcode_string(r));

		p->thread_completed_OK = p_body_reply->ends_OK;
		p->thread_completed_ERROR = !p_body_reply->ends_OK;
		p->thread_completed = 1;
	pthread_mutex_unlock(&p->mtx_id);
	extern void signal_thread_ASACZ_update_yourself_ends(void);
	signal_thread_ASACZ_update_yourself_ends();
	return NULL;
}



