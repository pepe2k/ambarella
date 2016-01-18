/*
 * ambhw/nor_onenand.h
 *
 * History:
 *	2007/01/27 - [Evan Chen] created file
 *
 * Copyright (C) 2006-2008, Ambarella, Inc.
 */

#ifndef __AMBHW__NOR_ONENAND_H__
#define __AMBHW__NOR_ONENAND_H__

#include <ambhw/chip.h>
#include <ambhw/busaddr.h>

/***********************************/
/* SNOR/ONAND Controller Registers */
/***********************************/
/* ONE NAND mode */
#define ONENAND_CMD_OFFSET		0x124
#define ONENAND_TIM0_OFFSET		0x128
#define ONENAND_TIM1_OFFSET		0x12c
#define ONENAND_TIM2_OFFSET		0x130
#define ONENAND_TIM3_OFFSET		0x134
#define ONENAND_TIM4_OFFSET		0x138
#define ONENAND_TIM5_OFFSET		0x13c
#define ONENAND_TIM6_OFFSET		0x140
#define ONENAND_TIM7_OFFSET		0x144
#define ONENAND_ID_STA_OFFSET		0x14c
#define ONENAND_INT_OFFSET		0x150
#define ONENAND_CMD_WORD0_OFFSET	0x154
#define ONENAND_CMD_WORD1_OFFSET	0x158
#define ONENAND_CMD_WORD2_OFFSET	0x15c
#define ONENAND_CMD_ADDR0_OFFSET	0x160
#define ONENAND_CMD_ADDR1_OFFSET	0x164
#define ONENAND_CMD_ADDR2_OFFSET	0x168
#define ONENAND_CTR2_OFFSET		0x16c
#define ONENAND_TIM8_OFFSET		0x170
#define ONENAND_TIM9_OFFSET		0x174
#define ONENAND_PROG_DTA_OFFSET		0x178
#define ONENAND_CTR1_OFFSET		0x17c

#define ONENAND_CMD_REG			FIO_REG(0x124)
#define ONENAND_TIM0_REG		FIO_REG(0x128)
#define ONENAND_TIM1_REG		FIO_REG(0x12c)
#define ONENAND_TIM2_REG		FIO_REG(0x130)
#define ONENAND_TIM3_REG		FIO_REG(0x134)
#define ONENAND_TIM4_REG		FIO_REG(0x138)
#define ONENAND_TIM5_REG		FIO_REG(0x13c)
#define ONENAND_TIM6_REG		FIO_REG(0x140)
#define ONENAND_TIM7_REG		FIO_REG(0x144)
#define ONENAND_ID_STA_REG		FIO_REG(0x14c)
#define ONENAND_INT_REG			FIO_REG(0x150)
#define ONENAND_CMD_WORD0_REG		FIO_REG(0x154)
#define ONENAND_CMD_WORD1_REG		FIO_REG(0x158)
#define ONENAND_CMD_WORD2_REG		FIO_REG(0x15c)
#define ONENAND_CMD_ADDR0_REG		FIO_REG(0x160)
#define ONENAND_CMD_ADDR1_REG		FIO_REG(0x164)
#define ONENAND_CMD_ADDR2_REG		FIO_REG(0x168)
#define ONENAND_CTR2_REG		FIO_REG(0x16c)
#define ONENAND_TIM8_REG		FIO_REG(0x170)
#define ONENAND_TIM9_REG		FIO_REG(0x174)
#define ONENAND_PROG_DTA_REG		FIO_REG(0x178)
#define ONENAND_CTR1_REG		FIO_REG(0x17c)

/* ONENAND_CTR1_REG */
#define ONENAND_CTR1_LATEN_OFFSET(x)	(((x) & 0x1f) << 24)
#define ONENAND_CTR1_POLL_MODE(x)	(((x) & 0x7) << 18)
#define ONENAND_CTR1_CE_SEL(x)		(((x) & 0x3) << 7)
#define ONENAND_CTR1_LEN_VALID_ADDR(x)	(((x) & 0x3f))

