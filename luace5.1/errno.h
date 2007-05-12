//self defined errno.h
#ifndef WINCE_ERRNO_H
#define WINCE_ERRNO_H
#ifdef UNDER_CE
#define strerror(errnum) ("(unsupported)")
static int errno=0;
#endif
#endif
