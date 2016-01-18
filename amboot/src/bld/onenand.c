/**
 * system/src/bld/onenand.c
 *
 * Flash controller functions with NOR chips.
 *
 * History:
 *    2009/10/09 - [Evan Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <fio/ftl_const.h>
#include <flash/onenand/command.h>
#include "partinfo.h"

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))

#ifdef ONENAND_DEBUG
#define DEBUG_MSG uart_putstr
#else
#define DEBUG_MSG(...)
#endif

#ifdef ONENAND_DEBUG
#define DEBUG_HEX_MSG uart_puthex
#else
#define DEBUG_HEX_MSG(...)
#endif

#ifdef ONENAND_DEBUG
#define DEBUG_DEC_MSG uart_putdec
#else
#define DEBUG_DEC_MSG(...)
#endif

/**
 * 0: don't use DMA
 * 1: use DMA engine
 * 2: DMA for all xfer
 */
#define USE_DMA_XFER			2

#define ONENAND_MIN_MAIN_DMA_SIZE	2048
#define ONENAND_MIN_SPARE_DMA_SIZE	64
#define ONENAND_READ_ALIGN		2048
#define ONENAND_PROG_ALIGN		2048

#define ONENAND_AREA_MAIN		0
#define ONENAND_AREA_SPARE		1

#if	(USE_DMA_XFER == 2)
static u8 g_onenand_buf[ONENAND_MIN_MAIN_DMA_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));
#endif

extern flnand_t flnand;

#define ONENAND_AMB_CMD_READ_REG		0x0
#define ONENAND_AMB_CMD_WRITE_REG		0x1
#define ONENAND_AMB_CMD_POLL_REG		0x2
#define ONENAND_AMB_CMD_READ_ASYNC		0x3
#define ONENAND_AMB_CMD_READ_SPARE_ASYNC	0x4
#define ONENAND_AMB_CMD_READ_SYNC		0x5
#define ONENAND_AMB_CMD_READ_SPARE_SYNC		0x6
#define ONENAND_AMB_CMD_PROG_ASYNC		0x7
#define ONENAND_AMB_CMD_PROG_SPARE_ASYNC	0x8
#define ONENAND_AMB_CMD_PROG_SYNC		0x9
#define ONENAND_AMB_CMD_PROG_SPARE_SYNC		0xa
#define ONENAND_AMB_CMD_MAX			0xb

#define ONENAND_POLL_MODE_NONE			0
#define ONENAND_POLL_MODE_INT_SIGNAL		3
#define ONENAND_POLL_MODE_REG_POLL		4
#define ONENAND_POLL_MODE_REG_INT_POLL		5

#define ONENAND_PROG_MODE_NORMAL		0

struct onenand_ctr_s {
	u32 ctr1;
	u32 ctr2;
	u32 poll_ctr1;		/* poll mode setting in ctr1 */
	u32 poll_ctr2;		/* poll mode setting in ctr2 */

	struct onenand_cmd_s {
		u16 addr[ONENAND_MAX_CMD_CYC];
		u16 data[ONENAND_MAX_ADDR_CYC];
		u8  cmd_cyc;
		u8  data_cyc;
		u8  poll_mode;
		u8  prog_mode;
		int timeout;
	} cmd[ONENAND_AMB_CMD_MAX];

	u8 data_poll_target_bit;	/* data poll or toggposition */
	u8 read_mode;
	u8 prog_mode;
/* ONE NAND PROG/READ mode support(for device) */
#define ONENAND_SUPPORT_ASYNC_MODE	0x0
#define ONENAND_SUPPORT_SYNC_MODE	0x1
} onenand_ctr;

#define BLD_ONENAND_RETURN_RVAL(rval)					\
{									\
	if (rval < 0)							\
		return rval;						\
}

#define PAGE_SIZE_512	512
#define PAGE_SIZE_2K	2048

static u8 buffer[PAGE_SIZE_2K] __attribute__ ((aligned(32)));

/**
 * Calculate address from the (block, page) pair.
 */
static u32 addr_from_block_page(u32 block, u32 page)
{
	u32 rval = (block * flnand.pages_per_block + page) *
			flnand.main_size;

#if 0
	if (flnand.main_size == PAGE_SIZE_512)
		return (rval << 9);
	else if (flnand.main_size == PAGE_SIZE_2K)
		return (rval << 11);

	return -1;
#endif
	return rval;
}

static void onenand_setup_cmd_data(struct onenand_cmd_s *cmd)
{
	u32 val;

	if (cmd->cmd_cyc >= 1) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[1]) |
		      ONENAND_CMD_ADDR_LO(cmd->addr[0]);
		writel(ONENAND_CMD_ADDR0_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[1]) |
		      ONENAND_CMD_WORD_LO(cmd->data[0]);
		writel(ONENAND_CMD_WORD0_REG, val);
	}

	if (cmd->cmd_cyc >= 3) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[3]) |
		      ONENAND_CMD_ADDR_LO(cmd->addr[2]);
		writel(ONENAND_CMD_ADDR1_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[3]) |
		      ONENAND_CMD_WORD_LO(cmd->data[2]);
		writel(ONENAND_CMD_WORD1_REG, val);
	}

	if (cmd->cmd_cyc >= 5) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[5]) |
		      ONENAND_CMD_WORD_LO(cmd->addr[4]);
		writel(ONENAND_CMD_ADDR2_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[5]) |
		      ONENAND_CMD_WORD_LO(cmd->data[4]);
		writel(ONENAND_CMD_WORD2_REG, val);
	}
}

static void onenand_setup_cmd_data_dma(struct onenand_cmd_s *cmd,
				       u32 block, u32 page)
{
	u32 val;

	if (cmd->cmd_cyc >= 1) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[1]) |
		      ONENAND_CMD_ADDR_LO(cmd->addr[0]);
		writel(ONENAND_CMD_ADDR0_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[1]) |
		      ONENAND_CMD_WORD_LO(cmd->data[0]);
		val |= (ONENAND_CMD_WORD_HI(page) + block);
		writel(ONENAND_CMD_WORD0_REG, val);
	}

	if (cmd->cmd_cyc >= 3) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[3]) |
		      ONENAND_CMD_ADDR_LO(cmd->addr[2]);
		writel(ONENAND_CMD_ADDR1_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[3]) |
		      ONENAND_CMD_WORD_LO(cmd->data[2]);
		writel(ONENAND_CMD_WORD1_REG, val);
	}

	if (cmd->cmd_cyc >= 5) {
		val = ONENAND_CMD_ADDR_HI(cmd->addr[5]) |
		      ONENAND_CMD_WORD_LO(cmd->addr[4]);
		writel(ONENAND_CMD_ADDR2_REG, val);

		val = ONENAND_CMD_WORD_HI(cmd->data[5]) |
		      ONENAND_CMD_WORD_LO(cmd->data[4]);
		writel(ONENAND_CMD_WORD2_REG, val);
	}
}

