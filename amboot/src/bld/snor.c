/**
 * system/src/bld/snor.c
 *
 * Flash controller functions with NOR chips.
 *
 * History:
 *    2009/09/17 - [Chien-Yang Chen] created file
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
#include "partinfo.h"

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
/**
 * 0: don't use DMA
 * 1: use DMA engine
 * 2: DMA for all xfer
 */
#define USE_DMA_XFER		2

#define SNOR_MIN_DMA_SIZE	2048
#define SNOR_READ_ALIGN		2048
#define SNOR_PROG_ALIGN		2048

#if	(USE_DMA_XFER == 2)
static u8 g_snor_buf[SNOR_MIN_DMA_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));
#endif

extern flnor_t flnor;

#define SNOR_AMB_CMD_RESET			0x0
#define SNOR_AMB_CMD_AUTOSEL			0x1
#define SNOR_AMB_CMD_CFI_QUERY			0x2
#define SNOR_AMB_CMD_READ			0x3
#define SNOR_AMB_CMD_WRITE			0x4
#define SNOR_AMB_CMD_CHIP_ERASE			0x5
#define SNOR_AMB_CMD_BLOCK_ERASE		0x6
#define SNOR_AMB_CMD_MAX			0x7

#define SNOR_POLL_MODE_RDY_BUSY		1
#define SNOR_POLL_MODE_DATA_POLL	2

#define SNOR_DATA_POLL_MODE_EQ_0	0
#define SNOR_DATA_POLL_MODE_EQ_1	1
#define SNOR_DATA_POLL_MODE_EQ_DATA	2

struct snor_ctr_s {
	u32 ctr1;
	u32 ctr2;
	u32 poll_ctr1;
	u32 poll_ctr2;

	struct snor_cmd_s {
		u16 addr[SNOR_MAX_CMD_CYC];
		u16 data[SNOR_MAX_ADDR_CYC];
		u8  cmd_cyc;
		u8  data_cyc;
		int timeout;
	} cmd[SNOR_AMB_CMD_MAX];
} snor_ctr;

static void snor_setup_cmd_data(struct snor_cmd_s *cmd)
{
	u32 val;

	if (cmd->cmd_cyc >= 1) {
		val = SNOR_CMD_ADDR_HI(cmd->addr[1]) |
		      SNOR_CMD_ADDR_LO(cmd->addr[0]);
		writel(SNOR_CMD_ADDR0_REG, val);

		val = SNOR_CMD_WORD_HI(cmd->data[1]) |
		      SNOR_CMD_WORD_LO(cmd->data[0]);
		writel(SNOR_CMD_WORD0_REG, val);
	}

	if (cmd->cmd_cyc >= 3) {
		val = SNOR_CMD_ADDR_HI(cmd->addr[3]) |
		      SNOR_CMD_ADDR_LO(cmd->addr[2]);
		writel(SNOR_CMD_ADDR1_REG, val);

		val = SNOR_CMD_WORD_HI(cmd->data[3]) |
		      SNOR_CMD_WORD_LO(cmd->data[2]);
		writel(SNOR_CMD_WORD1_REG, val);
	}

	if (cmd->cmd_cyc >= 5) {
		val = SNOR_CMD_ADDR_HI(cmd->addr[5]) |
		      SNOR_CMD_WORD_LO(cmd->addr[4]);
		writel(SNOR_CMD_ADDR2_REG, val);

		val = SNOR_CMD_WORD_HI(cmd->data[5]) |
		      SNOR_CMD_WORD_LO(cmd->data[4]);
		writel(SNOR_CMD_WORD2_REG, val);
	}
}

