/**
 * system/src/bld/host_partgen.c
 *
 * Generate headers for flash partion info.
 *
 * History:
 *    2005/07/21 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <ambhw.h>
#include <sm.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>

flnand_t	flnand;
flnor_t		flnor;

#define FLASH_TIMING_MIN(x) flash_timing(0, x)
#define FLASH_TIMING_MAX(x) flash_timing(1, x)

#if defined(ENABLE_FLASH)
#if !defined(FLASH_NONE)

static int flash_timing(int minmax, int val)
{
	/* Use a conservative value of 48MHz */
	u32 bclk = 216000000;
	u32 cyc_ns;
	u32 n, r;

	/* Calculate ns in one cycle */
	cyc_ns = 1000000000 / bclk;

	n = val / cyc_ns;
	r = val % cyc_ns;

	if (r != 0)
		n++;

	if (minmax)
		n--;

	return n < 1 ? 1 : n;
}

#endif  /* FLASH_NONE */

#if !defined(CONFIG_NAND_NONE)

static void calc_nand_timing(void)
{
	u32 t0_reg, t1_reg, t2_reg, t3_reg, t4_reg, t5_reg;
	u8 tcls, tals, tcs, tds;
	u8 tclh, talh, tch, tdh;
	u8 twp, twh, twb, trr;
	u8 trp, treh, trb, tceh;
	u8 trdelay, tclr, twhr, tir;
	u8 tww, trhz, tar;

	/* Nand control cycle counts down to 0.			*/
	/* To speed up nand read, TRP & TREH can reduce 1 cycle */
	tcls	= FLASH_TIMING_MIN(NAND_TCLS);
	tals	= FLASH_TIMING_MIN(NAND_TALS);
	tcs	= FLASH_TIMING_MIN(NAND_TCS);
	tds	= FLASH_TIMING_MIN(NAND_TDS);
	tclh	= FLASH_TIMING_MIN(NAND_TCLH);
	talh	= FLASH_TIMING_MIN(NAND_TALH);
	tch	= FLASH_TIMING_MIN(NAND_TCH);
	tdh	= FLASH_TIMING_MIN(NAND_TDH);
	twp	= FLASH_TIMING_MIN(NAND_TWP);
	twh	= FLASH_TIMING_MIN(NAND_TWH);
	twb	= FLASH_TIMING_MAX(NAND_TWB);
	trr	= FLASH_TIMING_MIN(NAND_TRR);
	trp	= FLASH_TIMING_MIN(NAND_TRP) - 1;
	treh	= FLASH_TIMING_MIN(NAND_TREH) - 1;
	trb	= FLASH_TIMING_MAX(NAND_TRB);
	tceh	= FLASH_TIMING_MAX(NAND_TCEH);
	trdelay = FLASH_TIMING_MAX(NAND_TRDELAY);
	tclr	= FLASH_TIMING_MIN(NAND_TCLR);
	twhr	= FLASH_TIMING_MIN(NAND_TWHR);
	tir	= FLASH_TIMING_MIN(NAND_TIR);
	tww	= FLASH_TIMING_MIN(NAND_TWW);
	trhz	= FLASH_TIMING_MAX(NAND_TRHZ);
        tar	= FLASH_TIMING_MAX(NAND_TAR);

	t0_reg = NAND_TIM0_TCLS(tcls) |
		 NAND_TIM0_TALS(tals) |
		 NAND_TIM0_TCS(tcs)   |
		 NAND_TIM0_TDS(tds);

	t1_reg = NAND_TIM1_TCLH(tclh) |
		 NAND_TIM1_TALH(talh) |
		 NAND_TIM1_TCH(tch)   |
		 NAND_TIM1_TDH(tdh);

	t2_reg = NAND_TIM2_TWP(twp) |
		 NAND_TIM2_TWH(twh) |
		 NAND_TIM2_TWB(twb) |
		 NAND_TIM2_TRR(trr);

	t3_reg = NAND_TIM3_TRP(trp)   |
		 NAND_TIM3_TREH(treh) |
		 NAND_TIM3_TRB(trb)   |
		 NAND_TIM3_TCEH(tceh);

	t4_reg = NAND_TIM4_TRDELAY(trdelay) |
		 NAND_TIM4_TCLR(tclr) |
		 NAND_TIM4_TWHR(twhr) |
		 NAND_TIM4_TIR(tir);

	t5_reg = NAND_TIM5_TWW(tww) |
		 NAND_TIM5_TRHZ(trhz) |
		 NAND_TIM5_TAR(tar);

	flnand.timing0 = t0_reg;
	flnand.timing1 = t1_reg;
	flnand.timing2 = t2_reg;
	flnand.timing3 = t3_reg;
	flnand.timing4 = t4_reg;
	flnand.timing5 = t5_reg;
}

