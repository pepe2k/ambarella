/**
 * @file system/include/flash/slcnand.h
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

#ifndef __FLASH_SLCNAND_H__
#define __FLASH_SLCNAND_H__

#include <basedef.h>
#include <ambhw.h>
#include <flash/nand_common.h>

/* ------------------------------------------------------------------------- */
/* define for flash_amb                                                      */
/* ------------------------------------------------------------------------- */

#define EC_MDSD		0	/* check main disable and spare disable */
#define EC_MDSE		1	/* check main disable and spare enable */
#define EC_MESD		2	/* check main enable and spare disable */
#define EC_MESE		3	/* check main enable and spare enable */

#define EG_MDSD		0	/* generate main disable and spare disable */
#define EG_MDSE		1	/* generate main disable and spare enable */
#define EG_MESD		2	/* generate main enable and spare disable */
#define EG_MESE		3	/* generate main enable and spare enable */

/* Macros for filed of operation in set_cmdword()*/
#define RESET_ALL_CMD_WORD	0
#define RESET_PROG_CMD_WORD	1
#define RESET_READ_CMD_WORD	2
#define SET_PROG_CMD_WORD	3
#define SET_READ_CMD_WORD	4

/* Macros for fileds of cmdwordx in set_cmdword()*/
#define CMD_READ_1ST				0x00
#define CMD_READ_2ND				0x30

#define CMD_PROG_1ST				0x80
#define CMD_PROG_2ND				0x10

#define CMD_READ_FOR_PAGE_COPY_1ST		0x00
#define CMD_READ_FOR_PAGE_COPY_2ND		0x3a

#define CMD_PROG_WITH_CACHE_FOR_PAGE_COPY_1ST	0x8c
#define CMD_PROG_WITH_CACHE_FOR_PAGE_COPY_2ND	0x15

#define CMD_PROG_FOR_PAGE_COPY_1ST		0x8c
#define CMD_PROG_FOR_PAGE_COPY_2ND		0x10

#define CMD_PROG_WITH_CACHE_1ST			0x80
#define CMD_PROG_WITH_CACHE_2ND			0x15

#ifndef __ASM__

__BEGIN_C_PROTO__

/**
 * NAND host controller.
 */
typedef struct nand_host_s
{
	nand_dev_t dev;		/**< The NAND device chip(s) */

	/**
	 * Reset NAND chip
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param bank - The bank which CE select
	 * @returns - 0 successful, < 0 failure
	 */
	int (*reset)(struct nand_host_s *host, nand_dev_t *dev, u8 bank);

	/**
	 * Read ID.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param id - ID variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_id)(struct nand_host_s *host, nand_dev_t *dev,
		       int addr_hi, u32 addr, u32 *id);

	/**
	 * Pre-init read ID.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param id - ID variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*pre_init_read_id)(struct nand_host_s *host, nand_dev_t *dev,
				int addr_hi, u32 addr, u32 *id);

	/**
	 * Read status.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param status - The status variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_status)(struct nand_host_s *host, nand_dev_t *dev,
			   u32 addr_hi, u32 addr, u32 *status);

	/**
	 * Read EDC status.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param status - The status variable to read.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read_edc_status)(struct nand_host_s *host, nand_dev_t *dev,
			       u32 addr_hi, u32 addr, u32 *status);

	/**
	 * Read auto status.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @returns - 0 successful, < 0 failure
	 */
	int (*read_auto_status)(struct nand_host_s *host, nand_dev_t *dev,
				u32 *status);


	/**
	 * Copyback an area.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param dst - Destination address.
	 * @param len - Length to copy.
	 * @param area - 0 main, 1 spare
	 * @param ecc - Enable ECC.
	 * @param intlve - number of interleave. Only 1, 2 ,4 is allowed.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*copyback)(struct nand_host_s *host, nand_dev_t *dev,
			u32 addr_hi, u32 addr,
			u32 dst, u8 intlve);

	/**
	 * Erase a block.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param intlve - number of interleave. Only 1, 2 ,4 is allowed.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*erase)(struct nand_host_s *host, nand_dev_t *dev,
		     u32 addr_hi, u32 addr, u8 intlve);

	/**
	 * Read operation.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param buf - Memory buffer to read to.
	 * @param len - Length to read.
	 * @param area - 0 main, 1 spare
	 * @param ecc - Enable ECC.
	 * @param intlve - number of interleave. Only 1, 2 ,4 is allowed.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*read)(struct nand_host_s *host, nand_dev_t *dev,
		    u32 addr_hi, u32 addr,
		    u8 *buf, u32 len, u8 area, u8 ecc, u8 intlve);

	/**
	 * Burst read operation.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param buf - Memory buffer to read to.
	 * @param len - Length to read.
	 * @param nbr - page number for spare burst read
	 * @param area - 0 main, 1 spare
	 * @param ecc - Enable ECC.
	 * @param intlve - number of interleave. Only 1, 2 ,4 is allowed.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*burst_read)(struct nand_host_s *host, nand_dev_t *dev,
		    u32 addr_hi, u32 addr,
		    u8 *buf, u32 len, u8 nbr, u8 area, u8 ecc, u8 intlve);

	/**
	 * Write (program) operation.
	 *
	 * @param host - The host.
	 * @param dev - NAND device.
	 * @param addr_hi -  [33:32] address.
	 * @param addr - [31:0] address.
	 * @param buf - Memory buffer to write from.
	 * @param len - Length to write.
	 * @param area - 0 main, 1 spare
	 * @param ecc - Enable ECC.
	 * @param intlve - number of interleave. Only 1, 2 ,4 is allowed.
	 * @returns - 0 successful, < 0 failure.
	 */
	int (*write)(struct nand_host_s *host, nand_dev_t *dev,
		     u32 addr_hi, u32 addr,
		     const u8 *buf, u32 len, u8 area, u8 ecc, u8 intlve);

	/**
	 * Set read/write command word operation.
	 * @param host - The host.
	 * @parem dev - NAND device.
	 * @parem operation - The operation corresponding to command words.
	 * @parem cmdword1 - Command word1.
	 * @parem cmdword2 - Command word2.
	 */
	void (*set_cmdword)(struct nand_host_s *host, nand_dev_t *dev,
			int operation, u8 cmdword1, u8 cmdword2);

	/**
	 * Set timing operation.
	 * @param host - The host.
	 * @parem dev - NAND device.
	 */
	void (*set_timing)(struct nand_host_s *host, nand_dev_t *dev);

} nand_host_t;

#ifdef __NAND_HOST_IMPL__

/**
 * This function sets the default controller for the system
 * (which may be retrieved by the call 'slcnand_get_host()' later).
 *
 * @param host - The NAND host controller.
 * @see slcnand_get_host
 */
extern void slcnand_set_host(nand_host_t *host);

/**
 * Initialize NAND device attached to controller.
 */
extern int slcnand_init_dev(nand_host_t *host, nand_dev_t *dev);

/**
 * Deconfigure NAND device attached to the host.
 */
extern int slcnand_deconf_dev(nand_host_t *host);

#endif

/**
 * Check to see if nand is initial.
 */
extern int slcnand_is_init(void);

extern nand_host_t *slcnand_get_host(void);

__END_C_PROTO__

#endif /* __ASM__ */

#endif
