/**
 * @file system/include/sdmmc.h
 *
 * SD/MMC/SDIO protocol stack from Ambarella
 *
 * History:
 *    2004/11/04 - [Charles Chiou] created file
 *    2007/01/24 - [Charles Chiou] merged from multiple headers into one
 *    2008/03/31 - [Charles Chiou] added Samsung MoviNAND support
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SDMMC_H__
#define __SDMMC_H__

#define SDMMC_SUPPORT_SDCIS	0

#include <basedef.h>
#if (SDMMC_SUPPORT_SDCIS == 1)
#include <pcmcia/cistpl.h>
#endif
#include <ambhw/sd.h>
#include <bsp.h>

__BEGIN_C_PROTO__

/**
 * @defgroup sdmmc SD/MMC Protocol Stack
 *
 * The SD/MMC Protocol Stack module provides an implementation of the SD
 * Memoery, SD IO, MMC protocols. It translates the commands into host
 * controller specific commands and work with upper-level drivers such as
 * block devices (for file-system) and NICs, etc.
 */
/** @addtogroup sdmmc */
/*@{*/

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/* Protocol definitions                                                     */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

/*      Command            name                  type argument           resp*/
/* class 1 */
#define	SDMMC_CMD0   0  /* GO_IDLE_STATE         bc   [31:0]             -   */
#define SDMMC_CMD1   1  /* SEND_OP_COND          bcr  [31:0] OCR         R3  */
#define SDMMC_CMD2   2  /* ALL_SEND_CID          bcr                     R2  */
#define SDMMC_CMD3   3  /* SET_RELATIVE_ADDR     ac   [31:16] RCA        R1  */
#define SDMMC_CMD4   4  /* SET_DSR               bc   [31:16] RCA            */
#define SDMMC_CMD5   5
#define SDMMC_CMD6   6  /* SWITCH                ac                      R1b */
#define SDMMC_CMD7   7  /* SELECT_CARD           ac   [31:16] RCA        R1  */
#define SDMMC_CMD8   8  /* SEND_EXT_CSD          adtc                    R1  */
#define SDMMC_CMD9   9  /* SEND_CSD              ac   [31:16] RCA        R2  */
#define SDMMC_CMD10 10  /* SEND_CID              ac   [31:16] RCA        R2  */
#define SDMMC_CMD11 11  /* READ_DAT_UNTIL_STOP   adtc [31:0] dadr        R1  */
#define SDMMC_CMD12 12  /* STOP_TRANSMISSION     ac                      R1b */
#define SDMMC_CMD13 13  /* SEND_STATUS	         ac   [31:16] RCA        R1  */
#define SDMMC_CMD14 14  /* BUSTEST_R             adtc                    R1  */
#define SDMMC_CMD15 15  /* GO_INACTIVE_STATE     ac   [31:16] RCA            */
/* class 2 */
#define SDMMC_CMD16 16  /* SET_BLOCKLEN          ac   [31:0] block len   R1  */
#define SDMMC_CMD17 17  /* READ_SINGLE_BLOCK     adtc [31:0] data addr   R1  */
#define SDMMC_CMD18 18  /* READ_MULTIPLE_BLOCK   adtc [31:0] data addr   R1  */
#define SDMMC_CMD19 19  /* BUSTEST_W             adtc                    R1  */
/* class 3 */
#define SDMMC_CMD20 20  /* WRITE_DAT_UNTIL_STOP  adtc [31:0] data addr   R1  */
/* class 4 */
#define SDMMC_CMD23 23  /* SET_BLOCK_COUNT       adtc [31:0] data addr   R1  */
#define SDMMC_CMD24 24  /* WRITE_BLOCK           adtc [31:0] data addr   R1  */
#define SDMMC_CMD25 25  /* WRITE_MULTIPLE_BLOCK  adtc                    R1  */
#define SDMMC_CMD26 26  /* PROGRAM_CID           adtc                    R1  */
#define SDMMC_CMD27 27  /* PROGRAM_CSD           adtc                    R1  */
/* class 6 */
#define SDMMC_CMD28 28  /* SET_WRITE_PROT        ac   [31:0] data addr   R1b */
#define SDMMC_CMD29 29  /* CLR_WRITE_PROT        ac   [31:0] data addr   R1b */
#define SDMMC_CMD30 30  /* SEND_WRITE_PROT       adtc [31:0] wpdata addr R1  */
/* class 5 */
#define SDMMC_CMD32 32  /* ERASE_WR_BLK_START    ac   [31:0] data addr   R1  */
#define SDMMC_CMD33 33  /* ERASE_WR_BLK_END      ac   [31:0] data addr   R1  */
#define SDMMC_CMD35 35  /* ERASE_GROUP_START     ac   [31:0] data addr   R1  */
#define SDMMC_CMD36 36  /* ERASE_GROUP_END       ac   [31:0] data addr   R1  */
/* #define SDMMC_CMD37 37 */
#define SDMMC_CMD38 38  /* ERASE                 ac                      R1b */
/* class 9 */
#define SDMMC_CMD39 39  /* FAST_IO               ac   <Complex>          R4  */
#define SDMMC_CMD40 40  /* GO_IRQ_STATE          bcr                     R5  */
/* class 7 */
#define SDMMC_CMD42 42  /* LOCK_UNLOCK           adtc                    R1b */
/* class 8 */
#define SDMMC_CMD55 55  /* APP_CMD               ac   [31:16] RCA        R1  */
#define SDMMC_CMD56 56  /* GEN_CMD               adtc [0] RD/WR          R1b */
/* --- */
#define SDMMC_CMD52 52  /* IO_RW_DIRECT */
#define SDMMC_CMD53 53  /* IO_RW_EXTENDED */

/**
 * SD Memory Card Application Specific Commands.
 */
/*      Command            name                  type argument           resp*/
#define SDMMC_ACMD6   6 /* SET_BUS_WIDTH         ac   [1:0] bus width    R1  */
#define SDMMC_ACMD13 13 /* SD_STATUS             adtc                    R1  */
#define SDMMC_ACMD22 22 /* SEND_NUM_WR_BLOCKS    adtc                    R1  */
#define SDMMC_ACMD23 23 /* SET_WR_BLK_ERASE_COUNT ac                     R1  */
#define SDMMC_ACMD41 41 /* SD_SEND_OP_COND       bcr                     R3  */
#define SDMMC_ACMD42 42 /* SET_CLR_CARD_DETECT   ac                      R1  */
#define SDMMC_ACMD51 51 /* SEND_SCR              adtc                    R1  */

/*
  SD/MMC status in R1
  Type
  	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
  	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		0x80000000	/* er, c */
#define R1_ADDRESS_ERROR	0x40000000	/* erx, c */
#define R1_BLOCK_LEN_ERROR	0x20000000	/* er, c */
#define R1_ERASE_SEQ_ERROR      0x10000000	/* er, c */
#define R1_ERASE_PARAM		0x08000000	/* ex, c */
#define R1_WP_VIOLATION		0x04000000	/* erx, c */
#define R1_CARD_IS_LOCKED	0x02000000	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	0x01000000	/* erx, c */
#define R1_COM_CRC_ERROR	0x00800000	/* er, b */
#define R1_ILLEGAL_COMMAND	0x00400000	/* er, b */
#define R1_CARD_ECC_FAILED	0x00200000	/* ex, c */
#define R1_CC_ERROR		0x00100000	/* erx, c */
#define R1_ERROR		0x00080000	/* erx, c */
#define R1_UNDERRUN		0x00040000	/* ex, c */
#define R1_OVERRUN		0x00020000	/* ex, c */
#define R1_CID_CSD_OVERWRITE	0x00010000	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	0x00008000	/* sx, c */
#define R1_CARD_ECC_DISABLED	0x00004000	/* sx, a */
#define R1_ERASE_RESET		0x00002000	/* sr, c */
#define R1_STATUS(x)            (x & 0xffffe000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001e00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	0x00000100	/* sx, a */
#define R1_APP_CMD		0x00000080	/* sr, c */

/* SD/MMC error code */
#define SDMMC_ERR_R1_OUT_OF_RANGE	-200
#define SDMMC_ERR_R1_ADDRESS_ERROR	-201
#define SDMMC_ERR_R1_BLOCK_LEN_ERROR	-202
#define SDMMC_ERR_R1_ERASE_SEQ_ERROR    -203
#define SDMMC_ERR_R1_ERASE_PARAM	-204
#define SDMMC_ERR_R1_WP_VIOLATION	-205
#define SDMMC_ERR_R1_CARD_IS_LOCKED	-206
#define SDMMC_ERR_R1_LOCK_UNLOCK_FAILED	-207
#define SDMMC_ERR_R1_COM_CRC_ERROR	-208
#define SDMMC_ERR_R1_ILLEGAL_COMMAND	-209
#define SDMMC_ERR_R1_CARD_ECC_FAILED	-210
#define SDMMC_ERR_R1_CC_ERROR		-211
#define SDMMC_ERR_R1_ERROR		-212
#define SDMMC_ERR_R1_UNDERRUN		-213
#define SDMMC_ERR_R1_OVERRUN		-214
#define SDMMC_ERR_R1_CID_CSD_OVERWRITE	-215
#define SDMMC_ERR_R1_WP_ERASE_SKIP	-216
#define SDMMC_ERR_R1_CARD_ECC_DISABLED	-217
#define SDMMC_ERR_R1_ERASE_RESET	-218