static void snor_set_timing(void)
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

	taa 		= FLASH_TIMING_MIN(SNOR_TAA);
	toe 		= FLASH_TIMING_MAX(SNOR_TOE);
	toeh		= FLASH_TIMING_MIN(SNOR_TOEH);
	tce 		= FLASH_TIMING_MAX(SNOR_TCE);

	tpa 		= FLASH_TIMING_MIN(SNOR_TPA);
	trp 		= FLASH_TIMING_MIN(SNOR_TRP);
	trh 		= FLASH_TIMING_MAX(SNOR_TRH);
	toes		= FLASH_TIMING_MIN(SNOR_TOES);

	tcs		= FLASH_TIMING_MIN(SNOR_TCS);
	tch		= FLASH_TIMING_MIN(SNOR_TCH);
	twp		= FLASH_TIMING_MIN(SNOR_TWP);
	twh		= FLASH_TIMING_MIN(SNOR_TWH);

	trb		= FLASH_TIMING_MIN(SNOR_TRB);
	taht		= FLASH_TIMING_MIN(SNOR_TAHT);
	taso		= FLASH_TIMING_MIN(SNOR_TASO);
	toeph		= FLASH_TIMING_MIN(SNOR_TOEPH);

	tavdcs		= FLASH_TIMING_MIN(SNOR_TAVDCS);
	tavdch		= FLASH_TIMING_MIN(SNOR_TAVDCH);
	tas		= FLASH_TIMING_MIN(SNOR_TAS);
	tah		= FLASH_TIMING_MIN(SNOR_TAH);

	telf		= FLASH_TIMING_MIN(SNOR_TELF);
	tds		= FLASH_TIMING_MIN(SNOR_TDS);
	tdh		= FLASH_TIMING_MIN(SNOR_TDH);
	tbusy		= FLASH_TIMING_MAX(SNOR_TBUSY);

	tdp		= FLASH_TIMING_MIN(SNOR_TDP);
	tceph		= FLASH_TIMING_MIN(SNOR_TCEPH);
	toez		= FLASH_TIMING_MIN(SNOR_TOEZ);
	tba		= FLASH_TIMING_MIN(SNOR_TBA);

	tready		= FLASH_TIMING_MIN(SNOR_TREADY);
	tach		= FLASH_TIMING_MIN(SNOR_TACH);
	tcehp		= FLASH_TIMING_MIN(SNOR_TCEHP);

	tavds		= FLASH_TIMING_MIN(SNOR_TAVDS);
	toh		= FLASH_TIMING_MIN(SNOR_TOH);
	tavdp		= FLASH_TIMING_MIN(SNOR_TAVDP);
	taavdh		= FLASH_TIMING_MIN(SNOR_TAAVDH);

	tiaa		= FLASH_TIMING_MIN(SNOR_TIAA);
	tclk_period 	= FLASH_TIMING_MIN(SNOR_TCLK_PERIOD);
	twea		= FLASH_TIMING_MIN(SNOR_TWEA);
	tacs		= FLASH_TIMING_MIN(SNOR_TACS);

	t0_reg = SNOR_TIM0_TAA(taa)	|
		 SNOR_TIM0_TOE(toe)	|
		 SNOR_TIM0_TOEH(toeh)	|
		 SNOR_TIM0_TCE(tce);

	t1_reg = SNOR_TIM1_TPA(tpa)	|
		 SNOR_TIM1_TRP(trp)	|
		 SNOR_TIM1_TRH(trh)	|
		 SNOR_TIM1_TOES(toes);

	t2_reg = SNOR_TIM2_TCS(tcs)	|
		 SNOR_TIM2_TCH(tch)	|
		 SNOR_TIM2_TWP(twp)	|
		 SNOR_TIM2_TWH(twh);

	t3_reg = SNOR_TIM3_TRB(trb)	|
		 SNOR_TIM3_TAHT(taht)	|
		 SNOR_TIM3_TASO(taso)	|
		 SNOR_TIM3_TOEPH(toeph);

	t4_reg = SNOR_TIM4_TAVDCS(tavdcs)	|
		 SNOR_TIM4_TAVDCH(tavdch)	|
		 SNOR_TIM4_TAS(tas)		|
		 SNOR_TIM4_TAH(tah);

	t5_reg = SNOR_TIM5_TELF(telf)	|
		 SNOR_TIM5_TDS(tds)	|
		 SNOR_TIM5_TDH(tdh)	|
		 SNOR_TIM5_TBUSY(tbusy);

	t6_reg = SNOR_TIM6_TDP(tdp)	|
		 SNOR_TIM6_TCEPH(tceph)	|
		 SNOR_TIM6_TOEZ(toez)	|
		 SNOR_TIM6_TBA(tba);

	t7_reg = SNOR_TIM7_TREADY(tready)	|
		 SNOR_TIM7_TACH(tach)		|
		 SNOR_TIM7_TCEHP(tcehp);

	t8_reg = SNOR_TIM8_TAVDS(tavds)	|
		 SNOR_TIM8_TOH(toh)	|
		 SNOR_TIM8_TAVDP(tavdp)	|
		 SNOR_TIM8_TAAVDH(taavdh);

	t9_reg = SNOR_TIM9_TIAA(tiaa)			|
		 SNOR_TIM9_TCLK_PERIOD(tclk_period)	|
		 SNOR_TIM9_TWEA(twea)			|
		 SNOR_TIM9_TACS(tacs);

	flnor.timing0 = t0_reg;
	flnor.timing1 = t1_reg;
	flnor.timing2 = t2_reg;
	flnor.timing3 = t3_reg;
	flnor.timing4 = t4_reg;
	flnor.timing5 = t5_reg;
	flnor.timing6 = t6_reg;
	flnor.timing7 = t7_reg;
	flnor.timing8 = t8_reg;
	flnor.timing9 = t9_reg;

	/* Setup flash timing register */
	writel(SNOR_TIM0_REG, flnor.timing0);
	writel(SNOR_TIM1_REG, flnor.timing1);
	writel(SNOR_TIM2_REG, flnor.timing2);
	writel(SNOR_TIM3_REG, flnor.timing3);
	writel(SNOR_TIM4_REG, flnor.timing4);
	writel(SNOR_TIM5_REG, flnor.timing5);
	writel(SNOR_TIM6_REG, flnor.timing6);
	writel(SNOR_TIM7_REG, flnor.timing7);
	writel(SNOR_TIM8_REG, flnor.timing8);
	writel(SNOR_TIM9_REG, flnor.timing9);
}

