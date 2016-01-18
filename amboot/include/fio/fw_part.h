/**
 * @file system/include/fio/fw_part.h
 *
 * History:
 *    2009/03/24 - [Evan(Kuan-Fu) Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FW_PART_H__
#define __FW_PART_H__

#define FW_PART_MAX_BB		126

typedef struct fw_part_bb_s{
	u32 nbb;
	u32 *bb_info;
	u32 *bb_info_raw;
	u32 is_scan;
} fw_part_bb_t;

#ifdef NAND_BB_PRE_SCAN
/* Pre-scan nand bad block and save to first page in  */
/* partition end block. 			      */
/* In PRI only reads a page to get partition bb info, */
/* so size of bb_pre_scan_t must smaller then a page. */
typedef struct bb_pre_scan_s {
	u32 nbb;
	u32 magic;
	u32 bb_info[FW_PART_MAX_BB];
} bb_pre_scan_t;
#define BB_PRE_SCAN_MAGIC	0x55667788
#endif

__BEGIN_C_PROTO__

extern void fw_part_nand_bb_scan(u32 id);
extern u32 fw_part_is_bad_blk(u32 id, u32 block);
extern u32 fw_part_chk_bb(u32 id, u32 s_blk, u32 e_blk);
extern int fw_part_nand_read(u32 block, u32 page, u32 pages, u8 *buf);

extern void fw_part_get_offset_adr(u32 id, u32 *block, u32 *page, u32 pages);

/**
 * Get partition start address on the device
 *
 * @param id - firmwarw partition ID.
 * @return - phyical address on boot device.
 */
extern u32 fw_get_part_addr(u32 id);

/**
 * Check firmware partition exist or not.
 *
 * @param id - Firmware partition id.
 * @returns - 0 for success, others for fail.
 */
extern int fw_part_check(u32 id);

/**
 * Read data from firmware partition
 *
 * @param dst - dst memory address.
 * @param src - Phy address in firmware bin file.
 * @returns - 0 for success, others for fail.
 */
extern int fw_part_pid_read(u32 pid, u32 offset, u8 *dst, int len);

__END_C_PROTO__

#endif /* __FW_PART_H__ */

