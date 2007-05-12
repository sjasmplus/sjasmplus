/***
*tchar.h - definitions for generic international text functions
*
*       Copyright (c) 1991-2000 Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	This file has been completely revamped to support the full set of CRT-like
*   functionality exported by COREDLL and *only* those functions (v2.10 and later)
*
****/

#ifndef _INC_CETCHAR
#define _INC_CETCHAR

#include <tchar.h>

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef  UNICODE
/* ++++++++++++++++++++ UNICODE ++++++++++++++++++++ */

#define _tunlink	_wunlink
#define _tremove	_wremove
#define _taccess	_waccess
#define	_tmktemp	_wmktemp
#define _tperror    _wperror
#define _tsystem	_wsystem
#define _trename    _wrename
#define _topen      _wopen
#define _tsopen     _wsopen
#define _tcsftime   wcsftime
#define _tutime     _wutime

#define _tsystem    _wsystem

#define _tsplitpath  _wsplitpath 
#define _tmakepath   _wmakepath 
#else   /* ndef UNICODE */

/* ++++++++++++++++++++ SBCS (MBCS in Not supported) ++++++++++++++++++++ */

#define _tcsrchr    strrchr


#define _tunlink	_unlink
#define _tremove	remove
#define _taccess	_access
#define _tmktemp	_mktemp
#define _tperror    perror
#define _tsystem    system
#define _trename    rename
#define _tcsftime   strftime
#define _tutime     _utime


#define _tsystem    system

#define _tsplitpath _splitpath
#define _tmakepath  _makepath 
#endif  /* UNICODE */

#ifdef __cplusplus
}
#endif

#endif  /* _INC_TCHAR */

