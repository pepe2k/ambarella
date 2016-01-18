/**
 * @file system/include/flash/snor.h
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

#ifndef __FLASH_SNOR__
#define __FLASH_SNOR__

#include <basedef.h>
#include <ambhw.h>
#include <flash/snor/command.h>

#ifndef __ASM__

__BEGIN_C_PROTO__

#define SNOR_AMB_CMD_RESET			0x0
#define SNOR_AMB_CMD_AUTOSEL			0x1
#define SNOR_AMB_CMD_CFI_QUERY			0x2
#define SNOR_AMB_CMD_READ			0x3
#define SNOR_AMB_CMD_WRITE			0x4
#define SNOR_AMB_CMD_UNLOCK_BYPASS		0x5
#define SNOR_AMB_CMD_UNLOCK_PROG		0x6
#define SNOR_AMB_CMD_UNLOCK_BLOCK_ERASE		0x7
#define SNOR_AMB_CMD_UNLOCK_CHIP_ERASE		0x8
#define SNOR_AMB_CMD_UNLOCK_RESET		0x9
#define SNOR_AMB_CMD_CHIP_ERASE			0xa
#define SNOR_AMB_CMD_BLOCK_ERASE		0xb
#define SNOR_AMB_CMD_BLOCK_PROTECT		0xc
#define SNOR_AMB_CMD_BGROUP_PROTECT		0xd
#define SNOR_AMB_CMD_SET_BURST_CFG		0xe
#define SNOR_AMB_CMD_MAX			0xf

typedef struct snor_cmd_s {
	struct snor_cmd_basic_s {
		u16 addr[SNOR_MAX_CMD_CYC];
		u16 data[SNOR_MAX_ADDR_CYC];
		u8  cmd_cyc;
		u8  data_cyc;
		int timeout;
	} basic[SNOR_AMB_CMD_MAX];

	struct snor_autosel_extra_s {
		u16 dev_id_len;
		u16 addr4_dev_id[3];
		u16 addr4_sector_prot_vrfy;
		u16 addr4_secure_dev_vrfy;
		u16 addr4_manu_id;
	} autosel_extra;
} snor_cmd_t;

typedef struct snor_dev_ctr_s {
	snor_cmd_t cmd;

	u8 poll_mode;
#define SNOR_POLL_MODE_RDY_BUSY		1
#define SNOR_POLL_MODE_DATA_POLL	2

	u8 data_poll_mode;
#define SNOR_DATA_POLL_MODE_EQ_0	0
#define SNOR_DATA_POLL_MODE_EQ_1	1
#define SNOR_DATA_POLL_MODE_EQ_DATA	2

	u8 data_poll_target_bit;	/* data poll or toggposition */
	u8 data_poll_fail_polarity;	/* poll fail polarity */
	u8 data_poll_fail_bit;		/* poll fail position */

	u32 poll_ctr1;		/* poll mode setting in ctr1 */
	u32 poll_ctr2;		/* poll mode setting in ctr2 */
	u32 snor_ctr1;
	u32 snor_ctr2;
} snor_dev_ctr_t;

/* ------------------------------------------------------------------------- */
/* NOR HOST                                                                  */
/* ------------------------------------------------------------------------- */

/**
 * NOR chip(s) host controller.
 */
