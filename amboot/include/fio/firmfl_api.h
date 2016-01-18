/**
 * @file system/include/fio/firmfl_api.h
 *
 * Embedded Flash and Firmware Programming APIs
 *
 * History:
 *    2007/03/30 - [Chien-Yang Chen] created file
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FIRMFL_API_H__
#define __FIRMFL_API_H__

#include <basedef.h>
#include <fio/firmfl.h>
#include <sm.h>

__BEGIN_C_PROTO__

/********************/
/* NAND firmfl APIs */
/********************/

extern int memcmp_oob(const void *dst, const void *src, u32 main_size, u32 spare_size);
extern int firmfl_nand_init(void);
extern int prog_firmware(u32 addr, u32 len, u32 flag);
extern int prog_firmware_file(const char *name, u32 flag,
			      int (*prgs_rpt)(int, void *), void *parg);
extern int prog_firmware_file_only(const char *name, u32 flag,
				   int (*prgs_rpt)(int, void *), void *parg);
extern int prog_firmware_fifo(int fifodes, int fifosize, u32 flag,
			      int (*prgs_rpt)(int, void *), void *parg);
extern int prog_firmware_fifo_retcode(void);
extern int gen_firmware_file(const char *name, u32 nfirms, char **argv,
		     	     int (*prgs_rpt)(int, void *), void *parg);
extern int prog_firmware_preprocess(const char *name, u32 flag,
			     	    int (*prgs_rpt)(int, void *), void *parg);

extern int prog_firmware_postprocess(const char *name, u32 flag,
			      	     int (*prgs_rpt)(int, void *), void *parg);
extern int flprog_erase_partition(int pid);

extern int flprog_update_part_info_from_meta(void);

/* Partition IDs are defined in partition.h */
extern int flprog_get_part_table(flpart_table_t *table);
extern int flprog_set_part_table(flpart_table_t *table);

/********************/
/* NOR firmfl APIs */
/********************/

extern int firmfl_nor_init(void);

/*********************/
/* SNOR firmfl APIs */
/*********************/

extern int firmfl_snor_init(void);

__END_C_PROTO__

#endif
