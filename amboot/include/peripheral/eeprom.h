/**
 * @file system/include/peripheral/eeprom.h
 *
 * EEPROM handling utilities.
 *
 * History:
 *    2007/07/16 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __EEPROM_H__
#define __EEPROM_H__

#include <ambhw/chip.h>
#include <basedef.h>

#define EEPROM_MAGIC_OFFSET		0x00
#define EEPROM_BAUD_OFFSET		0x04

#if	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)

#define EEPROM_INIT_DELAY_OFFSET	0x08
#define EEPROM_DRAM_CFG_OFFSET		0x10
#define EEPROM_DRAM_MODE_OFFSET		0x14
#define EEPROM_DRAM_EXT_MODE_OFFSET	0x18
#define EEPROM_DRAM_TIMING_OFFSET	0x1c
#define EEPROM_DRAM_TIMING2_OFFSET	0x20
#define EEPROM_DRAM_REF_CTL_OFFSET	0x24
#define EEPROM_DLL0_OFFSET		0x28
#define EEPROM_DLL1_OFFSET		0x2c
#define EEPROM_DLL_OCD_OFFSET		0x30
#define EEPROM_DDRIO_CALIB_OFFSET	0x34

#elif	(CHIP_REV == A5S)

#define EEPROM_INIT_DELAY_OFFSET	0x08
#define EEPROM_DRAM_CONTROL_OFFSET	0x10
#define EEPROM_DRAM_CONFIG_OFFSET	0x14
#define EEPROM_DRAM_TIMING1_OFFSET	0x18
#define EEPROM_DRAM_TIMING2_OFFSET	0x1c
#define EEPROM_DRAM_TIMING3_OFFSET	0x20
#define EEPROM_DRAM_INIT_CTL_OFFSET	0x24
#define EEPROM_DRAM_MODE_OFFSET		0x28
#define EEPROM_DRAM_EXT_MODE_OFFSET	0x2c
#define EEPROM_DRAM_EXT_MODE2_OFFSET	0x30
#define EEPROM_DRAM_EXT_MODE3_OFFSET	0x34
#define EEPROM_DRAM_SELF_REF_OFFSET	0x38
#define EEPROM_DRAM_DQS_SYNC_OFFSET	0x3c
#define EEPROM_DRAM_PAD_TERM_OFFSET	0x40
#define EEPROM_DRAM_ZQ_CALIB_OFFSET	0x44
#define EEPROM_DLL0_OFFSET		0x48
#define EEPROM_DLL1_OFFSET		0x4c
#define EEPROM_DDRIO_CALIB_OFFSET	0x50
#define EEPROM_DLL_CTRL_SEL_OFFSET	0x54
#define EEPROM_DLL_OCD_OFFSET		0x58

#elif	(CHIP_REV == A6)

#define EEPROM_INIT_DELAY_OFFSET	0x08
#define EEPROM_DRAM_CONTROL_OFFSET	0x10
#define EEPROM_DRAM_CONFIG_OFFSET	0x14
#define EEPROM_DRAM_TIMING1_OFFSET	0x18
#define EEPROM_DRAM_TIMING2_OFFSET	0x1c
#define EEPROM_DRAM_INIT_CTL_OFFSET	0x20
#define EEPROM_DRAM_EXT_MODE0_OFFSET	0x24
#define EEPROM_DRAM_EXT_MODE1_OFFSET	0x28
#define EEPROM_DRAM_EXT_MODE2_OFFSET	0x2c
#define EEPROM_DRAM_EXT_MODE3_OFFSET	0x30
#define EEPROM_DRAM_DQS_SYNC_OFFSET	0x34
#define EEPROM_DLL0_OFFSET		0x38
#define EEPROM_DLL1_OFFSET		0x3c
#define EEPROM_DLL2_OFFSET		0x40
#define EEPROM_DLL3_OFFSET		0x44
#define EEPROM_DLL_OCD_OFFSET		0x48
#define EEPROM_DDRIO_CALIB_OFFSET	0x4c

#endif

#define EEPROM_PAYLOAD_LEN_OFFSET	0x70
#define EEPROM_MEM_TARGET_OFFSET	0x74

#if	(CHIP_REV == A5S)
#define EEPROM_HEADER_MAGIC		0x00200209
#else
#define EEPROM_HEADER_MAGIC		0x90abc314
#endif
#define EEPROM_HEADER_SIZE		128

#ifndef __ASM__

__BEGIN_C_PROTO__

#if	(CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q)

struct eeprom_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int baud;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv;		/* 0x0c */
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
	unsigned int rsv1[14];		/* 0x38-0x6c */
	/* ----------------------------------------- */
	unsigned int payload_len;	/* 0x70 */
	unsigned int mem_target;	/* 0x74 */
	unsigned int rsv2[2];		/* 0x78-0x7c */
};

