/**
 * system/src/fio/sm.h
 *
 * History:
 *    2008/07/21 - [Chien-Yang Chen] created file
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __SM_H__
#define __SM_H__

#include <basedef.h>
#include <fio/partition.h>

#define SM_SECTOR_SIZE	512

#define SM_SECTOR_ALIGN	8192

typedef struct smpart_s {
	u32 ssec;
	u32 nsec;
} smpart_t;

typedef struct smdev_s {
	smpart_t part[PART_MAX];
	int init;
	u32 slot;			/* Slot to contain the firmware */
	u32 sector_size;
	u32 total_sectors;		/* Total sectors in media */
	int stg2_format;		/* File system type of storge2 */
	/* Read/Write function to interface with the slot of sector media */
	int (*read) (u8 *buf, u32 sector, u32 sectors);
	int (*write)(u8 *buf, u32 sector, u32 sectors);
} smdev_t;

/**
 * Initialize the Setor-Media device.
 */
extern int sm_dev_init(int slot);
extern smdev_t *sm_get_dev(void);
extern smpart_t *sm_get_part(int id);

/***********************************************************************/
/* The functions are for specific partition and compatiable with nftl. */
/***********************************************************************/
extern int sm_is_init(int id);
extern int sm_init(int id, int mode);
extern u32 sm_get_total_sectors(int id);
extern u32 sm_slot_get_total_sectors(int slot);
extern int sm_read (int id, u8 *buf, u32 sector, u32 sectors);
extern int sm_write(int id, u8 *buf, u32 sector, u32 sectors);
extern int sm_erase_partition(int id, int mode);
extern void sm_stg2_set_host(void);
extern u32 stg2_get_total_sectors(void);

/*****************************************************************/
/* Check firmware container device and get the start sector      */
/* corresponding to pid. This macro is for PrFILE drivers use.   */
/*****************************************************************/
#define  FIRMWARE_CONTAINER_CHECK(slot, pid) {				    \
	if (firmware_container() == slot) {				    \
		smpart_t *smpart = sm_get_part(pid);			    \
									    \
		sector += smpart->ssec;					    \
		if ((sector + sectors) > (smpart->ssec + smpart->nsec)) {   \
			printk("Slot %d of SM partition %d cross boundary", \
				slot, pid);				    \
			return -1;				    	    \
		}							    \
	}								    \
}

#define  FIRMWARE_CONTAINER_GET_LSEC(slot, pid) {			    \
	if (firmware_container() == slot) {				    \
		smpart_t *smpart = sm_get_part(pid);			    \
									    \
		sec -= smpart->ssec;					    \
	}								    \
}

#endif

