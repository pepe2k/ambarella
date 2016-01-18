/**
 * @file system/include/flash/snor/command.h
 *
 * History:
 *    2009/08/17 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SNOR_COMMAND_H__
#define __SNOR_COMMAND_H__

/**
 * Command table, x16
 */

/* The timeout values here are longer than device spec to guarateen host can
 * finish the request command. */

#define SNOR_MAX_CMD_CYC	6
#define SNOR_MAX_ADDR_CYC	6

/*******************************************************************/
/* Data poll mode						   */
/*******************************************************************/
/* The bit value when data# polling done */
#define SNOR_DATA_POLL_MODE		SNOR_DATA_POLL_MODE_EQ_DATA

/* Data# polling bit position */
#define SNOR_DATA_POLL_TARGET_BIT	7

/* Exceeded timing limits */
#define SNOR_DATA_POLL_FAIL_BIT		5

/* The bit polarity when exceeded timing limits */
#define SNOR_DATA_POLL_FAIL_POLARITY	1

/*******************************************************************/
/* reset 							   */
/*******************************************************************/
#define SNOR_RESET_CMD_CYC		0
#define SNOR_RESET_DATA_CYC		1
#define SNOR_RESET_TIMEOUT		500
#define SNOR_RESET_TOTAL_CYC		(SNOR_RESET_CMD_CYC + \
					 SNOR_RESET_DATA_CYC)

/* Data cycles */
#define SNOR_RESET_CYC1_ADDR		0x0	/* don't care */
#define SNOR_RESET_CYC1_DATA		0xf0

/*******************************************************************/
/* autoselect */
/*******************************************************************/
#define SNOR_AUTOSEL_CMD_CYC		3
#define SNOR_AUTOSEL_DATA_CYC		1
#define SNOR_AUTOSEL_TIMEOUT		500
#define SNOR_AUTOSEL_TOTAL_CYC		(SNOR_AUTOSEL_CMD_CYC + \
					 SNOR_AUTOSEL_DATA_CYC)

/* Command cycles */
#define SNOR_AUTOSEL_CYC1_ADDR		0x555
#define SNOR_AUTOSEL_CYC1_DATA		0xaa
#define SNOR_AUTOSEL_CYC2_ADDR		0x2aa
#define SNOR_AUTOSEL_CYC2_DATA		0x55
#define SNOR_AUTOSEL_CYC3_ADDR		0x555
#define SNOR_AUTOSEL_CYC3_DATA		0x90

/* Data cycles */
#define SNOR_AUTOSEL_MANU_ID_ADDR	0x0
#define SNOR_AUTOSEL_DEV_ID1_ADDR	0x1
#define SNOR_AUTOSEL_DEV_ID2_ADDR	0xe
#define SNOR_AUTOSEL_DEV_ID3_ADDR	0xf

#define SNOR_AUTOSEL_BLOCK_PROT_VRFY	0x2
#define SNOR_AUTOSEL_SECURE_DEV_VRFY	0x3

/*******************************************************************/
/* cfi query 							   */
/*******************************************************************/
#define SNOR_CFI_QUERY_CMD_CYC		1
#define SNOR_CFI_QUERY_DATA_CYC		0
#define SNOR_CFI_QUERY_TIMEOUT		500
#define SNOR_CFI_QUERY_TOTAL_CYC	(SNOR_CFI_QUERY_CMD_CYC + \
					 SNOR_CFI_QUERY_DATA_CYC)

/* Command cycles */
#define SNOR_CFI_QUERY_CYC1_ADDR	0x55
#define SNOR_CFI_QUERY_CYC1_DATA	0x98

/*******************************************************************/
/* read 							   */
/*******************************************************************/
#define SNOR_READ_CMD_CYC		0
#define SNOR_READ_DATA_CYC		1
#define SNOR_READ_TIMEOUT		5000
#define SNOR_READ_TOTAL_CYC		(SNOR_READ_CMD_CYC + \
					 SNOR_READ_DATA_CYC)

