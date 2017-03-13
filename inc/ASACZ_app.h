/*
 * ASACZ_app.h
 *
 *  Created on: Nov 9, 2016
 *      Author: michele
 */

#ifndef INC_ASACZ_APP_H_
#define INC_ASACZ_APP_H_

#include "ASAC_ZigBee_network_commands.h"

typedef enum
{
	enum_app_status_idle = 0,
	enum_app_status_init,
	enum_app_status_flush_messages_init,
	enum_app_status_flush_messages_do,
	enum_app_status_start_network,
	enum_app_status_restart_network_from_scratch,
	enum_app_status_get_IEEE_address,
	enum_app_status_sysOsalNvWrite,
	enum_app_status_set_TX_power,
	enum_app_status_wait_callbacks_init,
	enum_app_status_wait_callbacks_do,
	enum_app_status_display_devices_init,
	enum_app_status_display_devices_do,
	enum_app_status_display_devices_ends,
	enum_app_status_rx_msgs,
	enum_app_status_tx_msgs,
	enum_app_status_update_end_points,
	enum_app_status_firmware_update_init,
	enum_app_status_firmware_update_init_wait,
	enum_app_status_firmware_update_do,
	enum_app_status_firmware_update_ends,
	enum_app_status_diag_test,
	enum_app_status_shutdown,
	enum_app_status_error,
	enum_app_status_numof
}enum_app_status;

void start_handle_app(void);
void restart_handle_app(void);
unsigned int is_in_error_app(void);
enum_app_status get_app_status(void);
char * get_app_current_link_quality_string(void);
uint8_t get_app_current_link_quality_value_energy_detected(void);
int32_t get_app_current_link_quality_value_dBm(void);
void force_zigbee_shutdown(void);
unsigned int get_app_new_trans_id(uint32_t message_id);

typedef enum
{
	enum_link_quality_level_unknown = 0,
	enum_link_quality_level_low,
	enum_link_quality_level_normal,
	enum_link_quality_level_good,
	enum_link_quality_level_excellent,
	enum_link_quality_level_numof
}enum_link_quality_level;

typedef struct _type_link_quality_body
{
	enum_link_quality_level level;
	uint8_t v;
	int32_t v_dBm;
	int64_t t; // the last epoch time when the link quality was updated
}type_link_quality_body;

void get_app_last_link_quality(type_link_quality_body *p);

#ifdef ANDROID
	#define def_selected_serial_port "/dev/ttymxc2"
#else
	#ifdef OLINUXINO
			#define def_selected_serial_port "/dev/ttyS1"
	#else
			#define def_selected_serial_port "/dev/ttyUSB1"
	#endif
#endif

void get_CC2650_firmware_version(type_fwupd_CC2650_read_version_reply_body *p_dst);

typedef enum
{
	enum_request_CC2650_firmware_update_retcode_OK = 0,
	enum_request_CC2650_firmware_update_retcode_ERR_already_running,
	enum_request_CC2650_firmware_update_retcode_ERR_too_long_filename,

	enum_request_CC2650_firmware_update_retcode_numof
}enum_request_CC2650_firmware_update_retcode;

enum_request_CC2650_firmware_update_retcode request_CC2650_firmware_update(char *firmware_file_path, type_fwupd_CC2650_start_update_reply_body *p_dst);
void get_CC2650_firmware_update_status(type_fwupd_CC2650_query_update_status_reply_body *p);
void CC2650_query_firmware_file(char* CC2650_fw_query_filename , type_fwupd_CC2650_query_firmware_file_reply *p_reply_body);

#endif /* INC_ASACZ_APP_H_ */