/**
 * The CSD register value stored on the SD/MMC memory card.
 */
struct sdmmc_csd
{
	u8	csd_structure;
	u8	spec_vers;
	u8	taac;
	u8	nsac;
	u8	tran_speed;
	u16	ccc;
	u8	read_bl_len;
	u8	read_bl_partial;
	u8	write_blk_misalign;
	u8	read_blk_misalign;
	u8	dsr_imp;
	u32	c_size;
	u8	vdd_r_curr_min;
	u8	vdd_r_curr_max;
	u8	vdd_w_curr_min;
	u8	vdd_w_curr_max;
	u8	c_size_mult;
	/** The 'erase' field is different for different specs */
	union {
		/** MMC system specification version 3.1 */
		struct {
			u8	erase_grp_size;
			u8	erase_grp_mult;
		} mmc_v31;
		/** MMC system specification version 2.2 */
		struct {
			u8	sector_size;
			u8	erase_grp_size;
		} mmc_v22;
		/** SD-Memory specification version 1.01) */
		struct {
			u8	erase_blk_en;
			u8	sector_size;
		} sdmem;
	} erase;
	u8	wp_grp_size;
	u8	wp_grp_enable;
	u8	default_ecc;
	u8	r2w_factor;
	u8	write_bl_len;
	u8	write_bl_partial;
	u8	file_format_grp;
	u8	copy;
	u8	perm_write_protect;
	u8	tmp_write_protect;
	u8	file_format;
};

/**
 * Voltage constants.
 */
#define SDMMC_VDD_145_150	0x00000001	/**< VDD voltage 1.45 - 1.50 */
#define SDMMC_VDD_150_155	0x00000002	/**< VDD voltage 1.50 - 1.55 */
#define SDMMC_VDD_155_160	0x00000004	/**< VDD voltage 1.55 - 1.60 */
#define SDMMC_VDD_160_165	0x00000008	/**< VDD voltage 1.60 - 1.65 */
#define SDMMC_VDD_165_170	0x00000010	/**< VDD voltage 1.65 - 1.70 */
#define SDMMC_VDD_17_18		0x00000020	/**< VDD voltage 1.7 - 1.8 */
#define SDMMC_VDD_18_19		0x00000040	/**< VDD voltage 1.8 - 1.9 */
#define SDMMC_VDD_19_20		0x00000080	/**< VDD voltage 1.9 - 2.0 */
#define SDMMC_VDD_20_21		0x00000100	/**< VDD voltage 2.0 ~ 2.1 */
#define SDMMC_VDD_21_22		0x00000200	/**< VDD voltage 2.1 ~ 2.2 */
#define SDMMC_VDD_22_23		0x00000400	/**< VDD voltage 2.2 ~ 2.3 */
#define SDMMC_VDD_23_24		0x00000800	/**< VDD voltage 2.3 ~ 2.4 */
#define SDMMC_VDD_24_25		0x00001000	/**< VDD voltage 2.4 ~ 2.5 */
#define SDMMC_VDD_25_26		0x00002000	/**< VDD voltage 2.5 ~ 2.6 */
#define SDMMC_VDD_26_27		0x00004000	/**< VDD voltage 2.6 ~ 2.7 */
#define SDMMC_VDD_27_28		0x00008000	/**< VDD voltage 2.7 ~ 2.8 */
#define SDMMC_VDD_28_29		0x00010000	/**< VDD voltage 2.8 ~ 2.9 */
#define SDMMC_VDD_29_30		0x00020000	/**< VDD voltage 2.9 ~ 3.0 */
#define SDMMC_VDD_30_31		0x00040000	/**< VDD voltage 3.0 ~ 3.1 */
#define SDMMC_VDD_31_32		0x00080000	/**< VDD voltage 3.1 ~ 3.2 */
#define SDMMC_VDD_32_33		0x00100000	/**< VDD voltage 3.2 ~ 3.3 */
#define SDMMC_VDD_33_34		0x00200000	/**< VDD voltage 3.3 ~ 3.4 */
#define SDMMC_VDD_34_35		0x00400000	/**< VDD voltage 3.4 ~ 3.5 */
#define SDMMC_VDD_35_36		0x00800000	/**< VDD voltage 3.5 ~ 3.6 */
#define SDMMC_CARD_BUSY		0x80000000	/**< Card Power up status bit */

/**
 * Additional def. for SDIO CMD5 op cond (R4) result.
 */
#define SDIO_OP_COND_C		0x80000000
#define SDIO_OP_COND_NIO(x)	(((x) & 0x70000000) >> 28)
#define SDIO_OP_COND_MP		0x08000000

/**
 * CSD field definitions
 */
#define CSD_STRUCT_VER_1_0  0  /**< Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1  /**< Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2  /**< Valid for system specification 3.1       */

#define CSD_SPEC_VER_0      0  /**< Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1  /**< Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2  /**< Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3  /**< Implements system specification 3.1 */

struct sdmmc_ext_csd
{

	/** Modes segment */
	struct {
		u8	rsv0[177];
		u8 	boot_bus_width;
		u8	rsv7;
		u8      boot_config;
		u8  	rsv6;
		u8	erased_memory_content;
		u8	rsv1;
		u8	bus_width;
		u8	rsv2;
		u8	hs_timing;
		u8	rsv3;
		u8	power_class;
		u8	rsv4;
		u8	cmd_set_rev;
		u8	rsv5;
		u8	cmd_set;
	} modes;
	/** Properties segment */
	struct {
		u8	ext_csd_rev;
		u8	rsv0;
		u8	csd_structure;
		u8	rsv1;
		u8	card_type;
#define HIGH_SPEED_MMC_26MHZ	0x1
#define HIGH_SPEED_MMC_52MHZ	0x2
		u8	rsv2[3];
		u8	pwr_cl_52_195;
		u8	pwr_cl_26_195;
		u8	pwr_cl_52_360;
		u8	pwr_cl_26_360;
		u8	rsv3;
		u8	min_perf_r_4_26;
		u8	min_perf_w_4_26;
		u8	min_perf_r_8_26_4_52;
		u8	min_perf_w_8_26_4_52;
		u8	min_perf_r_8_52;
		u8	min_perf_w_8_52;
		u8	rsv4;
		u32	sec_cnt;
		u8	rsv5[288];
		u8	s_cmd_set;
		u8	rsv6[7];
	} properties;

};

/* DSR1 filed definitions */
#define DSR1_SWITCH_ON_500_200	0x1
#define DSR1_SWITCH_ON_100_50	0x2
#define DSR1_SWITCH_ON_20_10	0x4
#define DSR1_SWITCH_ON_5_2	0x8

/* DSR2 filed definitions */
#define DSR2_PEAK_RISE_1_2_500		0x1
#define DSR2_PEAK_RISE_5_10_100		0x2
#define DSR2_PEAK_RISE_20_50_20		0x4
#define DSR2_PEAK_RISE_100_200_5	0x8

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/* CARD definitions                                                         */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

/**
 * The CID register value stored on the SD/MMC memory card.
 */
struct sdmmc_cid
{
	u8	mid;        /**< Manufacturer ID */
	u16	oid;        /**< OEM/Application ID */
	char	pnm[5];     /**< Product name */
	u8	prv;        /**< Product revision */
	u32	psn;        /**< Product serial number */
	u8	mdt_y;      /**< Manufacture date (year) */
	u8	mdt_m;      /**< Manufacture date (month) */
};

/**
 * Status related to SD Memory Card proprietary features.
 */
struct sdmmc_sd_status
{
	u16	dat_bus_width_secure_mode;
#define DAT_BUS_WIDTH_1BIT	0x0
#define DAT_BUS_WIDTH_4BIT	0x2
#define	SD_CARD_IN_SECURED_MODE	0x1	/**< If card is in secure mode op. */
	u16	sd_card_type;		/**< Type of SD memory card */
#define SD_CARD_TYPE_REGULAR	0x0
#define SD_CARD_TYPE_SDROM	0x1
#define SD_CARD_TYPE_OTP	0x2
	u32	size_of_protected_area;	/**< Size of protected area */
	u8	speed_class;
#define SD_SPEED_CLASS_0	0x0
#define SD_SPEED_CLASS_2	0x1
#define SD_SPEED_CLASS_4	0x2
#define SD_SPEED_CLASS_6	0x3
#define SD_SPEED_CLASS_10	0x4
	u8	performance_move;
	u8	au_size;
	u16	erase_size;
	u8	erase_timeout_offset;
	u8	rsv[50];
};

/**
 * Unpacked data structure for storing the SCR register content.
 */
struct sdmmc_scr
{
	u8	scr_structure;		/**< SCR structure */
	u8	sd_spec;		/**< SD Mem Card - Spec. Version */
	u8	data_stat_after_erase;	/**< Data status after release */
	u8	sd_security;		/**< SD security support */
	u8	sd_bus_width;		/**< SD bus width */
	u8      sd_spec3;               /**< SD Mem Card - Spec. Version3 */
	u8	cmd_support;          	/**< Support bits of CMD20 and CMD23 */
#define SD_SUPPORT_CMD20	0x1
#define SD_SUPPORT_CMD23	0x2
};

/**
 * MMC/SD (logical) device.
 */
