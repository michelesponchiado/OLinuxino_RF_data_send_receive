/*
 * ASACZ_fw_update.h
 *
 *  Created on: Mar 1, 2017
 *      Author: michele
 */

#ifndef INC_ASACZ_FW_UPDATE_H_
#define INC_ASACZ_FW_UPDATE_H_

#include <CC2650_fw_update.h>

typedef enum
{
	enum_CC2650_fw_update_status_idle = 0,
	enum_CC2650_fw_update_status_init,
	enum_CC2650_fw_update_status_init_wait,
	enum_CC2650_fw_update_status_do,
	enum_CC2650_fw_update_status_ends,
	enum_CC2650_fw_update_status_numof
}enum_CC2650_fw_update_status;

typedef struct _type_CC2650_fw_update_handle
{
	enum_CC2650_fw_update_status status;
	uint32_t ends_OK;
	uint32_t ends_ERR;
	uint32_t is_running;
	uint32_t num_upd_OK;
	uint32_t num_upd_ERR;
	volatile uint32_t num_req;
	volatile uint32_t num_ack;
	uint32_t CC2650_fw_update_retcode_is_valid;
	enum_do_CC2650_fw_update_retcode CC2650_fw_update_retcode;
	uint8_t result[256];
	uint8_t CC2650_fw_signed_filename[256];
	uint32_t key_a55a0336;
	pthread_mutex_t mtx_id;
	pthread_mutex_t mtx_update;

}type_CC2650_fw_update_handle;

#endif /* INC_ASACZ_FW_UPDATE_H_ */
