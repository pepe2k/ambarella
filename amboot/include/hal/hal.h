/**
 * @file system/include/hal/hal.h
 *
 * History:
 *    2009/03/09 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __HAL_H__
#define __HAL_H__

#ifndef	__ASM__

#include <config.h>
#include <ambhw/chip.h>
#include <hal/header.h>
#include <ambhw/busaddr.h>

#if	(CHIP_REV == A5S)
#include "../sv/a5s/include/ambhal.h"
#include "../sv/a5s/include/ambhalmini.h"
#endif

#if	(CHIP_REV == A7)
#include <../sv/a7/include/ambhal.h>
#include <../sv/a7/include/ambhalmini.h>
#endif

#if	(CHIP_REV == A7L)
#include <../sv/a7l/include/ambhal.h>
#include <../sv/a7l/include/ambhalmini.h>
#endif

#if	(CHIP_REV == I1)
#include <../sv/i1/include/ambhal.h>
#include <../sv/i1/include/ambhalmini.h>
#endif

#if	(CHIP_REV == S2)
#include <../sv/s2/include/ambhal.h>
#include <../sv/s2/include/ambhalmini.h>
#endif

#if	(CHIP_REV == A8)
#include <../sv/a8/include/ambhal.h>
#include <../sv/a8/include/ambhalmini.h>
#endif

extern int hal_init(void);
extern int hal_call_name(int, char **);
extern int hal_call_fid(u32, u32, u32, u32, u32);
extern u32 get_bst_version(void);
extern void set_dram_arbitration(void);
extern u32 g_haladdr;
extern u32 g_halsize;

#define IS_HAL_INIT()	(g_haladdr == HAL_BASE_VIRT)

#endif	/* __ASM__ */

#define HAL_BASE_PHYS	(DRAM_START_ADDR + 0x000a0000)
#ifdef AMBARELLA_HAL_REMAP
#define HAL_BASE_VIRT	0xfee00000
#else
#define HAL_BASE_VIRT	(DRAM_START_ADDR + 0x000a0000)
#endif
#define HAL_BASE_VP	((void *) HAL_BASE_VIRT)

#if	(CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1) || \
	(CHIP_REV == A7L) || (CHIP_REV == S2) || (CHIP_REV == A8)
#define	USE_HAL		1
#endif

#endif
