/* $Id: ts_util.h,v 1.11 2009/09/09 22:38:13 alex Exp $ */
/*******************************************************************************

    ts_util.h

    "timespec" Manipulation Definitions.

*******************************************************************************/

#ifndef  TS_UTIL_H		/* Has the file been INCLUDE'd already? */
#define  TS_UTIL_H  yes

#ifdef __cplusplus		/* If this is a C++ compiler, use C linkage */
extern  "C"  {
#endif


#include  <sys/time.h>		/* System time definitions. */


/*******************************************************************************
    Public functions.
*******************************************************************************/

extern  struct  timespec tsAdd(struct timespec time1,
                                   struct timespec time2) ;

extern  int  tsCompare(struct timespec time1,
                           struct timespec time2) ;

extern  struct  timespec  tsCreate(long seconds,
                                       long nanoseconds) ;

extern  struct  timespec  tsCreateF(double fSeconds) ;

extern  double  tsFloat(struct timespec time) ;

extern  const  char  *tsShow(struct timespec binaryTime,
                                 int inLocal,
                                 const char *format) ;

extern  struct  timespec  tsSubtract(struct timespec time1,
                                         struct timespec time2) ;

unsigned int is_OK_push_message_to_Zigbee(char *message, unsigned int message_size);

#ifdef __cplusplus		/* If this is a C++ compiler, use C linkage */
}
#endif

#endif				/* If this file was not INCLUDE'd previously. */
