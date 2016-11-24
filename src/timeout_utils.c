/*
 * timeout_utils.c
 *
 *  Created on: Nov 9, 2016
 *      Author: michele
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>  /* ftime, timeb (for timestamp in millisecond) */
#include <sys/time.h>   /* gettimeofday, timeval (for timestamp in microsecond) */
#include "timeout_utils.h"

int64_t get_current_epoch_time_ms(void)
{
	struct timeb timer_msec;
	int64_t timestamp_msec; // timestamp in milliseconds
	if (!ftime(&timer_msec))
	{
		timestamp_msec = ((long long int) timer_msec.time) * 1000ll +	(long long int) timer_msec.millitm;
	}
	else
	{
		timestamp_msec = -1;
	}
	return timestamp_msec;
}


void initialize_my_timeout(type_my_timeout *p)
{
	*p = get_current_epoch_time_ms();
}

unsigned int is_my_timeout_elapsed_ms(type_my_timeout *p, unsigned int timeout_ms)
{
	unsigned int is_timeout = 0;
	type_my_timeout base = *p;
	type_my_timeout now;
	now = get_current_epoch_time_ms();
	if (now < base)
	{
		*p = now;
	}
	else if (now - base > timeout_ms)
	{
		is_timeout = 1;
	}
	return is_timeout;
}