/*******************************************************************/
/* program 							   */
/*******************************************************************/
#define SNOR_PROG_CMD_CYC		3
#define SNOR_PROG_DATA_CYC		1
#define SNOR_PROG_TIMEOUT		5000
#define SNOR_PROG_TOTAL_CYC		(SNOR_PROG_CMD_CYC + \
					 SNOR_PROG_DATA_CYC)

/* Command cycles */
#define SNOR_PROG_CYC1_ADDR		0x555
#define SNOR_PROG_CYC1_DATA		0xaa
#define SNOR_PROG_CYC2_ADDR		0x2aa
#define SNOR_PROG_CYC2_DATA		0x55
#define SNOR_PROG_CYC3_ADDR		0x555
#define SNOR_PROG_CYC3_DATA		0xa0

/*******************************************************************/
/* unlock bypass enter 						   */
/*******************************************************************/
#define SNOR_UNLOCK_BYPASS_CMD_CYC	3
#define SNOR_UNLOCK_BYPASS_DATA_CYC	0
#define SNOR_UNLOCK_BYPASS_TIMEOUT	500
#define SNOR_UNLOCK_BYPASS_TOTAL_CYC	(SNOR_UNLOCK_BYPASS_CMD_CYC + \
					 SNOR_UNLOCK_BYPASS_DATA_CYC)

/* Command cycles */
#define SNOR_UNLOCK_BYPASS_CYC1_ADDR	0x555
#define SNOR_UNLOCK_BYPASS_CYC1_DATA	0xaa
#define SNOR_UNLOCK_BYPASS_CYC2_ADDR	0x2aa
#define SNOR_UNLOCK_BYPASS_CYC2_DATA	0x55
#define SNOR_UNLOCK_BYPASS_CYC3_ADDR	0x555
#define SNOR_UNLOCK_BYPASS_CYC3_DATA	0x20

/*******************************************************************/
/* unlock bypass program					   */
/*******************************************************************/
#define SNOR_UNLOCK_PROG_CMD_CYC	1
#define SNOR_UNLOCK_PROG_DATA_CYC	1
#define SNOR_UNLOCK_PROG_TIMEOUT	5000
#define SNOR_UNLOCK_PROG_TOTAL_CYC	(SNOR_UNLOCK_PROG_CMD_CYC + \
					 SNOR_UNLOCK_PROG_DATA_CYC)

/* Command cycles */
#define SNOR_UNLOCK_PROG_CYC1_DATA	0xa0

/*******************************************************************/
/* unlock bypass sector erase 					   */
/*******************************************************************/
#define SNOR_UNLOCK_BLOCK_ERASE_CMD_CYC	1
#define SNOR_UNLOCK_BLOCK_ERASE_DATA_CYC	1
#define SNOR_UNLOCK_BLOCK_ERASE_TIMEOUT	5000
#define SNOR_UNLOCK_BLOCK_ERASE_TOTAL_CYC	\
				(SNOR_UNLOCK_BLOCK_ERASE_CMD_CYC + \
				 SNOR_UNLOCK_BLOCK_ERASE_DATA_CYC)

/* Command cycles */
#define SNOR_UNLOCK_BLOCK_ERASE_CYC1_ADDR	0x0	/* don't care */
#define SNOR_UNLOCK_BLOCK_ERASE_CYC1_DATA	0x80

/* Data cycles */
#define SNOR_UNLOCK_BLOCK_ERASE_CYC2_DATA	0x30

/*******************************************************************/
/* unlock bypass chip erase 					   */
/*******************************************************************/
#define SNOR_UNLOCK_CHIP_ERASE_CMD_CYC		2
#define SNOR_UNLOCK_CHIP_ERASE_DATA_CYC		0
#define SNOR_UNLOCK_CHIP_ERASE_TIMEOUT		-1
#define SNOR_UNLOCK_CHIP_ERASE_TOTAL_CYC	\
				(SNOR_UNLOCK_CHIP_ERASE_CMD_CYC + \
				 SNOR_UNLOCK_CHIP_ERASE_DATA_CYC)

