/**
 * am_types.h
 *
 * History:
 *	2007/11/5 - [Oliver Li] created file
 *	2009/12/2 - [Oliver Li] rewrite
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_TYPES_H__
#define __AM_TYPES_H__

//-----------------------------------------------------------------------
//
//	general types
//
//-----------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>

typedef int          AM_INT;
typedef unsigned int AM_UINT;
typedef int          AM_INTPTR;
typedef volatile int am_atomic_t;

typedef uint8_t      AM_U8;
typedef uint16_t     AM_U16;
typedef uint32_t     AM_U32;
typedef uint64_t     AM_U64;

typedef int8_t       AM_S8;
typedef int16_t      AM_S16;
typedef int32_t      AM_S32;
typedef int64_t      AM_S64;

typedef AM_U64       AM_PTS;
typedef AM_S32       am_file_off_t;
typedef uint8_t      am_char_t;

typedef bool         AM_BOOL;
#define AM_TRUE      true
#define AM_FALSE     false

//-----------------------------------------------------------------------
//
//	error code
//
//-----------------------------------------------------------------------

enum AM_ERR
{
  ME_OK = 0,

  ME_PENDING,
  ME_ERROR,
  ME_CLOSED,
  ME_BUSY,
  ME_NO_IMPL,
  ME_OS_ERROR,
  ME_IO_ERROR,
  ME_FILE_END,
  ME_TIMEOUT,
  ME_NO_MEMORY,
  ME_TOO_MANY,

  ME_NOT_EXIST,
  ME_NOT_SUPPORTED,
  ME_NO_INTERFACE,
  ME_BAD_STATE,
  ME_BAD_PARAM,
  ME_BAD_COMMAND,

  ME_BAD_FORMAT,
  ME_NO_ACTION,
};

//-----------------------------------------------------------------------
//
//	debug print, assert
//
//-----------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "utilities/am_define.h"
#include "utilities/am_log.h"

/* Convert amf2 use am_camera's log system */
#define AM_NOTICE           NOTICE
#define AM_PRINTF           DEBUG
#define AM_ERROR            ERROR
#define AM_INFO             INFO
#define AM_PERROR(msg)      PERROR(msg)
#define AM_LOG              INFO
#define PRINT_FUNCTION_NAME DEBUG("Calling %s", __func__)

/* Convert amf2's define into am_define */
#define LIKELY   AM_LIKELY
#define UNLIKELY AM_UNLIKELY

#ifdef AM_CAMERA_DEBUG
#define AM_ABORT()	abort()
#else
#define AM_ABORT()	(void)0
#endif

#define AM_ASSERT(expr) \
    do { \
      if (AM_UNLIKELY( !(expr) )) { \
        ERROR("assertion failed: %s", #expr); \
        AM_ABORT(); \
      } \
    } while (0);

/* Check return value of fuction call */
#define AM_ENSURE_(expr) \
    do { \
      if (AM_UNLIKELY( (expr) == NULL ) ) { \
        ERROR("%s == NULL", #expr); \
        AM_ABORT(); \
      } \
    } while (0);

#define AM_ENSURE_OK_(expr) \
    do { \
      if (AM_UNLIKELY( (expr) != ME_OK ) ) { \
        ERROR("%s != ME_OK", #expr); \
        AM_ABORT(); \
      } \
    } while (0);

#define AM_IOCTL(_filp, _cmd, _arg) \
    do { \
      if (AM_UNLIKELY(::ioctl(_filp, _cmd, _arg) < 0)) { \
        AM_PERROR(#_cmd); \
        return ME_ERROR; \
      } \
    } while (0)

#define AM_IOCTL2(_filp, _cmd, _arg) \
    do { \
      if (AM_UNLIKELY(::ioctl(_filp, _cmd, _arg) < 0)) { \
        AM_PERROR(#_cmd);  \
        return; \
      }   \
    } while (0)

//-----------------------------------------------------------------------
//
//	time, delay
//
//-----------------------------------------------------------------------
#include <time.h>
#include <sys/time.h>

#define AM_MSLEEP(_msec) \
    do { \
      struct timespec req; \
      time_t sec = _msec /1000; \
      req.tv_sec = sec; \
      req.tv_nsec = (_msec - sec * 1000) * 1000000L; \
      nanosleep(&req, NULL); \
    } while (0)

#ifndef timersub
#define	timersub(a, b, result) \
		do { \
			(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
			(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
			if ((result)->tv_usec < 0) { \
				--(result)->tv_sec; \
				(result)->tv_usec += 1000000; \
			} \
		} while (0)
#endif

#ifndef timermsub
#define	timermsub(a, b, result) \
    do { \
      (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
      (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
      if ((result)->tv_nsec < 0) { \
        --(result)->tv_sec; \
        (result)->tv_nsec += 1000000000L; \
      } \
    } while (0)
#endif

//-----------------------------------------------------------------------
//
//	macros
//
//-----------------------------------------------------------------------

// align must be power of 2
#ifndef ROUND_UP
#define ROUND_UP round_up
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN round_down
#endif

#ifndef AM_MIN
#define AM_MIN(_a, _b)			((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef AM_MAX
#define AM_MAX(_a, _b)			((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)		(sizeof(_array) / sizeof(_array[0]))
#endif

#ifndef OFFSET
#define OFFSET(_type, _member)		((int)&((_type*)0)->member)
#endif

#ifndef PTR_ADD
#define PTR_ADD(_ptr, _size)		(void*)((char*)(_ptr) + (_size))
#endif

#ifndef ABS
#define ABS(x) (((x) < 0) ? -(x) : (x))
#endif

//-----------------------------------------------------------------------
//
//  IID
//
//-----------------------------------------------------------------------

struct AM_GUID
{
    AM_U32 x;
    AM_U16 s1;
    AM_U16 s2;
    AM_U8 c[8];
};

typedef const AM_GUID& AM_REFGUID;

extern const AM_GUID GUID_NULL;

typedef AM_GUID AM_IID;
typedef AM_GUID AM_CLSID;

typedef const AM_IID& AM_REFIID;
typedef const AM_CLSID& AM_REFCLSID;

inline int operator==(AM_REFGUID guid1, AM_REFGUID guid2)
{
  if (&guid1 == &guid2) {
    return 1;
  }
  return ((AM_U32*) &guid1)[0] == ((AM_U32*) &guid2)[0]
      && ((AM_U32*) &guid1)[1] == ((AM_U32*) &guid2)[1]
      && ((AM_U32*) &guid1)[2] == ((AM_U32*) &guid2)[2]
      && ((AM_U32*) &guid1)[3] == ((AM_U32*) &guid2)[3];
}

inline int operator!=(AM_REFGUID guid1, AM_REFGUID guid2)
{
  return !(guid1 == guid2);
}

#define AM_DEFINE_IID(name, x, s1, s2, c0, c1, c2, c3, c4, c5, c6, c7) \
  extern const AM_IID name = {x, s1, s2, {c0, c1, c2, c3, c4, c5, c6, c7}}

#define AM_DEFINE_GUID(name, x, s1, s2, c0, c1, c2, c3, c4, c5, c6, c7) \
  extern const AM_GUID name = {x, s1, s2, {c0, c1, c2, c3, c4, c5, c6, c7}}

#endif

