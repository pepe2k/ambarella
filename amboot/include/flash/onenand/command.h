/**
 * @file system/include/flash/onenand/command.h
 *
 * History:
 *    2009/08/17 - [Evan Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __ONENAND_COMMAND_H__
#define __ONENAND_COMMAND_H__

/**
 * One nand device registers
 */
#define ONENAND_DEV_MAN_ID_REG		0xf000
#define ONENAND_DEV_DEV_ID_REG		0xf001
#define ONENAND_DEV_VER_ID_REG		0xf002
#define ONENAND_DEV_DTA_BUF_SZ_REG	0xf003
#define ONENAND_DEV_BOOT_BUF_SZ_REG	0xf004
#define ONENAND_DEV_BUF_NUMS_REG	0xf005
#define ONENAND_DEV_TECH_REG		0xf006
#define ONENAND_DEV_ADDR1_REG		0xf100
#define ONENAND_DEV_ADDR2_REG		0xf101
#define ONENAND_DEV_ADDR3_REG		0xf102
#define ONENAND_DEV_ADDR4_REG		0xf103
#define ONENAND_DEV_ADDR5_REG		0xf104
#define ONENAND_DEV_ADDR6_REG		0xf105
#define ONENAND_DEV_ADDR7_REG		0xf106
#define ONENAND_DEV_ADDR8_REG		0xf107
#define ONENAND_DEV_BUF_START_REG	0xf200
#define ONENAND_DEV_CMD_REG		0xf220
#define ONENAND_DEV_SYS_CFG1_REG	0xf221
#define ONENAND_DEV_SYS_CFG2_REG	0xf222
#define ONENAND_DEV_CTR_STA_REG		0xf240
#define ONENAND_DEV_INT_STA_REG		0xf241
#define ONENAND_DEV_BLK_ADDR_REG	0xf24c
#define ONENAND_DEV_WP_STA_REG		0xf24e
#define ONENAND_DEV_ECC_STA_REG		0xff00
#define ONENAND_DEV_ECC_MAIN1_REG	0xff01
#define ONENAND_DEV_ECC_SPARE1_REG	0xff02
#define ONENAND_DEV_ECC_MAIN2_REG	0xff03
#define ONENAND_DEV_ECC_SPARE2_REG	0xff04

/**
 * One nand data ram start
 */
#define ONENAND_DAT_RAM_START		0x200
#define ONENAND_SPARE_RAM_START		0x8010

/**
 * One nand number of buffers register
 */
#define ONENAND_BUF_NUMS_BOOT(x)	(2 << ((x) & 0xff))
#define ONENAND_BUF_NUMS_DATA(x)	(2 << ((x) & 0xff) >> 8)

/**
 * One nand start address1 register
 */
#define ONENAND_ADDR1_FBA(x)		(((x) & 0x3ff))
#define ONENAND_ADDR1_DFS		0x8000

/**
 * One nand start address2 register
 */
#define ONENAND_ADDR2_DBS		0x8000

/**
 * One nand start address3 register
 */
#define ONENAND_ADDR3_FCBA(x)		(((x) & 0x3ff))

/**
 * One nand start address4 register
 */
#define ONENAND_ADDR4_FCPA(x)		(((x) & 0x3f) << 2)
#define ONENAND_ADDR4_FCSA(x)		(((x) & 0x3))

/**
 * One nand start address5 register
 */
#define ONENAND_ADDR5_FPC(x)		(((x) & 0x3f) << 2)

/**
 * One nand start address8 register
 */
#define ONENAND_ADDR8_FPA(x)		(((x) & 0x3f) << 2)
#define ONENAND_ADDR8_FSA(x)		(((x) & 0x3))

/**
 * One nand start buffer register
 */
#define ONENAND_BUF_START_BSA(x)	(((x) & 0xf) << 8)
#define ONENAND_BUF_START_BSC(x)	(((x) & 0x3))
#define ONENAND_BSA_DATA_RAM_STRT	0x8

/**
 * One nand system config1 register
 */
