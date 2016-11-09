/*
 * ASACZ_app.h
 *
 *  Created on: Nov 9, 2016
 *      Author: michele
 */

#ifndef ASACZ_APP_H_
#define ASACZ_APP_H_

typedef enum
{
	enum_app_status_idle = 0,
	enum_app_status_init,
	enum_app_status_flush_messages_init,
	enum_app_status_flush_messages_do,
	enum_app_status_start_network,
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
	enum_app_status_error,
	enum_app_status_numof
}enum_app_status;

void start_handle_app(void);
void restart_handle_app(void);
unsigned int is_in_error_app(void);
enum_app_status get_app_status(void);

#endif /* ASACZ_APP_H_ */