static void onenand_set_timing(void)
{
	u32 t0_reg, t1_reg, t2_reg, t3_reg, t4_reg;
	u32 t5_reg, t6_reg, t7_reg, t8_reg, t9_reg;
	u8 taa, toe, toeh, tce;
	u8 tpa, trp, trh, toes;
	u8 tcs, tch, twp, twh;
	u8 trb, taht, taso, toeph;
	u8 tavdcs, tavdch, tas, tah;
	u8 telf, tds, tdh, tbusy;
	u8 tdp, tceph, toez, tba;
	u16 tready;	u8 tach, tcehp;
	u8 tavds, toh, tavdp, taavdh;
	u8 tiaa, tclk_period, twea, tacs;

	taa 		= FLASH_TIMING_MIN(ONENAND_TAA);
	toe 		= FLASH_TIMING_MAX(ONENAND_TOE);
	toeh		= FLASH_TIMING_MIN(ONENAND_TOEH);
	tce 		= FLASH_TIMING_MAX(ONENAND_TCE);

	tpa 		= FLASH_TIMING_MIN(ONENAND_TPA);
	trp 		= FLASH_TIMING_MIN(ONENAND_TRP);
	trh 		= FLASH_TIMING_MAX(ONENAND_TRH);
	toes		= FLASH_TIMING_MIN(ONENAND_TOES);

	tcs		= FLASH_TIMING_MIN(ONENAND_TCS);
	tch		= FLASH_TIMING_MIN(ONENAND_TCH);
	twp		= FLASH_TIMING_MIN(ONENAND_TWP);
	twh		= FLASH_TIMING_MIN(ONENAND_TWH);

	trb		= FLASH_TIMING_MIN(ONENAND_TRB);
	taht		= FLASH_TIMING_MIN(ONENAND_TAHT);
	taso		= FLASH_TIMING_MIN(ONENAND_TASO);
	toeph		= FLASH_TIMING_MIN(ONENAND_TOEPH);

	tavdcs		= FLASH_TIMING_MIN(ONENAND_TAVDCS);
	tavdch		= FLASH_TIMING_MIN(ONENAND_TAVDCH);
	tas		= FLASH_TIMING_MIN(ONENAND_TAS);
	tah		= FLASH_TIMING_MIN(ONENAND_TAH);

	telf		= FLASH_TIMING_MIN(ONENAND_TELF);
	tds		= FLASH_TIMING_MIN(ONENAND_TDS);
	tdh		= FLASH_TIMING_MIN(ONENAND_TDH);
	tbusy		= FLASH_TIMING_MAX(ONENAND_TBUSY);

	tdp		= FLASH_TIMING_MIN(ONENAND_TDP);
	tceph		= FLASH_TIMING_MIN(ONENAND_TCEPH);
	toez		= FLASH_TIMING_MIN(ONENAND_TOEZ);
	tba		= FLASH_TIMING_MIN(ONENAND_TBA);

	tready		= FLASH_TIMING_MIN(ONENAND_TREADY);
	tach		= FLASH_TIMING_MIN(ONENAND_TACH);
	tcehp		= FLASH_TIMING_MIN(ONENAND_TCEHP);

	tavds		= FLASH_TIMING_MIN(ONENAND_TAVDS);
	toh		= FLASH_TIMING_MIN(ONENAND_TOH);
	tavdp		= FLASH_TIMING_MIN(ONENAND_TAVDP);
	taavdh		= FLASH_TIMING_MIN(ONENAND_TAAVDH);

	tiaa		= FLASH_TIMING_MIN(ONENAND_TIAA);
	tclk_period 	= FLASH_TIMING_MIN(ONENAND_TCLK_PERIOD);
	twea		= FLASH_TIMING_MIN(ONENAND_TWEA);
	tacs		= FLASH_TIMING_MIN(ONENAND_TACS);

	t0_reg = ONENAND_TIM0_TAA(taa)	|
		 ONENAND_TIM0_TOE(toe)	|
		 ONENAND_TIM0_TOEH(toeh)|
		 ONENAND_TIM0_TCE(tce);

	t1_reg = ONENAND_TIM1_TPA(tpa)	|
		 ONENAND_TIM1_TRP(trp)	|
		 ONENAND_TIM1_TRH(trh)	|
		 ONENAND_TIM1_TOES(toes);

	t2_reg = ONENAND_TIM2_TCS(tcs)	|
		 ONENAND_TIM2_TCH(tch)	|
		 ONENAND_TIM2_TWP(twp)	|
		 ONENAND_TIM2_TWH(twh);

	t3_reg = ONENAND_TIM3_TRB(trb)		|
		 ONENAND_TIM3_TAHT(taht)	|
		 ONENAND_TIM3_TASO(taso)	|
		 ONENAND_TIM3_TOEPH(toeph);

	t4_reg = ONENAND_TIM4_TAVDCS(tavdcs)	|
		 ONENAND_TIM4_TAVDCH(tavdch)	|
		 ONENAND_TIM4_TAS(tas)		|
		 ONENAND_TIM4_TAH(tah);

	t5_reg = ONENAND_TIM5_TELF(telf)	|
		 ONENAND_TIM5_TDS(tds)		|
		 ONENAND_TIM5_TDH(tdh)		|
		 ONENAND_TIM5_TBUSY(tbusy);

	t6_reg = ONENAND_TIM6_TDP(tdp)		|
		 ONENAND_TIM6_TCEPH(tceph)	|
		 ONENAND_TIM6_TOEZ(toez)	|
		 ONENAND_TIM6_TBA(tba);

	t7_reg = ONENAND_TIM7_TREADY(tready)	|
		 ONENAND_TIM7_TACH(tach)	|
		 ONENAND_TIM7_TCEHP(tcehp);

	t8_reg = ONENAND_TIM8_TAVDS(tavds)	|
		 ONENAND_TIM8_TOH(toh)		|
		 ONENAND_TIM8_TAVDP(tavdp)	|
		 ONENAND_TIM8_TAAVDH(taavdh);

	t9_reg = ONENAND_TIM9_TIAA(tiaa)		|
		 ONENAND_TIM9_TCLK_PERIOD(tclk_period)	|
		 ONENAND_TIM9_TWEA(twea)		|
		 ONENAND_TIM9_TACS(tacs);

	flnand.timing0 = t0_reg;
	flnand.timing1 = t1_reg;
	flnand.timing2 = t2_reg;
	flnand.timing3 = t3_reg;
	flnand.timing4 = t4_reg;
	flnand.timing5 = t5_reg;
	flnand.timing6 = t6_reg;
	flnand.timing7 = t7_reg;
	flnand.timing8 = t8_reg;
	flnand.timing9 = t9_reg;

	/* Setup flash timing register */
	writel(ONENAND_TIM0_REG, flnand.timing0);
	writel(ONENAND_TIM1_REG, flnand.timing1);
	writel(ONENAND_TIM2_REG, flnand.timing2);
	writel(ONENAND_TIM3_REG, flnand.timing3);
	writel(ONENAND_TIM4_REG, flnand.timing4);
	writel(ONENAND_TIM5_REG, flnand.timing5);
	writel(ONENAND_TIM6_REG, flnand.timing6);
	writel(ONENAND_TIM7_REG, flnand.timing7);
	writel(ONENAND_TIM8_REG, flnand.timing8);
	writel(ONENAND_TIM9_REG, flnand.timing9);
}