#define ONENAND_CTR1_IE 		0x80000000
#define ONENAND_CTR1_SPARE_EN		0x20000000
#define ONENAND_CTR1_TYPE_MUXED  	0x00c00000
#define ONENAND_CTR1_TYPE_DEMUXED 	0x00800000
#define ONENAND_CTR1_POLL_REG	 	0x00100000
#define ONENAND_CTR1_POLL_INT 	 	0x000C0000
#define ONENAND_CTR1_POLL_DAT 	 	0x00080000
#define ONENAND_CTR1_POLL_RB 	 	0x00040000
#define ONENAND_CTR1_POLL_NONE		0x00000000
#define ONENAND_CTR1_BLK_256_PAGE 	0x00030000
#define ONENAND_CTR1_BLK_128_PAGE	0x00020000
#define ONENAND_CTR1_BLK_64_PAGE 	0x00010000
#define ONENAND_CTR1_BLK_32_PAGE 	0x00000000
#define ONENAND_CTR1_TYPE_QDP 	 	0x00008000
#define ONENAND_CTR1_TYPE_DDP 	 	0x00004000
#define ONENAND_CTR1_TYPE_SDP 	 	0x00000000
#define ONENAND_CTR1_PAGE_SZ_4K	 	0x00002000
#define ONENAND_CTR1_PAGE_SZ_2K	 	0x00001000
#define ONENAND_CTR1_PAGE_SZ_1K	 	0x00000000
#define ONENAND_CTR1_AVD_TYPE1	 	0x00000800
#define ONENAND_CTR1_AVD_TYPE0	 	0x00000000
#define ONENAND_CTR1_AVD_EN	 	0x00000400
#define ONENAND_CTR1_AVD_DIS	 	0x00000000
#define ONENAND_CTR1_BUS_SHARE	 	0x00000200
#define ONENAND_CTR1_CE_SEL3	 	0x00001800
#define ONENAND_CTR1_CE_SEL2	 	0x00001000
#define ONENAND_CTR1_CE_SEL1	 	0x00000800
#define ONENAND_CTR1_CE_SEL0	 	0x00000000
#define ONENAND_CTR1_FLAG_ONENAND 	0x00000040

/* ONENAND_CTR2_REG */
#define ONENAND_CTR2_DAT_POLL_MODE(x)		(((x) & 0x3) << 29)
#define ONENAND_CTR2_DAT_POLL_FAIL_POL(x)	(((x) & 0x1) << 28)
#define ONENAND_CTR2_PROG_MODE(x)		(((x) & 0x3) << 22)
#define ONENAND_CTR2_POS_POLL_FAIL(x)		(((x) & 0xf) << 24)
#define ONENAND_CTR2_POLL_TARGET_BIT(x)		(((x) & 0xf) << 16)
#define ONENAND_CTR2_DTA_CYC(x)	 		(((x) & 0x7ff) << 8)
#define ONENAND_CTR2_CMD_CYC(x)	 		(((x) & 0xf))

#define ONENAND_CTR2_DAT_POLL_MODE2	0x40000000
#define ONENAND_CTR2_DAT_POLL_MODE1	0x20000000
#define ONENAND_CTR2_DAT_POLL_MODE0	0x00000000
#define ONENAND_CTR2_PROG_2X_CACHE	0x00800000
#define ONENAND_CTR2_PROG_2X	 	0x00400000
#define ONENAND_CTR2_PROG_NORMAL 	0x00000000
#define ONENAND_CTR2_POLL_POLAR	 	0x00100000
#define ONENAND_CTR2_OP_SYNC	 	0x00000080
#define ONENAND_CTR2_ONENAND_DMA	0x00000040
#define ONENAND_CTR2_DMA_CMD	 	0x00000020

/* ONENAND_CMD_REG */
#define ONENAND_CMD_ADDR(x)	 	(((x) & 0xfffffff) << 4)

#define ONENAND_CMD_DMA		 	0x0
#define ONENAND_CMD_RESET	 	0x2
#define ONENAND_CMD_ERASE		0x9
#define ONENAND_CMD_ASEL_MODE	 	0xa
#define ONENAND_CMD_POLL_REG	 	0xc
#define ONENAND_CMD_READ_REG		0xe
#define ONENAND_CMD_WRITE_REG		0x9

/* ONENAND_ID_STA_REG */
#define ONENAND_ID_STA_FAIL		0x80000000

/* ONENAND_TIM0_REG */
#define ONENAND_TIM0_TAA(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM0_TOE(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM0_TOEH(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM0_TCE(x)		(((x) & 0xff))

/* ONENAND_TIM1_REG */
#define ONENAND_TIM1_TPA(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM1_TRP(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM1_TRH(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM1_TOES(x)		(((x) & 0xff))

/* ONENAND_TIM2_REG */
#define ONENAND_TIM2_TCS(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM2_TCH(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM2_TWP(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM2_TWH(x)		(((x) & 0xff))

/* ONENAND_TIM3_REG */
#define ONENAND_TIM3_TRB(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM3_TAHT(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM3_TASO(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM3_TOEPH(x)		(((x) & 0xff))