/* ------------------------------------------------------------------------- */
u32 snor_blocks_to_len(u32 block, u32 blocks)
{
	u32 block_in_bank;
	u32 len = 0;

	while (blocks > 0) {
		block_in_bank = block & (flnor.blocks_per_bank - 1);

		if (block_in_bank < flnor.block1s_per_bank)
			len += flnor.block1_size;
		else
			len += flnor.block2_size;

		blocks--;
		block++;
	}

	return len;
}

/**
 * Length should be located in the same bank.
 */
u32 snor_len_to_blocks(u32 start_block, u32 len)
{
	u32 block1_len, block2_len;
	u32 free_block1s, free_block2s;
	u32 free_block1s_len, free_block2s_len;
	u32 block1s, block2s;

	if (start_block < flnor.block1s_per_bank) {
		free_block1s = flnor.block1s_per_bank - start_block;
		free_block2s = flnor.block2s_per_bank;
	} else {
		free_block1s = 0;
		free_block2s = flnor.total_sectors - start_block;
	}

	free_block1s_len = free_block1s * flnor.block1_size;
	free_block2s_len = free_block2s * flnor.block2_size;

	if (len > free_block1s_len) {
		block1_len = free_block1s_len;
		block2_len = len - free_block1s_len;
	} else {
		block1_len = len;
		block2_len = 0;
	}

	K_ASSERT(block2_len <= flnor.total_block2s_size);

	if (flnor.block1_size != 0) {
		block1s = block1_len / flnor.block1_size;
		if (block1_len & (flnor.block1_size -1))
			block1s++;
	} else {
		block1s = 0;
	}

	if (flnor.block2_size != 0) {
		block2s = block2_len / flnor.block2_size;
		if (block2_len & (flnor.block2_size -1))
			block2s++;
	} else {
		block2s = 0;
	}

	return (block1s + block2s);
}

u32 snor_get_addr(u32 block ,u32 offset)
{
	u32 block_in_bank;
	u32 banks, addr;

	block_in_bank = block & (flnor.blocks_per_bank - 1);
	banks = block /	flnor.blocks_per_bank;

	if (block_in_bank < flnor.block1s_per_bank)
		addr = block_in_bank * flnor.block1_size;
	else
		addr = flnor.total_block1s_size +
			(block_in_bank - flnor.block1s_per_bank) *
			flnor.block2_size;

	addr += banks * flnor.bank_size + offset;
	K_ASSERT(addr < flnor.chip_size);

	return addr;
}

