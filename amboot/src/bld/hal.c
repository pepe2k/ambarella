/**
 * system/src/bld/hal.c
 *
 * History:
 *    2008/11/18 - [Charles Chiou] created file
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
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>
#include <hal/hal.h>

#if defined(USE_HAL)
#include "bst_ver.h"
#endif

extern int fwprog_load_partition(int dev, int part_id, const flpart_t *part,
				 int verbose, int override);
int hal_init(void)
{
	int rval = 0;
#if defined(USE_HAL)
	flpart_table_t ptb;
	amb_hal_header_t *header;
	extern void *__pagetable_end;

	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto done;

	/* Make sure page table does not overlap with HAL. */
	K_ASSERT((u32)&__pagetable_end < HAL_BASE_PHYS);

	if (g_part_dev[PART_HAL] == BOOT_DEV_NAND) {
		rval = fwprog_load_partition(BOOT_DEV_NAND, PART_HAL,
					     &ptb.part[PART_HAL], 0, 0);
	} else if (g_part_dev[PART_HAL] == BOOT_DEV_NOR) {
		rval = fwprog_load_partition(BOOT_DEV_NOR, PART_HAL,
					     &ptb.part[PART_HAL], 0, 0);
	} else if (g_part_dev[PART_HAL] == BOOT_DEV_SNOR) {
		rval = fwprog_load_partition(BOOT_DEV_SNOR, PART_HAL,
					     &ptb.part[PART_HAL], 0, 0);
	} else if (g_part_dev[PART_HAL] == BOOT_DEV_ONENAND) {
		rval = fwprog_load_partition(BOOT_DEV_ONENAND, PART_HAL,
					     &ptb.part[PART_HAL], 0, 0);
	} else if (g_part_dev[PART_HAL] == BOOT_DEV_SM) {
		rval = fwprog_load_partition(BOOT_DEV_SM, PART_HAL,
					     &ptb.part[PART_HAL], 0, 0);
	}

done:
#ifdef AMBARELLA_HAL_REMAP
{
		/* Remap MMU to point the pages in HAL_BASE_VIRT to */
		/* the physical address */
		extern u32 __pagetable_main;
		extern u32 __pagetable_hal;
		u32 pgtbl, base, x, size, val;

		if ((HAL_BASE_VIRT & 0x000fffff) != 0x0) {
			putstr("HAL_BASE_VIRT is not 1MB aligned!\r\n");
			K_ASSERT(0);
		}

		/* Disable the MMU */
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));
		x &= ~(0x1);
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));

		pgtbl = (u32) &__pagetable_main;

		/* Setup HAL_BASE_VIRT to point to a corse page table */
		base = HAL_BASE_VIRT >> 20;
		x = (u32) &__pagetable_hal;
#if defined(__ARM926EJS__)
		x |= 0x11;		/* ARM926EJS */
#endif
#if defined(__ARM1136JS__)
		x |= 0x01;		/* ARM1136JS */
#endif
		writel(pgtbl + (base  * 4), x);

		/* Now setup the 2nd-level pagetable for HAL */
		base = (u32) &__pagetable_hal;
		size = ptb.part[PART_HAL].img_len;
		val = ptb.part[PART_HAL].mem_addr;
		for (x = 0; x < 256; x++) {
			val &= 0xfffff000;
			if ((x * 4096) <= (size + 4096)) {
				val |= 0xffe;
			}
			writel(base + x * 4, val);
			val += 0x1000;
		}

		/* Flush the TLB and re-enable the MMU */
		x = 0x0;
		__asm__ __volatile__ ("mcr p15, 0, %0, c8, c6, 0": : "r" (x));
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));
		x |= 0x1;
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));

}
#endif
	header = (amb_hal_header_t *) HAL_BASE_VIRT;
	if (rval == 0 || (strcmp(header->magic, "Ambarella") == 0)) {
		/* Setup base pointer */
		g_haladdr = HAL_BASE_VIRT;

		/* Hack: write HAL size into the header offset 0xc */
		writel(g_haladdr + 0xc, ptb.part[PART_HAL].img_len);
#if defined(USE_HAL)
		/* Hack: write BST version into the header offset 0xc + img_len */
		writel(g_haladdr + ptb.part[PART_HAL].img_len, BST_VER);
#endif
#if (PHY_BUS_MAP_TYPE >= 1)
	/* Call the first setup function */
	amb_hal_init((void *)g_haladdr,
		     (void *)APB_BASE, (void *)AHB_BASE, (void *)DRAM_VIRT_BASE);
#else
		/* Call the first setup function */
		amb_hal_init((void *)g_haladdr,
			     (void *)APB_BASE, (void *)AHB_BASE);
#endif
	} else {
		putstr("HAL is not present in system!\r\n");
		K_ASSERT(0);
	}
#endif

	return rval;
}

