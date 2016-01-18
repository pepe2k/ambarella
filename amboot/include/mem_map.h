/**
 ** system/include/ram_addr.h
 **
 ** History:
 **    2011/06/23 - [Kerson Chen] created file
 **
 ** Copyright (C) 2004-2008, Ambarella, Inc.
 **
 ** All rights reserved. No Part of this file may be reproduced, stored
 ** in a retrieval system, or transmitted, in any form, or by any means,
 ** electronic, mechanical, photocopying, recording, or otherwise,
 ** without the prior consent of Ambarella, Inc.
 **/

#ifndef __RAMADDR_H__
#define __RAMADDR_H__

#include <ambhw/ambhw.h>
#include <board.h>


#ifndef DRAM_START_ADDR
#if	(PHY_BUS_MAP_TYPE == 1)
#define DRAM_START_ADDR		0x00000000
#else
#define DRAM_START_ADDR		0xc0000000
#endif
#endif

#ifndef FIO_MEM_ALIGN_ADDR
#if	(PHY_BUS_MAP_TYPE == 1)
#define FIO_MEM_ALIGN_VIRT_ADDR		0x80000000
#define FIO_MEM_ALIGN_PHY_ADDR		DRAM_START_ADDR
#else
#define FIO_MEM_ALIGN_VIRT_ADDR		DRAM_START_ADDR
#define FIO_MEM_ALIGN_PHY_ADDR		DRAM_START_ADDR
#endif
#endif

#ifndef DRAM_SIZE
#error "DRAM_SIZE undefined!"
#endif

#define DRAM_END_ADDR		(DRAM_START_ADDR + DRAM_SIZE - 1)

#ifndef AMBOOT_BST_RAM_START
#define AMBOOT_BST_RAM_START	(DRAM_START_ADDR + (DRAM_SIZE - 0x00100000))
#endif

#ifndef AMBOOT_BLD_RAM_START
#if	(CHIP_REV == I1)
#define AMBOOT_BLD_RAM_START	0x00000000
#else
#define AMBOOT_BLD_RAM_START	0xc0000000
#endif
#endif

#if	(CHIP_REV == I1)
#define MEMFWPROG_RAM_START	0x00100000
#else
#define MEMFWPROG_RAM_START	0xc0100000
#endif

#ifndef MEMFWPROG_HOOKCMD_SIZE
#define MEMFWPROG_HOOKCMD_SIZE 0x00010000
#endif

#ifndef DSP_FW_DOWNLOAD_ADDR
/* Align 1st payload nicely to the start of the IDSP area */
#define DSP_FW_DOWNLOAD_ADDR	(IDSP_RAM_START - 0x50)
#endif

#endif