/* ONENAND_TIM4_REG */
#define ONENAND_TIM4_TAVDCS(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM4_TAVDCH(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM4_TAS(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM4_TAH(x)		(((x) & 0xff))

/* ONENAND_TIM5_REG */
#define ONENAND_TIM5_TELF(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM5_TDS(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM5_TDH(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM5_TBUSY(x)		(((x) & 0xff))

/* ONENAND_TIM6_REG */
#define ONENAND_TIM6_TDP(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM6_TCEPH(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM6_TOEZ(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM6_TBA(x)		(((x) & 0xff))

/* ONENAND_TIM7_REG */
#define ONENAND_TIM7_TREADY(x)		(((x) & 0xffff) << 16)
#define ONENAND_TIM7_TACH(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM7_TCEHP(x)		(((x) & 0xff))

/* ONENAND_TIM8_REG */
#define ONENAND_TIM8_TAVDS(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM8_TOH(x)		(((x) & 0xff) << 16)
#define ONENAND_TIM8_TAVDP(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM8_TAAVDH(x)		(((x) & 0xff))

/* ONENAND_TIM9_REG */
#define ONENAND_TIM9_TIAA(x)		(((x) & 0xff) << 24)
#define ONENAND_TIM9_TCLK_PERIOD(x)	(((x) & 0xff) << 16)
#define ONENAND_TIM9_TWEA(x)		(((x) & 0xff) << 8)
#define ONENAND_TIM9_TACS(x)		(((x) & 0xff))

/* ONENAND_CMD_WORD_REG */
#define ONENAND_CMD_WORD_LO(x)		(((x) & 0xffff))
#define ONENAND_CMD_WORD_HI(x)		(((x) & 0xffff) << 16)

/* ONENAND_CMD_ADDR_REG */
#define ONENAND_CMD_ADDR_LO(x)		(((x) & 0xffff))
#define ONENAND_CMD_ADDR_HI(x)		(((x) & 0xffff) << 16)


/* ---------------------------------------------------------------------- */
/* SNOR mode definitions */

/* SNOR mode */
#define SNOR_CMD_OFFSET			0x124
#define SNOR_TIM0_OFFSET		0x128
#define SNOR_TIM1_OFFSET		0x12c
#define SNOR_TIM2_OFFSET		0x130
#define SNOR_TIM3_OFFSET		0x134
#define SNOR_TIM4_OFFSET		0x138
#define SNOR_TIM5_OFFSET		0x13c
#define SNOR_TIM6_OFFSET		0x140
#define SNOR_TIM7_OFFSET		0x144
#define SNOR_ID_STA_OFFSET		0x14c
#define SNOR_INT_OFFSET			0x150
#define SNOR_CMD_WORD0_OFFSET		0x154
#define SNOR_CMD_WORD1_OFFSET		0x158
#define SNOR_CMD_WORD2_OFFSET		0x15c
#define SNOR_CMD_ADDR0_OFFSET		0x160
#define SNOR_CMD_ADDR1_OFFSET		0x164
#define SNOR_CMD_ADDR2_OFFSET		0x168
#define SNOR_CTR2_OFFSET		0x16c
#define SNOR_TIM8_OFFSET		0x170
#define SNOR_TIM9_OFFSET		0x174
#define SNOR_PROG_DTA_OFFSET		0x178
#define SNOR_CTR1_OFFSET		0x17c


#define SNOR_CMD_REG			FIO_REG(0x124)
#define SNOR_TIM0_REG			FIO_REG(0x128)
#define SNOR_TIM1_REG			FIO_REG(0x12c)
#define SNOR_TIM2_REG			FIO_REG(0x130)
#define SNOR_TIM3_REG			FIO_REG(0x134)
#define SNOR_TIM4_REG			FIO_REG(0x138)
#define SNOR_TIM5_REG			FIO_REG(0x13c)
#define SNOR_TIM6_REG			FIO_REG(0x140)
#define SNOR_TIM7_REG			FIO_REG(0x144)
#define SNOR_ID_STA_REG			FIO_REG(0x14c)
#define SNOR_INT_REG			FIO_REG(0x150)
#define SNOR_CMD_WORD0_REG		FIO_REG(0x154)
#define SNOR_CMD_WORD1_REG		FIO_REG(0x158)
#define SNOR_CMD_WORD2_REG		FIO_REG(0x15c)
#define SNOR_CMD_ADDR0_REG		FIO_REG(0x160)
#define SNOR_CMD_ADDR1_REG		FIO_REG(0x164)
#define SNOR_CMD_ADDR2_REG		FIO_REG(0x168)
#define SNOR_CTR2_REG			FIO_REG(0x16c)
#define SNOR_TIM8_REG			FIO_REG(0x170)
#define SNOR_TIM9_REG			FIO_REG(0x174)
#define SNOR_PROG_DTA_REG		FIO_REG(0x178)
#define SNOR_CTR1_REG			FIO_REG(0x17c)