void snor_get_block_offset(u32 addr, u32 *block, u32 *offset)
{
	u32 banks;
	u32 addr_in_bank;

	K_ASSERT(addr < flnor.chip_size);

	banks = addr / flnor.bank_size;
	addr_in_bank = addr & (flnor.bank_size - 1);

	if (addr_in_bank < flnor.total_block1s_size) {
		*block = addr_in_bank / flnor.block1_size;
		*offset = addr_in_bank & (flnor.block1_size - 1);
	} else {
		addr_in_bank -= flnor.total_block1s_size;
		*block = addr_in_bank / flnor.block2_size;
		*offset = addr_in_bank & (flnor.block2_size - 1);
	}

	*block += banks * flnor.blocks_per_bank;
}

/**
 * Initialize NOR parameters.
 */
int snor_init(void)
{
	struct snor_cmd_s *cmd;

	flnor.chip_size = SNOR_CHIP_SIZE;
	flnor.block1_size = SNOR_BLOCK1_SIZE;
	flnor.block1s_per_bank = SNOR_BLOCK1S_PER_BANK;
	flnor.total_block1s_size = SNOR_BLOCK1_SIZE * SNOR_BLOCK1S_PER_BANK;
	flnor.block2_size = SNOR_BLOCK2_SIZE;
	flnor.block2s_per_bank = SNOR_BLOCK2S_PER_BANK;
	flnor.total_block2s_size = SNOR_BLOCK2_SIZE * SNOR_BLOCK2S_PER_BANK;
	flnor.blocks_per_bank = SNOR_BLOCKS_PER_BANK;
	flnor.total_sectors = SNOR_TOTAL_BLOCKS;
	flnor.bank_size = (SNOR_BLOCK1_SIZE * SNOR_BLOCK1S_PER_BANK) +
			  (SNOR_BLOCK2_SIZE * SNOR_BLOCK2S_PER_BANK);
	flnor.banks = SNOR_BANKS_PER_DEVICE;
	flnor.sector_size = SNOR_BLOCK1_SIZE;

	flnor.sblk[PART_BST] = SNOR_BST_SSEC;
	flnor.nblk[PART_BST] = SNOR_BST_NSEC;
	flnor.sblk[PART_PTB] = SNOR_PTB_SSEC;
	flnor.nblk[PART_PTB] = SNOR_PTB_NSEC;
	flnor.sblk[PART_BLD] = SNOR_BLD_SSEC;
	flnor.nblk[PART_BLD] = SNOR_BLD_NSEC;
	flnor.sblk[PART_HAL] = SNOR_HAL_SSEC;
	flnor.nblk[PART_HAL] = SNOR_HAL_NSEC;
	flnor.sblk[PART_PBA] = SNOR_PBA_SSEC;
	flnor.nblk[PART_PBA] = SNOR_PBA_NSEC;
	flnor.sblk[PART_PRI] = SNOR_PRI_SSEC;
	flnor.nblk[PART_PBA] = SNOR_PRI_NSEC;
	flnor.sblk[PART_SEC] = SNOR_SEC_SSEC;
	flnor.nblk[PART_SEC] = SNOR_SEC_NSEC;
	flnor.sblk[PART_BAK] = SNOR_BAK_SSEC;
	flnor.nblk[PART_BAK] = SNOR_BAK_NSEC;
	flnor.sblk[PART_RMD] = SNOR_RMD_SSEC;
	flnor.nblk[PART_RMD] = SNOR_RMD_NSEC;
	flnor.sblk[PART_ROM] = SNOR_ROM_SSEC;
	flnor.nblk[PART_ROM] = SNOR_ROM_NSEC;
	flnor.sblk[PART_DSP] = SNOR_DSP_SSEC;
	flnor.nblk[PART_DSP] = SNOR_DSP_NSEC;

	snor_ctr.ctr1 = SNOR_CONTROL1;
	snor_ctr.ctr2 = SNOR_CONTROL2;
	snor_ctr.poll_ctr1 = SNOR_CTR1_POLL_MODE(SNOR_POLL_MODE);
	snor_ctr.poll_ctr2 =
		SNOR_CTR2_DAT_POLL_MODE(SNOR_DATA_POLL_MODE)	     |
		SNOR_CTR2_POS_POLL_FAIL(SNOR_DATA_POLL_FAIL_BIT)     |
		SNOR_CTR2_POLL_TARGET_BIT(SNOR_DATA_POLL_TARGET_BIT) |
		SNOR_CTR2_DAT_POLL_FAIL_POL(SNOR_DATA_POLL_FAIL_POLARITY);


	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_RESET];
	cmd->addr[0]	= SNOR_RESET_CYC1_ADDR;
	cmd->data[0]	= SNOR_RESET_CYC1_DATA;
	cmd->cmd_cyc	= SNOR_RESET_CMD_CYC;
	cmd->data_cyc	= SNOR_RESET_DATA_CYC;
	cmd->timeout	= SNOR_RESET_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_AUTOSEL];
	cmd->addr[0]	= SNOR_AUTOSEL_CYC1_ADDR;
	cmd->addr[1]	= SNOR_AUTOSEL_CYC2_ADDR;
	cmd->addr[2]	= SNOR_AUTOSEL_CYC3_ADDR;
	cmd->data[0]	= SNOR_AUTOSEL_CYC1_DATA;
	cmd->data[1]	= SNOR_AUTOSEL_CYC2_DATA;
	cmd->data[2]	= SNOR_AUTOSEL_CYC3_DATA;
	cmd->cmd_cyc	= SNOR_AUTOSEL_CMD_CYC;
	cmd->data_cyc	= SNOR_AUTOSEL_DATA_CYC;
	cmd->timeout	= SNOR_AUTOSEL_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_CFI_QUERY];
	cmd->addr[0]	= SNOR_CFI_QUERY_CYC1_ADDR;
	cmd->data[0]	= SNOR_CFI_QUERY_CYC1_DATA;
	cmd->cmd_cyc	= SNOR_CFI_QUERY_CMD_CYC;
	cmd->data_cyc	= SNOR_CFI_QUERY_DATA_CYC;
	cmd->timeout	= SNOR_CFI_QUERY_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_READ];
	cmd->cmd_cyc	= SNOR_READ_CMD_CYC;
	cmd->data_cyc	= SNOR_READ_DATA_CYC;
	cmd->timeout	= SNOR_READ_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_WRITE];
	cmd->addr[0]	= SNOR_PROG_CYC1_ADDR;
	cmd->addr[1]	= SNOR_PROG_CYC2_ADDR;
	cmd->addr[2]	= SNOR_PROG_CYC3_ADDR;
	cmd->data[0]	= SNOR_PROG_CYC1_DATA;
	cmd->data[1]	= SNOR_PROG_CYC2_DATA;
	cmd->data[2]	= SNOR_PROG_CYC3_DATA;
	cmd->cmd_cyc	= SNOR_PROG_CMD_CYC;
	cmd->data_cyc	= SNOR_PROG_DATA_CYC;
	cmd->timeout	= SNOR_PROG_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_CHIP_ERASE];
	cmd->addr[0]	= SNOR_CHIP_ERASE_CYC1_ADDR;
	cmd->addr[1]	= SNOR_CHIP_ERASE_CYC2_ADDR;
	cmd->addr[2]	= SNOR_CHIP_ERASE_CYC3_ADDR;
	cmd->addr[3]	= SNOR_CHIP_ERASE_CYC4_ADDR;
	cmd->addr[4]	= SNOR_CHIP_ERASE_CYC5_ADDR;
	cmd->addr[5]	= SNOR_CHIP_ERASE_CYC6_ADDR;
	cmd->data[0]	= SNOR_CHIP_ERASE_CYC1_DATA;
	cmd->data[1]	= SNOR_CHIP_ERASE_CYC2_DATA;
	cmd->data[2]	= SNOR_CHIP_ERASE_CYC3_DATA;
	cmd->data[3]	= SNOR_CHIP_ERASE_CYC4_DATA;
	cmd->data[4]	= SNOR_CHIP_ERASE_CYC5_DATA;
	cmd->data[5]	= SNOR_CHIP_ERASE_CYC6_DATA;
	cmd->cmd_cyc	= SNOR_CHIP_ERASE_CMD_CYC;
	cmd->data_cyc	= SNOR_CHIP_ERASE_DATA_CYC;
	cmd->timeout	= SNOR_CHIP_ERASE_TIMEOUT;

	cmd = &snor_ctr.cmd[SNOR_AMB_CMD_BLOCK_ERASE];
	cmd->addr[0]	= SNOR_BLOCK_ERASE_CYC1_ADDR;
	cmd->addr[1]	= SNOR_BLOCK_ERASE_CYC2_ADDR;
	cmd->addr[2]	= SNOR_BLOCK_ERASE_CYC3_ADDR;
	cmd->addr[3]	= SNOR_BLOCK_ERASE_CYC4_ADDR;
	cmd->addr[4]	= SNOR_BLOCK_ERASE_CYC5_ADDR;
	cmd->data[0]	= SNOR_BLOCK_ERASE_CYC1_DATA;
	cmd->data[1]	= SNOR_BLOCK_ERASE_CYC2_DATA;
	cmd->data[2]	= SNOR_BLOCK_ERASE_CYC3_DATA;
	cmd->data[3]	= SNOR_BLOCK_ERASE_CYC4_DATA;
	cmd->data[4]	= SNOR_BLOCK_ERASE_CYC5_DATA;
	cmd->data[5]	= SNOR_BLOCK_ERASE_CYC6_DATA;
	cmd->cmd_cyc	= SNOR_BLOCK_ERASE_CMD_CYC;
	cmd->data_cyc	= SNOR_BLOCK_ERASE_DATA_CYC;
	cmd->timeout	= SNOR_BLOCK_ERASE_TIMEOUT;

	snor_set_timing();

	return 0;
}