#elif	(CHIP_REV == A5S)

struct eeprom_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int baud;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_control;	/* 0x10 */
	unsigned int dram_config;	/* 0x14 */
	unsigned int dram_timing1;	/* 0x18 */
	unsigned int dram_timing2;	/* 0x1c */
	unsigned int dram_timing3;	/* 0x20 */
	unsigned int dram_init_ctl;	/* 0x24 */
	unsigned int dram_mode;		/* 0x28 */
	unsigned int dram_ext_mode;	/* 0x2c */
	unsigned int dram_ext_mode2;	/* 0x30 */
	unsigned int dram_ext_mode3;	/* 0x34 */
	unsigned int dram_self_ref;	/* 0x38 */
	unsigned int dram_dqs_sync;	/* 0x3c */
	unsigned int dram_pad_term;	/* 0x40 */
	unsigned int dram_zq_calib;	/* 0x44 */
	unsigned int dll0;		/* 0x48 */
	unsigned int dll1;		/* 0x4c */
	unsigned int ddrio_calib;	/* 0x50 */
	unsigned int dll_ctrl_sel;	/* 0x54 */
	unsigned int dll_ocd;		/* 0x58 */
	unsigned int rsv1[5];		/* 0x5c-0x6c */
	/* ----------------------------------------- */
	unsigned int payload_len;	/* 0x70 */
	unsigned int mem_target;	/* 0x74 */
	unsigned int rsv2[2];		/* 0x78-0x7c */
};

#elif	(CHIP_REV == A5L)

struct eeprom_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int baud;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_control;	/* 0x10 */
	unsigned int dram_config;	/* 0x14 */
	unsigned int dram_timing1;	/* 0x18 */
	unsigned int dram_timing2;	/* 0x1c */
	unsigned int dram_timing3;	/* 0x20 */
	unsigned int por_delay;		/* 0x24 */
	unsigned int dram_mode;		/* 0x28 */
	unsigned int dram_ext_mode;	/* 0x2c */
	unsigned int dram_ext_mode2;	/* 0x30 */
	unsigned int dram_ext_mode3;	/* 0x34 */
	unsigned int dram_self_ref;	/* 0x38 */
	unsigned int dram_dqs_sync;	/* 0x3c */
	unsigned int dram_pad_term;	/* 0x40 */
	unsigned int dram_zq_calib;	/* 0x44 */
	unsigned int dll0;		/* 0x48 */
	unsigned int dll1;		/* 0x4c */
	unsigned int pll_ddr_ctrl;	/* 0x50 */
	unsigned int dram_dll_ctrl;	/* 0x54 */
	unsigned int pll_ddr_ctrl2;	/* 0x58 */
	unsigned int pll_ddr_ctrl3;	/* 0x5c */
	unsigned int delay2;		/* 0x60 */
	unsigned int rsv1[3];		/* 0x64-0x6c */
	/* ----------------------------------------- */
	unsigned int rsv2[32];		/* padding for A5L */
	unsigned int payload_len;	/* 0xf0 */
	unsigned int mem_target;	/* 0xf4 */
	unsigned int rsv3[2];		/* 0xf8-0xfc */
};

#elif	(CHIP_REV == A6)

struct eeprom_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int baud;		/* 0x04 */
	unsigned int init_delay;	/* 0x08 */
	unsigned int rsv;		/* 0x0c */
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
	unsigned int rsv1[8];		/* 0x50-0x6c */
	/* ----------------------------------------- */
	unsigned int payload_len;	/* 0x70 */
	unsigned int mem_target;	/* 0x74 */
	unsigned int rsv2[2];		/* 0x78-0x7c */
};

#elif	(CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == A7L)
struct eeprom_header_s {
	unsigned int magic;		/* 0x00 */
	unsigned int baud;		/* 0x04 */
	unsigned int dram_delay;	/* 0x08 */
	unsigned int dram_cg;		/* 0x0c */
	/* ----------------------------------------- */
	unsigned int dram_timing1;	/* 0x10 */
	unsigned int dram_timing2;	/* 0x14 */
	unsigned int dram_timing3;	/* 0x18 */
	unsigned int dram_por_delay;	/* 0x1c */
	unsigned int dram_mode;	/* 0x20 */
	unsigned int dram_ext_mode1;	/* 0x24 */
	unsigned int dram_ext_mode2;	/* 0x28 */
	unsigned int dram_ext_mode3;	/* 0x2c */
	unsigned int dram_self_ref;	/* 0x30 */
	unsigned int dram_dqs_sync;	/* 0x34 */
	unsigned int dram_pad_term;		/* 0x38 */
	unsigned int dram_zq_calib;		/* 0x3c */
	unsigned int delay2;		/* 0x40 */
	unsigned int pll_ddr_ctrl_param;		/* 0x44 */
	unsigned int pll_ddr_ctrl2_param;		/* 0x48 */
	unsigned int pll_ddr_ctrl3_param;	/* 0x4c */
	unsigned int dll0;		/* 0x50 */
	unsigned int dll1;		/* 0x54 */
	unsigned int dll2;		/* 0x58 */
	unsigned int dll3;		/* 0x5c */
	unsigned int dram_dll_ctrl_sel_0_param;		/* 0x60 */
	unsigned int dram_dll_ctrl_sel_1_param;		/* 0x64 */
	unsigned int dram_dll_ctrl_sel_2_param;		/* 0x68 */
	unsigned int dram_dll_ctrl_sel_3_param;		/* 0x6c */
	unsigned int dram_dll_ctrl_sel_misc_param;		/* 0x70 */
	/* ----------------------------------------- */

