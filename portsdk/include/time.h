/***
*time.h - definitions/declarations for time routines
*
*       Copyright (c) 2002 SymbolicTools.
*
*Purpose:
*       This file has declarations of time routines and defines
*       the structure returned by the localtime and gmtime routines and
*       used by asctime.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_TIME
#define _INC_TIME


#ifdef  __cplusplus
extern "C" {
#endif

typedef long clock_t;

#ifndef _TM_DEFINED
struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };
#endif

/* Clock ticks macro - ANSI version */

#define CLOCKS_PER_SEC  1000


struct tm * __cdecl localtime(const time_t *);
struct tm * __cdecl gmtime(const time_t *);
time_t __cdecl time(time_t *);
clock_t __cdecl clock(void);
char *asctime( const struct tm *timeptr );
wchar_t *_wasctime( const struct tm *timeptr );
char *ctime( const time_t *timer );
wchar_t *_wctime( const time_t *timer );
time_t mktime( struct tm *timeptr );
double difftime( time_t b, time_t a );
_CRTIMP size_t __cdecl strftime(char *, size_t, const char *, const struct tm *);

_CRTIMP size_t __cdecl wcsftime(wchar_t *, size_t, const wchar_t *,const struct tm *);

#ifdef  __cplusplus
}
#endif


#endif  /* _INC_TIME */