#endif  /* !CONFIG_NAND_NONE */

#if !defined(CONFIG_ONENAND_NONE)
static void calc_onenand_timing(void)
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
	u32 ssec, nsec;

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
}

void output_onenand_parameter(u32 *part_size, const char **part_str)
{
	int sblk;
	int nblk;
	int block_size;
	int i;

	flnand.main_size = ONENAND_MAIN_SIZE;
	flnand.spare_size = ONENAND_SPARE_SIZE;
	flnand.pages_per_block = ONENAND_PAGES_PER_BLOCK;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}

	calc_onenand_timing();

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define ONENAND_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define ONENAND_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}

	printf("#define ONENAND_TIM0_VAL 0x%.8x\n", flnand.timing0);
	printf("#define ONENAND_TIM1_VAL 0x%.8x\n", flnand.timing1);
	printf("#define ONENAND_TIM2_VAL 0x%.8x\n", flnand.timing2);
	printf("#define ONENAND_TIM3_VAL 0x%.8x\n", flnand.timing3);
	printf("#define ONENAND_TIM4_VAL 0x%.8x\n", flnand.timing4);
	printf("#define ONENAND_TIM5_VAL 0x%.8x\n", flnand.timing5);
	printf("#define ONENAND_TIM6_VAL 0x%.8x\n", flnand.timing6);
	printf("#define ONENAND_TIM7_VAL 0x%.8x\n", flnand.timing7);
	printf("#define ONENAND_TIM8_VAL 0x%.8x\n", flnand.timing8);
	printf("#define ONENAND_TIM9_VAL 0x%.8x\n", flnand.timing9);
	printf("\n");

	/* Generate the constants of ONENAND page size 512 bytes for bst use */
	/* because to support run-time nand selection. */
	flnand.main_size = 512;
	flnand.pages_per_block = 32;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define ONENAND_512_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define ONENAND_512_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}
	printf("\n");

	/* Generate the constants of ONENAND page size 1024 bytes for bst use */
	/* because to support run-time nand selection. */
	flnand.main_size = 1024;
	flnand.pages_per_block = 64;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define ONENAND_1k_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define ONENAND_1k_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}
	printf("\n");

	/* Generate the constants of ONENAND page size 2048 bytes for bst use */
	/* because to support run-time nand selection. */
	flnand.main_size = 2048;
	flnand.pages_per_block = 64;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define ONENAND_2k_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define ONENAND_2k_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}
	printf("\n");
}

#endif /* !CONFIG_ONENAND_NONE */

#if !defined(CONFIG_NOR_NONE)

static void calc_nor_timing(void)
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

	flnor.timing0 = t0_reg;
	flnor.timing1 = t1_reg;
	flnor.timing2 = t2_reg;
	flnor.timing3 = t3_reg;
	flnor.timing4 = t4_reg;
	flnor.timing5 = t5_reg;
}

#endif  /* !CONFIG_NOR_NONE */

#if !defined(CONFIG_SNOR_NONE)
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

	if (block2_len > flnor.total_block2s_size) {
		printf("#error invalid block size\n");
	}

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

