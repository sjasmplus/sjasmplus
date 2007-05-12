/*
 * File to have Windows CE Toolkit for VC++ 5.0 working with STL
 * 09 - 03 - 1999
 * Rev 1.0 - Now eVC++ compatible
 * 20 - 07 - 2001
 * Giuseppe Govi - g.govi@iol.it
 */

#ifndef INC_WCE_DEFS_H
#define INC_WCE_DEFS_H

#if defined (UNDER_CE)

#include <windows.h>

#ifdef __STL_WCE_USE_OUTPUTDEBUGSTRING
#define STLTRACE(msg)   OutputDebugStringW(msg)
#else
#define STLTRACE(msg)   MessageBoxW(NULL,(msg),NULL,MB_OK)
#endif

#define abort()	TerminateProcess(GetCurrentProcess(), 0)

#ifndef __THROW_BAD_ALLOC
#define __THROW_BAD_ALLOC STLTRACE(L"out of memory"); ExitThread(1)
#endif

template <class charT> //charT == TCHAR under Widnows CE
void wce_assert(bool cond, charT* file, int line, charT* exp)
{
charT buffer[512];
	if (!cond)
	{
		wsprintf(buffer, _T("%s:%d assertion failure:\n%s"), file, line, exp);
#ifdef _DEBUG
		if (MessageBoxW(NULL, buffer, NULL, MB_RETRYCANCEL) == IDCANCEL)
			DebugBreak();
		else
#else
		MessageBoxW(NULL, buffer, NULL, MB_RETRYCANCEL);
#endif
		abort();

	}
}

#define assert(expr)	wce_assert<TCHAR>((expr), TEXT(__FILE__), __LINE__, L# expr)

#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#  ifndef _MFC_VER
   inline void * operator new(size_t, void *_P) {return (_P); }
#  endif
#endif

//size_t and wchar_t are defined in many different places in all SDKs.
//let's put them here (just to be sure)

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif

//ptrdiff_t is not defined in Windows CE SDK
#ifndef _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif

#endif //UNDER_CE

#endif //INC_WCE_DEFS_H