int hal_call_name(int argc, char **argv)
{
	u32 fid;
	u32 arg0 = 0, arg1 = 0, arg2 = 0, arg3 = 0;
	amb_hal_function_info_t *finfo;

	if (argc <= 0)
		return -1;

	finfo = (amb_hal_function_info_t *) (g_haladdr + 128);

	if (strtou32(argv[0], &fid) < 0) {
		for (fid = 0; ; fid++) {
			if (finfo[fid].function == NULL)
				return -1;
			if (strcmp(argv[0],
				   (char *) (g_haladdr + (u32) finfo[fid].name))
			    == 0)
				break;
		}
	}

	if (argc >= 3)
		strtou32(argv[2], &arg0);
	if (argc >= 4)
		strtou32(argv[3], &arg1);
	if (argc >= 5)
		strtou32(argv[4], &arg2);
	if (argc >= 6)
		strtou32(argv[5], &arg3);


	return hal_call_fid(fid, arg0, arg1, arg2, arg3);
}

int hal_call_fid(u32 fid, u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
	amb_hal_function_info_t *finfo;
	int (*f)(u32, u32, u32, u32);

	if (g_haladdr < 0)
		return -1;

	finfo = (amb_hal_function_info_t *) (g_haladdr + 128);
	f = (int (*) (u32, u32, u32, u32))
		(g_haladdr + (u32) finfo[fid].function);

	return f(arg0, arg1, arg2, arg3);
}

void set_dram_arbitration(void)
{
#if !defined(USE_HAL)

#if (DRAM_ARB_SUPPORT_THROTTLE_DL == 0)
#ifndef DRAM_ARB_CFG_VAL
#define DRAM_ARB_CFG_VAL	0x0bb80000
#endif
	writel(DRAM_ARB_CFG_REG, DRAM_ARB_CFG_VAL);
#endif

#if (DRAM_ARB_SUPPORT_THROTTLE_DL == 1)
#ifdef DRAM_ARB_THROTTLE_DL_VAL
	writel(DRAM_ARB_THROTTLE_DL_REG, DRAM_ARB_THROTTLE_DL_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_0_VAL
	writel(DRAM_ARB_SMM_TRANS_0, DRAM_ARB_SMM_TRANS_0_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_1_VAL
	writel(DRAM_ARB_SMM_TRANS_1, DRAM_ARB_SMM_TRANS_1_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_2_VAL
	writel(DRAM_ARB_SMM_TRANS_2, DRAM_ARB_SMM_TRANS_2_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_3_VAL
	writel(DRAM_ARB_SMM_TRANS_3, DRAM_ARB_SMM_TRANS_3_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_4_VAL
	writel(DRAM_ARB_SMM_TRANS_4, DRAM_ARB_SMM_TRANS_4_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_5_VAL
	writel(DRAM_ARB_SMM_TRANS_5, DRAM_ARB_SMM_TRANS_5_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_6_VAL
	writel(DRAM_ARB_SMM_TRANS_6, DRAM_ARB_SMM_TRANS_6_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_7_VAL
	writel(DRAM_ARB_SMM_TRANS_7, DRAM_ARB_SMM_TRANS_7_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_8_VAL
	writel(DRAM_ARB_SMM_TRANS_8, DRAM_ARB_SMM_TRANS_8_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_9_VAL
	writel(DRAM_ARB_SMM_TRANS_9, DRAM_ARB_SMM_TRANS_9_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_10_VAL
	writel(DRAM_ARB_SMM_TRANS_10, DRAM_ARB_SMM_TRANS_10_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_11_VAL
	writel(DRAM_ARB_SMM_TRANS_11, DRAM_ARB_SMM_TRANS_11_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_12_VAL
	writel(DRAM_ARB_SMM_TRANS_12, DRAM_ARB_SMM_TRANS_12_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_13_VAL
	writel(DRAM_ARB_SMM_TRANS_13, DRAM_ARB_SMM_TRANS_13_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_14_VAL
	writel(DRAM_ARB_SMM_TRANS_14, DRAM_ARB_SMM_TRANS_14_VAL);
#endif
#ifdef DRAM_ARB_SMM_TRANS_15_VAL
	writel(DRAM_ARB_SMM_TRANS_15, DRAM_ARB_SMM_TRANS_15_VAL);
#endif
#endif

#endif

#ifdef DRAMARB_PRIORITY_VAL
	amb_set_dram_arbiter_priority(HAL_BASE_VP, DRAMARB_PRIORITY_VAL);
#endif

#if (CHIP_REV == I1)
	writel((DRAM_VIRT_BASE + 0x24), 0x1);
#if !defined(BOARD_DISABLE_ETH)
#if defined(BOARD_USE_INTERNAL_GTX)
	amb_set_gtx_clock_source(HAL_BASE_VP, AMB_GTX_CLOCK_SOURCE_GTX_PLL);
#else
	amb_set_gtx_clock_source(HAL_BASE_VP,
		AMB_GTX_CLOCK_SOURCE_ENET_GTX_CLK);
#endif
#if defined(BOARD_USE_MII_PHY)
	amb_set_gtx_clock_frequency(HAL_BASE_VP, 25000000);
#else
	amb_set_gtx_clock_frequency(HAL_BASE_VP, 125000000);
#endif
#endif
#endif
}

