/**
 * @file system/include/flash/inor.h
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

#ifndef __FLASH_INOR_H__
#define __FLASH_INOR_H__

#include <basedef.h>
#include <ambhw.h>

/* The high byte is manufature code, low byte is device code */

#define ID_E28F320	0x8916
#define ID_E28F640	0x8917
#if 0	/* CAN'T READ THE CORRECT DEVICE CODE!! */
#define ID_E28F128	0x8918
#else
#define ID_E28F128	0x8989
#endif
#define ID_JS28F320	0x8916	/* 32 Mbit */
#define ID_JS28F640	0x8917	/* 64 Mbit */
#define ID_JS28F128	0x8918	/* 128 Mbit */
#define ID_JS28F256	0x891D	/* 256 Mbit */

#ifndef __ASM__

__BEGIN_C_PROTO__

/* ------------------------------------------------------------------------- */
/* NOR HOST                                                                  */
/* ------------------------------------------------------------------------- */

/**
 * NOR chip(s) host controller.
 */
typedef struct nor_host_s
{
	nor_dev_t	dev;	/**< NOR device chip(s). */

	/**
	 * Reset NOR chip
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
 	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure
	 */
	int (*reset)(struct nor_host_s *host, nor_dev_t *dev, u8 bank);

	/**
	 * Read ID.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @param id - ID variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_id)(struct nor_host_s *host, nor_dev_t *dev,
		       u8 bank, u32 addr, u32 *id);

	/**
	 * Read status.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @param status - The status variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_status)(struct nor_host_s *host, nor_dev_t *dev,
			   u8 bank, u32 addr, u32 *status);

	/**
	 * Clear status.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*clear_status)(struct nor_host_s *host, nor_dev_t *dev,
			    u8 bank, u32 addr);

	/**
	 * Lock a block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*lock)(struct nor_host_s *host, nor_dev_t *dev,
		    u8 bank, u32 addr);

	/**
	 * Unlock a block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*unlock)(struct nor_host_s *host, nor_dev_t *dev,
		      u8 bank, u32 addr);

	/**
	 * Lock down a block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*lockdown)(struct nor_host_s *host, nor_dev_t *dev,
			u8 bank, u32 addr);

	/**
	 * Erase a block.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*erase)(struct nor_host_s *host, nor_dev_t *dev,
		     u8 bank, u32 addr);

	/**
	 * Read in the CFI structure area.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @param buf - Buffer for read operation result.
	 * @param len - Length of data to read.
	 */
	int (*read_cfi)(struct nor_host_s *host, nor_dev_t *dev,
			u8 bank, u32 addr, u32 *cfi);

	/**
	 * Read data from NOR chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @param buf - Buffer for read operation result.
	 * @param len - Length of data to read.
	 */
	int (*read)(struct nor_host_s *host, nor_dev_t *dev,
		    u8 bank, u32 addr, u8 *buf, int len);

	/**
	 * Write (program) data to NOR chip.
	 *
	 * @param host - The host.
	 * @param dev - NOR device.
	 * @param bank - [31:30] address.
	 * @param addr - [29:4] address.
	 * @param buf - Buffer for write operation result.
	 * @param len - Length of data to write.
	 */
	int (*write)(struct nor_host_s *host, nor_dev_t *dev,
		     u8 bank, u32 addr, const u8 *buf, int len);

	/**
  	 * Set timing operation.
	 * @param host - The host.
	 * @parem dev - NOR device.
	 */
	void (*set_timing)(struct nor_host_s *host, nor_dev_t *dev);

} nor_host_t;


#ifdef __NOR_HOST_IMPL__

/**
 * This function sets the default controller for the system
 * (which may be retrieved by the call 'nor_get_host()' later).
 *
 * @param host - The NOR host controller.
 * @see nor_get_host
 */
extern void inor_set_host(nor_host_t *host);

extern int inor_init_dev(nor_host_t *host, nor_dev_t *dev);

extern int inor_deconf_dev(nor_host_t *host);

#endif

/**
 * Check to see if nor is initial.
 */
extern int inor_is_init(void);

extern nor_host_t *inor_get_host(void);

__END_C_PROTO__

#endif /* __ASM__ */

#endif