/* Command cycles */
#define SNOR_UNLOCK_CHIP_ERASE_CYC1_ADDR	0x0	/* don't care */
#define SNOR_UNLOCK_CHIP_ERASE_CYC1_DATA	0x80
#define SNOR_UNLOCK_CHIP_ERASE_CYC2_ADDR	0x0	/* don't care */
#define SNOR_UNLOCK_CHIP_ERASE_CYC2_DATA	0x10

/*******************************************************************/
/* unlock bypass exit				   		   */
/*******************************************************************/
#define SNOR_UNLOCK_RESET_CMD_CYC	2
#define SNOR_UNLOCK_RESET_DATA_CYC	0
#define SNOR_UNLOCK_RESET_TIMEOUT	500
#define SNOR_UNLOCK_RESET_TOTAL_CYC	(SNOR_UNLOCK_RESET_CMD_CYC + \
					 SNOR_UNLOCK_RESET_DATA_CYC)

/* Command cycles */
#define SNOR_UNLOCK_RESET_CYC1_ADDR	0x0		/* don't care */
#define SNOR_UNLOCK_RESET_CYC1_DATA	0x90
#define SNOR_UNLOCK_RESET_CYC2_ADDR	0x0		/* don't care */
#define SNOR_UNLOCK_RESET_CYC2_DATA	0x00

/*******************************************************************/
/* chip erase 							   */
/*******************************************************************/
#define SNOR_CHIP_ERASE_CMD_CYC		6
#define SNOR_CHIP_ERASE_DATA_CYC	0
#define SNOR_CHIP_ERASE_TIMEOUT		-1
#define SNOR_CHIP_ERASE_TOTAL_CYC	(SNOR_CHIP_ERASE_CMD_CYC + \
					 SNOR_CHIP_ERASE_DATA_CYC)

/* Command cycles */
#define SNOR_CHIP_ERASE_CYC1_ADDR	0x555
#define SNOR_CHIP_ERASE_CYC1_DATA	0xaa
#define SNOR_CHIP_ERASE_CYC2_ADDR	0x2aa
#define SNOR_CHIP_ERASE_CYC2_DATA	0x55
#define SNOR_CHIP_ERASE_CYC3_ADDR	0x555
#define SNOR_CHIP_ERASE_CYC3_DATA	0x80
#define SNOR_CHIP_ERASE_CYC4_ADDR	0x555
#define SNOR_CHIP_ERASE_CYC4_DATA	0xaa
#define SNOR_CHIP_ERASE_CYC5_ADDR	0x2aa
#define SNOR_CHIP_ERASE_CYC5_DATA	0x55
#define SNOR_CHIP_ERASE_CYC6_ADDR	0x555
#define SNOR_CHIP_ERASE_CYC6_DATA	0x10

/*******************************************************************/
/* Sector erase 						   */
/*******************************************************************/
#define SNOR_BLOCK_ERASE_CMD_CYC	5
#define SNOR_BLOCK_ERASE_DATA_CYC	1
#define SNOR_BLOCK_ERASE_TIMEOUT	5000
#define SNOR_BLOCK_ERASE_TOTAL_CYC	(SNOR_BLOCK_ERASE_CMD_CYC + \
					 SNOR_BLOCK_ERASE_DATA_CYC)

/* Command cycles */
#define SNOR_BLOCK_ERASE_CYC1_ADDR	0x555
#define SNOR_BLOCK_ERASE_CYC1_DATA	0xaa
#define SNOR_BLOCK_ERASE_CYC2_ADDR	0x2aa
#define SNOR_BLOCK_ERASE_CYC2_DATA	0x55
#define SNOR_BLOCK_ERASE_CYC3_ADDR	0x555
#define SNOR_BLOCK_ERASE_CYC3_DATA	0x80
#define SNOR_BLOCK_ERASE_CYC4_ADDR	0x555
#define SNOR_BLOCK_ERASE_CYC4_DATA	0xaa
#define SNOR_BLOCK_ERASE_CYC5_ADDR	0x2aa
#define SNOR_BLOCK_ERASE_CYC5_DATA	0x55