static int onenand_read_dma(u32 addr, u32 len, const u8 *buf, u8 area)
{
	struct onenand_cmd_s *cmd;
	u32 val;
	u32 block, page;
	u8  bank;

	bank = addr / (flnand.block_size * flnand.blocks_per_bank);
	page = (addr / flnand.main_size) & (flnand.pages_per_block - 1);
	block = (addr / flnand.main_size - page) / flnand.pages_per_block;

	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE) {
		cmd = (area == ONENAND_AREA_MAIN) ?
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SYNC]) :
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_SYNC]);
	} else {
		cmd = (area == ONENAND_AREA_MAIN) ?
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_ASYNC]) :
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_ASYNC]);
	}

	clean_flush_d_cache((void *) buf, len);

	/* Setup Flash IO Control Register */
	val = (area == ONENAND_AREA_MAIN) ? (0) : (FIO_CTR_RS);
#if defined(__FPGA__)
	/* Force to use FDMA on FPGA */
	val |= 0x40000;
#endif
	writel(FIO_CTR_REG, val);

	/* Setup Flash Control Register */
	val = onenand_ctr.ctr1 |
	      ONENAND_CTR1_CE_SEL(bank) |
	      ONENAND_CTR1_POLL_MODE(cmd->poll_mode)|
	      ONENAND_CTR1_AVD_EN;
	if (area == ONENAND_AREA_SPARE)
		val |= ONENAND_CTR1_SPARE_EN;
	writel(ONENAND_CTR1_REG, val);

	val = onenand_ctr.ctr2 |
	      onenand_ctr.ctr2 |
	      ONENAND_CTR2_CMD_CYC(cmd->cmd_cyc) |
	      ONENAND_CTR2_DTA_CYC(cmd->data_cyc) |
	      ONENAND_CTR2_PROG_MODE(cmd->prog_mode);
    	val &= ~ONENAND_CTR2_POLL_POLAR;
	val |= ONENAND_CTR2_ONENAND_DMA;
	val |= ONENAND_CTR2_DMA_CMD;
	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_CTR2_OP_SYNC;
	writel(ONENAND_CTR2_REG, val);

	/* Setup ONENAND command/data words */
	onenand_setup_cmd_data_dma(cmd, block, page);

	/* setup Command Register */
	val = ONENAND_CMD_DMA;
	val |= (area == ONENAND_AREA_MAIN) ?
	       (ONENAND_CMD_ADDR(ONENAND_DAT_RAM_START)) :
	       (ONENAND_CMD_ADDR(ONENAND_SPARE_RAM_START));
	writel(ONENAND_CMD_REG, val);

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
	while ((readl(ONENAND_INT_REG) & NAND_INT_DI) == 0x0);

	writel(FIO_DMASTA_REG, 0x0);
	writel(ONENAND_INT_REG, 0x0);

	return 0;
}

static int onenand_prog_dma(u32 addr, u32 len, const u8 *buf, u8 area)
{
	struct onenand_cmd_s *cmd;
	u32 val;
	u32 block, page;
	u8  bank;

	bank = addr / (flnand.block_size * flnand.blocks_per_bank);
	page = (addr / flnand.main_size) & (flnand.pages_per_block - 1);
	block = (addr / flnand.main_size - page) / flnand.pages_per_block;

	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE) {
		cmd = (area == ONENAND_AREA_MAIN) ?
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SYNC]) :
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_SYNC]);
	} else {
		cmd = (area == ONENAND_AREA_MAIN) ?
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_ASYNC]) :
		      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_ASYNC]);
	}

	clean_flush_d_cache((void *) buf, len);

	/* Setup Flash IO Control Register */
	val = (area == ONENAND_AREA_MAIN) ? (0) : (FIO_CTR_RS);
#if defined(__FPGA__)
	/* Force to use FDMA on FPGA */
	val |= 0x40000;
#endif
	writel(FIO_CTR_REG, val);

	/* Setup Flash Control Register */
	val = onenand_ctr.ctr1 |
	      ONENAND_CTR1_CE_SEL(bank) |
	      ONENAND_CTR1_POLL_MODE(cmd->poll_mode) |
	      ONENAND_CTR1_AVD_EN;
	writel(ONENAND_CTR1_REG, val);

	val = onenand_ctr.ctr2 |
	      onenand_ctr.ctr2 |
	      ONENAND_CTR2_CMD_CYC(cmd->cmd_cyc) |
	      ONENAND_CTR2_DTA_CYC(cmd->data_cyc) |
	      ONENAND_CTR2_PROG_MODE(cmd->prog_mode);
    	val &= ~ONENAND_CTR2_POLL_POLAR;
	val |= ONENAND_CTR2_ONENAND_DMA;
	val |= ONENAND_CTR2_DMA_CMD;
	if (onenand_ctr.prog_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_CTR2_OP_SYNC;
	writel(ONENAND_CTR2_REG, val);

	/* Setup ONENAND command/data words */
	onenand_setup_cmd_data_dma(cmd, block, page);

	/* setup Command Register */
	val = ONENAND_CMD_DMA |
	      ONENAND_CMD_ADDR(ONENAND_DEV_CTR_STA_REG);
	writel(ONENAND_CMD_REG, val);

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
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(FIO_DMASTA_REG, 0x0);
	writel(SNOR_INT_REG, 0x0);

	/* Restore write protect */
	val = readl(SNOR_CTR1_REG) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	return 0;
}

/* ------------------------------------------------------------------------- */