/**
 * Reset NOR chip(s).
 */
int snor_reset(void)
{
	u32 ctr;

	/* Setup Flash IO Control Register (exit random mode) */
	ctr = readl(FIO_CTR_REG);
	if (ctr & FIO_CTR_RR) {
		ctr &= ~FIO_CTR_RR;
		writel(FIO_CTR_REG, ctr);
	}

	/* Clear the FIO DMA Status Register */
	writel(FIO_DMASTA_REG, 0x0);

	/* Setup FIO DMA Control Register */
	writel(FIO_DMACTR_REG, FIO_DMACTR_FL | FIO_DMACTR_TS4B);

	return 0;
}

#if	(USE_DMA_XFER <= 1)
/**
 * Read from NOR flash (DMA mode).
 */
static int snor_read_word(u32 addr, u16 *buf)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_READ];
	u32 bank;
	u32 val;

	if (addr & 0x1) {
		/* Address must be word aligned. */
		return -1;
	}

	bank = addr / flnor.bank_size;
	addr = addr >> 1; /* Convert to word address */

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = snor_ctr.ctr1 | SNOR_CTR1_CE_SEL(bank) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	val = snor_ctr.ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	writel(SNOR_CMD_REG, SNOR_CMD_READ_SINGLE | SNOR_CMD_ADDR(addr));
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(SNOR_INT_REG, 0x0);
	*buf = readw(SNOR_ID_STA_REG);

	return 0;
}