typedef struct snor_host_s
{
	nor_dev_t	dev;		/**< NOR device chip(s). */
	snor_dev_ctr_t	dev_ctr;

	/**
	 * Reset NOR chip
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure
	 */
	int (*reset)(struct snor_host_s *host, nor_dev_t *dev, u8 bank);

	/**
	 * Pre-init autoselect (Read ID).
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @param addr - The address of autoselect mode (present on addr bus)
	 * @param buf - Buffer for autoselect result.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*pre_init_autoselect)(struct snor_host_s *host, nor_dev_t *dev,
				  u8 bank, u32 addr, u16 *buf);

	/**
	 * Autoselect (Read ID).
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @param addr - The address of autoselect mode (present on addr bus)
	 * @param buf - Buffer for autoselect result.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*autoselect)(struct snor_host_s *host, nor_dev_t *dev,
			  u8 bank, u32 addr, u16 *buf);

	/**
	 * Read data from NOR chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param addr - Addr in NOR
	 * @param buf - Buffer for read operation result.
	 * @param len - Length of data to read.
	 * @param mode - Read mode
	 */
	int (*read)(struct snor_host_s *host, nor_dev_t *dev,
		    u32 addr, u8 *buf, u32 len, u8 mode);
#define SNOR_READ_DMA		0
#define SNOR_READ_ASYNC		1
#define SNOR_READ_SYNC		2

	/**
	 * Write (program) data to NOR chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param addr - Addr in NOR
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 * @param mode - Write mode
	 */
	int (*write)(struct snor_host_s *host, nor_dev_t *dev,
		     u32 addr, const u8 *buf, u32 len, u8 mode);
#define SNOR_WRITE_DMA			0
#define SNOR_WRITE_ASYNC		1
#define SNOR_WRITE_ASYNC_UNLOCK		2

	/**
	 * Erase a block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param block - Block number in NOR
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*erase)(struct snor_host_s *host, nor_dev_t *dev, u32 block);

	/**
	 * Erase chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*erase_chip)(struct snor_host_s *host, nor_dev_t *dev, u8 bank);

	/**
	 * Enter unlock bypass mode.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock_bypass)(struct snor_host_s *host, nor_dev_t *dev, u8 bank);


	/**
	 * Exit unlock bypass mode.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock_reset)(struct snor_host_s *host, nor_dev_t *dev, u8 bank);

	/**
	 * Unlock bypass program.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param addr - Addr in NOR
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock_write)(struct snor_host_s *host, nor_dev_t *dev,
			    u32 addr, u8 *buf, u32 len);
	/**
	 * Unlock bypass erase block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param block - Block number in NOR
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock_erase)(struct snor_host_s *host, nor_dev_t *dev, u32 block);

	/**
	 * Unlock bypass erase chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock_erase_chip)(struct snor_host_s *host, nor_dev_t *dev,
				 u8 bank);

	/**
	 * Set burst mode configuration register.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - The bank which CE select
	 * @param cfg - Configuration data.
	 */
	int (*set_burst_cfg)(struct snor_host_s *host, nor_dev_t *dev,
				u8 bank, u32 cfg);

	/**
	 * Protect block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param block - Block number in NOR.
	 * @param mode - Protect or unprotect
	 */
	int (*protect_block)(struct snor_host_s *host, nor_dev_t *dev,
			     u32 block, u8 mode);
#define SNOR_PROTECT_BLOCK	0
#define SNOR_UNPROTECT_BLOCK	1

	/**
	 * Protect block group.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param block - Block number in NOR.
	 * @param mode - Protect or unprotect
	 */
	int (*protect_block_group)(struct snor_host_s *host, nor_dev_t *dev,
				     u32 block, u8 mode);
#define SNOR_PROTECT_BLOCK_GROUP	0
#define SNOR_UNPROTECT_BLOCK_GROUP	1

	/**
	 * Enter CFI query mode.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
 	 * @param bank - The bank which CE select
	 */
	int (*cfi_query)(struct snor_host_s *host, nor_dev_t *dev, u8 bank);

	/**
  	 * Set timing operation.
	 * @param host - The host.
	 * @parem dev - NOR device.
	 */
	void (*set_timing)(struct snor_host_s *host, nor_dev_t *dev);

} snor_host_t;


#ifdef __SNOR_HOST_IMPL__

/**
 * This function sets the default controller for the system
 * (which may be retrieved by the call 'nor_get_host()' later).
 *
 * @param host - The NOR host controller.
 * @see nor_get_host
 */
extern void snor_set_host(snor_host_t *host);

extern int snor_init_dev(snor_host_t *host, nor_dev_t *dev);

extern int snor_deconf_dev(snor_host_t *host);

#endif

/**
 * Check to see if nor is initial.
 */
extern int snor_is_init(void);

extern snor_host_t *snor_get_host(void);

extern nor_dev_t *snor_get_dev(void);

#ifndef __BLDFUNC_H__

extern u32 snor_blocks_to_len(nor_dev_t *dev, u32 block, u32 blocks);

extern u32 snor_len_to_blocks(nor_dev_t *dev, u32 start_block, u32 len);

extern u32 snor_get_addr(nor_dev_t *dev, u32 block ,u32 offset);

extern void snor_get_block_offset(nor_dev_t *dev, u32 addr,
					u32 *block, u32 *offset);
#endif

__END_C_PROTO__

#endif /* __ASM__ */

#endif