#define ONENAND_SYS_CFG1_RM_SYNC	0x8000
#define ONENAND_SYS_CFG1_BRL(x)		(((x) & 0x1) << 12)
#define ONENAND_SYS_CFG1_DEF_BRL	0x6000
#define ONENAND_SYS_CFG1_BL(x)		(((x) & 0x1) << 9)
#define ONENAND_SYS_CFG1_ECC		0x100
#define ONENAND_SYS_CFG1_RDY_POL	0x80
#define ONENAND_SYS_CFG1_INT_POL	0x40
#define ONENAND_SYS_CFG1_IOBE		0x20
#define ONENAND_SYS_CFG1_RDY_CONF	0x10
#define ONENAND_SYS_CFG1_HF		0x4
#define ONENAND_SYS_CFG1_WM		0x2
#define ONENAND_SYS_CFG1_BWPS		0x1

/**
 * One nand control status register
 */
#define ONENAND_CTR_STA_ONGO		0x8000
#define ONENAND_CTR_STA_LOCK		0x4000
#define ONENAND_CTR_STA_LOAD		0x2000
#define ONENAND_CTR_STA_PROG		0x1000
#define ONENAND_CTR_STA_ERASE		0x0800
#define ONENAND_CTR_STA_ERR		0x0400
#define ONENAND_CTR_STA_SUS		0x0200
#define ONENAND_CTR_STA_RSTB		0x0080
#define ONENAND_CTR_STA_OTP_L_STA	0x0040
#define ONENAND_CTR_STA_OTP_1ST_L_STA	0x0020
#define ONENAND_CTR_STA_PLANE1_PRE	0x0010
#define ONENAND_CTR_STA_PLANE1_CUR	0x0008
#define ONENAND_CTR_STA_PLANE2_PRE	0x0004
#define ONENAND_CTR_STA_PLANE2_CUR	0x0002
#define ONENAND_CTR_STA_TIMEOUT		0x0001

/**
 * One nand interrupt status register
 */
#define ONENAND_INT_STA_INT		0x8000
#define ONENAND_INT_STA_RI		0x0080
#define ONENAND_INT_STA_WI		0x0040
#define ONENAND_INT_STA_EI		0x0020
#define ONENAND_INT_STA_RSTI		0x0010

/**
 * One nand start block address register
 */
#define ONENAND_BLK_ADDR_SBA(x)		(((x) & 0x3ff))

/**
 * One nand write protect status register
 */
#define ONENAND_WP_STA_US		0x0004
#define ONENAND_WP_STA_LS		0x0002
#define ONENAND_WP_STA_LTS		0x0001

/**
 * One nand ECC status register
 */
#define ONENAND_ECC_STA_ERS0_ERR_NONE	0x0000
#define ONENAND_ECC_STA_ERS0_ERR_1BIT	0x0001
#define ONENAND_ECC_STA_ERS0_ERR_2BIT	0x0010

/**
 * One nand device command
 */
#define ONENAND_LOAD_SEC		0x0000
#define ONENAND_LOAD_SPARE		0x0013
#define ONENAND_PROG_SEC		0x0080
#define ONENAND_PROG_SPARE		0x001a
#define ONENAND_COPY_BACK		0x001b
#define ONENAND_PROG_2X			0x007d
#define ONENAND_PROG_2X_CACHE		0x007f
#define ONENAND_UNLOCK_BLK		0x0023
#define ONENAND_LOCK_BLK		0x002a
#define ONENAND_LOCK_TIGHT_BLK		0x002c
#define ONENAND_UNLOCK_ALL_BLK		0x0027
#define ONENAND_ERASE_VERIFY		0x0071
#define ONENAND_CACHE_READ		0x000e
#define ONENAND_FIN_CACHE_READ		0x000c
#define ONENAND_SYNC_BURST_READ		0x000a
#define ONENAND_ERASE_BLK		0x0094
#define ONENAND_MULTI_BLK_ERASE 	0x0095
#define ONENAND_ERASE_SUSPEND		0x00b0
#define ONENAND_ERASE_RESUME		0x0030
#define ONENAND_RESET_CORE		0x00f0
#define ONENAND_RESET_ONENAND		0x00f3
#define ONENAND_OTP_ACCESS		0x0065

/* The timeout values here are longer than device spec to guarateen host can
 * finish the request command. */

#define ONENAND_MAX_CMD_CYC	6
#define ONENAND_MAX_ADDR_CYC	6

/*******************************************************************/
/* Data poll mode						   */
/*******************************************************************/

/* Data# polling bit position */
#define ONENAND_DATA_POLL_TARGET_BIT	15