struct sdmmc_card
{
        u8                      id;     /**< Card ID, 0-SD, 1-SDIO */
#define SD_CARD                 0
#define SDIO_CARD               1

	u32			ocr;	/**< Operation condition Register */
	struct sdmmc_cid	cid;	/**< Card Identification Register */
	struct sdmmc_csd	csd;	/**< Card Specific Register */
	struct sdmmc_ext_csd	ext_csd;
	u8			rsv[16];
	u16			rca;	/**< Relative Card Address */
	u32			dsr;	/**< Driver Stage Register */
	struct sdmmc_scr	scr;	/**< SD card Configuration Register */

	/* SDIO-specific variables */
	u8		nio;		/**< Number of IO functions */
	u8		mp;		/**< Memory present */
	u16		fnblksz[8];	/**< Function block sizes */
	u32		cisptr[8];	/**< CIS pointers */
#if (SDMMC_SUPPORT_SDCIS == 1)
	cistpl_vers_1_t vers;		/**< CISTPL_VERS_1 tuple result. */
#endif
	/* Other card info */
	u32		taac_ns;	/**< Data access time in ns */
	u32		taac_clks;	/**< Data access time in clks */
	u32		max_dtr;	/**< Maximum data transfer rate */
	u64		capacity;	/**< Card capacity */
	int		mmc4_bustest;	/**< Mask for MMC4 bus test */
#define MMC4_BUS_TEST_FAIL_8	0x2
#define MMC4_BUS_TEST_FAIL_4	0x1
	int		desired_clock;	/**< Desired SD_CLK in Khz */
#define SDMMC_DEFAULT_CLK	24000000/**< Default 24MHz */
#define SDMMC_DEFAULT_INIT_CLK	400000	/**< Default 400KHz */
#define SDMMC_CLK_DECREASE	2000000	/**< Reduce 2Mhz one time */

	int		i_desired_clock;/**< Desired SD_CLK in Khz for Itron use */

	int		f_clock;	/**< User defined SD_CLK */
	int		dbus_width;	/**< Data bus width */
	int		high_speed;	/**< High speed (MMC 4.0) */
	u32		phase;		/**< phase steps for tuning command */
	u16		vdd;		/**< Current working voltage */

	int		format;		/**< File system format ID */
	u32		hcs;		/**< Host capacity support */
	u32		ccs;		/**< 0: Standard, 1:High capacity */
	u32		uhs;		/**< UHS-I mode 1.8V supported */
	u32		mode;		/**< Current data bus mode */
#define	SDMMC_SDR_MODE	0x0
#define	SDMMC_DDR_MODE	0x1
	/** Switch command (spec 3.x) command) result */
	struct sw_cmd_s {
		u32 mode;
#define	SDMMC_DDR50_MODE	0x4
#define	SDMMC_SDR104_MODE	0x3
#define	SDMMC_SDR50_MODE 	0x2
#define	SDMMC_SDR25_MODE 	0x1
#define	SDMMC_SDR12_MODE 	0x0

		u32 drv_stg;	/**< Driver Strength for UHS-I mode */
#define SDMMC_DRV_STG_TYPE_D	0x3
#define SDMMC_DRV_STG_TYPE_C	0x2
#define SDMMC_DRV_STG_TYPE_A	0x1
#define SDMMC_DRV_STG_TYPE_B	0x0

		u32 cur_limit;	/**< Current limit for UHS-I mode */
#define SDMMC_CUR_800MA		0x3
#define SDMMC_CUR_600MA		0x2
#define SDMMC_CUR_400MA		0x1
#define SDMMC_CUR_200MA		0x0

	} sw_cmd;
	u32		drv_stg;	/**< Driver Strength for UHS-I mode */
	u32		cur_limit;	/**< Current limit for UHS-I mode */
	unsigned int		addr_mode;  /**< Address mode: byte/sector */
#define SDMMC_ADDR_MODE_BYTE	0
#define SDMMC_ADDR_MODE_SECTOR	1
	unsigned int		movi_nand;

	/* States */
	unsigned int init_loop;	/**< Initialization loop count */
	u8	pseudo_eject;	/**< State: card is psudo eject */
	u8	present;	/**< State: card is present */
	u8	wp;		/**< State: write protect */
	u8	suspend;	/**< State: card is suspend */
	u8	is_init;	/**< State: card is init */
	u8	is_mmc;		/**< State: card is MMC
				   (mutually exclusive with SD) */
	u8	is_sdmem;	/**< State: card is SDMem
				   (could be combo with SDIO) */
	u8	is_sdio;	/**< State: card is SDIO
				   (could be combo with SDMem) */
	u32	status;		/**< State: status */
#define SDMMC_STATE_INA		0xff
#define SDMMC_STATE_IDLE	0
#define SDMMC_STATE_READY	1
#define SDMMC_STATE_IDENT	2
#define SDMMC_STATE_STBY	3
#define SDMMC_STATE_TRAN	4
#define SDMMC_STATE_DATA	5
#define SDMMC_STATE_RCV		6
#define SDMMC_STATE_PRG		7
#define SDMMC_STATE_DIS		8
	u8	is_busy;
	u8	scc_ctrl;	/**< Is in SDXC speed class ctrl */
};

#define SD_UPDATE_STATUS_ERROR		-300
#define SD_ALLOCATE_BUFFER_ERROR	-301

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/* SDIO definitions                                                         */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

#define MAX_SDIO_TUPLE_BODY_LEN 255

/************************************************/
/* CCCR (Card Common Control Registers).	*/
/* This is for FN0.				*/
/************************************************/
#define CCCR_SDIO_REV_REG         0x00
#   define CCCR_FORMAT_VERSION(reg)    (((reg) & 0x0f))
#   define SDIO_SPEC_REVISION(reg)     (((reg) & 0xf0 >> 4))
#define SD_SPEC_REV_REG           0x01
#   define SD_PHYS_SPEC_VERSION(reg)  (((reg) & 0x0f))
#define IO_ENABLE_REG             0x02
#   define IOE(x)                      (0x1U << (x))
#   define IOE_1                       IOE(1)
#   define IOE_2                       IOE(2)
#   define IOE_3                       IOE(3)
#   define IOE_4                       IOE(4)
#   define IOE_5                       IOE(5)
#   define IOE_6                       IOE(6)
#   define IOE_7                       IOE(7)
#define IO_READY_REG              0x03
#   define IOR(x)                      (0x1U << (x))
#   define IOR_1                       IOR(1)
#   define IOR_2                       IOR(2)
#   define IOR_3                       IOR(3)
#   define IOR_4                       IOR(4)
#   define IOR_5                       IOR(5)
#   define IOR_6                       IOR(6)
#   define IOR_7                       IOR(7)
#define INT_ENABLE_REG            0x04
#   define IENM                        0x1
#   define IEN(x)                      (0x1U << (x))
#   define IEN_1                       IEN(1)
#   define IEN_2                       IEN(2)
#   define IEN_3                       IEN(3)
#   define IEN_4                       IEN(4)
#   define IEN_5                       IEN(5)
#   define IEN_6                       IEN(6)
#   define IEN_7                       IEN(7)
#define INT_PENDING_REG           0x05
#   define INT(x)                      (0x1U << (x))
#   define INT_1                       INT(1)
#   define INT_2                       INT(2)
#   define INT_3                       INT(3)
#   define INT_4                       INT(4)
#   define INT_5                       INT(5)
#   define INT_6                       INT(6)
#   define INT_7                       INT(7)
#define IO_ABORT_REG              0x06
#   define AS(x)                       ((x) & 0x7)
#   define AS_1                        AS(1)
#   define AS_2                        AS(2)
#   define AS_3                        AS(3)
#   define AS_4                        AS(4)
#   define AS_5                        AS(5)
#   define AS_6                        AS(6)
#   define AS_7                        AS(7)
#   define RES                         (0x1U << 3)
#define BUS_INTERFACE_CONTROL_REG 0x07
#   define BUS_WIDTH(reg)              ((reg) & 0x3)
#   define CD_DISABLE                  (0x1U << 7)
#define CARD_CAPABILITY_REG       0x08
#   define SDC                         (0x1U << 0)
#   define SMB                         (0x1U << 1)
#   define SRW                         (0x1U << 2)
#   define SBS                         (0x1U << 3)
#   define S4MI                        (0x1U << 4)
#   define E4MI                        (0x1U << 5)
#   define LSC                         (0x1U << 6)
#   define S4BLS                       (0x1U << 7)
#define COMMON_CIS_POINTER_0_REG  0x09
#define COMMON_CIS_POINTER_1_REG  0x0a
#define COMMON_CIS_POINTER_2_REG  0x0b
#define BUS_SUSPEND_REG           0x0c
#   define BUS_STATUS                  (0x1U << 0)
#   define BUS_REL_REQ_STATUS          (0x1U << 1)
#define FUNCTION_SELECT_REG       0x0d
#define    FS(x)                       ((x) & 0xf)
#   define FS_1                        FS(1)
#   define FS_2                        FS(2)
#   define FS_3                        FS(3)
#   define FS_4                        FS(4)
#   define FS_5                        FS(5)
#   define FS_6                        FS(6)
#   define FS_7                        FS(7)
#   define FS_MEM                      FS(8)
#   define DF                          (0x1U << 7)
#define EXEC_FLAGS_REG            0x0e
#   define EXM                         0x0
#   define EX(x)                       (0x1U << (x))
#   define EX_1                        EX(1)
#   define EX_2                        EX(2)
#   define EX_3                        EX(3)
#   define EX_4                        EX(4)
#   define EX_5                        EX(5)
#   define EX_6                        EX(6)
#   define EX_7                        EX(7)
#define READY_FLAGS_REG           0x0f
#   define RFM                         0x0
#   define RF(x)                       (0x1U << (x))
#   define RF_1                        RF(1)
#   define RF_2                        RF(2)
#   define RF_3                        RF(3)
#   define RF_4                        RF(4)
#   define RF_5                        RF(5)
#   define RF_6                        RF(6)
#   define RF_7                        IEN(7)
#if 0   /* Defined below in FBR */
#define FN0_BLOCK_SIZE_0_REG      0x10
#define FN0_BLOCK_SIZE_1_REG      0x11
#endif
#define POWER_CONTROL_REG	  0x12
#   define SMPC				(0x1U << 0)
#   define EMPC				(0x1U << 1)

