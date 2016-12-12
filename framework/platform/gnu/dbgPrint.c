/*
 * dbgPrint.c
 *
 * This module contains the API for debug print.
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*********************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <stdarg.h>

#include "dbgPrint.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * LOCAL VARIABLE
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * API FUNCTIONS
 */


const char * str_dbg_level(int level)
{
	if (level == PRINT_LEVEL_ERROR)
		return "ERROR";
	if (level == PRINT_LEVEL_WARNING)
		return "warning";
	if (level == PRINT_LEVEL_INFO)
		return "info";
	if (level == PRINT_LEVEL_INFO_LOWLEVEL)
		return "low level";
	return "unk";

}
/**************************************************************************************************
 * @fn          dbgPrint
 *
 * @brief       This function checks the print level and prints if required.
 *
 * input parameters
 *
 * @param       simpleDesc - A pointer to the simpleDesc descriptor to register.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 **************************************************************************************************
 */
void dbg_print(int print_level, const char *fmt, ...)
{
	if (print_level > PRINT_LEVEL)
	{
		return;
	}
	else
	{
		va_list argp;
		va_start(argp, fmt);
		char c_aux[256];
		vsnprintf(c_aux,sizeof(c_aux),fmt, argp);
		va_end(argp);
		unsigned long get_system_time_ms(void);
		printf("[%lu]%s %s", get_system_time_ms(), str_dbg_level(print_level), c_aux);
	}
}
#ifdef ANDROID
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>

#include "private/android_filesystem_config.h"
#include "cutils/log.h"
#endif
void my_log(int print_level, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char c_aux[256];
	vsnprintf(c_aux,sizeof(c_aux),fmt, argp);
	va_end(argp);
#ifdef ANDROID
	ALOGI("%s", c_aux);
#else
	syslog(print_level,"%s", c_aux);
#endif
}