/* SNOR_CTR1_REG */
#define SNOR_CTR1_LATEN_OFFSET(x)	(((x) & 0x1f) << 24)
#define SNOR_CTR1_POLL_MODE(x)		(((x) & 0x7) << 18)
#define SNOR_CTR1_LEN_VALID_ADDR(x)	((x) & 0x3f)
#define SNOR_CTR1_CE_SEL(x)		(((x) & 0x3) << 7)

#define SNOR_CTR1_IE 		 	0x80000000
#define SNOR_CTR1_WP 		 	0x40000000
#define SNOR_CTR1_TYPE_MUXED  	 	0x00800000
#define SNOR_CTR1_TYPE_ONENAND	 	0x00600000
#define SNOR_CTR1_TYPE_DEMUXED_NOR	0x00400000
#define SNOR_CTR1_TYPE_INTEL	 	0x00000000
#define SNOR_CTR1_POLL_REG	 	0x00100000
#define SNOR_CTR1_POLL_INT 	 	0x000C0000
#define SNOR_CTR1_POLL_DAT 	 	0x00080000
#define SNOR_CTR1_POLL_RB 	 	0x00040000
#define SNOR_CTR1_POLL_NONE 	 	0x00000000
#define SNOR_CTR1_AVD_TYPE1	 	0x00000800
#define SNOR_CTR1_AVD_TYPE0	 	0x00000000
#define SNOR_CTR1_SNOR_AVD_EN	 	0x00000400
#define SNOR_CTR1_SNOR_AVD_DIS	 	0x00000000
#define SNOR_CTR1_BUS_SHARE	 	0x00000200
#define SNOR_CTR1_NO_BUS_SHARE	 	0x00000000
#define SNOR_CTR1_CE_SEL3	 	0x00001800
#define SNOR_CTR1_CE_SEL2	 	0x00001000
#define SNOR_CTR1_CE_SEL1		0x00000800
#define SNOR_CTR1_CE_SEL0	 	0x00000000


/* SNOR_CTR2_REG */
#define SNOR_CTR2_DAT_POLL_MODE(x)	(((x) & 0x3) << 29)
#define SNOR_CTR2_DAT_POLL_FAIL_POL(x)	(((x) & 0x1) << 29)
#define SNOR_CTR2_POS_POLL_FAIL(x)	(((x) & 0xf) << 24)
#define SNOR_CTR2_DTA_CYC(x)	 	(((x) & 0x7ff) << 8)
#define SNOR_CTR2_CMD_CYC(x)	 	(((x) & 0xf))
#define SNOR_CTR2_POLL_TARGET_BIT(x)	(((x) & 0xf) << 16)

#define SNOR_CTR2_DAT_POLL_MODE2	0x40000000
#define SNOR_CTR2_DAT_POLL_MODE1	0x20000000
#define SNOR_CTR2_DAT_POLL_MODE0	0x00000000
#define SNOR_CTR2_DAT_POLL_FAIL_POL_HI  0x10000000
#define SNOR_CTR2_POLL_POLAR  		0x00100000
#define SNOR_CTR2_OP_SYNC	 	0x00000080
#define SNOR_CTR2_DMA_CMD	 	0x00000020

/* SNOR_CMD_REG */
#define SNOR_CMD_ADDR(x)	 	(((x) & 0xfffffff) << 4)

#define SNOR_CMD_DMA		 	0x1
#define SNOR_CMD_RESET			0x9
#define SNOR_CMD_ERASE		 	0x9
#define SNOR_CMD_CHIP_ERASE		0x9
#define SNOR_CMD_BLOCK_ERASE		0x9
#define SNOR_CMD_UNLOCK_CHIP_ERASE	0x9
#define SNOR_CMD_UNLOCK_BLOCK_ERASE	0x9
#define SNOR_CMD_UNLOCK_BYPASS		0x9
#define SNOR_CMD_UNLOCK_BYPASS_RESET	0x9
#define SNOR_CMD_UNLOCK_BYPASS_PROG	0x9
#define SNOR_CMD_CFI_QUERY		0x9
#define	SNOR_CMD_SET_BURST_CFG		0x9
#define SNOR_CMF_PROTECT_BLOCK		0x9
#define SNOR_CMF_PROTECT_BGROUP		0x9
#define SNOR_CMD_PROG_SINGLE	 	0x9
#define SNOR_CMD_READ_SINGLE	 	0xe
#define SNOR_CMD_AUTOSELECT	 	0xa
#define SNOR_CMD_READ_STA	 	0xc
#define SNOR_CMD_READ			0xe
#define SNOR_CMD_PROG 			0xf