	unsigned int rsv2[31];		/* 0x74-0xec */
	unsigned int payload_len;	/* 0xf0 */
	unsigned int mem_target;	/* 0xf4 */
	unsigned int rsv3[2];	 	/* 0xf8-0xfc*/
};


#endif

struct bus_i2c_s {
	/* I2C configurate parameters */
	u32 chan_id;
};

struct bus_spi_s {
	/* SPI configurate parameters */
	u32 chan_id;		/* channel id */
#define	EEPROM_DEV0_SPI_CHAN		0
#define	EEPROM_DEV1_SPI_CHAN		1

	u32 mode;		/* mode */
	u32 frame_size;		/* frame size */
	u32 baud_rate;		/* baud rate */
};

struct eeprom_s {
	u32 bus_type;
#define BUS_TYPE_SPI		0
#define BUS_TYPE_I2C		1
	union {
		struct bus_spi_s spi;
		struct bus_i2c_s i2c;
	} bus;

	/* EEPROM spec. */
	u32 rom_type;		/* EEPROM type (eeprom/flash). */
	u32 addr_size;		/* address size (2 / 3 bytes) */
	u32 chip_size;		/* EEPROM capacity size */
	u32 page_size;		/* EEPROM page size */

	/* User parameters. (Software configuration) */
	u32 rblock_size;	/* Read block size. */

	/* Read/Write function to interface */
	int (*read) (struct eeprom_s*, u32 addr, u8 *buf, int len);
	int (*write)(struct eeprom_s*, u32 addr, u8 *buf, int len);
	int (*erase)(struct eeprom_s*, u32 addr, int len);
	int (*set_status)(struct eeprom_s*, u8);
	int (*get_status)(struct eeprom_s*);
};

#define EEPROM_ID_DEV0		0
#define EEPROM_ID_DEV1		1
#define EEPROM_ID_MAX		2

/**
 * Read EEPROM data to buffer.
 *
 * @param id - id to represent EEPROM device
 * @param addr - target address in EEPROM
 * @param buf - buffer
 * @param len - length to read
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_read(int id, unsigned int addr,
		       unsigned char *buf, int len);

/**
 * Write buffer data to EEPROM.
 *
 * @param id - id to represent EEPROM device
 * @param addr - target address in EEPROM
 * @param buf - buffer
 * @param len - length to write
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_write(int id, unsigned int addr,
			unsigned char *buf, int len);

/**
 * Erase EEPROM.
 *
 * @param id - id to represent EEPROM device
 * @param addr - target address in EEPROM
 * @param len - length to write
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_erase(int id, unsigned int addr, int len);

/**
 * Set EEPROM status.
 *
 * @param id - id to represent EEPROM device
 * @param val - EEPROM status
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_set_status(int id, unsigned char val);

/**
 * Get EEPROM status.
 *
 * @param id - id to represent EEPROM device
 * @return - EEPROM status
 */
extern int eeprom_get_status(int id);

/**
 * Program binary code in buffer to EEPROM with data check.
 *
 * @param id - id to represent EEPROM device
 * @param payload - payload
 * @param payload_len - payload length
 * @param prgs_rpt - the function to report program progress
 * @param arg - the argument of prgs_rpt function
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_prog(int id, unsigned char *payload,
		       unsigned int payload_len,
		       int (*prgs_rpt)(int, void *), void *arg);

/**
 * Get EEPROM object.
 *
 * @param id - id to represent EEPROM device
 * @return - EEPROM configuration.
 */
extern struct eeprom_s *get_eeprom(int id);

/**
 * Get EEPROM object.
 *
 * @param id 	- id to represent EEPROM device
 * @param devid - memory to store SPI FLASH device ID
 * @return - 0 if success, < 0 otherwise
 */
extern int eeprom_read_id(int id, u16 * devid);

__END_C_PROTO__

#endif  /* __ASM__ */

#endif