/***********************************************/
/* FBR (Function Basic Registers).	       */
/* This is for FN1 ~ FN7.		       */
/***********************************************/
#define FN_CSA_REG(x)             (0x100 * (x) + 0x00)
#define FN_EFIC_REG(x)		  (0x100 * (x) + 0x01)
#define FN_POWER_REG(x)		  (0x100 * (x) + 0x02)
#define FN_CIS_POINTER_0_REG(x)   (0x100 * (x) + 0x09)
#define FN_CIS_POINTER_1_REG(x)   (0x100 * (x) + 0x0a)
#define FN_CIS_POINTER_2_REG(x)   (0x100 * (x) + 0x0b)
#define FN_CSA_POINTER_0_REG(x)   (0x100 * (x) + 0x0c)
#define FN_CSA_POINTER_1_REG(x)   (0x100 * (x) + 0x0d)
#define FN_CSA_POINTER_2_REG(x)   (0x100 * (x) + 0x0e)
#define FN_CSA_DAT_REG(x)         (0x100 * (x) + 0x0f)
#define FN_BLOCK_SIZE_0_REG(x)    (0x100 * (x) + 0x10)
#define FN_BLOCK_SIZE_1_REG(x)    (0x100 * (x) + 0x11)

/*      Function 0  -- duplicate, see the CCRC section */
#define FN0_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(0)
#define FN0_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(0)
#define FN0_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(0)
#define FN0_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(0)
#define FN0_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(0)
/*      Function 1       */
#define FN1_CSA_REG               FN_CSA_REG(1)
#define FN1_EFIC_REG		  FN_EFIC_REG(1)
#define FN1_POWER_REG		  FN_POWER_REG(1)
#define FN1_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(1)
#define FN1_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(1)
#define FN1_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(1)
#define FN1_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(1)
#define FN1_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(1)
#define FN1_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(1)
#define FN1_CSA_DAT_REG           FN_CSA_DAT_REG(1)
#define FN1_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(1)
#define FN1_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(1)
/*      Function 2       */
#define FN2_CSA_REG               FN_CSA_REG(2)
#define FN2_EFIC_REG		  FN_EFIC_REG(2)
#define FN2_POWER_REG		  FN_POWER_REG(2)
#define FN2_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(2)
#define FN2_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(2)
#define FN2_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(2)
#define FN2_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(2)
#define FN2_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(2)
#define FN2_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(2)
#define FN2_CSA_DAT_REG           FN_CSA_DAT_REG(2)
#define FN2_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(2)
#define FN2_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(2)
/*      Function 3       */
#define FN3_CSA_REG               FN_CSA_REG(3)
#define FN3_EFIC_REG		  FN_EFIC_REG(3)
#define FN3_POWER_REG		  FN_POWER_REG(3)
#define FN3_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(3)
#define FN3_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(3)
#define FN3_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(3)
#define FN3_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(3)
#define FN3_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(3)
#define FN3_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(3)
#define FN3_CSA_DAT_REG           FN_CSA_DAT_REG(3)
#define FN3_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(3)
#define FN3_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(3)
/*      Function 4       */
#define FN4_CSA_REG               FN_CSA_REG(4)
#define FN4_EFIC_REG		  FN_EFIC_REG(4)
#define FN4_POWER_REG		  FN_POWER_REG(4)
#define FN4_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(4)
#define FN4_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(4)
#define FN4_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(4)
#define FN4_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(4)
#define FN4_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(4)
#define FN4_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(4)
#define FN4_CSA_DAT_REG           FN_CSA_DAT_REG(4)
#define FN4_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(4)
#define FN4_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(4)
/*      Function 5       */
#define FN5_CSA_REG               FN_CSA_REG(5)
#define FN5_EFIC_REG		  FN_EFIC_REG(5)
#define FN5_POWER_REG		  FN_POWER_REG(5)
#define FN5_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(5)
#define FN5_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(5)
#define FN5_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(5)
#define FN5_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(5)
#define FN5_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(5)
#define FN5_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(5)
#define FN5_CSA_DAT_REG           FN_CSA_DAT_REG(5)
#define FN5_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(5)
#define FN5_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(5)
/*      Function 6       */
#define FN6_CSA_REG               FN_CSA_REG(6)
#define FN6_EFIC_REG		  FN_EFIC_REG(6)
#define FN6_POWER_REG		  FN_POWER_REG(6)
#define FN6_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(6)
#define FN6_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(6)
#define FN6_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(6)
#define FN6_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(6)
#define FN6_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(6)
#define FN6_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(6)
#define FN6_CSA_DAT_REG           FN_CSA_DAT_REG(6)
#define FN6_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(6)
#define FN6_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(6)
/*      Function 7       */
#define FN7_CSA_REG               FN_CSA_REG(7)
#define FN7_EFIC_REG		  FN_EFIC_REG(7)
#define FN7_POWER_REG		  FN_POWER_REG(7)
#define FN7_CIS_POINTER_0_REG     FN_CIS_POINTER_0_REG(7)
#define FN7_CIS_POINTER_1_REG     FN_CIS_POINTER_1_REG(7)
#define FN7_CIS_POINTER_2_REG     FN_CIS_POINTER_2_REG(7)
#define FN7_CSA_POINTER_0_REG	  FN_CSA_POINTER_0_REG(7)
#define FN7_CSA_POINTER_1_REG	  FN_CSA_POINTER_1_REG(7)
#define FN7_CSA_POINTER_2_REG	  FN_CSA_POINTER_2_REG(7)
#define FN7_CSA_DAT_REG           FN_CSA_DAT_REG(7)
#define FN7_BLOCK_SIZE_0_REG      FN_BLOCK_SIZE_0_REG(7)
#define FN7_BLOCK_SIZE_1_REG      FN_BLOCK_SIZE_1_REG(7)

/***********************************************/
/* FBR bits definitions. This is for FN1 ~ FN7 */
/***********************************************/
/* FN_CSA_REG */
#define FN_IODEV_INTERFACE_CODE(reg)	((reg) & 0xf)
#   define NO_SDIO_FUNC			0x0
#   define SDIO_UART			0x1
#   define SDIO_TYPEA_BLUETOOTH		0x2
#   define SDIO_TYPEB_BLUETOOTH		0x3
#   define SDIO_GPS			0x4
#   define SDIO_CAMERA			0x5
#   define SDIO_PHS			0x6
#   define SDIO_WLAN			0x7
/* Reference interface code in FN_EFIC_REG */
#   define SDIO_OTHERS			0xf
#define FN_SUPPORTS_CSA		(0x1U << 6)
#define FN_CSA_ENABLE		(0x1U << 7)


/*******************************************/
/* Misc definition for SDIO		   */
/*******************************************/

/* Misc. helper definitions */
#define FN(x)                     (x)
#define FN0                       FN(0)
#define FN1                       FN(1)
#define FN2                       FN(2)
#define FN3                       FN(3)
#define FN4                       FN(4)
#define FN5                       FN(5)
#define FN6                       FN(6)
#define FN7                       FN(7)

/* IO_RW_DIRECT response(R5) flags */
#define R5_COM_CRC_ERROR	0x80
#define R5_ILLEGAL_COMMAND	0x40

/* bit 5:4 IO_CURRENT_STATE */
#define R5_CURRENT_STATE(x)    	((x & 0x30) >> 4)
#define R5_IO_CUR_STA_DIS	0x0
#define R5_IO_CUR_STA_CMD	0x1
#define R5_IO_CUR_STA_TRN	0x2
#define R5_IO_CUR_STA_RFU	0x3

#define R5_ERROR		0x08
#define R5_FUNC_NUM_ERROR	0x02
#define R5_OUT_OF_RANGE		0x01

/* Modified R6 response status */
#define MR6_COM_CRC_ERROR	0x8000
#define MR6_ILLEGAL_COMMAND	0x4000
#define MR6_ERROR		0x2000

/* SDIO error code */
#define ERR_SDIO_R5_COM_CRC_ERROR	-230
#define ERR_SDIO_R5_ILLEGAL_COMMAND	-231
#define ERR_SDIO_R5_ERROR		-232
#define ERR_SDIO_R5_FUNC_NUM_ERROR	-233
#define ERR_SDIO_R5_OUT_OF_RANGE	-234
#define ERR_SDIO_MR6_COM_CRC_ERROR	-240
#define ERR_SDIO_MR6_ILLEGAL_COMMAND	-241
#define ERR_SDIO_MR6_ERROR		-242