static int snor_prog_word(u32 addr, u16 *buf)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_WRITE];
	u32 bank;
	u32 val;

	if (addr & 0x1) {
		return -1;
	}

	bank = addr / flnor.bank_size;
	addr = addr >> 1; /* Convert to word address */

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val =	snor_ctr.ctr1 |
		SNOR_CTR1_CE_SEL(bank) |
		snor_ctr.poll_ctr1;
	writel(SNOR_CTR1_REG, val);

	val =	snor_ctr.ctr2 |
		snor_ctr.poll_ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	writew(SNOR_PROG_DTA_REG, *buf);
	writel(SNOR_CMD_REG, SNOR_CMD_PROG_SINGLE | SNOR_CMD_ADDR(addr));

	/* Wait for interrupt for NOR operation done */
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(SNOR_INT_REG, 0x0);

	/* Restore write protect */
	val = readl(SNOR_CTR1_REG) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	return 0;
}
#endif

/**
 * Read from NOR flash (DMA mode).
 */
static int snor_read_dma(u32 addr, u32 len, u8 *buf)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_READ];
	u32 bank;
	u32 val;

	if ((len & (SNOR_READ_ALIGN - 1)) ||
	    (addr & (SNOR_READ_ALIGN - 1))) {
		/* Transfer length must be multiple of 16 due the DMA */
		/* limitation. */
		return -1;
	}

	if (len == 0)
		return 0;

	bank = addr / flnor.bank_size;

	_clean_flush_d_cache();

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = snor_ctr.ctr1 | SNOR_CTR1_CE_SEL(bank) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	val = snor_ctr.ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc) |
		SNOR_CTR2_DMA_CMD;
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

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
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(FIO_DMASTA_REG, 0x0);
	writel(SNOR_INT_REG, 0x0);

	return 0;
}