int onenand_read_reg(u8 bank, u16 addr, u16 *single_r_data)
{
	struct onenand_cmd_s *cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_REG];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = onenand_ctr.ctr1 |
	      ONENAND_CTR1_CE_SEL(bank) |
	      ONENAND_CTR1_POLL_MODE(cmd->poll_mode) |
	      ONENAND_CTR1_AVD_EN;;
	writel(ONENAND_CTR1_REG, val);

	val = onenand_ctr.ctr2 |
	      ONENAND_CTR2_CMD_CYC(cmd->cmd_cyc) |
	      ONENAND_CTR2_DTA_CYC(cmd->data_cyc) |
	      ONENAND_CTR2_PROG_MODE(cmd->prog_mode);
	writel(ONENAND_CTR2_REG, val);

	/* Setup ONENAND command/data words */
	onenand_setup_cmd_data(cmd);

	/* Setup command register */
	val = ONENAND_CMD_ADDR(addr) |
	      ONENAND_CMD_READ_REG;
	writel(ONENAND_CMD_REG, val);

	/* Wait for interrupt for NOR operation done */
	while ((readl(FLASH_INT_REG) & NAND_INT_DI) == 0x0);

	*(single_r_data) = readw(ONENAND_ID_STA_REG);
	writew(FLASH_INT_REG, 0x0);

	return 0;
}

int onenand_write_reg(u8 bank, u16 addr, const u16 single_w_data)
{
	struct onenand_cmd_s *cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_WRITE_REG];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = onenand_ctr.ctr1 |
	      ONENAND_CTR1_CE_SEL(bank) |
	      ONENAND_CTR1_POLL_MODE(cmd->poll_mode);
	writel(ONENAND_CTR1_REG, val);

	val = onenand_ctr.ctr2 |
	      ONENAND_CTR2_CMD_CYC(cmd->cmd_cyc) |
	      ONENAND_CTR2_DTA_CYC(cmd->data_cyc) |
	      ONENAND_CTR2_PROG_MODE(cmd->prog_mode);
	writel(ONENAND_CTR2_REG, val);

	/* Setup ONENAND command/data words */
	onenand_setup_cmd_data(cmd);

	writew(ONENAND_PROG_DTA_REG, single_w_data);

	/* Setup command register */
	val = ONENAND_CMD_ADDR(addr) |
	      ONENAND_CMD_WRITE_REG;
	writel(ONENAND_CMD_REG, val);

	/* Wait for interrupt for NAND operation done */
	while ((readl(FLASH_INT_REG) & NAND_INT_DI) == 0x0);

	writew(FLASH_INT_REG, 0x0);

	return 0;
}

int onenand_poll_reg(u8 bank, u16 addr, u8 pos)
{
	struct onenand_cmd_s *cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_POLL_REG];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = onenand_ctr.ctr1 |
	      ONENAND_CTR1_CE_SEL(bank) |
	      ONENAND_CTR1_POLL_MODE(cmd->poll_mode) |
	      ONENAND_CTR1_AVD_EN;
	writel(ONENAND_CTR1_REG, val);

	val = onenand_ctr.ctr2 |
	      ONENAND_CTR2_CMD_CYC(cmd->cmd_cyc) |
	      ONENAND_CTR2_DTA_CYC(cmd->data_cyc) |
	      ONENAND_CTR2_PROG_MODE(cmd->prog_mode);
	val &= ~0xf0000;
	val |= ONENAND_CTR2_POLL_TARGET_BIT(pos);
	writel(ONENAND_CTR2_REG, val);

	/* Setup ONENAND command/data words */
	onenand_setup_cmd_data(cmd);

	/* Setup command register */
	val = ONENAND_CMD_ADDR(addr) |
	      ONENAND_CMD_POLL_REG;
	writel(ONENAND_CMD_REG, val);

	/* Wait for interrupt for NAND operation done */
	while ((readl(FLASH_INT_REG) & NAND_INT_DI) == 0x0);

	writew(FLASH_INT_REG, 0x0);

	return 0;
}