void output_nor_parameter(u32 *part_size, const char **part_str)
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
	u32 sblk, nblk;
	int i;

	memset(&flnor, 0x0, sizeof(flnor));

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

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		nblk = snor_len_to_blocks(sblk, part_size[i]);
		flnor.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnor.nblk[i] = nblk;
	}

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define SNOR_%s_SSEC %d\n",
					part_str[i], flnor.sblk[i]);
		printf("#define SNOR_%s_NSEC %d\n",
					part_str[i], flnor.nblk[i]);
	}

	printf("#define SNOR_TIM0_VAL 0x%.8x\n", flnor.timing0);
	printf("#define SNOR_TIM1_VAL 0x%.8x\n", flnor.timing1);
	printf("#define SNOR_TIM2_VAL 0x%.8x\n", flnor.timing2);
	printf("#define SNOR_TIM3_VAL 0x%.8x\n", flnor.timing3);
	printf("#define SNOR_TIM4_VAL 0x%.8x\n", flnor.timing4);
	printf("#define SNOR_TIM5_VAL 0x%.8x\n", flnor.timing5);
	printf("#define SNOR_TIM6_VAL 0x%.8x\n", flnor.timing6);
	printf("#define SNOR_TIM7_VAL 0x%.8x\n", flnor.timing7);
	printf("#define SNOR_TIM8_VAL 0x%.8x\n", flnor.timing8);
	printf("#define SNOR_TIM9_VAL 0x%.8x\n", flnor.timing9);
	printf("\n");
}
#endif

#endif	/* ENABLE_FLASH */


int main(int argc, char **argv)
{
#if (!defined(FLASH_NONE) || defined(ENABLE_SD))
	int sblk;
	int nblk;
	int block_size, i;
	u32 part_size[TOTAL_FW_PARTS];
	const char *part_str[] = {"BST", "PTB", "BLD", "HAL", "PBA",
				  "PRI", "SEC", "BAK", "RMD", "ROM",
				  "DSP", "LNX", "SWP", "ADD", "ADC"};
#endif

	memset(&flnand, 0x0, sizeof(flnand));
	memset(&flnor, 0x0, sizeof(flnor));

	printf("/*\n");
	printf(" * Automatically generated header file: don't edit\n");
	printf(" */\n");
	printf("\n");
	printf("#ifndef __FLASH_PARTGEN_H__\n");
	printf("#define __FLASH_PARTGEN_H__\n");
	printf("\n");

	printf("/* Offset from start of PTB */\n");
	printf("#define PTB_BST_MAGIC_OFFSET %d\n", 24);
	printf("#define PTB_BLD_LEN_OFFSET   %d\n",
	       (unsigned int) sizeof(flpart_t) * PART_BLD + 12);
	printf("#define PTB_BLD_MAGIC_OFFSET %d\n",
	       (unsigned int) sizeof(flpart_t) * PART_BLD + 24);
	printf("\n");

	printf("/* Offset from start of PTB META */\n");
	printf("#define META_BLD_SBLK_OFFSET %d\n", 32);
	printf("#define META_BLD_NBLK_OFFSET %d\n", 36);
	printf("\n");


#if (!defined(FLASH_NONE) || defined(ENABLE_SD))
#ifdef AMBOOT_BST_FIXED_SIZE
	part_size[PART_BST] = AMBOOT_BST_FIXED_SIZE;
#else
	part_size[PART_BST] = AMBOOT_BST_SIZE;
#endif /* AMBOOT_BST_FIXED_SIZE */
	part_size[PART_PTB] = AMBOOT_PTB_SIZE;
	part_size[PART_BLD] = AMBOOT_BLD_SIZE;
	part_size[PART_HAL] = AMBOOT_HAL_SIZE;
	part_size[PART_PBA] = AMBOOT_PBA_SIZE;
	part_size[PART_PRI] = AMBOOT_PRI_SIZE;
	part_size[PART_SEC] = AMBOOT_SEC_SIZE;
	part_size[PART_BAK] = AMBOOT_BAK_SIZE;
	part_size[PART_RMD] = AMBOOT_RMD_SIZE;
	part_size[PART_ROM] = AMBOOT_ROM_SIZE;
	part_size[PART_DSP] = AMBOOT_DSP_SIZE;
	part_size[PART_LNX] = AMBOOT_LNX_SIZE;
	part_size[PART_SWP] = AMBOOT_SWP_SIZE;
	part_size[PART_ADD] = AMBOOT_ADD_SIZE;
	part_size[PART_ADC] = AMBOOT_ADC_SIZE;
#endif

#if defined (ENABLE_FLASH)
#if !defined(CONFIG_NAND_NONE)
	flnand.main_size = NAND_MAIN_SIZE;
	flnand.spare_size = NAND_SPARE_SIZE;
	flnand.pages_per_block = NAND_PAGES_PER_BLOCK;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}

	calc_nand_timing();
