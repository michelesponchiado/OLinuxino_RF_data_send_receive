/*
 * ASACZ_update_yourself.h
 *
 *  Created on: Mar 29, 2017
 *      Author: michele
 */

#ifndef INC_ASACZ_UPDATE_YOURSELF_H_
#define INC_ASACZ_UPDATE_YOURSELF_H_

typedef enum
{
	enum_asacz_package_retcode_OK =0,
	enum_asacz_package_retcode_ERR_unable_to_start_procedure,
	enum_asacz_package_retcode_ERR_unable_to_open_file,
	enum_asacz_package_retcode_ERR_unable_to_seek_end_input_file,
	enum_asacz_package_retcode_ERR_unable_to_get_input_file_size,
	enum_asacz_package_retcode_ERR_unable_to_seek_begin_input_file,
	enum_asacz_package_retcode_ERR_unable_to_alloc_input_file_body_buffer,
	enum_asacz_package_retcode_ERR_unable_to_read_input_file_body,
	enum_asacz_package_retcode_ERR_close_input_file,
	enum_asacz_package_retcode_ERR_magic_name_too_long,
	enum_asacz_package_retcode_ERR_unable_to_sprint_output_filename,
	enum_asacz_package_retcode_ERR_unable_to_open_output_file,
	enum_asacz_package_retcode_ERR_unable_to_write_header,
	enum_asacz_package_retcode_ERR_unable_to_write_body,
	enum_asacz_package_retcode_ERR_unable_to_close_output_file,

	enum_asacz_package_retcode_ERR_filelength_no_header,
	enum_asacz_package_retcode_ERR_filelength_invalid_body,
	enum_asacz_package_retcode_ERR_header_CRC,
	enum_asacz_package_retcode_ERR_magic_name,
	enum_asacz_package_retcode_ERR_body_CRC,
	enum_asacz_package_retcode_ERR_unable_to_get_exe_name,
	enum_asacz_package_retcode_ERR_unable_to_delete_old_exe,
	enum_asacz_package_retcode_ERR_unable_to_change_permissions,
	enum_asacz_package_retcode_ERR_too_long_input_filename,

	enum_asacz_package_retcode_numof
}enum_asacz_package_retcode;

typedef struct _type_ASACZ_update_yourself_inout
{
	pthread_mutex_t mtx_id;
	unsigned int thread_completed;
	unsigned int thread_completed_OK;
	unsigned int thread_completed_ERROR;
	type_fwupd_ASACZ_do_update_req_body body_req;
	type_fwupd_ASACZ_status_update_reply_body body_reply;
}type_ASACZ_update_yourself_inout;


void *thread_ASACZ_update_yourself(void *arg);
const char *get_enum_asacz_package_retcode_string(enum_asacz_package_retcode e);

#endif /* INC_ASACZ_UPDATE_YOURSELF_H_ */