/**
 * Program to NOR flash.
 */
static int snor_prog_dma(u32 addr, u32 len, const u8 *buf)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_WRITE];
	u32 bank;
	u32 val;

	if ((len & (SNOR_PROG_ALIGN - 1)) ||
	    (addr & (SNOR_PROG_ALIGN - 1))) {
		/* Transfer length must be multiple of 2048 due the IP design */
		/* limitation. */
		return -1;
	}

	if (len == 0)
		return 0;

	bank = addr / flnor.bank_size;

	_clean_d_cache();

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val =	snor_ctr.ctr1 |
		SNOR_CTR1_CE_SEL(bank) |
		snor_ctr.poll_ctr1;
	writel(SNOR_CTR1_REG, val);

	val =	snor_ctr.ctr2 |
		snor_ctr.poll_ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc) |
		SNOR_CTR2_DMA_CMD;
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

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

#if	(USE_DMA_XFER >= 1)
int snor_read(u32 addr, u32 len, u8 *buf)
{
	u32 remain_size;
	int rval;

	if (len & 0x1 || addr & 0x1) {
		return -1;
	}

	remain_size = len & (SNOR_READ_ALIGN - 1);
	len -= remain_size;

	rval = snor_read_dma(addr, len, buf);
	if (rval < 0)
		return rval;

#if	(USE_DMA_XFER == 2)
	if (remain_size) {
		addr += len;
		buf += len;
		rval = snor_read_dma(addr, SNOR_READ_ALIGN, g_snor_buf);
		if (rval < 0)
			return rval;
		memcpy(buf, g_snor_buf, remain_size);
	}
#else
	if (remain_size) {
		int i;
		addr += len;
		buf += len;
		for (i = 0; i < remain_size; i += 2) {
			rval = snor_read_word(addr + i, (u16 *) (buf + i));
			if (rval < 0)
				return rval;
		}
	}
#endif

	return rval;
}

int snor_prog(u32 addr, u32 len, const u8 *buf)
{
	u32 remain_size;
	int rval;

	if (len & 0x1 || addr & 0x1) {
		return -1;
	}

	remain_size = len & (SNOR_PROG_ALIGN - 1);
	len -= remain_size;

	rval = snor_prog_dma(addr, len, buf);
	if (rval < 0)
		return rval;

#if	(USE_DMA_XFER == 2)
	if (remain_size) {
		addr += len;
		buf += len;
		memset(g_snor_buf, 0xff, SNOR_PROG_ALIGN);
		memcpy(g_snor_buf, buf, remain_size);
		rval = snor_prog_dma(addr, SNOR_PROG_ALIGN, g_snor_buf);
		if (rval < 0)
			return rval;
	}
#else
	if (remain_size) {
		int i;
		addr += len;
		buf += len;
		for (i = 0; i < remain_size; i += 2) {
			rval = snor_prog_word(addr + i, (u16 *) (buf + i));
			if (rval < 0)
				return rval;
		}
	}
#endif

	return rval;
}
#else
int snor_read(u32 addr, u32 len, u8 *buf)
{
	int i, rval;

	if (len & 0x1 || addr & 0x1) {
		return -1;
	}

	for (i = 0; i < len; i += 2) {
		rval = snor_read_word(addr + i, (u16 *) (buf + i));
		if (rval < 0)
			return rval;
	}

	return 0;
}