/* SNOR_ID_STA_REG */
#define SNOR_ID_STA_FAIL		0x80000000

/* SNOR_TIM0_REG */
#define SNOR_TIM0_TAA(x)		(((x) & 0xff) << 24)
#define SNOR_TIM0_TOE(x)		(((x) & 0xff) << 16)
#define SNOR_TIM0_TOEH(x)		(((x) & 0xff) << 8)
#define SNOR_TIM0_TCE(x)		(((x) & 0xff))

/* SNOR_TIM1_REG */
#define SNOR_TIM1_TPA(x)		(((x) & 0xff) << 24)
#define SNOR_TIM1_TRP(x)		(((x) & 0xff) << 16)
#define SNOR_TIM1_TRH(x)		(((x) & 0xff) << 8)
#define SNOR_TIM1_TOES(x)		(((x) & 0xff))

/* SNOR_TIM2_REG */
#define SNOR_TIM2_TCS(x)		(((x) & 0xff) << 24)
#define SNOR_TIM2_TCH(x)		(((x) & 0xff) << 16)
#define SNOR_TIM2_TWP(x)		(((x) & 0xff) << 8)
#define SNOR_TIM2_TWH(x)		(((x) & 0xff))

/* SNOR_TIM3_REG */
#define SNOR_TIM3_TRB(x)		(((x) & 0xff) << 24)
#define SNOR_TIM3_TAHT(x)		(((x) & 0xff) << 16)
#define SNOR_TIM3_TASO(x)		(((x) & 0xff) << 8)
#define SNOR_TIM3_TOEPH(x)		(((x) & 0xff))

/* SNOR_TIM4_REG */
#define SNOR_TIM4_TAVDCS(x)		(((x) & 0xff) << 24)
#define SNOR_TIM4_TAVDCH(x)		(((x) & 0xff) << 16)
#define SNOR_TIM4_TAS(x)		(((x) & 0xff) << 8)
#define SNOR_TIM4_TAH(x)		(((x) & 0xff))

/* SNOR_TIM5_REG */
#define SNOR_TIM5_TELF(x)		(((x) & 0xff) << 24)
#define SNOR_TIM5_TDS(x)		(((x) & 0xff) << 16)
#define SNOR_TIM5_TDH(x)		(((x) & 0xff) << 8)
#define SNOR_TIM5_TBUSY(x)		(((x) & 0xff))

/* SNOR_TIM6_REG */
#define SNOR_TIM6_TDP(x)		(((x) & 0xff) << 24)
#define SNOR_TIM6_TCEPH(x)		(((x) & 0xff) << 16)
#define SNOR_TIM6_TOEZ(x)		(((x) & 0xff) << 8)
#define SNOR_TIM6_TBA(x)		(((x) & 0xff))

/* SNOR_TIM7_REG */
#define SNOR_TIM7_TREADY(x)		(((x) & 0xffff) << 16)
#define SNOR_TIM7_TACH(x)		(((x) & 0xff) << 8)
#define SNOR_TIM7_TCEHP(x)		(((x) & 0xff))

/* SNOR_TIM8_REG */
#define SNOR_TIM8_TAVDS(x)		(((x) & 0xff) << 24)
#define SNOR_TIM8_TOH(x)		(((x) & 0xff) << 16)
#define SNOR_TIM8_TAVDP(x)		(((x) & 0xff) << 8)
#define SNOR_TIM8_TAAVDH(x)		(((x) & 0xff))

/* SNOR_TIM9_REG */
#define SNOR_TIM9_TIAA(x)		(((x) & 0xff) << 24)
#define SNOR_TIM9_TCLK_PERIOD(x)	(((x) & 0xff) << 16)
#define SNOR_TIM9_TWEA(x)		(((x) & 0xff) << 8)
#define SNOR_TIM9_TACS(x)		(((x) & 0xff))

/* SNOR_CMD_WORD_REG */
#define SNOR_CMD_WORD_LO(x)		(((x) & 0xffff))
#define SNOR_CMD_WORD_HI(x)		(((x) & 0xffff) << 16)

/* SNOR_CMD_ADDR_REG */
#define SNOR_CMD_ADDR_LO(x)		(((x) & 0xffff))
#define SNOR_CMD_ADDR_HI(x)		(((x) & 0xffff) << 16)

#endif /* __AMBHW__SNOR_ONENAND_H__ */

