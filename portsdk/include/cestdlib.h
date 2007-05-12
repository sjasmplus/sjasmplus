/***
*cestdlib.h - declarations/definitions for commonly used library functions
*
*       Copyright (c) 2002 SymbolicTools
*
*Purpose:
*       This include file contains the function declarations for commonly
*       used library functions which either don't fit somewhere else, or,
*       cannot be declared in the normal place for other reasons.
*       [ANSI]
*
*       [Public]
*
****/

#ifndef _INC_CESTDLIB
#define _INC_CESTDLIB

#if     !defined(_WIN32_WCE) 
#error ERROR: Only  WinCE targets supported!
#endif

#include <stdlib.h>


#ifdef  __cplusplus
extern "C" {
#endif

/*
 * Argument values for _set_error_mode().
 */
#define _OUT_TO_DEFAULT 0
#define _OUT_TO_STDERR  1
#define _OUT_TO_MSGBOX  2
#define _REPORT_ERRMODE 3


/* External variable declarations */

_CRTIMP int * __cdecl _errno(void);
#define errno       (*_errno())


/* function prototypes */
#ifdef UNICODE
_CRTIMP void __cdecl _wperror(const wchar_t *);
_CRTIMP int  __cdecl _wsystem(const wchar_t *);
_CRTIMP int  __cdecl _wunlink(const wchar_t *);
_CRTIMP int  __cdecl _wremove(const wchar_t *);
_CRTIMP wchar_t * __cdecl _wmktemp(wchar_t *);
_CRTIMP int __cdecl _waccess(const wchar_t *, int);
_CRTIMP int __cdecl _wrename(const wchar_t *, const wchar_t *);
_CRTIMP int __cdecl _wopen(const wchar_t *, int, ...);
_CRTIMP int __cdecl _wsopen(const wchar_t *, int, int, ...);

/* wide function prototypes, also declared in wchar.h  */
_CRTIMP int __cdecl _wsystem(const wchar_t *);

_CRTIMP void   __cdecl _wsplitpath(const wchar_t *, wchar_t *, wchar_t *, wchar_t *, wchar_t *);
_CRTIMP void   __cdecl _wmakepath(wchar_t *, const wchar_t *, const wchar_t *, const wchar_t *,
        const wchar_t *);



#else

_CRTIMP void   __cdecl perror(const char *);
_CRTIMP char*  __cdecl strerror( int errnum );


_CRTIMP int    __cdecl system(const char *);
_CRTIMP int	   __cdecl remove(const char *);
_CRTIMP int	   __cdecl _unlink(const char *);
_CRTIMP char * __cdecl _mktemp(char *);
_CRTIMP int __cdecl _access(const char *, int);
_CRTIMP int __cdecl rename(const char *, const char *);
_CRTIMP int __cdecl system(const char *);

_CRTIMP void   __cdecl _splitpath(const char *, char *, char *, char *, char *);
_CRTIMP void   __cdecl _makepath(char *, const char *, const char *, const char *,
        const char *);


#endif

_CRTIMP int __cdecl _getpid(void);
_CRTIMP int __cdecl _kbhit(void);
_CRTIMP int __cdecl _isatty(void*);
_CRTIMP void __cdecl abort (void);





#define kbhit	_kbhit
#define mktemp	_mktemp
#define access  _access
#define isatty  _isatty
#define unlink  _unlink




#ifdef  __cplusplus
}
#endif


#endif  /* _INC_CESTDLIB */
