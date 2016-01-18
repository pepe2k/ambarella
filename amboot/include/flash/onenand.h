/**
 * @file system/include/flash/onenand.h
 *
 * History:
 *	2009/08/13 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FLASH_ONENAND_H__
#define __FLASH_ONENAND_H__

#include <basedef.h>
#include <ambhw.h>
#include <flash/nand_common.h>
#include <flash/onenand/command.h>


/* ONE NAND PROG/READ mode support(for device) */
#define ONENAND_SUPPORT_ASYNC_MODE		0x0
#define ONENAND_SUPPORT_DUAL_MODE		0x1
#define ONENAND_SUPPORT_SYNC_MODE		0x2
#define ONENAND_SUPPORT_PROG_2X			0x4
#define ONENAND_SUPPORT_PROG_2X_CACHE		0x8

#define ONENAND_AMB_CMD_READ_REG		0x0
#define ONENAND_AMB_CMD_WRITE_REG		0x1
#define ONENAND_AMB_CMD_POLL_REG		0x2
#define ONENAND_AMB_CMD_READ_ASYNC		0x3
#define ONENAND_AMB_CMD_READ_SPARE_ASYNC	0x4
#define ONENAND_AMB_CMD_READ_SYNC		0x5
#define ONENAND_AMB_CMD_READ_SPARE_SYNC		0x6
#define ONENAND_AMB_CMD_PROG_ASYNC		0x7
#define ONENAND_AMB_CMD_PROG_SPARE_ASYNC	0x8
#define ONENAND_AMB_CMD_PROG_DUAL		0x9
#define ONENAND_AMB_CMD_PROG_DUAL_SPARE_ASYNC	0xa
#define ONENAND_AMB_CMD_PROG_SYNC		0xb
#define ONENAND_AMB_CMD_PROG_SPARE_SYNC		0xc
#define ONENAND_AMB_CMD_PROG_2X_ASYNC		0xd
#define ONENAND_AMB_CMD_PROG_2X_SYNC		0xe
#define ONENAND_AMB_CMD_PROG_2X_CACHE_ASYNC	0xf
#define ONENAND_AMB_CMD_PROG_2X_CACHE_SYNC	0x10
#define ONENAND_AMB_CMD_MAX			0x11

#define ONENAND_POLL_MODE_NONE			0
#define ONENAND_POLL_MODE_INT_SIGNAL		3
#define ONENAND_POLL_MODE_REG_POLL		4
#define ONENAND_POLL_MODE_REG_INT_POLL		5

#define ONENAND_PROG_MODE_NORMAL		0
#define ONENAND_PROG_MODE_2X			1
#define ONENAND_PROG_MODE_2X_CACHE		2
#define ONENAND_PROG_MODE_DUAL			3

#ifndef __ASM__

__BEGIN_C_PROTO__

typedef struct onenand_cmd_s {
	struct onenand_cmd_basic_s {
		u16 addr[ONENAND_MAX_CMD_CYC];
		u16 data[ONENAND_MAX_ADDR_CYC];
		u8  cmd_cyc;
		u8  data_cyc;
		u8  poll_mode;
		u8  prog_mode;
		int timeout;
	} basic[ONENAND_AMB_CMD_MAX];
} onenand_cmd_t;

typedef struct onenand_dev_ctr_s {
	onenand_cmd_t cmd;

	u8 data_poll_target_bit;	/* data poll or toggposition */
	u8 read_mode;

	u8 prog_mode;
#define ONENAND_PROG_MODE_NORMAL_ASYNC		0
#define ONENAND_PROG_MODE_DUAL_ASYNC		1
#define ONENAND_PROG_MODE_NORMAL_SYNC		2
#define ONENAND_PROG_MODE_2X_ASYNC		3
#define ONENAND_PROG_MODE_2X_SYNC		4
#define ONENAND_PROG_MODE_2X_CACHE_ASYNC	5
#define ONENAND_PROG_MODE_2X_CACHE_SYNC		6

	u32 poll_ctr1;		/* poll mode setting in ctr1 */
	u32 poll_ctr2;		/* poll mode setting in ctr2 */
	u32 onenand_ctr1;
	u32 onenand_ctr2;
} onenand_dev_ctr_t;