/* SDIO CISTPL_FUNCE tuple for function 0 */
__ARMCC_PACK__ typedef struct sdio_fn0_funce_t {
	u8	type;
	u16	blk_size;
	u8	max_tran_speed;
} __ATTRIB_PACK__ sdio_fn0_funce_t;

/* SDIO CISTPL_FUNCE tuple for function 1 ~ 7 */
__ARMCC_PACK__ typedef struct sdio_fnx_funce_t {
	u8	type;
	u8	func_info;
	u8	std_io_rev;
	u32	card_psn;
	u32	csa_size;
	u8	csa_property;
	u16	max_blk_size;
	u32	ocr;
	u8	op_min_pwr;
	u8	op_avg_pwr;
	u8	op_max_pwr;
	u8	sb_min_pwr;
	u8	sb_avg_pwr;
	u8	sb_max_pwr;
	u16	min_bw;
	u16	opt_bw;
	u16	enable_timeout_val;
	u16	sp_avg_pwr_33v;
	u16	sp_max_pwr_33v;
	u16	hp_avg_pwr_33v;
	u16	hp_max_pwr_33v;
	u16	lp_avg_pwr_33v;
	u16	lp_max_pwr_33v;
} __ATTRIB_PACK__ sdio_fnx_funce_t;

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/* HOST controller definitions                                              */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

/**
 * Basic configuration of the host controller.
 */
struct sdmmc_ios
{
	u32	desired_clock;	/**< The desired clock rate - in KHZ */
#define SDMMC_INIT_CLK		100
	u32	actual_clock;	/**< The actual clock rate - in HZ */
	u16	vdd;		/**< VDD value */
#define	SDMMC_VDD_150	0	/**< 1.50 V */
#define	SDMMC_VDD_155	1	/**< 1.55 V */
#define	SDMMC_VDD_160	2	/**< 1.60 V */
#define	SDMMC_VDD_165	3	/**< 1.65 V */
#define	SDMMC_VDD_170	4	/**< 1.70 V */
#define	SDMMC_VDD_180	5	/**< 1.80 V */
#define	SDMMC_VDD_190	6	/**< 1.90 V */
#define	SDMMC_VDD_200	7	/**< 2.00 V */
#define	SDMMC_VDD_210	8	/**< 2.10 V */
#define	SDMMC_VDD_220	9	/**< 2.20 V */
#define	SDMMC_VDD_230	10	/**< 2.30 V */
#define	SDMMC_VDD_240	11	/**< 2.40 V */
#define	SDMMC_VDD_250	12	/**< 2.50 V */
#define	SDMMC_VDD_260	13	/**< 2.60 V */
#define	SDMMC_VDD_270	14	/**< 2.70 V */
#define	SDMMC_VDD_280	15	/**< 2.80 V */
#define	SDMMC_VDD_290	16	/**< 2.90 V */
#define	SDMMC_VDD_300	17	/**< 3.00 V */
#define	SDMMC_VDD_310	18	/**< 3.10 V */
#define	SDMMC_VDD_320	19	/**< 3.20 V */
#define	SDMMC_VDD_330	20	/**< 3.30 V */
#define	SDMMC_VDD_340	21	/**< 3.40 V */
#define	SDMMC_VDD_350	22	/**< 3.50 V */
#define	SDMMC_VDD_360	23	/**< 3.60 V */

	u8	bus_mode;	/**< command output mode */
#define SDMMC_BUSMODE_OPENDRAIN	1
#define SDMMC_BUSMODE_PUSHPULL	2

	u8	power_mode;	/**< power supply mode */
#define SDMMC_POWER_OFF		0
#define SDMMC_POWER_UP		1
#define SDMMC_POWER_ON		2
};

#define MAX_SDMMC_SLOT 		2

#define MAX_SDMMC_HOST 		SD_INSTANCES

struct sdmmc_host;
struct sdmmc_card;
struct sdmmc_request;

typedef void (*sdioirqhdlr_f)(struct sdmmc_host *, void *);

/**
 * Host controller properties and state.
 */
struct sdmmc_host
{
	u32	f_min;
	u32	f_max;
	u32	ocr_avail;
	u32	support_pll_scaler;
	u32	support_sdxc;
	int	id;		/**< sdmmc host instance ID */
#define SD_HOST_1	0
#define SD_HOST_2	1
	/* host specific block data */
	u32	max_seg_size;	/**< see blk_queue_max_segment_size */
	u16	max_hw_segs;	/**< see blk_queue_max_hw_segments */
	u16	max_phys_segs;	/**< see blk_queue_max_phys_segments */
	u16	max_sectors;	/**< see blk_queue_max_sectors */
	u16	unused;

	/* private data */
	struct sdmmc_ios	ios;	/**< current io bus settings */
	/** cards controlled by host */
	struct sdmmc_card       card[MAX_SDMMC_SLOT];

/**
 * The slot IDs
 */
#define SD_HOST1_SD	0
#define SD_HOST2_SD	1
#define SD_HOST1_SDIO	2

#if (SD_HAS_INTERNAL_MUXER == 1) || (SD_HAS_EXTERNAL_MUXER == 1)
#define SD_MAX_SLOT	(SD_HOST1_SDIO + 1)
#else
#define SD_MAX_SLOT	SD_INSTANCES
#endif

	int	cards;			/**< total number of active cards */
	u16	rca;			/**< the current active card's RCA */
	struct sdmmc_request	*mrq;	/**< saved request */

	int	flgid;			/**< event flag to use with ISR */
	u32	regbase;		/**< SDHC register base address */
	int	irqno;			/**< IRQ number */

	/* Host operations */
	int	(*card_in_slot)(struct sdmmc_host *host, struct sdmmc_card *card);
	int	(*write_protect)(struct sdmmc_host *host, struct sdmmc_card *card);
	void	(*request)(struct sdmmc_host *host, struct sdmmc_card *card,
			   struct sdmmc_request *req);
	void	(*abort)(struct sdmmc_host *host);
	void	(*set_ios)(struct sdmmc_host *host, struct sdmmc_ios *ios,
		           struct sdmmc_card *card);
	int	(*suspend)(struct sdmmc_host *host, struct sdmmc_card *card);
	int	(*resume)(struct sdmmc_host *host, struct sdmmc_card *card);
	int	(*get_bus_status)(struct sdmmc_host *host, struct sdmmc_card *card);

	/* SDIO IRQ */
	int	(*enable_sdio_irq)(struct sdmmc_host *host, int enable);
	int	(*add_sdioirqhdlr)(struct sdmmc_host *host,
				   sdioirqhdlr_f hdlr, void *arg);
	int	(*del_sdioirqhdlr)(struct sdmmc_host *host,
				   sdioirqhdlr_f hdlr);
#define MAX_SDIO_IRQ_HANDLERS	4
	struct {
		sdioirqhdlr_f hdlr;
		void *arg;
	} sdioirqhdlr[MAX_SDIO_IRQ_HANDLERS];
};

#define CHK_CARD_SLOTS(id)					\
{								\
	K_ASSERT((id) < SD_MAX_SLOT);				\
}

#define get_sd_host_id(id)	((id) & 0x1)
#define get_sd_card_id(id)	(((id) >> 1) & 0x1)

/* ======================================================================== */
/* ------------------------------------------------------------------------ */
/* The protocol stack                                                       */
/* ------------------------------------------------------------------------ */
/* ======================================================================== */

#define SDMMC_ERR_NONE		 0	/**< No error */
#define SDMMC_ERR_TIMEOUT	-1	/**< Timeout in SDHC I/O */
#define SDMMC_ERR_BADCRC	-2	/**< CRC error in SDHC I/O */
#define SDMMC_ERR_FAILED	-3	/**< Command failure in SDHC I/O */
#define SDMMC_ERR_INVALID	-4	/**< Invalid cmd/arg in SDHC I/O */
#define SDMMC_ERR_UNUSABLE	-5	/**< Unusable card */
#define SDMMC_ERR_ISR_TIMEOUT	-10	/**< Can't wait SDHC ISR and timeout in
					     sd driver. */
#define SDMMC_ERR_DLINE_TIMEOUT	-11	/**< Can't wait SDHC dat line ready and
					     timeout in sd driver. */
#define SDMMC_ERR_CLINE_TIMEOUT	-12	/**< Can't wait SDHC cmd line ready and
					     timeout in sd driver. */
#define SDMMC_ERR_NO_CARD	-13
#define SDMMC_ERR_CHECK_PATTERN	-14	/**< CMD19 check pattern mismatch */

/* New command8 argument bits field definition in SD 2.0 spec */
#define SDMMC_CHECK_PATTERN	0xaa	/* Recommanded value in SD 2.0 */
#define SDMMC_HIGH_VOLTAGE	0x01	/* 2.7~3.6V */
#define SDMMC_LOW_VOLTAGE	0x02	/* Reserved for Low Voltage Range */

/* HCS & CCS bit shift position in OCR */
#define HCS_SHIFT_BIT		30
#define CCS_SHIFT_BIT		30
#define XPC_SHIFT_BIT		28
#define S18R_SHIFT_BIT		24

