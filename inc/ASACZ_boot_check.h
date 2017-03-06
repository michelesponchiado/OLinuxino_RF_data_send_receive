/*
 * ASACZ_boot_check.h
 *
 *  Created on: Feb 15, 2017
 *      Author: michele
 */

#ifndef INC_ASACZ_BOOT_CHECK_H_
#define INC_ASACZ_BOOT_CHECK_H_

typedef enum
{
	enum_boot_check_retcode_OK = 0,
	enum_boot_check_retcode_ERR_snprintf,
	enum_boot_check_retcode_ERR_socket_open,
	enum_boot_check_retcode_ERR_ioctl_SIOCGIFINDEX,
	enum_boot_check_retcode_ERR_ioctl_SIOCGIFHWADDR,
	enum_boot_check_retcode_ERR_unable_set_socket_tx_non_blocking,
	enum_boot_check_retcode_ERR_unable_open_socket_rx,
	enum_boot_check_retcode_ERR_unable_read_flags_socket_rx,
	enum_boot_check_retcode_ERR_unable_write_flags_socket_rx,
	enum_boot_check_retcode_ERR_unable_to_send,
	enum_boot_check_retcode_ERR_unable_to_bind_socket_rx,
	enum_boot_check_retcode_ERR_snprintf_idx_ifr_name,
	enum_boot_check_retcode_ERR_snprintf_mac_ifr_name,

	enum_boot_check_retcode_numof
}enum_boot_check_retcode;

enum_boot_check_retcode boot_check(void);

#endif /* INC_ASACZ_BOOT_CHECK_H_ */
