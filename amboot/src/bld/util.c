/**
 * system/src/bld/util.c
 *
 * History:
 *    2005/07/25 - [Charles Chiou] created file
 *    2008//5/15 - [Allen Wang] moved to separate files for different
 *    		   chip revisions
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

/* Should be set to a valid address if hal is present. */
u32 g_haladdr = 0x0;

#if (CHIP_REV == A1)
#include "rct_a1.c"
#elif (CHIP_REV == A2)
#include "rct_a2.c"
#elif (CHIP_REV == A2M) || (CHIP_REV == A2S) || (CHIP_REV == A2Q)
#include "rct_a2s.c"
#elif (CHIP_REV == A3)
#include "rct_a3.c"
#elif (CHIP_REV == A5)
#include "rct_a5.c"
#elif (CHIP_REV == A5S)
#include "rct_a5s.c"
#elif (CHIP_REV == A5L)
#include "rct_a5l.c"
#elif (CHIP_REV == A6)
#include "rct_a6.c"
#elif (CHIP_REV == A7)
#include "rct_a7.c"
#elif (CHIP_REV == A7L)
#include "rct_a7l.c"
#elif (CHIP_REV == A7M)
#include "rct_a7m.c"
#elif (CHIP_REV == I1)
#include "rct_i1.c"
#elif (CHIP_REV == S2)
#include "rct_s2.c"
#else
#error "Architecture not supported!"
#endif

unsigned int g_hugebuf_addr = (unsigned int) &__bld_hugebuf;

void fio_exit_random_mode(void)
{
	u32 fio_ctr;

	/* Exit random read mode */
	fio_ctr = readl(FIO_CTR_REG);
	fio_ctr &= (~FIO_CTR_RR);
	writel(FIO_CTR_REG, fio_ctr);
}

void enable_fio_dma(void)
{
#if ((HOST_MAX_AHB_CLK_EN_BITS == 10) || (I2S_24BITMUX_MODE_REG_BITS == 4))
	u32 val;
#endif

#if (HOST_MAX_AHB_CLK_EN_BITS == 10)
	/* Disable boot-select */
	val = readl(HOST_AHB_CLK_ENABLE_REG);
	val &= ~(HOST_AHB_BOOT_SEL);
	val &= ~(HOST_AHB_FDMA_BURST_DIS);
	writel(HOST_AHB_CLK_ENABLE_REG, val);
#endif

#if (I2S_24BITMUX_MODE_REG_BITS == 4)
	val = readl(I2S_24BITMUX_MODE_REG);
	val &= ~(I2S_24BITMUX_MODE_DMA_BOOTSEL);
	val &= ~(I2S_24BITMUX_MODE_FDMA_BURST_DIS);
	writel(I2S_24BITMUX_MODE_REG, val);
#endif
}

/**
 * Check if memory access is sane.
 */
int check_mem_access(u32 start_addr, u32 end_addr)
{
	u32 CHK_START = DRAM_START_ADDR;
	u32 CHK_END = DRAM_START_ADDR + DRAM_SIZE - 1;

	if (start_addr <  CHK_START || start_addr > CHK_END   ||
	    end_addr   <  CHK_START || end_addr   > CHK_END) {
		return -1;
	}

	return 0;
}

int flash_timing(int minmax, int val)
{
	u32 clk, x;
	int n, r;

	/* to avoid overflow, divid clk by 1000000 first */
	clk = get_core_bus_freq_hz() / 1000000;
	x = val * clk;
#if (FIO_USE_2X_FREQ == 1)
	x *= 2;
#endif

	n = x / 1000;
	r = x % 1000;

	if (r != 0)
		n++;

	if (minmax)
		n--;

	return n < 1 ? 1 : n;
}

void clock_source_select(int src)
{
#if (RCT_SUPPORT_CLOCK_SEL == 1)
	if (readl(CORE_ARM_CLK_SRC_REG) != src)
		writel(CORE_ARM_CLK_SRC_REG, src);
#endif
}
