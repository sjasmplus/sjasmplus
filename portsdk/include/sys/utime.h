/***
*sys/utime.h - definitions/declarations for utime()
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the structure used by the utime routine to set
*       new file access and modification times.  NOTE - MS-DOS
*       does not recognize access time, so this field will
*       always be ignored and the modification time field will be
*       used to set the new time.
*
*       [Public]
*
****/

#pragma once

#ifndef _INC_UTIME
#define _INC_UTIME


#pragma pack(push,8)

#ifdef __cplusplus
extern "C" {
#endif



/* define struct used by _utime() function */

#ifndef _UTIMBUF_DEFINED

struct _utimbuf {
        time_t actime;          /* access time */
        time_t modtime;         /* modification time */
        };

#if     !__STDC__
/* Non-ANSI name for compatibility */
struct utimbuf {
        time_t actime;          /* access time */
        time_t modtime;         /* modification time */
        };
#endif

#define _UTIMBUF_DEFINED
#endif


/* Function Prototypes */

_CRTIMP int __cdecl _utime(const char *, struct _utimbuf *);
_CRTIMP int __cdecl _futime(void*, struct _utimbuf *);
/* Wide Function Prototypes */
_CRTIMP int __cdecl _wutime(const wchar_t *, struct _utimbuf *);

#if     !__STDC__
/* Non-ANSI name for compatibility */
_CRTIMP int __cdecl utime(const char *, struct utimbuf *);
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif  /* _INC_UTIME */