/**
 * ONENAND host controller.
 */
typedef struct onenand_host_s
{
	 nand_dev_t dev;		/**< The ONENAND device chip(s) */
	 onenand_dev_ctr_t dev_ctr;

	/**
	 * Reset ONENAND chip
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param bank - The number of device flash core
	 * @param mode - warm or hot reset
	 * @returns - 0 successful, < 0 failure
	 */
	int (*reset)(struct onenand_host_s *host, nand_dev_t *dev,
			u8 bank, int mode);
#define ONENAND_WARM_RESET	0
#define ONENAND_HOT_RESET	1

	/**
	 * Read register.
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param bank - The number of device flash core
	 * @param addr - Address of register in device
	 * @param reg - Buffer of read register result
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_reg)(struct onenand_host_s *host, nand_dev_t *dev,
		        u8 bank, u32 addr, u16 *reg);

	/**
	 * Write register.
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param bank - The number of device flash core
	 * @param addr - Address of register in device
	 * @param reg - Buffer of write register result
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*write_reg)(struct onenand_host_s *host, nand_dev_t *dev,
		        u8 bank, u32 addr, u16 *reg);

	/**
	 * Poll single register.
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param bank - The number of device flash core
	 * @param addr - Address of register in device
	 * @param pos - Bit position of polled register
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*poll_reg)(struct onenand_host_s *host, nand_dev_t *dev,
		        u8 bank, u32 addr, u8 pos);

	/**
	 * Read data from ONENAND chip
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param addr - Addr in ONENAND
	 * @param buf - Buffer for read operation result.
	 * @param len - Length of data to read.
	 * @param mode - Read mode
	 */
	int (*read)(struct onenand_host_s *host, nand_dev_t *dev,
		     u32 addr, const u8 *buf, u32 len, u8 mode);
#define ONENAND_MAIN_SYNC		0
#define ONENAND_MAIN_ASYNC		1
#define ONENAND_SPARE_SYNC		2
#define ONENAND_SPARE_ASYNC		3

	/**
	 * Write data to ONENAND chip
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param addr - Addr in ONENAND
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 * @param mode - Write mode (same as read)
	 */
	int (*write)(struct onenand_host_s *host, nand_dev_t *dev,
		     u32 addr, const u8 *buf, u32 len, u8 mode);

	/**
	 * Write (2X program) data to ONENAND chip
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param addr - Addr in ONENAND
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 * @param mode - Write mode (same as read)
	 */
	int (*write_2x)(struct onenand_host_s *host, nand_dev_t *dev,
		     	u32 addr, const u8 *buf, u32 len, u8 mode);

	/**
	 * Write (2X cache program) data to ONENAND chip
	 *
	 * @param host - The host.
	 * @param dev - ONENAND device.
	 * @param addr - Addr in ONENAND
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 * @param mode - Write mode (same as read)
	 */
	int (*cache_write_2x)(struct onenand_host_s *host, nand_dev_t *dev,
		     		u32 addr, const u8 *buf, u32 len, u8 mode);

	/**
	 * Set timing operation.
	 * @param host - The host.
	 * @parem dev - ONENAND device.
	 */
	void (*set_timing)(struct onenand_host_s *host, nand_dev_t *dev);

} onenand_host_t;

#ifdef __ONENAND_HOST_IMPL__

/**
 * This function sets the default controller for the system
 * (which may be retrieved by the call 'onenand_get_host()' later).
 *
 * @param host - The ONENAND host controller.
 * @see onenand_get_host
 */
extern void onenand_set_host(onenand_host_t *host);

/**
 * Initialize ONENAND device attached to controller.
 */
extern int onenand_init_dev(onenand_host_t *host, nand_dev_t *dev);

/**
 * Deconfigure ONENAND device attached to the host.
 */
extern int onenand_deconf_dev(onenand_host_t *host);

#endif

/**
 * Check to see if nand is initial.
 */
extern int onenand_is_init(void);

extern onenand_host_t *onenand_get_host(void);

__END_C_PROTO__

#endif /* __ASM__ */

#endif