/*******************************************************************/
/* Read register						   */
/*******************************************************************/
#define ONENAND_READ_REG_CMD_CYC	0
#define ONENAND_READ_REG_DATA_CYC	0
#define ONENAND_READ_REG_TIMEOUT	500
#define ONENAND_READ_REG_TOTAL_CYC	0

/* Command cycles */
#define ONENAND_READ_REG_CYC1_ADDR	0x0	/* don't care */
#define ONENAND_READ_REG_CYC1_DATA	0x0

#define ONENAND_READ_REG_POLL_MODE	ONENAND_POLL_MODE_NONE
#define ONENAND_READ_REG_PROG_MODE	0x0

/*******************************************************************/
/* Write register						   */
/*******************************************************************/
#define ONENAND_WRITE_REG_CMD_CYC	0
#define ONENAND_WRITE_REG_DATA_CYC	1
#define ONENAND_WRITE_REG_TIMEOUT	500
#define ONENAND_WRITE_REG_TOTAL_CYC	0

/* Command cycles */
#define ONENAND_WRITE_REG_CYC1_ADDR	0x0	/* don't care */
#define ONENAND_WRITE_REG_CYC1_DATA	0x0

/* Poll & Prog mode */
#define ONENAND_WRITE_REG_POLL_MODE	ONENAND_POLL_MODE_NONE
#define ONENAND_WRITE_REG_PROG_MODE	ONENAND_PROG_MODE_NORMAL

/*******************************************************************/
/* Poll register						   */
/*******************************************************************/
#define ONENAND_POLL_REG_CMD_CYC	0
#define ONENAND_POLL_REG_DATA_CYC	0
#define ONENAND_POLL_REG_TIMEOUT	500
#define ONENAND_POLL_REG_TOTAL_CYC	0

/* Command cycles */
#define ONENAND_POLL_REG_CYC1_ADDR	0x0	/* don't care */
#define ONENAND_POLL_REG_CYC1_DATA	0x0

/* Poll & Prog mode */
#define ONENAND_POLL_REG_POLL_MODE	ONENAND_POLL_MODE_REG_POLL
#define ONENAND_POLL_REG_PROG_MODE	0x0

/*******************************************************************/
/* Read MAIN ASYNC						   */
/*******************************************************************/
#define ONENAND_READ_ASYNC_CMD_CYC	5
#define ONENAND_READ_ASYNC_DATA_CYC	1
#define ONENAND_READ_ASYNC_TIMEOUT	5000
#define ONENAND_READ_ASYNC_TOTAL_CYC	(ONENAND_READ_ASYNC_CMD_CYC + \
					 ONENAND_READ_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_READ_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_READ_ASYNC_CYC1_DATA	0x0
#define ONENAND_READ_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_READ_ASYNC_CYC2_DATA	0x0
#define ONENAND_READ_ASYNC_CYC3_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_READ_ASYNC_CYC3_DATA	0x0
#define ONENAND_READ_ASYNC_CYC4_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_READ_ASYNC_CYC4_DATA	0x0
#define ONENAND_READ_ASYNC_CYC5_ADDR	ONENAND_DEV_CTR_STA_REG
#define ONENAND_READ_ASYNC_CYC5_DATA	0x0

/* Poll & Prog mode */
#define ONENAND_READ_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_READ_ASYNC_PROG_MODE	0x0

/*******************************************************************/
/* Read SPARE ASYNC						   */
/*******************************************************************/
#define ONENAND_READ_S_ASYNC_CMD_CYC	5
#define ONENAND_READ_S_ASYNC_DATA_CYC	1
#define ONENAND_READ_S_ASYNC_TIMEOUT	5000
#define ONENAND_READ_S_ASYNC_TOTAL_CYC	(ONENAND_READ_ASYNC_CMD_CYC + \
					 ONENAND_READ_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_READ_S_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_READ_S_ASYNC_CYC1_DATA	0x0
#define ONENAND_READ_S_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_READ_S_ASYNC_CYC2_DATA	0x0
#define ONENAND_READ_S_ASYNC_CYC3_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_READ_S_ASYNC_CYC3_DATA	0x0
#define ONENAND_READ_S_ASYNC_CYC4_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_READ_S_ASYNC_CYC4_DATA	ONENAND_LOAD_SPARE
#define ONENAND_READ_S_ASYNC_CYC5_ADDR	ONENAND_DEV_CTR_STA_REG
#define ONENAND_READ_S_ASYNC_CYC5_DATA	0x0

