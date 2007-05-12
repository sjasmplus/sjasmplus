/***
*sys/timeb.h - definition/declarations for _ftime()
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file define the _ftime() function and the types it uses.
*       [System V]
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_TIMEB
#define _INC_TIMEB


#pragma pack(push,8)

#ifdef __cplusplus
extern "C" {
#endif



#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif


/* Structure returned by _ftime system call */

#ifndef _TIMEB_DEFINED
struct _timeb {
        time_t time;
        unsigned short millitm;
        short timezone;
        short dstflag;
        };


/* Non-ANSI name for compatibility */

struct timeb {
        time_t time;
        unsigned short millitm;
        short timezone;
        short dstflag;
        };


#define _TIMEB_DEFINED
#endif


/* Function prototypes */

_CRTIMP void __cdecl _ftime(struct _timeb *);

/* Non-ANSI name for compatibility */

_CRTIMP void __cdecl ftime(struct timeb *);




#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif  /* _INC_TIMEB */
