/**
 * @file system/include/fio/sdboot.h
 *
 * History:
 *    2007/12/19 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FIO__SDBOOT_H__
#define __FIO__SDBOOT_H__

#define SDBOOT_MAGIC_OFFSET		0x00

#define SDBOOT_START_BLOCK_OFFSET	0x70
#define SDBOOT_BLOCKS_OFFSET		0x74
#define SDBOOT_MEM_TARGET_OFFSET	0x78

#if	(CHIP_REV == A5S)
#define SDBOOT_HEADER_MAGIC		0x00200209
#else
#define SDBOOT_HEADER_MAGIC		0x90abc314
#endif

#ifndef __ASM__

#ifdef  __cplusplus
extern "C" {
#endif

#if	(CHIP_REV == A2S) || (CHIP_REV == A2M)

#define SUPPORT_MKBOOT

struct sdmmc_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int rsv0;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv1;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_cfg;		/* 0x10 */
	unsigned int dram_mode;		/* 0x14 */
	unsigned int dram_ext_mode;	/* 0x18 */
	unsigned int dram_timing1;	/* 0x1c */
	unsigned int dram_timing2;	/* 0x20 */
	unsigned int dram_ref_ctl;	/* 0x24 */
	unsigned int dll0;		/* 0x28 */
	unsigned int dll1;		/* 0x2c */
	unsigned int dll_ocd;		/* 0x30 */
	unsigned int ddrio_calib;	/* 0x34 */
	unsigned int rsv2[14];		/* 0x38-0x6c */
	/* ----------------------------------------- */
	unsigned int start_block;	/* 0x70 */
	unsigned int blocks;		/* 0x74 */
	unsigned int mem_target;	/* 0x78 */
	unsigned int rsv3;		/* 0x7c */
};

#elif	(CHIP_REV == A5S)

#define SUPPORT_MKBOOT

struct sdmmc_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int rsv0;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv1;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_control;	/* 0x10 */
	unsigned int dram_config;	/* 0x14 */
	unsigned int dram_timing1;	/* 0x18 */
	unsigned int dram_timing2;	/* 0x1c */
	unsigned int dram_init_ctl;	/* 0x20 */
	unsigned int dram_ext_mode0;	/* 0x24 */
	unsigned int dram_ext_mode1;	/* 0x28 */
	unsigned int dram_ext_mode2;	/* 0x2c */
	unsigned int dram_ext_mode3;	/* 0x30 */
	unsigned int dram_dqs_sync;	/* 0x34 */
	unsigned int dll0;		/* 0x38 */
	unsigned int dll1;		/* 0x3c */
	unsigned int dll2;		/* 0x40 */
	unsigned int dll3;		/* 0x44 */
	unsigned int dll_ocd;		/* 0x48 */
	unsigned int ddrio_calib;	/* 0x4c */
	unsigned int rsv2[8];		/* 0x50-0x6c */
	/* ----------------------------------------- */
	unsigned int start_block;	/* 0x70 */
	unsigned int blocks;		/* 0x74 */
	unsigned int mem_target;	/* 0x78 */
	unsigned int rsv3;		/* 0x7c */
};

#elif	(CHIP_REV == A6)

#define SUPPORT_MKBOOT

struct sdmmc_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int rsv0;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv1;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_control;	/* 0x10 */
	unsigned int dram_config;	/* 0x14 */
	unsigned int dram_timing1;	/* 0x18 */
	unsigned int dram_timing2;	/* 0x1c */
	unsigned int dram_init_ctl;	/* 0x20 */
	unsigned int dram_ext_mode0;	/* 0x24 */
	unsigned int dram_ext_mode1;	/* 0x28 */
	unsigned int dram_ext_mode2;	/* 0x2c */
	unsigned int dram_ext_mode3;	/* 0x30 */
	unsigned int dram_dqs_sync;	/* 0x34 */
	unsigned int dll0;		/* 0x38 */
	unsigned int dll1;		/* 0x3c */
	unsigned int dll2;		/* 0x40 */
	unsigned int dll3;		/* 0x44 */
	unsigned int dll_ocd;		/* 0x48 */
	unsigned int ddrio_calib;	/* 0x4c */
	unsigned int rsv2[8];		/* 0x50-0x6c */
	/* ----------------------------------------- */
	unsigned int start_block;	/* 0x70 */
	unsigned int blocks;		/* 0x74 */
	unsigned int mem_target;	/* 0x78 */
	unsigned int rsv3;		/* 0x7c */
};

#else

struct sdmmc_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int rsv0;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv1;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int rsv2[24];		/* 0x10-0x6c */
	/* ----------------------------------------- */
	unsigned int start_block;	/* 0x70 */
	unsigned int blocks;		/* 0x74 */
	unsigned int mem_target;	/* 0x78 */
	unsigned int rsv3;		/* 0x7c */
};

#endif

/* Obsolete the mkboot utilities. */
#undef SUPPORT_MKBOOT

#ifdef  __cplusplus
}
#endif

#endif  /* __ASM__ */

#endif