/* Poll & Prog mode */
#define ONENAND_READ_S_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_READ_S_ASYNC_PROG_MODE	0x0

/*******************************************************************/
/* Read MAIN SYNC						   */
/*******************************************************************/
#define ONENAND_READ_SYNC_CMD_CYC	6
#define ONENAND_READ_SYNC_DATA_CYC	1
#define ONENAND_READ_SYNC_TIMEOUT	5000
#define ONENAND_READ_SYNC_TOTAL_CYC	(ONENAND_POLL_REG_CMD_CYC + \
					 ONENAND_POLL_REG_DATA_CYC)

/* Command cycles */
#define ONENAND_READ_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_READ_SYNC_CYC1_DATA	0x0
#define ONENAND_READ_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_READ_SYNC_CYC2_DATA	0x0
#define ONENAND_READ_SYNC_CYC3_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_READ_SYNC_CYC3_DATA	0x0
#define ONENAND_READ_SYNC_CYC4_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_READ_SYNC_CYC4_DATA	0x0
#define ONENAND_READ_SYNC_CYC5_ADDR	ONENAND_DEV_CTR_STA_REG
#define ONENAND_READ_SYNC_CYC5_DATA	0x0
#define ONENAND_READ_SYNC_CYC6_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_READ_SYNC_CYC6_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
					 ONENAND_SYS_CFG1_DEF_BRL | \
					 ONENAND_SYS_CFG1_RDY_POL | \
					 ONENAND_SYS_CFG1_INT_POL | \
					 ONENAND_SYS_CFG1_IOBE)
/* Poll & Prog mode */
#define ONENAND_READ_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_READ_SYNC_PROG_MODE	0x0

/*******************************************************************/
/* Read SPARE SYNC						   */
/*******************************************************************/
#define ONENAND_READ_S_SYNC_CMD_CYC	6
#define ONENAND_READ_S_SYNC_DATA_CYC	1
#define ONENAND_READ_S_SYNC_TIMEOUT	5000
#define ONENAND_READ_S_SYNC_TOTAL_CYC	(ONENAND_POLL_REG_CMD_CYC + \
					 ONENAND_POLL_REG_DATA_CYC)

/* Command cycles */
#define ONENAND_READ_S_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_READ_S_SYNC_CYC1_DATA	0x0
#define ONENAND_READ_S_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_READ_S_SYNC_CYC2_DATA	0x0
#define ONENAND_READ_S_SYNC_CYC3_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_READ_S_SYNC_CYC3_DATA	0x0
#define ONENAND_READ_S_SYNC_CYC4_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_READ_S_SYNC_CYC4_DATA	ONENAND_LOAD_SPARE
#define ONENAND_READ_S_SYNC_CYC5_ADDR	ONENAND_DEV_CTR_STA_REG
#define ONENAND_READ_S_SYNC_CYC5_DATA	0x0
#define ONENAND_READ_S_SYNC_CYC6_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_READ_S_SYNC_CYC6_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
					 ONENAND_SYS_CFG1_DEF_BRL | \
					 ONENAND_SYS_CFG1_RDY_POL | \
					 ONENAND_SYS_CFG1_INT_POL | \
					 ONENAND_SYS_CFG1_IOBE)
/* Poll & Prog mode */
#define ONENAND_READ_S_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_READ_S_SYNC_PROG_MODE	0x0

/*******************************************************************/
/* Program ASYNC						   */
/*******************************************************************/
#define ONENAND_PROG_ASYNC_CMD_CYC	5
#define ONENAND_PROG_ASYNC_DATA_CYC	1
#define ONENAND_PROG_ASYNC_TIMEOUT	5000
#define ONENAND_PROG_ASYNC_TOTAL_CYC	(ONENAND_PROG_ASYNC_CMD_CYC + \
					 ONENAND_PROG_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_ASYNC_CYC1_DATA	0x0
#define ONENAND_PROG_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_ASYNC_CYC2_DATA	0x0
#define ONENAND_PROG_ASYNC_CYC3_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_ASYNC_CYC3_DATA	0x0
#define ONENAND_PROG_ASYNC_CYC4_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_ASYNC_CYC4_DATA	0x0
#define ONENAND_PROG_ASYNC_CYC5_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_ASYNC_CYC5_DATA	ONENAND_PROG_SEC