/**
 * SD/MMC command abstraction. This data structure is used when sending a
 * a command to the SD host controller.
 */
struct sdmmc_command
{
	u32		opcode;		/**< SD/MMC command code */
	u32		arg;		/**< Argument */
	u32		resp[4];	/**< Response from card */
	u32		expect;		/**< Expected response type */
#define SDMMC_EXPECT_NONE	0
#define SDMMC_EXPECT_R1		1
#define SDMMC_EXPECT_R1B	2
#define SDMMC_EXPECT_R2		3
#define SDMMC_EXPECT_R3		4
#define SDMMC_EXPECT_R4		5
#define SDMMC_EXPECT_R5		6
#define SDMMC_EXPECT_R5B	7
#define SDMMC_EXPECT_R6		8
#define SDMMC_EXPECT_R7		9
	u32		retries;	/**< Max number of retries */
	u32		to;		/**< Timeout to wait for command/data */
	int		error;		/**< Command error code */
};

/**
 * SD/MMC command log. This data structure is used for log command
 * execute time.
 */
typedef struct sdmmc_cmd_log_s
{
	u32 		enable;
	u32		opcode;		/**< SD/MMC command code */
	u32		cnt;		/**< SD/MMC command count */
	u32		max_arg;	/**< Argument */
	u32		min_arg;	/**< Argument */
	u32		blk_cnt_min;	/**< Min block count */
	u32		blk_cnt_max;	/**< Max block count */
	u32		tim_min;	/**< Min elapsed time  */
	u32		tim_max;	/**< Max elapsed time */
	u32		tim_total;	/**< total excute time */
} sdmmc_cmd_log_t;

/**
 * Data transfer. This data structure is used when sending a command to the
 * SD host controller and a data is expected to transferred on the DAT line
 * in association with the command.
 */
struct sdmmc_data
{
	u8		*buf;		/**< System memory buffer */
	u32		timeout_ns;	/**< Data timeout (in ns, max 80ms) */
	u32		timeout_clks;	/**< Data timeout (in clocks) */
	u32		blksz;		/**< Data block size */
	u32		blocks;		/**< Number of blocks */
	int		error;		/**< Data error */
	u32		flags;		/**< Data transfer flags */
#define SDMMC_DATA_READ		(0 << 1)	/**< Data read (from card) */
#define SDMMC_DATA_WRITE	(1 << 1)	/**< Data write (to card) */
#define SDMMC_DATA_STREAM	(1 << 2)	/**< Stream access (ACMD12) */
#define SDMMC_DATA_1BIT_WIDTH	(0 << 3)	/**< 1 DAT line */
#define SDMMC_DATA_4BIT_WIDTH	(1 << 3)	/**< 4 DAT lines */
#define SDMMC_DATA_8BIT_WIDTH	(2 << 3)	/**< 8 DAT lines */
#define SDMMC_DATA_HIGH_SPEED	(1 << 5)	/**< High speed mode */
	u32		bytes_xfered;	/**< The actual bytes transferred */
};

/**
 * This data structure represents a requst for a transaction to be issued by
 * the host controller to the card.
 */
struct sdmmc_request
{
	struct sdmmc_command	*cmd;	/**< transaction command */
	struct sdmmc_command	*stop;	/**< transaction end command */
	struct sdmmc_data	*data;	/**< data to be transferred */
};

extern int sdmmc_set_timeout(u32 id, u32 type, u32 val);
extern int sdmmc_get_timeout(u32 id, u32 type, u32* val);
#define SDMMC_INIT_TIMEOUT 0x1
#define SDMMC_TRAN_TIMEOUT 0x2

extern int sdmmc_cmd_log_set(u32 id, u32 enable, u32 opcode);
extern int sdmmc_cmd_log_get(u32 id, sdmmc_cmd_log_t *cmd_log);

extern void sdmmc_set_dly(void);

#ifdef __SDHOST_IMPL__

/* ----------------------------------------------------------------------------
 * The following set of functions are internal to the host controller
 * interface, as well as the protocol stack.
 * ----------------------------------------------------------------------------
 */

/**
 * Initialize a card attached to the SD/MMC host controller. This function
 * goes through an indentification and status gathering process for each of
 * the MMC, SD memory, SDIO, or multi-function cards as specified in the
 * respective protocol specifications.
 *
 * This function should be called by a task that is set-up to perform
 * initialization procedures. The sequence should be:
 * 1. Host controller driver sets a host event (card insertion)
 * 2. The event wakes up (or notifies) a task that an initialization is
 *    required.
 * 3. The initialization task uses its context to perform the initialization.
 *
 * @param host - The SDHC.
 * @param repeat - The numbers of times to repeat SD initialization.
 * @param fclock - The specific clock to be initialized.
 * @returns - 0 successful, < 0 failure
 * @see scm
 */
extern int sdmmc_init_card(struct sdmmc_host *host, struct sdmmc_card *card,
			   int repeat, u32 fclock);

/**
 * Deinitialize the cad that has been removed from the SD/MMC host controller.
 * This function puts the controller into a clean state. It should run
 * from the context of a task/thread that also performs the
 * 'sdmmc_init_card()' function.
 *
 * @param host - The SDHC.
 * @returns - 0 successful, < 0 failure
 * @see sdmmc_init_card
 */
extern int sdmmc_deconf_card(struct sdmmc_host *host, struct sdmmc_card *card);

/**
 * Submit a SD/MMC command transaction and wait for its completion.
 *
 * @param host - The SDHC.
 * @param mrq - The SD/MMC transaction request.
 * @returns - 0 successful, < 0 failure
 */
extern int sdmmc_wait_for_req(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      struct sdmmc_request *mrq);

/**
 * Send a command to the card and wait for its completion.
 *
 * This function is used as a wrapper to the function sdmmc_wait_for_req().
 *
 * @param host - The SDHC.
 * @param mrq - The SD/MMC transaction request.
 * @returns - 0 suceessful, < 0 failure.
 * @see sdmmc_wait_for_req
 */
extern int sdmmc_wait_for_cmd(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      struct sdmmc_command *mrq);

/**
 * This function sets the default controller for the system
 * (which may be retrieved by the call 'sdmmc_get_host()' later).
 *
 * @param host - The SDHC.
 * @see sdmmc_get_host
 */
extern void sdmmc_set_host(struct sdmmc_host *host);

/**
 * Diagnostic for printing R1.
 */
extern void sdmmc_print_r1(u32 *resp);

#endif  /* __SDHOST_IMPL__ */

/**
 * Get the default SD/MMC slot number of the system.
 *
 * @returns - The scardmgr slot number.
 */
extern int sdmmc_get_slot(int id);

/**
 * Get the default SD/MMC host controller instanace of the system.
 *
 * @returns - The SDHC.
 */
extern struct sdmmc_host *sdmmc_get_host(int id);

/**
 * Enable host1 SDIO support. To enable SD host1 SDIO,
 * this function should be called before "sdmmc_set_host()".
 *
 */
extern void sdmmc_enable_host1_sdio(void);

/**
 * Enable SDXC 1.8v voltage switch support. To enable SDXC UHS mode,
 * this function should be called before "sdmmc_set_host()".
 *
 */
extern void sdmmc_set_18v_support(u32 id, u32 val);

/**
 * Resets all cards to idle state (CMD0).
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_go_idle_state(struct sdmmc_host *host,
			       struct sdmmc_card *card);

/**
 * Send the operating condition (CMD1).
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @param wv - Working voltage argument.
 * @param ocr - Read/Write variable of the Operating Condition Register.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_send_op_cond_cmd1(struct sdmmc_host *host,
				   struct sdmmc_card *card,
				   u32 wv, u32 *ocr);

/**
 * Asks any card to send the CID numbers on the CMD line. (any card that is
 * connected to the host will respond). (CMD1).
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @param cid - Write-back variable of the CID.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_all_send_cid(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      struct sdmmc_cid *cid);

/**
 * Ask the card to publish a new relative address (RCA) (CMD3).
 *
 * @param host - The SDHC.
 * @param rca - Write-back variable of the RCA.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_send_relative_addr(struct sdmmc_host *host,
				    struct sdmmc_card *card,
				    u16 *rca);

/**
 * Programs the DSR of all records (CMD4).
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @param dsr - The DSR.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_set_dsr(struct sdmmc_host *host,
			 struct sdmmc_card *card,
			 u16 dsr);

/**
 * Send CMD5 to which only an SDIO card would reply.
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @param wv - Working voltages (I/O OCR).
 * @param c -  C bit.
 * @param nio - Number of I/O functions.
 * @param m - Memory present bit.
 * @param *ocr - Operating condition register.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_io_send_op_cond(struct sdmmc_host *host,
				 struct sdmmc_card *card,
				 u32 wv, u8 *c, u8 *nio, u8 *m, u32 *ocr);

/**
 * Switches the mode of operation of the selected card or modifies the
 * EXT_CSD registers (CMD6).
 *
 * @param host - The SDHC.
 * @param card - The card.
 * @param access - Access type.
 * @param index - Index to EXT_CSD.
 * @param value - Value to modify.
 * @param cmd_set - Command set.
 * @returns - 0 successful, < 0 failed.
 */
extern int sdmmc_switch(struct sdmmc_host *host,
			struct sdmmc_card *card,
			u8 access, u8 index, u8 value, u8 cmd_set);