int snor_prog(u32 addr, u32 len, const u8 *buf)
{
	int i, rval;

	if (len & 0x1 || addr & 0x1) {
		return -1;
	}

	for (i = 0; i < len; i += 2) {
		rval = snor_prog_word(addr + i, (u16 *) (buf + i));
		if (rval < 0)
			return rval;
	}

	return 0;
}
#endif

/**
 * Read a NOR flash block.
 */
int snor_read_block(u32 block, u32 blocks, u8 *buf)
{
	u32 addr, size;

	addr = snor_get_addr(block, 0);
	size = snor_blocks_to_len(block, blocks);
	return snor_read_dma(addr, size, buf);
}

/**
 * Program a NOR flash block.
 */
int snor_prog_block(u32 block, u32 blocks, u8 *buf)
{
	u32 addr, size;

	addr = snor_get_addr(block, 0);
	size = snor_blocks_to_len(block, blocks);
	return snor_prog_dma(addr, size, buf);
}

/**
 * Erase a NOR flash block.
 */
int snor_erase_block(u32 block)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_BLOCK_ERASE];
	u32 addr, bank, val;

	addr = snor_get_addr(block, 0);
	bank = addr / flnor.bank_size;
	addr = addr >> 1; /* Convert to word address */

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val =	snor_ctr.ctr1 |
		SNOR_CTR1_CE_SEL(bank) |
		snor_ctr.poll_ctr1;
	writel(SNOR_CTR1_REG, val);

	val =	snor_ctr.ctr2 |
		snor_ctr.poll_ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	/* Erase block */
	writew(SNOR_PROG_DTA_REG, cmd->data[5]);
	writel(SNOR_CMD_REG, SNOR_CMD_BLOCK_ERASE | SNOR_CMD_ADDR(addr));

	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(SNOR_INT_REG, 0x0);

	/* Restore write protect */
	val = readl(SNOR_CTR1_REG) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	return 0;
}

/**
 * Erase whole NOR flash chip.
 */
int snor_erase_chip(u8 bank)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_CHIP_ERASE];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val =	snor_ctr.ctr1 |
		SNOR_CTR1_CE_SEL(bank) |
		snor_ctr.poll_ctr1;
	writel(SNOR_CTR1_REG, val);

	val =	snor_ctr.ctr2 |
		snor_ctr.poll_ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	/* Erase chip */
	writel(SNOR_CMD_REG, SNOR_CMD_CHIP_ERASE);

	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);
	writel(SNOR_INT_REG, 0x0);

	/* Restore write protect */
	val = readl(SNOR_CTR1_REG) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	return 0;
}

/**
 * Enter CFI Query mode.
 */
int snor_cfi_query(u8 bank)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_CFI_QUERY];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = snor_ctr.ctr1 | SNOR_CTR1_CE_SEL(bank) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	val = snor_ctr.ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	/* Read ID */
	writel(SNOR_CMD_REG, SNOR_CMD_CFI_QUERY);
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(SNOR_INT_REG, 0x0);

	return 0;
}

/**
 * Read ID command.
 */
int snor_autosel(u8 bank, u32 addr, u16 *buf)
{
	struct snor_cmd_s *cmd = &snor_ctr.cmd[SNOR_AMB_CMD_AUTOSEL];
	u32 val;

	/* Setup Flash IO Control Register */
	writel(FIO_CTR_REG, 0x0);

	/* Setup Flash Control Register */
	val = snor_ctr.ctr1 | SNOR_CTR1_CE_SEL(bank) | SNOR_CTR1_WP;
	writel(SNOR_CTR1_REG, val);

	val = snor_ctr.ctr2 |
		SNOR_CTR2_CMD_CYC(cmd->cmd_cyc) |
		SNOR_CTR2_DTA_CYC(cmd->data_cyc);
	writel(SNOR_CTR2_REG, val);

	/* Setup SNOR command/data words */
	snor_setup_cmd_data(cmd);

	/* Read ID */
	writel(SNOR_CMD_REG, SNOR_CMD_AUTOSELECT | SNOR_CMD_ADDR(addr));
	while ((readl(SNOR_INT_REG) & NOR_INT_DI) == 0x0);

	writel(SNOR_INT_REG, 0x0);
	*buf = readw(SNOR_ID_STA_REG);

	return 0;
}
#endif