/* Poll & Prog mode */
#define ONENAND_PROG_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_ASYNC_PROG_MODE	ONENAND_PROG_MODE_NORMAL

/*******************************************************************/
/* Program SPARE ASYNC						   */
/*******************************************************************/
#define ONENAND_PROG_S_ASYNC_CMD_CYC	5
#define ONENAND_PROG_S_ASYNC_DATA_CYC	1
#define ONENAND_PROG_S_ASYNC_TIMEOUT	5000
#define ONENAND_PROG_S_ASYNC_TOTAL_CYC	(ONENAND_PROG_ASYNC_CMD_CYC + \
					 ONENAND_PROG_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_S_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_S_ASYNC_CYC1_DATA	0x0
#define ONENAND_PROG_S_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_S_ASYNC_CYC2_DATA	0x0
#define ONENAND_PROG_S_ASYNC_CYC3_ADDR	ONENAND_SPARE_RAM_START
#define ONENAND_PROG_S_ASYNC_CYC3_DATA	0x0
#define ONENAND_PROG_S_ASYNC_CYC4_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_S_ASYNC_CYC4_DATA	0x0
#define ONENAND_PROG_S_ASYNC_CYC5_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_S_ASYNC_CYC5_DATA	ONENAND_PROG_SPARE

/* Poll & Prog mode */
#define ONENAND_PROG_S_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_S_ASYNC_PROG_MODE	ONENAND_PROG_MODE_NORMAL

/*******************************************************************/
/* Program ASYNC DUAL						   */
/*******************************************************************/
#define ONENAND_PROG_DUAL_ASYNC_CMD_CYC		6
#define ONENAND_PROG_DUAL_ASYNC_DATA_CYC	1
#define ONENAND_PROG_DUAL_ASYNC_TIMEOUT		5000
#define ONENAND_PROG_DUAL_ASYNC_TOTAL_CYC	(ONENAND_PROG_DUAL_ASYNC_CMD_CYC + \
					 	 ONENAND_PROG_DUAL_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_DUAL_ASYNC_CYC1_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_DUAL_ASYNC_CYC1_DATA	0x0
#define ONENAND_PROG_DUAL_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_DUAL_ASYNC_CYC2_DATA	0x0
#define ONENAND_PROG_DUAL_ASYNC_CYC3_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_DUAL_ASYNC_CYC3_DATA	0x0
#define ONENAND_PROG_DUAL_ASYNC_CYC4_ADDR	ONENAND_DEV_BUF_START_REG
#define ONENAND_PROG_DUAL_ASYNC_CYC4_DATA	ONENAND_BUF_START_BSA(ONENAND_BSA_DATA_RAM_STRT)
#define ONENAND_PROG_DUAL_ASYNC_CYC5_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_DUAL_ASYNC_CYC5_DATA	0x0
#define ONENAND_PROG_DUAL_ASYNC_CYC6_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_DUAL_ASYNC_CYC6_DATA	ONENAND_PROG_SEC

/* Poll & Prog mode */
#define ONENAND_PROG_DUAL_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_DUAL_ASYNC_PROG_MODE	ONENAND_PROG_MODE_DUAL

/*******************************************************************/
/* Program SYNC							   */
/*******************************************************************/
#define ONENAND_PROG_SYNC_CMD_CYC	6
#define ONENAND_PROG_SYNC_DATA_CYC	1
#define ONENAND_PROG_SYNC_TIMEOUT	5000
#define ONENAND_PROG_SYNC_TOTAL_CYC	(ONENAND_PROG_SYNC_CMD_CYC + \
					 ONENAND_PROG_SYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_SYNC_CYC1_DATA	0x0
#define ONENAND_PROG_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_SYNC_CYC2_DATA	0x0
#define ONENAND_PROG_SYNC_CYC3_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_PROG_SYNC_CYC3_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
			   		 ONENAND_SYS_CFG1_DEF_BRL | \
			   		 ONENAND_SYS_CFG1_RDY_POL | \
			   		 ONENAND_SYS_CFG1_INT_POL | \
			   		 ONENAND_SYS_CFG1_IOBE	  | \
			   		 ONENAND_SYS_CFG1_WM)

#define ONENAND_PROG_SYNC_CYC4_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_SYNC_CYC4_DATA	0x0
#define ONENAND_PROG_SYNC_CYC5_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_SYNC_CYC5_DATA	0x0
#define ONENAND_PROG_SYNC_CYC6_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_SYNC_CYC6_DATA	ONENAND_PROG_SEC