#define ACCESS_COMMAND_SET	0x0
#define ACCESS_SET_BITS		0x1
#define ACCESS_CLEAR_BITS	0x2
#define ACCESS_WRITE_BYTE	0x3

/**
 * The switch function status is the returned data block that contains function
 * and current consumption information. The block length is predefined to
 * 512 bits and the use of SET_BLK_LEN command is not necessary.
 */
struct sw_func_status {
	u16 max_current;
	u16 support_group6;
	u16 support_group5;
	u16 support_group4;
	u16 support_group3;
	u16 support_group2;
	u16 support_group1;
	u8  status_group6 : 4;
	u8  status_group5 : 4;
	u8  status_group4 : 4;
	u8  status_group3 : 4;
	u8  status_group2 : 4;
	u8  status_group1 : 4;
	u8  struct_ver;
	u16 busy_group6;
	u16 busy_group5;
	u16 busy_group4;
	u16 busy_group3;
	u16 busy_group2;
	u16 busy_group1;
	u8  rsv1[34];
};

struct sw_func_switch {
	u8 rsv2;
	u8 rsv1;
	u8 cur_limit;
	u8 drv_stg;
	u8 cmd_sys;
	u8 access_mode;
};

/**
 * CMD6 for SD card with version larger than 1.11.
 */
extern int sdmmc_switch_func(struct sdmmc_host *host,
			     struct sdmmc_card *card,
			     u8 mode, struct sw_func_switch *switch_req,
			     struct sw_func_status *sw);
#define MODE_CHECK_FUNC		0x0
#define MODE_SWITCH_FUNC	0x1
#define CURRENT_FUNC		0xf
#define DEFAULT_FUNC		0x0
#define CMD_SYS_ECOMMERCE	0x1
#define ACCESS_HIGH_SPEED	0x1

/**
 * Select a card - send the RCA to the card specified to the SD/MMC bus (CMD7).
 *
 * @param host - The SDHC.
 * @param card - The card to select on.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_select_card(struct sdmmc_host *host,
			     struct sdmmc_card *card);

/**
 * The card sends its EXT_CSD register as a block of data (CMD8).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param ext_csd - Write-back variable of the EXT_CSD register.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_ext_csd(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      struct sdmmc_ext_csd *ext_csd);

/**
 * Commad8 is defined to initialize SD memory cards compilant to the SD ver2.00 (NEW CMD8).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_if_cond(struct sdmmc_host *host, struct sdmmc_card *card);

/**
 * Addressed card sends its card-specific data (CSD) on the CMD line (CMD9).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param csd - Write-back variable of the CSD.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_csd(struct sdmmc_host *host,
			  struct sdmmc_card *card,
			  struct sdmmc_csd *csd);

/**
 * Addressed card sends its card identification (CID) on the CMD line (CMD10).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param cid - Write-back variable of the CID.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_cid(struct sdmmc_host *host,
			  struct sdmmc_card *card,
			  struct sdmmc_cid *cid);

/**
 * Reads data stream from the card, starting at the given address, until a
 * STOP_TRANSMISSION follows (CMD11).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_read_dat_until_stop(struct sdmmc_host *host,
				     struct sdmmc_card *card,
				     u32 addr, u8 *buf);

/**
 * Do singal voltage switch sequence for spec 3.x(CMD11).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_voltage_switch(struct sdmmc_host *host,
			 	struct sdmmc_card *card);
#define CLK_SWITCH_DLY_MS	5
#define CMDLINE_SWITCH_DLY_MS	1

/**
 * Forces the card to stop transmission (CMD12).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_stop_transmission(struct sdmmc_host *host,
				   struct sdmmc_card *card);

/**
 * Addressed card sends its status register (CMD13).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param status - Write-back variable of the status register.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_status(struct sdmmc_host *host,
			     struct sdmmc_card *card,
			     u32 *status);

/**
 * A host reads the reversed bus testing pattern from a card (CMD14).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param buf - Pointer to the data buffer on host memory.
 * @param buflen - Length of data buffer.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_bustest_r(struct sdmmc_host *host,
			   struct sdmmc_card *card,
			   u8 *buf, int buflen);

/**
 * Sets the card to inactive state in order to protect the card stack
 * against communication breakdowns (CMD15).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_go_inactive_state(struct sdmmc_host *host,
				   struct sdmmc_card *card);

/**
 * Sets the block length (in bytes) for all following block commands (read
 * and write). Default block length is specified in the CSD. Supported only
 * if partial block RD/WR operation are allowed in CSD (CMD16).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param blklen - Block length.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_blocklen(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      int blklen);
/**
 * Reads a block of the size selected by the SET_BLOCKLEN command (CMD17).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_read_single_block(struct sdmmc_host *host,
				   struct sdmmc_card *card,
				   u32 addr, u8 *buf);

/**
 * Continuously transfers data blocks from card to host until interrupted
 * by a STOP_TRANSMISSION command (CMD18).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param blocks - The number of blocks.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_read_multiple_block(struct sdmmc_host *host,
				     struct sdmmc_card *card,
				     u32 addr, int blocks, u8 *buf);

/**
 * A host sends the bus test data pattern to a card (CMD19).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param buf - Pointer to the data buffer on host memory.
 * @param buflen - Length of data buffer.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_bustest_w(struct sdmmc_host *host,
			   struct sdmmc_card *card,
			   const u8 *buf, int buflen);

/**
 * A host sends the bus test data pattern to a card for spec 3.x (CMD19).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param buf - Pointer to the data buffer on host memory, used by SW mode.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_send_tuning_pattern(struct sdmmc_host *host,
		    	      	     struct sdmmc_card *card,
		    	      	     u8 *buf);
#define TUNING_BLOCK_SIZE 	64

/**
 * Writes a data stream from the host, starting at the given address,
 * until a STOP_TRANSMISSION follows (CMD20).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_write_dat_until_stop(struct sdmmc_host *host,
				      struct sdmmc_card *card,
				      u32 addr, u8 *buf);

/**
 * Defines to optimize card operation to support Speed Class recording
 * for spec 3.x (CMD20).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param ssc - Speed Class Control.
 * @returns - 0 suceessful, < 0 failure.
 */
extern int sdmmc_speed_class_control(struct sdmmc_host * host,
				     struct sdmmc_card * card,
				     unsigned short scc);
#define SCC_START_RECORDING		0x0
#define SCC_CREATE_DIR			0x1
#define SCC_END_REC_WITHOUT_MOVE	0x2
#define SCC_END_REC_WITH_MOVE		0x3

extern int sdxc_speed_class_ctrl(int slot, u16 scc);
#define SCC_ERR_UNSUPPORT_SLOT		-1	/**< Not a SD slot */
#define SCC_ERR_UNSUPPORT_CMD		-2	/**< Card doesn't support SSC */


