/*
 * timeout_utils.h
 *
 *  Created on: Nov 9, 2016
 *      Author: michele
 */

#ifndef TIMEOUT_UTILS_H_
#define TIMEOUT_UTILS_H_

#include <stdint.h>
int64_t get_current_epoch_time_ms(void);
typedef int64_t type_my_timeout;
void initialize_my_timeout(type_my_timeout *p);
unsigned int is_my_timeout_elapsed_ms(type_my_timeout *p, unsigned int timeout_ms);

#endif /* TIMEOUT_UTILS_H_ */