#endif

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define NAND_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define NAND_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}

	printf("#define NAND_TIM0_VAL 0x%.8x\n", flnand.timing0);
	printf("#define NAND_TIM1_VAL 0x%.8x\n", flnand.timing1);
	printf("#define NAND_TIM2_VAL 0x%.8x\n", flnand.timing2);
	printf("#define NAND_TIM3_VAL 0x%.8x\n", flnand.timing3);
	printf("#define NAND_TIM4_VAL 0x%.8x\n", flnand.timing4);
	printf("#define NAND_TIM5_VAL 0x%.8x\n", flnand.timing5);
	printf("\n");

#if !defined (CONFIG_NAND_NONE)
	/* Generate the constants of NAND page size 512 bytes for bst use */
	/* because to support run-time nand selection. */
	flnand.main_size = 512;
	flnand.pages_per_block = 32;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}
#endif

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define NAND_512_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define NAND_512_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}
	printf("\n");

#if !defined (CONFIG_NAND_NONE)
	/* Generate the constants of NAND page size 2K bytes for bst use */
	/* because to support run-time nand selection. */
	flnand.main_size = 2048;
	flnand.pages_per_block = 64;
	block_size = flnand.main_size * flnand.pages_per_block;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnand.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnand.nblk[i] = nblk;
	}
#endif

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define NAND_2K_%s_SBLK %d\n",
					part_str[i], flnand.sblk[i]);
		printf("#define NAND_2K_%s_NBLK %d\n",
					part_str[i], flnand.nblk[i]);
	}
	printf("\n");

#if !defined(CONFIG_NOR_NONE)
	flnor.sector_size = NOR_SECTOR_SIZE;
	block_size = NOR_SECTOR_SIZE;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		flnor.sblk[i] = (nblk == 0) ? 0 : sblk;
		flnor.nblk[i] = nblk;
	}

	calc_nor_timing();
#endif

	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		printf("#define NOR_%s_SSEC %d\n",
					part_str[i], flnor.sblk[i]);
		printf("#define NOR_%s_NSEC %d\n",
					part_str[i], flnor.nblk[i]);
	}

	printf("#define NOR_TIM0_VAL 0x%.8x\n", flnor.timing0);
	printf("#define NOR_TIM1_VAL 0x%.8x\n", flnor.timing1);
	printf("#define NOR_TIM2_VAL 0x%.8x\n", flnor.timing2);
	printf("#define NOR_TIM3_VAL 0x%.8x\n", flnor.timing3);
	printf("#define NOR_TIM4_VAL 0x%.8x\n", flnor.timing4);
	printf("#define NOR_TIM5_VAL 0x%.8x\n", flnor.timing5);
	printf("\n");

#if !defined(CONFIG_ONENAND_NONE)
	output_onenand_parameter(part_size, part_str);
#endif

#if !defined(CONFIG_SNOR_NONE)
	output_nor_parameter(part_size, part_str);
#endif
#endif	/* ENABLE_FLASH */

#if defined(ENABLE_SD)
	block_size = 512;

	sblk = nblk = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		sblk += nblk;
		for (nblk = 0; (nblk * block_size) < part_size[i]; nblk++);
		printf("#define SM_%s_SSEC %d\n", part_str[i], ((nblk == 0) ?
								0 : sblk));
		printf("#define SM_%s_NSEC %d\n", part_str[i], nblk);
	}
#endif

	printf("#endif\n");

	return 0;
}