/* Poll & Prog mode */
#define ONENAND_PROG_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_SYNC_PROG_MODE	ONENAND_PROG_MODE_NORMAL

/*******************************************************************/
/* Program SYNC	SPARE						   */
/*******************************************************************/
#define ONENAND_PROG_S_SYNC_CMD_CYC	6
#define ONENAND_PROG_S_SYNC_DATA_CYC	1
#define ONENAND_PROG_S_SYNC_TIMEOUT	5000
#define ONENAND_PROG_S_SYNC_TOTAL_CYC	(ONENAND_PROG_SYNC_CMD_CYC + \
					 ONENAND_PROG_SYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_S_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_S_SYNC_CYC1_DATA	0x0
#define ONENAND_PROG_S_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_S_SYNC_CYC2_DATA	0x0
#define ONENAND_PROG_S_SYNC_CYC3_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_PROG_S_SYNC_CYC3_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
			   		 ONENAND_SYS_CFG1_DEF_BRL | \
			   		 ONENAND_SYS_CFG1_RDY_POL | \
			   		 ONENAND_SYS_CFG1_INT_POL | \
			   		 ONENAND_SYS_CFG1_IOBE	  | \
			   		 ONENAND_SYS_CFG1_WM)

#define ONENAND_PROG_S_SYNC_CYC4_ADDR	ONENAND_SPARE_RAM_START
#define ONENAND_PROG_S_SYNC_CYC4_DATA	0x0
#define ONENAND_PROG_S_SYNC_CYC5_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_S_SYNC_CYC5_DATA	0x0
#define ONENAND_PROG_S_SYNC_CYC6_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_S_SYNC_CYC6_DATA	ONENAND_PROG_SPARE

/* Poll & Prog mode */
#define ONENAND_PROG_S_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_S_SYNC_PROG_MODE	ONENAND_PROG_MODE_NORMAL

/*******************************************************************/
/* Program 2X ASYNC						   */
/*******************************************************************/
#define ONENAND_PROG_2X_ASYNC_CMD_CYC	5
#define ONENAND_PROG_2X_ASYNC_DATA_CYC	1
#define ONENAND_PROG_2X_ASYNC_TIMEOUT	5000
#define ONENAND_PROG_2X_ASYNC_TOTAL_CYC	(ONENAND_PROG_2X_ASYNC_CMD_CYC + \
					 ONENAND_PROG_2X_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_2X_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_2X_ASYNC_CYC1_DATA	0x0
#define ONENAND_PROG_2X_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_2X_ASYNC_CYC2_DATA	0x0
#define ONENAND_PROG_2X_ASYNC_CYC3_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_2X_ASYNC_CYC3_DATA	0x0
#define ONENAND_PROG_2X_ASYNC_CYC4_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_2X_ASYNC_CYC4_DATA	0x0
#define ONENAND_PROG_2X_ASYNC_CYC5_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_2X_ASYNC_CYC5_DATA	ONENAND_PROG_2X

/* Poll & Prog mode */
#define ONENAND_PROG_2X_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_2X_ASYNC_PROG_MODE	ONENAND_PROG_MODE_2X

/*******************************************************************/
/* Program 2X SYNC						   */
/*******************************************************************/
#define ONENAND_PROG_2X_SYNC_CMD_CYC	6
#define ONENAND_PROG_2X_SYNC_DATA_CYC	1
#define ONENAND_PROG_2X_SYNC_TIMEOUT	5000
#define ONENAND_PROG_2X_SYNC_TOTAL_CYC	(ONENAND_PROG_SYNC_CMD_CYC + \
					 ONENAND_PROG_SYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_2X_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_2X_SYNC_CYC1_DATA	0x0
#define ONENAND_PROG_2X_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_2X_SYNC_CYC2_DATA	0x0
#define ONENAND_PROG_2X_SYNC_CYC3_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_PROG_2X_SYNC_CYC3_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
			   		 ONENAND_SYS_CFG1_DEF_BRL | \
			   		 ONENAND_SYS_CFG1_RDY_POL | \
			   		 ONENAND_SYS_CFG1_INT_POL | \
			   		 ONENAND_SYS_CFG1_IOBE	  | \
			   		 ONENAND_SYS_CFG1_WM)

