/***
*signal.h - defines signal values and routines
*
*       Copyright (c) 1985-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This file defines the signal values and declares the signal functions.
*       [ANSI/System V]
*
*       [Public]
*
****/

#if     _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_SIGNAL
#define _INC_SIGNAL


#ifdef  __cplusplus
extern "C" {
#endif


#define NSIG 23     /* maximum signal number + 1 */


/* Signal types */

#define SIGINT          2       /* interrupt */
#define SIGILL          4       /* illegal instruction - invalid function image */
#define SIGFPE          8       /* floating point exception */
#define SIGSEGV         11      /* segment violation */
#define SIGTERM         15      /* Software termination signal from kill */
#define SIGBREAK        21      /* Ctrl-Break sequence */
#define SIGABRT         22      /* abnormal termination triggered by abort call */


/* signal action codes */

#define SIG_DFL (void (__cdecl *)(int))0           /* default signal action */
#define SIG_IGN (void (__cdecl *)(int))1           /* ignore signal */
#define SIG_SGE (void (__cdecl *)(int))3           /* signal gets error */
#define SIG_ACK (void (__cdecl *)(int))4           /* acknowledge */


/* signal error value (returned by signal call on error) */

#define SIG_ERR (void (__cdecl *)(int))-1          /* signal error value */

/* pointer to exception information pointers structure */

#if     defined(_MT) || defined(_DLL)
extern void * * __cdecl __pxcptinfoptrs(void);
#define _pxcptinfoptrs  (*__pxcptinfoptrs())
#else   /* ndef _MT && ndef _DLL */
extern void * _pxcptinfoptrs;
#endif  /* _MT || _DLL */



/* Function prototypes */

void (__cdecl * __cdecl signal(int, void (__cdecl *)(int)))(int);
int __cdecl raise(int);


#ifdef  __cplusplus
}
#endif

#endif  /* _INC_SIGNAL */