static int onenand_is_block_lock(u32 block)
{
	int rval;
	u16 val;
	u32 addr;
	u8  bank = block / flnand.blocks_per_bank;;

	/* Write FBA : set ONENAND_DEV_ADDR1_REG */
	val = ONENAND_ADDR1_FBA(block);
	if ((block & 0xffff) > val)
		val |= ONENAND_ADDR1_DFS;
	rval = onenand_write_reg(bank, ONENAND_DEV_ADDR1_REG, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Check WP status : read ONENAND_DEV_WP_STA_REG */
	addr = ONENAND_DEV_WP_STA_REG;
	rval = onenand_read_reg(bank, addr, &val);
	BLD_ONENAND_RETURN_RVAL(rval);

	DEBUG_MSG("ONENAND_DEV_WP_STA_REG  is 0x");
	DEBUG_HEX_MSG(val);
	DEBUG_MSG("\r\n");

	if (val & ONENAND_WP_STA_US)
		return 0;
	else if (val & ONENAND_WP_STA_LS)
		return 1;
	else
		return 1;
}

static int onenand_unlock_block(u32 block)
{
	int rval;
	u32 addr;
	u16 val;
	u8  bank;

	bank = block / flnand.blocks_per_bank;

	if (flnand.banks > 1) {
		/* DDP device */

		/* Write SRT_ADDR1 */
		addr = ONENAND_DEV_ADDR1_REG;
		val = (bank & 0x1)? ONENAND_ADDR1_DFS : 0;
		DEBUG_MSG("set ONENAND_DEV_ADDR1_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);

		/* Write SRT_ADDR2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("set ONENAND_DEV_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write SRT_BLK_ADDR */
	addr = ONENAND_DEV_BLK_ADDR_REG;
	val = ONENAND_BLK_ADDR_SBA(block);
	DEBUG_MSG("set ONENAND_DEV_BLK_ADDR_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Write INT */
	addr = ONENAND_DEV_INT_STA_REG;
	val = 0;
	DEBUG_MSG("set ONENAND_DEV_INT_STA_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Write CMD : set ONENAND_DEV_CMD_REG */
	addr = ONENAND_DEV_CMD_REG;
	val = ONENAND_UNLOCK_BLK;
	DEBUG_MSG("set ONENAND_DEV_CMD_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Poll INT */
	addr = ONENAND_DEV_INT_STA_REG;
	DEBUG_MSG("poll ONENAND_DEV_INT_STA_REG\r\n");
	rval = onenand_poll_reg(bank, addr, 15);
	BLD_ONENAND_RETURN_RVAL(rval);

	return rval;
}

/**
 * Erase a ONENAND flash block.
 */
int onenand_erase_block(u32 block)
{
	u32 addr;
	u16 val;
	int rval;
	u8  bank = block / flnand.blocks_per_bank;

	DEBUG_MSG("onenand_erase_block( ");
	DEBUG_DEC_MSG(block);
	DEBUG_MSG(" )\r\n");

	if (onenand_is_block_lock(block)) {
		rval = onenand_unlock_block(block);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write SRT_ADDR1 */
	addr = ONENAND_DEV_ADDR1_REG;
	val = ONENAND_ADDR1_FBA(block);
	DEBUG_MSG("set ONENAND_DEV_ADDR1_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	if (flnand.banks > 1) {
		/* Write SRT_ADDR2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("set ONENAND_DEV_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write INT */
	addr = ONENAND_DEV_INT_STA_REG;
	val = 0;
	DEBUG_MSG("set ONENAND_DEV_INT_STA_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Write CMD */
	addr = ONENAND_DEV_CMD_REG;
	val = ONENAND_ERASE_BLK;
	DEBUG_MSG("Set ONENAND_DEV_CMD_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* Poll INT */
	addr = ONENAND_DEV_INT_STA_REG;
	DEBUG_MSG("poll ONENAND_DEV_INT_STA_REG\r\n");
	rval = onenand_poll_reg(bank, addr, 15);
	return rval;
}

/**
 * Read multiple pages from ONENAND flash with ecc check.
 */
int onenand_read_pages(u32 block, u32 page, u32 pages, const u8 *buf)
{
	struct onenand_cmd_s *cmd;
	int i, rval;
	u32 addr, val, len = 0;
	u32 bank = block / flnand.blocks_per_bank;

#if defined(ONENAND_DEBUG)
	putstr("onenand_read_pages( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in onenand_read_pages()\r\n");
		return -1;
	}

	/* ONENAND MAIN DMA size must be align to 2048 */
	K_ASSERT(((pages * flnand.main_size) &
		 (ONENAND_MIN_MAIN_DMA_SIZE - 1)) == 0x0);

	cmd = (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE) ?
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SYNC]) :
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_ASYNC]);

	for (i = 0; i < pages; i++)
		len += flnand.main_size;

	if (onenand_is_block_lock(block)) {
		rval = onenand_unlock_block(block);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	if (flnand.banks > 1) {
		/* Write Start addr2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write Start buffer */
	addr = ONENAND_DEV_BUF_START_REG;
	val  = ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT);
	DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_BUF_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* set SYS_CFG1 */
	addr = ONENAND_DEV_SYS_CFG1_REG;
	val = ONENAND_SYS_CFG1_DEF_BRL | ONENAND_SYS_CFG1_RDY_POL |
	      ONENAND_SYS_CFG1_INT_POL | ONENAND_SYS_CFG1_IOBE |
	      ONENAND_SYS_CFG1_RDY_CONF;
	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_SYS_CFG1_RM_SYNC;
	DEBUG_MSG("set ONENAND_DEV_SYS_CFG1_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	addr = addr_from_block_page(block, page);
	onenand_read_dma(addr, len, buf, ONENAND_AREA_MAIN);

	return 0;
}

/**
 * Read spare area from NAND flash.
 */
int onenand_read_spare(u32 block, u32 page, u32 pages, const u8 *buf)
{
	struct onenand_cmd_s *cmd;
	int i, rval;
	u32 val;
	u32 addr, len = 0;
	u32 bank = block / flnand.blocks_per_bank;

#if defined(DEBUG)
	putstr("nand_read_spare( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_read_spare()\r\n");
		return -1;
	}

	/* ONENAND SPARE DMA size must be align to 64 */
	K_ASSERT(((pages * flnand.spare_size) &
		 (ONENAND_MIN_SPARE_DMA_SIZE - 1)) == 0x0);

	cmd = (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE) ?
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_SYNC]) :
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_ASYNC]);

	for (i = 0; i < pages; i++)
		len += flnand.spare_size;

	if (onenand_is_block_lock(block)) {
		rval = onenand_unlock_block(block);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	if (flnand.banks > 1) {
		/* Write Start addr2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write Start buffer */
	addr = ONENAND_DEV_BUF_START_REG;
	val  = ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT);
	DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_BUF_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* set SYS_CFG1 */
	addr = ONENAND_DEV_SYS_CFG1_REG;
	val = ONENAND_SYS_CFG1_DEF_BRL | ONENAND_SYS_CFG1_RDY_POL |
	      ONENAND_SYS_CFG1_INT_POL | ONENAND_SYS_CFG1_IOBE |
	      ONENAND_SYS_CFG1_RDY_CONF;
	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_SYS_CFG1_RM_SYNC;
	DEBUG_MSG("set ONENAND_DEV_SYS_CFG1_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	addr = addr_from_block_page(block, page);
	onenand_read_dma(addr, len, buf, ONENAND_AREA_SPARE);

	return 0;
}

/**
 * Check for bad block.
 */
int onenand_is_bad_block(u32 block)
{
	int i, rval = 0;
	u8  bi;

	rval = onenand_read_spare(block, 0, BAD_BLOCK_PAGES, g_onenand_buf);
	if (rval < 0) {
		putstr("check bad block failed >> "
				"read spare data error.\r\n");
		return 1;
	}

	for (i = 0; i < BAD_BLOCK_PAGES; i++) {
		if (flnand.main_size == 512)
			bi = *(g_onenand_buf + i * (1 << NAND_SPARE_16_SHF)+ 5);
		else if (flnand.main_size == 1024)
			bi = *(g_onenand_buf + i * (1 << NAND_SPARE_32_SHF));
		else
			bi = *(g_onenand_buf + i * (1 << NAND_SPARE_64_SHF));

		if (bi != 0xff) {
			if (i < AMB_BB_START_PAGE) {
				/* Initial invalid blocks. */
				return NAND_INITIAL_BAD_BLOCK;
			} else {
				/* Late developed bad blocks. */
				return NAND_LATE_DEVEL_BAD_BLOCK;
			}
		}
	}

	/* Good block */
	return 0;
}

static int onenand_read(u32 block, u32 page, u32 pages, u8 *buf)
{
	int rval = 0;
	u32 first_blk_pages, blocks, last_blk_pages;
	u32 bad_blks = 0;

#if defined(ONENAND_DEBUG)
	putstr("onenand_read( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif

	first_blk_pages = flnand.pages_per_block - page;
	if (pages > first_blk_pages) {
		pages -= first_blk_pages;
		blocks = pages / flnand.pages_per_block;
		last_blk_pages = pages % flnand.pages_per_block;
	} else {
		first_blk_pages = pages;
		blocks = 0;
		last_blk_pages = 0;
	}

	if (first_blk_pages) {
		while (onenand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = onenand_read_pages(block, page, first_blk_pages, buf);
		if (rval < 0)
			return -1;
		block++;
		buf += first_blk_pages * flnand.main_size;
	}

	while (blocks > 0) {
		while (onenand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = onenand_read_pages(block, 0, flnand.pages_per_block, buf);
		if (rval < 0)
			return -1;
		block++;
		blocks--;
		buf += flnand.block_size;
	}

	if (last_blk_pages) {
		while (onenand_is_bad_block(block)) {
			/* if bad block, find next */
			block++;
			bad_blks++;
		}
		rval = onenand_read_pages(block, 0, last_blk_pages, buf);
		if (rval < 0)
			return -1;
	}

	return bad_blks;
}

static void onenand_get_offset_adr(u32 *block, u32 *page, u32 pages,
					u32 bad_blks)
{
	u32 blocks;

	blocks = pages / flnand.pages_per_block;
	pages  = pages % flnand.pages_per_block;

	*block =  *block + blocks;
	*page += pages;

	if (*page >= flnand.pages_per_block) {
		*page -= flnand.pages_per_block;
		*block += 1;
	}

	*block += bad_blks;
}

/**
 * Read data from NAND flash to memory.
 * dst - address in dram.
 * src - address in nand device.
 * len - length to be read from nand.
 * return - length of read data.
 */
int onenand_read_data(u8 *dst, u8 *src, int len)
{
	u32 block, page, remind_pages, pos;
	u32 page_mult, pages;
	u32 main_size;
	u32 first_ppage_size, last_ppage_size;
	int val, rval = -1;

#if defined(ONENAND_DEBUG)
	putstr("onenand_read_data( 0x");
	puthex((u32)dst);
	putstr(", 0x");
	puthex((u32)src);
	putstr(", ");
	putdec(len);
	putstr(" )\r\n");
#endif
	page_mult = ONENAND_MIN_MAIN_DMA_SIZE / flnand.main_size;
	main_size = (flnand.main_size != ONENAND_MIN_MAIN_DMA_SIZE) ?
		    ONENAND_MIN_MAIN_DMA_SIZE : flnand.main_size;

	/* translate address to block, page, address */
	val = (int) src;
	block = val / flnand.block_size;
	val  -= block * flnand.block_size;
	page  = val / main_size;
	pos   = val % main_size;
	remind_pages = len / main_size;	/*<** data outside first page  */

	/* Get the start addr of first page */
	if (pos == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = main_size - pos;

	if (len >= first_ppage_size) {
		remind_pages = (len - first_ppage_size) / main_size;

		last_ppage_size = (len - first_ppage_size) % main_size;
	} else {
		first_ppage_size = len;
		remind_pages = 0;
		last_ppage_size = 0;
	}

	if (len !=
	    (first_ppage_size + remind_pages * main_size + last_ppage_size)) {
		return -1;
	}

	len = 0;
	if (first_ppage_size) {
		pages = 1 * page_mult;
		rval = onenand_read(block, page, pages, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) (buffer + pos), first_ppage_size);
		dst += first_ppage_size;
		len += first_ppage_size;
		onenand_get_offset_adr(&block, &page, pages, rval);
	}

	if (remind_pages > 0) {
		pages = remind_pages * page_mult;
		rval = onenand_read(block, page, pages, dst);
		if (rval < 0)
			return len;

		dst += remind_pages * main_size;
		len += remind_pages * main_size;
		onenand_get_offset_adr(&block, &page, pages, rval);
	}

	if (last_ppage_size > 0) {
		pages = 1 * page_mult;
		rval = onenand_read(block, page, pages, buffer);
		if (rval < 0)
			return len;

		memcpy(dst, (void *) buffer, last_ppage_size);
		len += last_ppage_size;
	}

	return len;
}

/**
 * Program a page to NAND flash.
 */
int onenand_prog_pages(u32 block, u32 page, u32 pages, const u8 *buf)
{
	struct onenand_cmd_s *cmd;
	int i, rval;
	u32 addr;
	u32 val, len = 0;
	u32 bank = block / flnand.blocks_per_bank;

#if defined(DEBUG)
	putstr("onenand_prog_pages( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in nand_prog_pages()\r\n");
		return -1;
	}

	cmd = (onenand_ctr.prog_mode & ONENAND_SUPPORT_SYNC_MODE) ?
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SYNC]) :
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_ASYNC]);

	for (i = 0; i < pages; i++)
		len += flnand.main_size;

	if (onenand_is_block_lock(block)) {
		rval = onenand_unlock_block(block);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	if (flnand.banks > 1) {
		/* Write Start addr2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write Start buffer */
	addr = ONENAND_DEV_BUF_START_REG;
	val  = ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT);
	DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_BUF_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* set SYS_CFG1 */
	addr = ONENAND_DEV_SYS_CFG1_REG;
	val = ONENAND_SYS_CFG1_DEF_BRL | ONENAND_SYS_CFG1_RDY_POL |
	      ONENAND_SYS_CFG1_INT_POL | ONENAND_SYS_CFG1_IOBE |
	      ONENAND_SYS_CFG1_RDY_CONF;
	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_SYS_CFG1_RM_SYNC;
	DEBUG_MSG("set ONENAND_DEV_SYS_CFG1_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	addr = addr_from_block_page(block, page);
	onenand_prog_dma(addr, len, buf, ONENAND_AREA_MAIN);

	return 0;

}

/**
 * Program a page to NAND flash.
 */
int onenand_prog_spare(u32 block, u32 page, u32 pages, const u8 *buf)
{
	struct onenand_cmd_s *cmd;
	int i, rval;
	u32 addr, len = 0;
	u32 val;
	u32 bank = block / flnand.blocks_per_bank;

#if defined(ONENAND_DEBUG)
	putstr("onenand_prog_spare( ");
	putdec(block);
	putstr(", ");
	putdec(page);
	putstr(", ");
	putdec(pages);
	putstr(", 0x");
	puthex((u32)buf);
	putstr(" )\r\n");
#endif
	/* check parameters */
	if ((page < 0 || page >= flnand.pages_per_block)	||
	    (pages <= 0 || pages > flnand.pages_per_block)	||
	    ((page + pages) > flnand.pages_per_block)		||
	    (buf == NULL)) {
		putstr("ERR: parameter error in onenand_prog_spare()\r\n");
		return -1;
	}

	cmd = (onenand_ctr.prog_mode & ONENAND_SUPPORT_SYNC_MODE) ?
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_SYNC]) :
	      (&onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_ASYNC]);

	for (i = 0; i < pages; i++)
		len += flnand.main_size;

	if (onenand_is_block_lock(block)) {
		rval = onenand_unlock_block(block);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	if (flnand.banks > 1) {
		/* Write Start addr2 */
		addr = ONENAND_DEV_ADDR2_REG;
		val = (bank & 0x1)? ONENAND_ADDR2_DBS : 0;
		DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_ADDR2_REG\r\n");
		rval = onenand_write_reg(bank, addr, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Write Start buffer */
	addr = ONENAND_DEV_BUF_START_REG;
	val  = ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT);
	DEBUG_MSG("ONENAND_OP:set ONENAND_DEV_SRT_BUF_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	/* set SYS_CFG1 */
	addr = ONENAND_DEV_SYS_CFG1_REG;
	val = ONENAND_SYS_CFG1_DEF_BRL | ONENAND_SYS_CFG1_RDY_POL |
	      ONENAND_SYS_CFG1_INT_POL | ONENAND_SYS_CFG1_IOBE |
	      ONENAND_SYS_CFG1_RDY_CONF;
	if (onenand_ctr.read_mode & ONENAND_SUPPORT_SYNC_MODE)
		val |= ONENAND_SYS_CFG1_RM_SYNC;
	DEBUG_MSG("set ONENAND_DEV_SYS_CFG1_REG\r\n");
	rval = onenand_write_reg(bank, addr, val);
	BLD_ONENAND_RETURN_RVAL(rval);

	addr = addr_from_block_page(block, page);
	onenand_prog_dma(addr, len, buf, ONENAND_AREA_SPARE);

	return 0;
}

/**
 * Initialize ONENAND parameters.
 */
int onenand_init(void)
{
	struct onenand_cmd_s *cmd;
	flnand_t *fn = &flnand;
	int sblk;
	int nblk;
	int block_size;
	int i, part_size[TOTAL_FW_PARTS];

	/* set device info */
	fn->main_size = ONENAND_MAIN_SIZE;
	fn->spare_size = ONENAND_SPARE_SIZE;
	fn->blocks_per_bank = ONENAND_BLOCKS_PER_BANK;
	fn->pages_per_block = ONENAND_PAGES_PER_BLOCK;
	fn->block_size = fn->main_size * fn->pages_per_block;
	fn->banks = ONENAND_BANKS_PER_DEVICE;

	get_part_size(part_size);

	block_size = fn->block_size;
	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		nblk = part_size[i] / block_size;
		if ((part_size[i] % block_size) != 0x0)
			nblk++;
		fn->sblk[i] = (nblk == 0) ? 0 : sblk;
		fn->nblk[i] = nblk;
	}

	if (fn->banks == 0)
		return -1;

	/* set control & command info */
	onenand_ctr.ctr1 = ONENAND_CONTROL1;

	onenand_ctr.ctr2 =
		ONENAND_CONTROL2 		|
		ONENAND_CTR2_DAT_POLL_MODE(0)	|
		ONENAND_CTR2_POLL_POLAR		|
		ONENAND_CTR2_POLL_TARGET_BIT(ONENAND_DATA_POLL_TARGET_BIT);

	onenand_ctr.read_mode	= ONENAND_SUPPORT_ASYNC_MODE;
	onenand_ctr.prog_mode	= ONENAND_SUPPORT_ASYNC_MODE;

	onenand_ctr.data_poll_target_bit = ONENAND_DATA_POLL_TARGET_BIT;

	onenand_ctr.poll_ctr1 = 0x0;
	onenand_ctr.poll_ctr2 =
		ONENAND_CTR2_DAT_POLL_MODE(0)		|
		ONENAND_CTR2_POLL_POLAR			|
		ONENAND_CTR2_POLL_TARGET_BIT(onenand_ctr.data_poll_target_bit);

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_REG];
	cmd->addr[0]		= ONENAND_READ_REG_CYC1_ADDR;
	cmd->data[0]		= ONENAND_READ_REG_CYC1_DATA;
	cmd->cmd_cyc		= ONENAND_READ_REG_CMD_CYC;
	cmd->data_cyc		= ONENAND_READ_REG_DATA_CYC;
	cmd->poll_mode		= ONENAND_READ_REG_POLL_MODE;
	cmd->prog_mode		= ONENAND_READ_REG_PROG_MODE;
	cmd->timeout		= ONENAND_READ_REG_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_WRITE_REG];
	cmd->addr[0]		= ONENAND_WRITE_REG_CYC1_ADDR;
	cmd->data[0]		= ONENAND_WRITE_REG_CYC1_DATA;
	cmd->cmd_cyc		= ONENAND_WRITE_REG_CMD_CYC;
	cmd->data_cyc		= ONENAND_WRITE_REG_DATA_CYC;
	cmd->poll_mode		= ONENAND_WRITE_REG_POLL_MODE;
	cmd->prog_mode		= ONENAND_WRITE_REG_PROG_MODE;
	cmd->timeout		= ONENAND_WRITE_REG_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_POLL_REG];
	cmd->addr[0]		= ONENAND_POLL_REG_CYC1_ADDR;
	cmd->data[0]		= ONENAND_POLL_REG_CYC1_DATA;
	cmd->cmd_cyc		= ONENAND_POLL_REG_CMD_CYC;
	cmd->data_cyc		= ONENAND_POLL_REG_DATA_CYC;
	cmd->poll_mode		= ONENAND_POLL_REG_POLL_MODE;
	cmd->prog_mode		= ONENAND_POLL_REG_PROG_MODE;
	cmd->timeout		= ONENAND_POLL_REG_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_ASYNC];
	cmd->addr[0]		= ONENAND_READ_ASYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_READ_ASYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_READ_ASYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_READ_ASYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_READ_ASYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_READ_ASYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_READ_ASYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_READ_ASYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_READ_ASYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_READ_ASYNC_CYC5_DATA;
	cmd->cmd_cyc		= ONENAND_READ_ASYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_READ_ASYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_READ_ASYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_READ_ASYNC_PROG_MODE;
	cmd->timeout		= ONENAND_READ_ASYNC_TIMEOUT;

	/* spare async */
	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_ASYNC];
	cmd->addr[0]		= ONENAND_READ_S_ASYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_READ_S_ASYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_READ_S_ASYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_READ_S_ASYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_READ_S_ASYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_READ_S_ASYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_READ_S_ASYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_READ_S_ASYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_READ_S_ASYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_READ_S_ASYNC_CYC5_DATA;
	cmd->cmd_cyc		= ONENAND_READ_S_ASYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_READ_S_ASYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_READ_S_ASYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_READ_S_ASYNC_PROG_MODE;
	cmd->timeout		= ONENAND_READ_S_ASYNC_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SYNC];
	cmd->addr[0]		= ONENAND_READ_SYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_READ_SYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_READ_SYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_READ_SYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_READ_SYNC_CYC5_ADDR;
	cmd->addr[5]		= ONENAND_READ_SYNC_CYC6_ADDR;
	cmd->data[0]		= ONENAND_READ_SYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_READ_SYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_READ_SYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_READ_SYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_READ_SYNC_CYC5_DATA;
	cmd->data[5]		= ONENAND_READ_SYNC_CYC6_DATA;
	cmd->cmd_cyc		= ONENAND_READ_SYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_READ_SYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_READ_SYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_READ_SYNC_PROG_MODE;
	cmd->timeout		= ONENAND_READ_SYNC_TIMEOUT;

	/* spare sync */
	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_READ_SPARE_SYNC];
	cmd->addr[0]		= ONENAND_READ_S_SYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_READ_S_SYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_READ_S_SYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_READ_S_SYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_READ_S_SYNC_CYC5_ADDR;
	cmd->addr[5]		= ONENAND_READ_S_SYNC_CYC6_ADDR;
	cmd->data[0]		= ONENAND_READ_S_SYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_READ_S_SYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_READ_S_SYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_READ_S_SYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_READ_S_SYNC_CYC5_DATA;
	cmd->data[5]		= ONENAND_READ_S_SYNC_CYC6_DATA;
	cmd->cmd_cyc		= ONENAND_READ_S_SYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_READ_S_SYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_READ_S_SYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_READ_S_SYNC_PROG_MODE;
	cmd->timeout		= ONENAND_READ_S_SYNC_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_ASYNC];
	cmd->addr[0]		= ONENAND_PROG_ASYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_PROG_ASYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_PROG_ASYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_PROG_ASYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_PROG_ASYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_PROG_ASYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_PROG_ASYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_PROG_ASYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_PROG_ASYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_PROG_ASYNC_CYC5_DATA;
	cmd->cmd_cyc		= ONENAND_PROG_ASYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_PROG_ASYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_PROG_ASYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_PROG_ASYNC_PROG_MODE;
	cmd->timeout		= ONENAND_PROG_ASYNC_TIMEOUT;

	/* spare async */
	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_ASYNC];
	cmd->addr[0]		= ONENAND_PROG_S_ASYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_PROG_S_ASYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_PROG_S_ASYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_PROG_S_ASYNC_CYC4_ADDR;
	cmd->addr[4]		= ONENAND_PROG_S_ASYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_PROG_S_ASYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_PROG_S_ASYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_PROG_S_ASYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_PROG_S_ASYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_PROG_S_ASYNC_CYC5_DATA;
	cmd->cmd_cyc		= ONENAND_PROG_S_ASYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_PROG_S_ASYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_PROG_S_ASYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_PROG_S_ASYNC_PROG_MODE;
	cmd->timeout		= ONENAND_PROG_S_ASYNC_TIMEOUT;

	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SYNC];
	cmd->addr[0]		= ONENAND_PROG_SYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_PROG_SYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_PROG_SYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_PROG_SYNC_CYC3_ADDR;
	cmd->addr[4]		= ONENAND_PROG_SYNC_CYC4_ADDR;
	cmd->addr[5]		= ONENAND_PROG_SYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_PROG_SYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_PROG_SYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_PROG_SYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_PROG_SYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_PROG_SYNC_CYC5_DATA;
	cmd->data[5]		= ONENAND_PROG_SYNC_CYC6_DATA;
	cmd->cmd_cyc		= ONENAND_PROG_SYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_PROG_SYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_PROG_SYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_PROG_SYNC_PROG_MODE;
	cmd->timeout		= ONENAND_PROG_SYNC_TIMEOUT;

	/* spare sync */
	cmd = &onenand_ctr.cmd[ONENAND_AMB_CMD_PROG_SPARE_SYNC];
	cmd->addr[0]		= ONENAND_PROG_S_SYNC_CYC1_ADDR;
	cmd->addr[1]		= ONENAND_PROG_S_SYNC_CYC2_ADDR;
	cmd->addr[2]		= ONENAND_PROG_S_SYNC_CYC3_ADDR;
	cmd->addr[3]		= ONENAND_PROG_S_SYNC_CYC3_ADDR;
	cmd->addr[4]		= ONENAND_PROG_S_SYNC_CYC4_ADDR;
	cmd->addr[5]		= ONENAND_PROG_S_SYNC_CYC5_ADDR;
	cmd->data[0]		= ONENAND_PROG_S_SYNC_CYC1_DATA;
	cmd->data[1]		= ONENAND_PROG_S_SYNC_CYC2_DATA;
	cmd->data[2]		= ONENAND_PROG_S_SYNC_CYC3_DATA;
	cmd->data[3]		= ONENAND_PROG_S_SYNC_CYC4_DATA;
	cmd->data[4]		= ONENAND_PROG_S_SYNC_CYC5_DATA;
	cmd->data[5]		= ONENAND_PROG_S_SYNC_CYC6_DATA;
	cmd->cmd_cyc		= ONENAND_PROG_S_SYNC_CMD_CYC;
	cmd->data_cyc		= ONENAND_PROG_S_SYNC_DATA_CYC;
	cmd->poll_mode		= ONENAND_PROG_S_SYNC_POLL_MODE;
	cmd->prog_mode		= ONENAND_PROG_S_SYNC_PROG_MODE;
	cmd->timeout		= ONENAND_PROG_S_SYNC_TIMEOUT;

	onenand_set_timing();

	return 0;
}

