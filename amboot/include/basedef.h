/**
 * system/include/basedef.h
 *
 * History:
 *    2004/10/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __BASEDEF_H__
#define __BASEDEF_H__

#include <hwio.h>

#ifndef __ASM__

typedef unsigned char  	    u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short 	    u16;/**< UNSIGNED 16-bit data type */
typedef unsigned int   	   u32;	/**< UNSIGNED 32-bit data type */
typedef unsigned long long u64; /**< UNSIGNED 64-bit data type */
typedef signed char         s8;	/**< SIGNED 8-bit data type */
typedef signed short       s16;	/**< SIGNED 16-bit data type */
typedef signed int         s32;	/**< SIGNED 32-bit data type */
typedef signed long long   s64; /**< SIGNED 64-bit data type */

#ifndef _BSDTYPES_DEFINED
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
#define _BSDTYPES_DEFINED
#define UINT_ALREADY
#endif

#ifndef NULL
#define NULL ((void *) 0x0)
#endif

#if defined __cplusplus && defined(__GNUC__)
#undef NULL
#define NULL 0x0
#endif

#if defined(__GNUC__)
/* GNU Compiler Setting */
#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__  __attribute__ ((packed))
#endif
#ifndef __ARMCC_PACK__
#define __ARMCC_PACK__
#endif
#ifndef __GNU_ALIGN
#define __GNU_ALIGN(x)	__attribute__ ((aligned(x)))
#endif
#ifndef __ARMCC_ALIGN
#define __ARMCC_ALIGN(x)
#endif
#ifndef __ATTRIB_WEAK__
#define __ATTRIB_WEAK__	__attribute__((weak))
#endif
#ifndef __ARMCC_WEAK__
#define __ARMCC_WEAK__
#endif
#elif defined(__arm)
/* RVCT or ADS Compiler setting */
#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__
#endif
#ifndef __ARMCC_PACK__
#define __ARMCC_PACK__  __packed
#endif
#ifndef __GNU_ALIGN
#define __GNU_ALIGN(x)
#endif
#ifndef __ARMCC_ALIGN
#define __ARMCC_ALIGN(x) __align(x)
#endif
#define inline __inline
#ifndef __ATTRIB_WEAK__
#define __ATTRIB_WEAK__
#endif
#ifndef __ARMCC_WEAK__
#define __ARMCC_WEAK__	__weak
#endif
#else
/* Unknown compiler */
#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__
#endif
#ifndef __ARMCC_PACK__
#define __ARMCC_PACK__
#endif
#ifndef __GNU_ALIGN
#define __GNU_ALIGN(x)
#endif
#ifndef __ARMCC_ALIGN
#define __ARMCC_ALIGN(x)
#endif
#ifndef __ATTRIB_WEAK__
#define __ATTRIB_WEAK__
#endif
#ifndef __ARMCC_WEAK__
#define __ARMCC_WEAK__
#endif
#endif

#ifdef  __cplusplus
#ifndef __BEGIN_C_PROTO__
#define __BEGIN_C_PROTO__ extern "C" {
#endif
#ifndef __END_C_PROTO__
#define __END_C_PROTO__ }
#endif
#else
#ifndef __BEGIN_C_PROTO__
#define __BEGIN_C_PROTO__
#endif
#ifndef __END_C_PROTO__
#define __END_C_PROTO__
#endif
#endif

#endif  /* !__ASM__ */

#ifndef xstr
#define xstr(s) str(s)
#endif

#ifndef str
#define str(s) #s
#endif

#define AMB_VER_NUM(major,minor) ((major << 16) | (minor))
#define AMB_VER_DATE(year,month,day) ((year << 16) | (month << 8) | day)

#endif
