/**
 * system/src/bld/nor.c
 *
 * Flash controller functions with NOR chips.
 *
 * History:
 *    2005/04/13 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
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
#include "partinfo.h"

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))

extern flnor_t flnor;

/**
 * Initialize NOR parameters.
 */
int nor_init(void)
{
	u32 t0_reg, t1_reg, t2_reg, t3_reg, t4_reg, t5_reg;
	u8 tas, tcs, tds;
	u8 tah, tch, tdh;
	u8 twp, twh;
	u8 trc, tadelay, tcdelay, trpdelay;
	u8 trdelay, tabdelay, twhr, tir;
	u8 trhz, trph;

	tas	= FLASH_TIMING_MIN(NOR_TAS);
	tcs	= FLASH_TIMING_MIN(NOR_TCS);
	tds	= FLASH_TIMING_MIN(NOR_TDS);
	tah	= FLASH_TIMING_MIN(NOR_TAH);
	tch	= FLASH_TIMING_MIN(NOR_TCH);
	tdh	= FLASH_TIMING_MIN(NOR_TDH);
	twp	= FLASH_TIMING_MIN(NOR_TWP);
	twh	= FLASH_TIMING_MIN(NOR_TWH);
	trc	= FLASH_TIMING_MIN(NOR_TRC);
	tadelay	= FLASH_TIMING_MIN(NOR_TADELAY);
	tcdelay	= FLASH_TIMING_MIN(NOR_TCDELAY);
	trpdelay= FLASH_TIMING_MIN(NOR_TRPDELAY);
	trdelay	= FLASH_TIMING_MIN(NOR_TRDELAY);
	tabdelay= FLASH_TIMING_MIN(NOR_TABDELAY);
	twhr	= FLASH_TIMING_MIN(NOR_TWHR);
	tir	= FLASH_TIMING_MIN(NOR_TIR);
	trhz	= FLASH_TIMING_MIN(NOR_TRHZ);
	trph	= FLASH_TIMING_MIN(NOR_TRPH);

	t0_reg = NOR_TIM0_TAS(tas) |
		 NOR_TIM0_TCS(tcs) |
		 NOR_TIM0_TDS(tds);

	t1_reg = NOR_TIM1_TAH(tah) |
		 NOR_TIM1_TCH(tch) |
		 NOR_TIM1_TDH(tdh);

	t2_reg = NOR_TIM2_TWP(twp) |
 		 NOR_TIM2_TWH(twh);

	t3_reg = NOR_TIM3_TRC(trc) |
		 NOR_TIM3_TADELAY(tadelay) |
		 NOR_TIM3_TCDELAY(tcdelay) |
		 NOR_TIM3_TRPDELAY(trpdelay);

	t4_reg = NOR_TIM4_TRDELAY(trdelay) |
		 NOR_TIM4_TABDELAY(tabdelay) |
		 NOR_TIM4_TWHR(twhr) |
		 NOR_TIM4_TIR(tir);

	t5_reg = NOR_TIM5_TRHZ(trhz) |
		 NOR_TIM5_TRPH(trph);

	flnor.control = NOR_CONTROL;
	flnor.timing0 = t0_reg;
	flnor.timing1 = t1_reg;
	flnor.timing2 = t2_reg;
	flnor.timing3 = t3_reg;
	flnor.timing4 = t4_reg;
	flnor.timing5 = t5_reg;
	flnor.sector_size = NOR_SECTOR_SIZE;
	flnor.bst_ssec = NOR_BST_SSEC;
	flnor.bst_nsec = NOR_BST_NSEC;
	flnor.ptb_ssec = NOR_PTB_SSEC;
	flnor.ptb_nsec = NOR_PTB_NSEC;
	flnor.bld_ssec = NOR_BLD_SSEC;
	flnor.bld_nsec = NOR_BLD_NSEC;
	flnor.hal_ssec = NOR_HAL_SSEC;
	flnor.hal_nsec = NOR_HAL_NSEC;
	flnor.pba_ssec = NOR_PBA_SSEC;
	flnor.pba_nsec = NOR_PBA_NSEC;
	flnor.pri_ssec = NOR_PRI_SSEC;
	flnor.pri_nsec = NOR_PRI_NSEC;
	flnor.sec_ssec = NOR_SEC_SSEC;
	flnor.sec_nsec = NOR_SEC_NSEC;
	flnor.bak_ssec = NOR_BAK_SSEC;
	flnor.bak_nsec = NOR_BAK_NSEC;
	flnor.rmd_ssec = NOR_RMD_SSEC;
	flnor.rmd_nsec = NOR_RMD_NSEC;
	flnor.rom_ssec = NOR_ROM_SSEC;
	flnor.rom_nsec = NOR_ROM_NSEC;
	flnor.dsp_ssec = NOR_DSP_SSEC;
	flnor.dsp_nsec = NOR_DSP_NSEC;

	return 0;
}

