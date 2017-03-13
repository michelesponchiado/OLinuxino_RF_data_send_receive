/*
 * ASACZ_diag_test.h
 *
 *  Created on: Mar 9, 2017
 *      Author: michele
 */

#ifndef ASACZ_DIAG_TEST_H_
#define ASACZ_DIAG_TEST_H_

enum_admin_diag_test_start_retcode start_CC2650_diag_test(type_admin_diag_test_req_start_body *p_req);
void get_CC2650_start_diag_test_retcode_string(enum_admin_diag_test_start_retcode r, char *s, int size_s);
enum_admin_diag_test_status_req_format get_CC2650_diag_test_status(enum_admin_diag_test_status_req_format f, char* p_dst);
enum_admin_diag_test_stop_retcode stop_CC2650_diag_test(void);
void get_CC2650_stop_diag_test_retcode_string(enum_admin_diag_test_stop_retcode r, char *s, int size_s);

// this is the interface reserved to the inner modules
#ifdef ASACZ_ZAP_H_
	void init_diag_test(void);
	unsigned int is_OK_AF_callback_diag_transId(uint8_t transId);
	unsigned int is_OK_handle_diag_test(void);
	unsigned int is_running_diag_test(void);
	unsigned int is_request_start_diag_test(void);
	void diag_rx_callback(IncomingMsgFormat_t *msg);
#endif

#endif /* ASACZ_DIAG_TEST_H_ */