/* Data cycles */
#define SNOR_BLOCK_ERASE_CYC6_DATA	0x30

/*******************************************************************/
/* Block protection 						   */
/*******************************************************************/
#define SNOR_BLOCK_PROTECT_CMD_CYC	2
#define SNOR_BLOCK_PROTECT_DATA_CYC	1
#define SNOR_BLOCK_PROTECT_TIMEOUT	1000
#define SNOR_BLOCK_PROTECT_TOTAL_CYC	(SNOR_BLOCK_PROTECT_CMD_CYC + \
					 SNOR_BLOCK_PROTECT_DATA_CYC)

/* Command cycles */
#define SNOR_BLOCK_PROTECT_CYC1_ADDR	0x0	/* don't care */
#define SNOR_BLOCK_PROTECT_CYC1_DATA	0x60
#define SNOR_BLOCK_PROTECT_CYC2_ADDR	0x0	/* don't care */
#define SNOR_BLOCK_PROTECT_CYC2_DATA	0x60

/* Data cycles */
#define SNOR_BLOCK_PROTECT_CYC3_ADDR(x)		(((x) & (~0x7f)) | 0x2)
#define SNOR_BLOCK_UNPROTECT_CYC3_ADDR(x)	(((x) & (~0x7f)) | 0x42)
#define SNOR_BLOCK_PROTECT_CYC3_DATA		0x60

/*******************************************************************/
/* Block group protection 					   */
/*******************************************************************/
#define SNOR_BGROUP_PROTECT_CMD_CYC	2
#define SNOR_BGROUP_PROTECT_DATA_CYC	1
#define SNOR_BGROUP_PROTECT_TIMEOUT	1000
#define SNOR_BGROUP_PROTECT_TOTAL_CYC	(SNOR_BGROUP_PROTECT_CMD_CYC + \
					 SNOR_BGROUP_PROTECT_DATA_CYC)

/* Command cycles */
#define SNOR_BGROUP_PROTECT_CYC1_ADDR	0x0	/* don't care */
#define SNOR_BGROUP_PROTECT_CYC1_DATA	0x60
#define SNOR_BGROUP_PROTECT_CYC2_ADDR	0x0	/* don't care */
#define SNOR_BGROUP_PROTECT_CYC2_DATA	0x60

/* Data cycles */
#define SNOR_BGROUP_PROTECT_CYC3_ADDR(x)	(((x) & (~0x7f)) | 0x2)
#define SNOR_BGROUP_UNPROTECT_CYC3_ADDR(x)	(((x) & (~0x7f)) | 0x42)
#define SNOR_BGROUP_PROTECT_CYC3_DATA		0x40

/*******************************************************************/
/* Set burst mode configuration					   */
/*******************************************************************/
#define SNOR_SET_BURST_CFG_CMD_CYC	2
#define SNOR_SET_BURST_CFG_DATA_CYC	1
#define SNOR_SET_BURST_CFG_TIMEOUT	1000
#define SNOR_SET_BURST_CFG_TOTAL_CYC	(SNOR_SET_BURST_CFG_CMD_CYC + \
					 SNOR_SET_BURST_CFG_DATA_CYC)

/* Command cycles */
#define SNOR_SET_BURST_CFG_CYC1_ADDR	0x555	/* don't care */
#define SNOR_SET_BURST_CFG_CYC1_DATA	0xaa
#define SNOR_SET_BURST_CFG_CYC2_ADDR	0x2aa	/* don't care */
#define SNOR_SET_BURST_CFG_CYC2_DATA	0x55

/* Data cycles */
#define SNOR_SET_BURST_CFG_CYC3_ADDR(x)		(((x) & (0x7f000)) | 0x555)
#define SNOR_SET_BURST_CFG_CYC3_DATA		0xc0

#endif