#define ONENAND_PROG_2X_SYNC_CYC4_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_2X_SYNC_CYC4_DATA	0x0
#define ONENAND_PROG_2X_SYNC_CYC5_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_2X_SYNC_CYC5_DATA	0x0
#define ONENAND_PROG_2X_SYNC_CYC6_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_2X_SYNC_CYC6_DATA	ONENAND_PROG_2X

/* Poll & Prog mode */
#define ONENAND_PROG_2X_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_2X_SYNC_PROG_MODE	ONENAND_PROG_MODE_2X

/*******************************************************************/
/* Program 2X cache ASYNC					   */
/*******************************************************************/
#define ONENAND_PROG_2X_CACHE_ASYNC_CMD_CYC	5
#define ONENAND_PROG_2X_CACHE_ASYNC_DATA_CYC	1
#define ONENAND_PROG_2X_CACHE_ASYNC_TIMEOUT	5000
#define ONENAND_PROG_2X_CACHE_ASYNC_TOTAL_CYC	(ONENAND_PROG_2X_ASYNC_CMD_CYC \
					       + ONENAND_PROG_2X_ASYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC1_DATA	0x0
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC2_DATA	0x0
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC3_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC3_DATA	0x0
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC4_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC4_DATA	0x0
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC5_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_2X_CACHE_ASYNC_CYC5_DATA	ONENAND_PROG_2X_CACHE

/* Poll & Prog mode */
#define ONENAND_PROG_2X_CACHE_ASYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_2X_CACHE_ASYNC_PROG_MODE	ONENAND_PROG_MODE_2X_CACHE

/*******************************************************************/
/* Program 2X cache SYNC					   */
/*******************************************************************/
#define ONENAND_PROG_2X_CACHE_SYNC_CMD_CYC	6
#define ONENAND_PROG_2X_CACHE_SYNC_DATA_CYC	1
#define ONENAND_PROG_2X_CACHE_SYNC_TIMEOUT	5000
#define ONENAND_PROG_2X_CACHE_SYNC_TOTAL_CYC	(ONENAND_PROG_SYNC_CMD_CYC + \
						 ONENAND_PROG_SYNC_DATA_CYC)

/* Command cycles */
#define ONENAND_PROG_2X_CACHE_SYNC_CYC1_ADDR	ONENAND_DEV_ADDR1_REG
#define ONENAND_PROG_2X_CACHE_SYNC_CYC1_DATA	0x0
#define ONENAND_PROG_2X_CACHE_SYNC_CYC2_ADDR	ONENAND_DEV_ADDR8_REG
#define ONENAND_PROG_2X_CACHE_SYNC_CYC2_DATA	0x0
#define ONENAND_PROG_2X_CACHE_SYNC_CYC3_ADDR	ONENAND_DEV_SYS_CFG1_REG
#define ONENAND_PROG_2X_CACHE_SYNC_CYC3_DATA	(ONENAND_SYS_CFG1_RM_SYNC | \
						 ONENAND_SYS_CFG1_DEF_BRL | \
			   		 	 ONENAND_SYS_CFG1_RDY_POL | \
			   		 	 ONENAND_SYS_CFG1_INT_POL | \
			   		 	 ONENAND_SYS_CFG1_IOBE	  | \
			   		 	 ONENAND_SYS_CFG1_WM)

#define ONENAND_PROG_2X_CACHE_SYNC_CYC4_ADDR	ONENAND_DAT_RAM_START
#define ONENAND_PROG_2X_CACHE_SYNC_CYC4_DATA	0x0
#define ONENAND_PROG_2X_CACHE_SYNC_CYC5_ADDR	ONENAND_DEV_INT_STA_REG
#define ONENAND_PROG_2X_CACHE_SYNC_CYC5_DATA	0x0
#define ONENAND_PROG_2X_CACHE_SYNC_CYC6_ADDR	ONENAND_DEV_CMD_REG
#define ONENAND_PROG_2X_CACHE_SYNC_CYC6_DATA	ONENAND_PROG_2X_CACHE

/* Poll & Prog mode */
#define ONENAND_PROG_2X_CACHE_SYNC_POLL_MODE	ONENAND_POLL_MODE_REG_INT_POLL
#define ONENAND_PROG_2X_CACHE_SYNC_PROG_MODE	ONENAND_PROG_MODE_2X_CACHE

#endif
