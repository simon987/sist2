/* include/plib/plib.h.  Generated from plib.h by configure.  */
/*
 Copyright (c) 2010, Florian Reuter
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without 
 modification, are permitted provided that the following conditions 
 are met:
 
 * Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright 
   notice, this list of conditions and the following disclaimer in 
   the documentation and/or other materials provided with the 
   distribution.
 * Neither the name of Florian Reuter nor the names of its contributors 
   may be used to endorse or promote products derived from this 
   software without specific prior written permission.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
 
*/
#ifndef _PLIB_PLIB_H_
#define _PLIB_PLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#define HAVE_STDINT_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDIO_H 1
#define HAVE_STRING_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDLIB_H 1
/* #undef HAVE_IO_H */
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TYPES_H 1
#define IS_CONFIGURED 1

#if !defined(IS_CONFIGURED)
#if defined(WIN32)
#define HAVE_STRING_H 1
#define HAVE_STDINT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_STDDEF_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_IO_H
#define snprintf _snprintf
#else
#error "configure not executed and we are not on a win32 machine? please run configure or define WIN32 is you are on a WIN32 platform."
#endif
#endif

#ifdef HAVE_STDDEF_H
#include <stddef.h>
typedef size_t pofs_t; // maximum file offset for eg. read write ops
#else
#error "system types can not be determined"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#else
#error "system io can not be determined"
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>

typedef int8_t pint8_t;
typedef uint8_t puint8_t;

typedef int16_t pint16_t;
typedef uint16_t puint16_t;

typedef int32_t pint32_t;
typedef uint32_t puint32_t;

typedef int64_t pint64_t;
typedef uint64_t puint64_t;

typedef int pbool_t;

typedef size_t psize_t;

// INTN_MAX, INTN_MIN, UINTN_MAX
#else
#error "system types can not be determined"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#define PUINT8_MAX UCHAR_MAX 
#define PINT32_MAX INT_MAX 
#define PINT32_MIN INT_MIN 
#define PUINT32_MAX UINT_MAX 
#define PUINT32_MIN 0 
#define PUINT16_MAX	USHRT_MAX 
#define PUINT16_MIN 0 
#else
#error "limits can not be determined"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/**
 Converts an ASCII string to a xmlChar string. This only works for ASCII strings.
 */
#ifndef _X
#define _X(s) BAD_CAST(s) 
#endif


/**
 Converts an xmlChar string to an ASCII string. This only works for ASCII charsets.
 */
#ifndef _X2C
#define _X2C(s) ((char*)(s))
#endif


#define PASSERT(e) assert(e)
#ifdef NDEBUG
#define PENSURE(e) (void)(e)
#else
#define PENSURE(e) assert(e)
#endif
#define PTRUE (0==0)
#define PFALSE (0==1)


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* _PLIB_PLIB_H_ */