/**
 * Reset NOR chip(s).
 */
int nor_reset(void)
{
	/* Setup Flash IO Control Register (exit random mode) */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Clear the FIO DMA Status Register */
	writel(FIO_DMASTA_REG, 0x0);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Setup NOR Flash Control Register */
	writel(NOR_CTR_REG, flnor.control);

#if 0
	/* Setup flash timing register */
	writel(NOR_TIM0_REG, flnor.timing0);
	writel(NOR_TIM1_REG, flnor.timing1);
	writel(NOR_TIM2_REG, flnor.timing2);
	writel(NOR_TIM3_REG, flnor.timing3);
	writel(NOR_TIM4_REG, flnor.timing4);
	writel(NOR_TIM5_REG, flnor.timing5);
#endif

	return 0;
}

/**
 * Read from NOR flash.
 */
static int nor_read(u32 addr, u8 *buf, int len)
{
	u32 status;

	_clean_flush_d_cache();

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, flnor.control);
#if (DMA_SUPPORT_DMA_FIOS == 1)
	/* Setup external DMA engine transfer */
	writel(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(DMA_FIOS_CHAN_SRC_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
	writel(DMA_FIOS_CHAN_DST_REG(FIO_DMA_CHAN), (u32) buf);
	writel(DMA_FIOS_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_WM		|
	       DMA_CHANX_CTR_NI		|
	       DMA_NODC_MN_BURST_SIZE	|
	       len);
#else
	/* Setup external DMA engine transfer */
	writel(DMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(DMA_CHAN_SRC_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
	writel(DMA_CHAN_DST_REG(FIO_DMA_CHAN), (u32) buf);
	writel(DMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_WM		|
	       DMA_CHANX_CTR_NI		|
	       DMA_NODC_MN_BURST_SIZE	|
	       len);
#endif

	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_FL		|
	       FIO_MN_BURST_SIZE	|
	       len);

	/* Wait for interrupt for DMA done */
#if (DMA_SUPPORT_DMA_FIOS == 1)
	while ((readl(DMA_FIOS_INT_REG) & DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0);
	writel(DMA_FIOS_INT_REG, 0x0);
#else
	while ((readl(DMA_INT_REG) & DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0);
	writel(DMA_INT_REG, 0x0);
#endif

	/* Wait for interrupt for NOR operation done */
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(FIO_DMASTA_REG, 0x0);
	writel(NOR_INT_REG, 0x0);

	/* Read Status */
	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Program to NOR flash.
 */
static int nor_prog(u32 addr, const u8 *buf, int len)
{
	u32 status;

	_clean_d_cache();

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, flnor.control);
#if (DMA_SUPPORT_DMA_FIOS == 1)
	/* Setup external DMA engine transfer */
	writel(DMA_FIOS_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(DMA_FIOS_CHAN_SRC_REG(FIO_DMA_CHAN), (u32) buf);
	writel(DMA_FIOS_CHAN_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
	writel(DMA_FIOS_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_RM		|
	       DMA_CHANX_CTR_NI		|
	       DMA_NODC_MN_BURST_SIZE	|
	       len);
#else
	/* Setup external DMA engine transfer */
	writel(DMA_CHAN_STA_REG(FIO_DMA_CHAN), 0x0);
	writel(DMA_CHAN_SRC_REG(FIO_DMA_CHAN), (u32) buf);
	writel(DMA_CHAN_DST_REG(FIO_DMA_CHAN), FIO_FIFO_BASE);
	writel(DMA_CHAN_CTR_REG(FIO_DMA_CHAN),
	       DMA_CHANX_CTR_EN		|
	       DMA_CHANX_CTR_RM		|
	       DMA_CHANX_CTR_NI		|
	       DMA_NODC_MN_BURST_SIZE	|
	       len);
#endif
	/* Write start address for memory target to */
	/* FIO DMA Address Register. */
	writel(FIO_DMAADR_REG, addr);

	/* Setup the Flash IO DMA Control Register */
	writel(FIO_DMACTR_REG,
	       FIO_DMACTR_EN		|
	       FIO_DMACTR_RM		|
	       FIO_DMACTR_FL		|
	       FIO_MN_BURST_SIZE	|
	       len);

	/* Wait for interrupt for DMA done */
#if (DMA_SUPPORT_DMA_FIOS == 1)
	while ((readl(DMA_FIOS_INT_REG) & DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0);
	writel(DMA_FIOS_INT_REG, 0x0);
#else
	while ((readl(DMA_INT_REG) & DMA_INT_CHAN(FIO_DMA_CHAN)) == 0x0);
	writel(DMA_INT_REG, 0x0);
#endif

	/* Wait for interrupt for NOR operation done */
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(FIO_DMASTA_REG, 0x0);
	writel(NOR_INT_REG, 0x0);

	/* Read Status */
	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Read a NOR flash sector.
 */
int nor_read_sector(u32 sector, u8 *buf, int len)
{
	return nor_read(sector * flnor.sector_size, buf, len);
}

/**
 * Program a NOR flash sector.
 */
int nor_prog_sector(u32 sector, u8 *buf, int len)
{
	return nor_prog(sector * flnor.sector_size, buf, len);
}

/**
 * Read data from NOR flash to memory.
 */
int nor_read_data(u8 *dst, u8 *src, int len)
{
	return -1;
}

/**
 * Erase a NOR flash sector.
 */
int nor_erase_sector(u32 sector)
{
	u32 addr = flnor.sector_size * sector;
	u32 status;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, flnor.control);

	/* Erase sector */
	writel(NOR_CMD_REG, NOR_CMD_ERASE | addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	/* Read Status */
	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Lock command.
 */
int nor_lock(u32 sector)
{
	u32 addr = flnor.sector_size * sector;
	u32 status;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, (flnor.control | NOR_CTR_LE));

	/* Lock sector */
	writel(NOR_CMD_REG, NOR_CMD_LOCK | addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	/* Read Status */
	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Unlock command.
 */
int nor_unlock(u32 sector)
{
	u32 addr = flnor.sector_size * sector;
	u32 status;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, (flnor.control | NOR_CTR_LE));

	/* Unlock sector */
	writel(NOR_CMD_REG, NOR_CMD_UNLOCK | addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	/* Read Status */
	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Lock down command.
 */
int nor_lock_down(u32 sector)
{
	u32 addr = flnor.sector_size * sector;
	u32 status;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, (flnor.control | NOR_CTR_LE | NOR_CTR_LD));

	/* Lock down sector */
	writel(NOR_CMD_REG, NOR_CMD_LOCKDOWN | addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	do {
		writel(NOR_CMD_REG, NOR_CMD_READSTATUS);
		while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
		writel(NOR_INT_REG, 0x0);

		status = readl(NOR_STA_REG);

	} while ((status & 0x80) != 0x80);

	return 0;
}

/**
 * Read CFI command.
 */
int nor_read_cfi(u8 bank, u32 *cfi)
{
	u32 addr = 0;
	u32 _addr = ((bank & 0x3) << 30) | ((addr & 0x3ffffff) << 4);

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, (flnor.control | NOR_CTR_CE));

	/* Read CFI */
	writel(NOR_CMD_REG, NOR_CMD_READCFI | _addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	*cfi = readl(NOR_CFI_REG);

	return 0;
}

/**
 * Read ID command.
 */
int nor_read_id(u8 bank, u32 *id)
{
	u32 addr = 0;
	u32 _addr = ((bank & 0x3) << 30) | ((addr & 0x3ffffff) << 4);

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, FIO_CTR_XD);

	/* Setup Flash Control Register */
	writel(NOR_CTR_REG, flnor.control);

	/* Read ID */
	writel(NOR_CMD_REG, NOR_CMD_READID | _addr);
	while ((readl(NOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(NOR_INT_REG, 0x0);

	*id = readl(NOR_ID_REG);

	return 0;
}
#else

/**
 * Read data from NOR flash to memory.
 */
int nor_read_data(u8 *dst, u8 *src, int len)
{
	return -1;
}

#endif