/**
 * Reset NOR chip(s).
 */
int onenand_reset(void)
{
	u32 ctr;
	u16 val;
	int i, rval;

	/* Setup Flash IO Control Register (exit random mode) */
	ctr = readl(FIO_CTR_REG);
	if (ctr & FIO_CTR_RR) {
		ctr &= ~FIO_CTR_RR;
		ctr |= FIO_CTR_XD;
		writel(FIO_CTR_REG, ctr);
	}

	/* Clear the FIO DMA Status Register */
	writel(FIO_DMASTA_REG, 0x0);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	/* Reset ONENAND chip */
	val = ONENAND_RESET_ONENAND;
	for (i = 0; i < flnand.banks; i++) {
		rval = onenand_write_reg(i, ONENAND_DEV_CMD_REG, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	/* Set ONENAND SYS_CFG1 */
	val = ONENAND_SYS_CFG1_DEF_BRL | ONENAND_SYS_CFG1_RDY_POL |
	      ONENAND_SYS_CFG1_INT_POL | ONENAND_SYS_CFG1_IOBE |
	      ONENAND_SYS_CFG1_RDY_CONF;
	for (i = 0; i < flnand.banks; i++) {
		rval = onenand_write_reg(i, ONENAND_DEV_SYS_CFG1_REG, val);
		BLD_ONENAND_RETURN_RVAL(rval);
	}

	return 0;
}

/**
 * Erase whole ONENAND flash chip.
 */
int onenand_erase_chip(u8 bank)
{
	return 0;
}

/**
 * Read ID command.
 */
int onenand_read_id(u8 bank, u32 addr, u16 *buf)
{
	return 0;
}

int onenand_update_bb_info(void)
{
	u32 i, blk, lim, bb_offset = 0;
	flnand_t *fn = &flnand;

	/* skip initial bad block */
	for (i = PART_BB_SKIP_START; i < TOTAL_FW_PARTS; i++) {
		if (fn->nblk[i] == 0)
			continue;
		fn->sblk[i] += bb_offset;
		bb_offset = 0;
		lim = fn->sblk[i] + fn->nblk[i];
		for (blk = fn->sblk[i]; blk < lim; blk ++) {
			if (onenand_is_bad_block(blk) == 0x1){
				fn->nblk[i] ++;
				bb_offset ++;
			}
		}
	}
	return 0;
}

#endif /* (ENABLE_FLASH && !(CONFIG_ONENAND_NONE) */

