//lua_user.h
//define some user macro for wince
#ifndef LUA_WINCE_H
#define LUA_WINCE_H

#ifdef UNDER_CE
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <windef.h>


/* these are terrible, but just for CE quick-dirty (pedro) */
#define strcoll strcmp
#ifndef isalpha
#define isalpha(c)			( ('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z') )
#define isdigit(c)			( '0' <= (c) && (c) <= '9' )
#define isalnum(c)			( isalpha(c) || isdigit(c) )
#define isspace(c)			( (c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\r' )
#define iscntrl(c)			( (0x00 <= (c) && (c) <= 0x1F) || (c) == 0x7F)
#define islower(c)			( 'a' <= (c) && (c) <= 'z' )
#define isprint(c)			( (0x20 <= (c) && (c) <= 0x7E) )
#define ispunct(c)			( isprint(c) && ( !isalnum(c) && !isspace(c) ))
#define isupper(c)			( 'A' <= (c) && (c) <= 'Z' )
#define isxdigit(c)			( isdigit(c) || ('a' <= (c) && (c) <= 'f') || ('a' <= (c) && (c) <= 'f') )
#endif

#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#ifndef MAX_PATH
#define MAX_PATH 2048
#endif

#ifndef L_tmpnam
#define L_tmpnam MAX_PATH
#endif

//typedef unsigned long ptrdiff_t;

#ifndef _TM_DEFINED
#define _TM_DEFINED
#ifndef _INC_TIME
struct tm
{
  int tm_sec;			/* Seconds.	[0-60] (1 leap second) */
  int tm_min;			/* Minutes.	[0-59] */
  int tm_hour;			/* Hours.	[0-23] */
  int tm_mday;			/* Day.		[1-31] */
  int tm_mon;			/* Month.	[0-11] */
  int tm_year;			/* Year	- 1900.  */
  int tm_wday;			/* Day of week.	[0-6] */
  int tm_yday;			/* Days in year.[0-365]	*/
  int tm_isdst;			/* DST.		[-1/0/1]*/
};
#endif
#endif

//wince 4.2 has support this, don't to define it anymore
/*
enum {
	LC_ALL, 
	LC_COLLATE,
	LC_CTYPE,
	LC_MONETARY,
	LC_NUMERIC,
	LC_TIME
};
*/

//char *strdup( const char *str );
//double strtod( const char *nptr, char const * *endptr );
//unsigned long strtoul( const char *nptr, char const **endptr, int base );

//const char *getenv( const char *name );
//FILE *tmpfile();

//int system( const char * );
//int rename( const char *, const char * );
//int remove( const char * );
//char *tmpnam( char * );

//char *setlocale( int category, const char *locale );
//struct lconv *localeconv( void );

/*
char *strpbrk( const char *string, const char *strCharSet );

void *calloc( size_t num, size_t size );
*/
#endif//UNDER_CE
#endif