/**
 * Defines the number of blocks which are going to be transferred in the
 * immediately succeeding multiple block read or write command. If the
 * argument is all 0s, the subsequent read/write operation will be
 * open-ended (CMD23).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param count - Number of blocks.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_block_count(struct sdmmc_host *host,
				 struct sdmmc_card *card,
				 int count);

/**
 * Writes a block of the size selected by the SET_BLOCKLEN command (CMD24).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_write_block(struct sdmmc_host *host,
			     struct sdmmc_card *card,
			     u32 addr, const u8 *buf);

/**
 * Continuously writes blocks of data until 'Stop Tran' token is sent
 * (instead of 'Start Block') (CMD25).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of target memory.
 * @param blocks - The number of blocks.
 * @param buf - Pointer to the data buffer on host memory.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_write_multiple_block(struct sdmmc_host *host,
				      struct sdmmc_card *card,
				      u32 addr, int blocks, const u8 *buf);

/**
 * Programming of the card identification register. This command shall be
 * issued only once. The card contains hardware to prevent this operation
 * after the first programming. Normally this command is reserved for the
 * manufacturer (CMD26).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_program_cid(struct sdmmc_host *host,
			     struct sdmmc_card *card);

/**
 * Programming of the programmable bits of the CSD (CMD27).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_program_csd(struct sdmmc_host *host,
			     struct sdmmc_card *card);

/**
 * If the card has write protection features, this command sets the write
 * protection bit of the addressed group. The properties of write protection
 * are coded in the card specific data (WP_GRP_SIZE) (CMD28).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the group.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_write_prot(struct sdmmc_host *host,
				struct sdmmc_card *card,
				u32 addr);

/**
 * If the card has write protection deatures, this command clears the write
 * protection bit of the addressed group (CMD29).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the group.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_clr_write_prot(struct sdmmc_host *host,
				struct sdmmc_card *card,
				u32 addr);


/**
 * If the card has write protection features, this command asks the card to
 * send the status of the write protection bits (CMD30).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The protected data address.
 * @param prot - Write-back variable of prot sent.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_send_write_prot(struct sdmmc_host *host,
				 struct sdmmc_card *card,
				 u32 addr, u32 *prot);

/**
 * Sets the addresse of the first write block to be erased (CMD32).
 * This is an SD memory command.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the write block.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_erase_wr_blk_start(struct sdmmc_host *host,
				    struct sdmmc_card *card,
				    u32 addr);

/**
 * Sets the address of the last write block of the continuous range
 * to be erased (CMD33).
 * This is an SD memory command.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the write block.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_erase_wr_blk_end(struct sdmmc_host *host,
				  struct sdmmc_card *card,
				  u32 addr);

/**
 * Sets the addresse of the first group to be erased (CMD35).
 * This is an MMC command.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the group.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_erase_group_start(struct sdmmc_host *host,
			     	   struct sdmmc_card *card,
			    	   u32 addr);

/**
 * Sets the address of the last group continuous range to be erased (CMD36).
 * This is an MMC command.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param addr - The data address of the group.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_erase_group_end(struct sdmmc_host *host,
				  struct sdmmc_card *card,
				  u32 addr);

/**
 * Erases all previously selected write blocks (CMD38).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_erase(struct sdmmc_host *host,
		       struct sdmmc_card *card);

/**
 * Used to write and read 8 bit (register) data fields. The command addresses
 * a card and a register and provides the data for writing if the write flag
 * is set. The R4 response contains data read from the addressed register.
 * This command accesses application dependent registers which are not defined
 * in the MultiMediaCard standard (CMD39).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param flag - Register write flag.
 * @param addr - Register address.
 * @param data - Pointer to register data to read/write.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_fast_io(struct sdmmc_host *host,
			 struct sdmmc_card *card,
			 u8 flag, u8 addr, u8 *data);

/**
 * Ses the system into interrupt mode (CMD40).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_go_irq_state(struct sdmmc_host *host,
			      struct sdmmc_card *card);


struct lock_data{
	u8 op;
	u8 pwds_len;
	u8 *pw_data;
};
/**
 * Used to set/reset the password or lock/unlock the card. A transferred
 * data block includes all the command details - refer to Chapter 4.3.6. The
 * size of the Data Block is defined with SET_BLOCK_LEN command (CMD42).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_lock_unlock(struct sdmmc_host *host,
			     struct sdmmc_card *card,
			     struct lock_data *loc);
#define SDMMC_LOCK_OP_ERASE	0x8
#define SDMMC_LOCK_OP_LOCK	0x4
#define SDMMC_LOCK_OP_CLR_PWD	0x2
#define SDMMC_LOCK_OP_SET_PWD	0x1

/**
 * The IO_RW_DIRECT is the simplest means to access a single register within
 * the total 128K of register space in any I/O function, including the common
 * I/O area (CIA). This command reads or writes 1 byte using only 1
 * command/response pair. A common use is to initialize registers or monitor
 * status values for I/O functions. This command is the fastest means to read
 * or write single I/O registers, as it requires only a single command/response
 * pair. The SDIO card's response to CMD52 will be in one of two formats.
 * If the communication between the card and host is in the 1-bit or 4-bit
 * SD mode, the response will be in a 48-bit response (R5). If the
 * communication is using the SPI mode, the response will be a 16-bit
 * R5 response.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param rw - Read/Write/Read-After-Write
 * @param func - Function number
 * @param reg - Register address
 * @param val - Input (or input + output) variable
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_io_rw_direct(struct sdmmc_host *host,
			      struct sdmmc_card *card,
			      int rw,
			      u8 func,
			      u32 reg,
			      u8 *val);

#define SDMMC_SDIO_REG_READ		0	/**< SDIO: Read register */
#define SDMMC_SDIO_REG_WRITE		1	/**< SDIO: Write register */
#define SDMMC_SDIO_REG_WRITE_READ	2	/**< SDIO: Read after write */

/**
 * In order to read and write multiple I/O registers with a single command,
 * a new command, IO_RW_EXTENDED is defined. This command is included in
 * command class 9 (I/O Commands). This command allows the reading or writing
 * of a large number of I/O registers with a single command. Since this is a
 * data transfer command, it provides the highest possible transfer rate. The
 * response from the SDIO card to CMD53 will be R5 (the same as CMD52). For
 * CMD53, the 8-bit data field will be stuff bits and shall be read as 0x00.
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param rw - Read/Write.
 * @param func - Function number
 * @param blockmode - The block mode
 * @param opcode - 0 = fixed address, 1 = incremental address
 * @param reg - Register address
 * @param count - Number of block to transfer
 * @param blocksize - The block size
 * @param dat - Data buffer (input or output)
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_io_rw_extended(struct sdmmc_host *host,
				struct sdmmc_card *card,
				int rw,
				u8 func,
				u8 blockmode,
				u8 opcode,
				u32 reg,
				u32 count,
				u32 blocksize,
				u8 *dat);
/* R/W flag */
#define SDMMC_SDIO_EXT_READ		0
#define SDMMC_SDIO_EXT_WRITE		1

/* Block mode */
#define SDMMC_SDIO_BYTE_MODE		0
#define SDMMC_SDIO_BLOCK_MODE		1

/* OP code */
#define SDMMC_SDIO_FIXED_ADDR		0
#define SDMMC_SDIO_INCRE_ADDR		1


#define SDMMC_SDIO_MEM_READ	0	/**< SDIO: Read memory */
#define SDMMC_SDIO_MEM_WRITE	1	/**< SDIO: Write memory */
#define SDMMC_SDIO_BYTE_MODE	0	/**< SDIO: blockmode = byte mode */
#define SDMMC_SDIO_BLOCK_MODE	1	/**< SDIO: blockmode = block mode */
#define SDMMC_SDIO_FIXED_ADDR	0	/**< SDIO: opcode = fixed address */
#define SDMMC_SDIO_INCR_ADDR	1	/**< SDIO: opcode = incremental addr */

/**
 * Defines the data bus width ('00' = 1 bit or '10' = 4 bit bus) to be used
 * for data transfer. The allowed data bus width are given in SCR register
 * (ACMD6).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param width - Data bus width.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_bus_width(struct sdmmc_host *host,
			       struct sdmmc_card *card,
			       int width);
#define SDMMC_BUS_WIDTH_1_BIT 0
#define SDMMC_BUS_WIDTH_4_BIT 2

/**
 * Send the SD Memory Card status. The status fields are given in Table 24
 * (ACMD13).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param status - The SD status.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_send_sd_status(struct sdmmc_host *host,
				struct sdmmc_card *card,
				struct sdmmc_sd_status *status);

/**
 * Send the number of the well written (without errors) blocks. Responds with
 * 32-bit + CRC data block (ACMD22).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param blocks - The number of blocks.
 * @returns - 0 successful, < 0 failed

 */
extern int sdmmc_send_num_wr_blocks(struct sdmmc_host *host,
				    struct sdmmc_card *card,
				    int blocks);

/**
 * Set the number of write blocks to be pre-erased before writing (to be
 * used for faster Multiple Block WR command) (ACMD23).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param blocks - The number of blocks.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_wr_blk_erase_count(struct sdmmc_host *host,
					struct sdmmc_card *card,
					int blocks);

/**
 * Activates the card's initialization process (ACMD41).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param wv - Working voltage argument.
 * @param ocr - Write-back register of the Operating Condition Register.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_send_op_cond_acmd41(struct sdmmc_host *host,
				     struct sdmmc_card *card,
				     u32 wv, u32 *ocr);

/**
 * Connect(1)/Disconnect(0) the 50KOhm pull-up resistor on CD/DAT3
 * (pin 1) of the card. The pull-up may be used for card detection (ACMD42).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param set_cd - Connect or disconnect the resistor.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_set_clr_card_detect(struct sdmmc_host *host,
				     struct sdmmc_card *card,
				     int set_cd);

/**
 * Reads the SD Configuration Register (ACMD51).
 *
 * @param host - The SDHC.
 * @param card - The card to use.
 * @param scr - Write-back variable of the SCR register.
 * @returns - 0 successful, < 0 failed
 */
extern int sdmmc_send_scr(struct sdmmc_host *host,
			  struct sdmmc_card *card,
			  struct sdmmc_scr *scr);

/* ------------------------------------------------------------------------- */

#if (SDMMC_SUPPORT_SDCIS == 1)
/**
 * Get the next SDIO card CIS tuple.
 *
 * @param host - The SD host
 * @param card - The SDIO card
 * @param fn - Function number
 * @param tuple - The CIS tuple data structure
 * @returns - 0 successful, < 0 failure
 */
extern int sdio_get_next_tuple(struct sdmmc_host *host,
			       struct sdmmc_card *card,
			       int fn,
			       tuple_t *tuple);

/**
 * Get SDIO card CIS tuple data.
 *
 * @param host - The SD host
 * @param card - The SDIO card
 * @param fn - Function number
 * @param tuple - The CIS tuple data structure
 * @returns - 0 successful, < 0 failure
 */
extern int sdio_get_tuple_data(struct sdmmc_host *host,
			       struct sdmmc_card *card,
			       int fn,
			       tuple_t *tuple);
#endif	/* SDMMC_SUPPORT_SDCIS == 1 */
/**
 * Dump CIS info of an SDIO card in PCCard mode.
 *
 * @param host - The CF host
 * @param card - The SDIO card
 */
extern int sdio_dump_cis(struct sdmmc_host *host, struct sdmmc_card *card);

/**
 * Dump SDIO interface code.
 *
 * @param fic  - value of function interface code register
 * @param efic - value of extend function interface code register
 */
extern void sdmmc_dump_sdio_interface_code(u8 fic, u8 efic);

/**
 * Dump SDIO card capability.
 *
 * @param cap  - value of card capability register
 */
extern void sdmmc_dump_sdio_capability(u8 cap);

/*@}*/

__END_C_PROTO__

#endif
