/**
 * @file system/src/bld/sm.c
 *
 * Embedded sector media and Firmware Programming Utilities
 *
 * History:
 *    2008/07/16 - [Dragon Chiang] created file
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

#ifdef __BUILD_AMBOOT__
#include <bldfunc.h>
#else
#include <kutil.h>
#endif

#include <sm.h>

#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>
#include "partinfo.h"
#include <fio/sdboot.h>
#include <peripheral/eeprom.h>
#include <sdmmc.h>

#define	SM_SECTOR_SIZE	512

static u8 sector_buffer[SM_SECTOR_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));

static smdev_t G_smdev;

static u32 sm_dev_get_total_sectors(int slot)
{
	u32 nsec = 0;

	switch (slot) {
#if defined(ENABLE_CF)
	case SCARDMGR_SLOT_CF: {
		break;
	}
#endif

#if defined(ENABLE_SD)
	case SCARDMGR_SLOT_SD: {
		nsec = sdmmc_get_total_sectors();
		break;
	}
#if (SD_INSTANCES == 2)
	case SCARDMGR_SLOT_SD2: {
		nsec = sdmmc_get_total_sectors();
		break;
	}
#endif
#if (SD_HAS_INTERNAL_MUXER == 1) || (SD_HAS_EXTERNAL_MUXER == 1)
	case SCARDMGR_SLOT_SDIO: {
		nsec = sdmmc_get_total_sectors();
		break;
	}
#endif
#endif

#if (ENABLE_MS)
	case SCARDMGR_SLOT_MS: {
		break;
	}
	case SCARDMGR_SLOT_CFMS: {
		break;
	}
	case SCARDMGR_SLOT_CFMS2: {
		break;
	}
#endif
	default:
		goto done;
	}

done:
	return nsec;
}

static int _sm_dev_init(int slot, smdev_t *smdev)
{
	int rval = -1;

	smdev->slot = slot;
	smdev->sector_size = SM_SECTOR_SIZE;

	switch (slot) {
#if defined(ENABLE_CF)
	case SCARDMGR_SLOT_CF:
		break;
#endif	/* ENABLE_CF */

#if defined(ENABLE_SD)
	case SCARDMGR_SLOT_SD:
	case SCARDMGR_SLOT_SDIO:
 #if (SD_INSTANCES == 2)
	case SCARDMGR_SLOT_SD2:
 #endif	/* SD_INSTANCES */

 #if defined(FIRMWARE_CONTAINER_TYPE)
		rval = sdmmc_init(slot, FIRMWARE_CONTAINER_TYPE);
 #else
		rval = sdmmc_init(slot, SDMMC_TYPE_AUTO);
 #endif	/* FIRMWARE_CONTAINER_TYPE */
		break;
#endif	/* ENABLE_SD */

#if defined(ENABLE_MS)
	case SCARDMGR_SLOT_MS:
	case SCARDMGR_SLOT_CFMS:
	case SCARDMGR_SLOT_CFMS2:
		break;
#endif	/* ENABLE_MS */

	default:
		rval = -1;
		goto done;
	}

	smdev->total_sectors = sm_dev_get_total_sectors(slot);

done:
	return rval;
}

/**
 * Initialize sector media parameters.
 */
int sm_dev_init(int slot)
{
	int ssec, nsec;
	u32 esec;
	int sector_size = SM_SECTOR_SIZE;
	smdev_t *smdev = &G_smdev;
	smpart_t *smpart;
	int part_size[TOTAL_FW_PARTS];
	int i, rval = 0;
	u32 align_en;

	if (slot < 0)
		return slot;

	memzero(smdev, sizeof(*smdev));
	rval = _sm_dev_init(slot, smdev);
	if (rval < 0)
		goto done;

	align_en = ((slot == SCARDMGR_SLOT_SDIO) ||
	 	    (slot == SCARDMGR_SLOT_SD) ||
	 	    (slot == SCARDMGR_SLOT_SD2)) ? (1) : (0);

	esec = smdev->total_sectors;
	if (esec == 0) {
		rval = -1;
		goto done;
	}

	get_part_size(part_size);

	ssec = nsec = 0;
	for (i = 0; i < TOTAL_FW_PARTS; i++) {
		/* Always put a PTB on SM */
		if ((g_part_dev[i] != BOOT_DEV_SM) &&
		    (i != PART_PTB))
			continue;

		smpart = &smdev->part[i];
		for (nsec = 0; (nsec * sector_size) < part_size[i]; nsec++);
		smpart->ssec = ssec;
		smpart->nsec = nsec;
		ssec += nsec;
	}

	/*****************************************************/
	/* Partitions grow from the end of the sector media. */
	/*****************************************************/
	if ((g_part_dev[PART_CAL] == BOOT_DEV_SM) &&
	    (g_part_dev[PART_PRF] == BOOT_DEV_SM)) {

		smpart = &smdev->part[PART_CAL];
		for (nsec = 0; (nsec * sector_size) < MP_CAL_SIZE; nsec++);
		smpart->ssec = esec - nsec;
		smpart->nsec = nsec;

		smpart = &smdev->part[PART_PRF];
		for (nsec = 0; (nsec * sector_size) < MP_PRF_SIZE; nsec++);
		smpart->ssec = esec - smdev->part[PART_CAL].nsec - nsec;
		smpart->nsec = nsec;

		/* The end of partitions growing from start of sector media. */
		esec = smpart->ssec;
	}
	/*******************************************************/
	/* Partitions grow from the start of the sector media. */
	/*******************************************************/
	nsec = 0;
	if (g_part_dev[PART_RAW] == BOOT_DEV_SM) {
		smpart = &smdev->part[PART_RAW];
		for (nsec = 0; (nsec * sector_size) < MP_RAW_SIZE; nsec++);
		smpart->ssec = (nsec == 0) ? 0 : ssec;
		smpart->nsec = nsec;
		ssec += nsec;
	}

	if (g_part_dev[PART_STG2] == BOOT_DEV_SM) {
		smpart = &smdev->part[PART_STG2];
		for (nsec = 0; (nsec * sector_size) < MP_STG2_SIZE; nsec++);
		if (align_en) {
			/* Let start sector align to Boundary Unit */
		    	u32 misalign = ssec & (SM_SECTOR_ALIGN - 1);
			ssec += (misalign) ? (SM_SECTOR_ALIGN - misalign) : (0);
		}
		smpart->ssec = (nsec == 0) ? 0 : ssec;
		smpart->nsec = nsec;
		ssec += nsec;
	}

	/******************************************************************/
	/* Caculate the STG partition from the rest space of sector media */
	/******************************************************************/
	if (ssec > esec) {
		/* Not enough space for previous partition. */
		rval = -1;
		goto done;
	} else if (ssec == esec) {
		/* Only no space for storage partition. */
		goto done;
	}

	smpart = &smdev->part[PART_STG];
	nsec = esec - ssec;
	if (align_en) {
		/* Let start sector align to Boundary Unit */
	    	u32 misalign = ssec & (SM_SECTOR_ALIGN - 1);
		if (misalign) {
			ssec += SM_SECTOR_ALIGN - misalign;
			nsec -= SM_SECTOR_ALIGN - misalign;
		}
	}
	smpart->ssec = ssec;
	smpart->nsec = nsec;

done:
	if (rval < 0) {
		putstr("Secotr-Media initial failure: ");
		putdec(slot);
		putstr("\r\n");
	} else
		smdev->init = 1;

	return rval;
}

int sm_is_init(int id)
{
	if (id > PART_MAX)
                return 0;

	/* If smdev is initialized, then it is initilalized. */
	return G_smdev.init;
}

smdev_t *sm_get_dev(void)
{
	return &G_smdev;
}

smpart_t *sm_get_part(int id)
{
	smdev_t *smdev = &G_smdev;

	if (id >= PART_MAX)
		return NULL;

	return &smdev->part[id];
}

/**
 * Read a sector media.
 */
int sm_read_sector(int sector, unsigned char *target, int len)
{
	int rval = -1;
	int sectors = len / SM_SECTOR_SIZE;
	int remain  = len % SM_SECTOR_SIZE;

	if (sectors > 0) {
		rval = sdmmc_read_sector(sector, sectors,
					 (unsigned int *)target);
	}

	if (remain != 0) {
		rval = sdmmc_read_sector((sector + sectors), 1,
					 (unsigned int *)sector_buffer);

		memcpy((target + sectors * SM_SECTOR_SIZE), sector_buffer, remain);
	}

	return rval;
}

/**
 * Program a sector media.
 */
int sm_write_sector(int sector, unsigned char *image, int len)
{
	int rval = -1;
	unsigned char buffer[SM_SECTOR_SIZE];
	int sectors = len / SM_SECTOR_SIZE;
	int remain  = len % SM_SECTOR_SIZE;

	if (sectors > 0) {
		rval = sdmmc_write_sector(sector, sectors,
					  (unsigned int *)image);
	}

	if (remain != 0) {
		memcpy(buffer, (image + sectors * SM_SECTOR_SIZE), remain);

		rval = sdmmc_write_sector((sector + sectors), 1,
					  (unsigned int *)buffer);
	}

	return rval;
}

/**
 * Erase a sector media.
 */
int sm_erase_sector(int sector, int sectors)
{
	return sdmmc_erase_sector(sector, sectors);
}

/*****************************************************************************/
/**
 * read data from sector media to memory.
 */
int sm_read_data(unsigned char *dst, unsigned char *src, int len)
{
	return -1;
}

/*****************************************************************************/
/**
 * generate parameter structure.
 */
int create_parameter(int target, struct sdmmc_header_s *param)
{
	smdev_t *smdev = &G_smdev;
	smpart_t *smpart = NULL;

	memzero(param, sizeof(struct sdmmc_header_s));

	param->magic	  = EEPROM_HEADER_MAGIC;
	param->init_delay = 0x100;

	switch(target) {
	case PART_BLD:
		smpart = &smdev->part[PART_BLD];
		param->start_block	= smpart->ssec;
		param->blocks		= smpart->nsec;
		param->mem_target	= AMBOOT_BLD_RAM_START;
		break;
	case PART_PBA:
		smpart = &smdev->part[PART_PBA];
		param->start_block	= smpart->ssec;
		param->blocks		= smpart->nsec;
		param->mem_target	= KERNEL_RAM_START;
		break;
	case PART_PRI:
		smpart = &smdev->part[PART_PRI];
		param->start_block	= smpart->ssec;
		param->blocks		= smpart->nsec;
		param->mem_target	= KERNEL_RAM_START;
		break;
	default:
		return -1;
	}

#if	(CHIP_REV == A5S)

	param->dram_control   = 0x00000000;
	param->dram_config    = 0x00000000;
	param->dram_timing1   = 0x00000000;
	param->dram_timing2   = 0x00000000;
	param->dram_init_ctl  = 0x00000000;
	param->dram_ext_mode0 = 0x00000000;
	param->dram_ext_mode1 = 0x00000000;
	param->dram_ext_mode2 = 0x00000000;
	param->dram_ext_mode3 = 0x00000000;
	param->dram_dqs_sync  = 0x00000000;
	param->dll0           = 0x00000000;
	param->dll1           = 0x00000000;
	param->dll2           = 0x00000000;
	param->dll3           = 0x00000000;
	param->dll_ocd        = 0x00000000;
	param->ddrio_calib    = 0x00000000;

#elif	(CHIP_REV == A6)

	param->dram_control   = 0x00000000;
	param->dram_config    = DRAM_CFG_VAL;
	param->dram_timing1   = DRAM_TIMING1_216MHZ;
	param->dram_timing2   = DRAM_TIMING2_216MHZ;
	param->dram_init_ctl  = 0x00000000;
	param->dram_ext_mode0 = DRAM_EXT_MODE0_216MHZ;
	param->dram_ext_mode1 = DRAM_EXT_MODE1_216MHZ;
	param->dram_ext_mode2 = DRAM_EXT_MODE2_216MHZ;
	param->dram_ext_mode3 = DRAM_EXT_MODE3_216MHZ;
	param->dram_dqs_sync  = 0x00000000;
	param->dll0           = RCT_DLL0_216MHZ;
	param->dll1           = RCT_DLL1_216MHZ;
	param->dll2           = RCT_DLL2_216MHZ;
	param->dll3           = RCT_DLL3_216MHZ;
	param->dll_ocd        = RCT_DLL_OCD_BITS_216MHZ;
	param->ddrio_calib    = RCT_DDRIO_CALIB_216MHZ;

#endif

	return 0;
}

#define sd_cmd_6 0x061b
#define EMMC_BOOT_BUS_WIDTH 	177
#define	EMMC_BOOT_CONFIG    	179
#define	EMMC_HW_RESET		162

#define EMMC_ACCP_USER		0
#define EMMC_ACCP_BP_1		1
#define EMMC_ACCP_BP_2		2

#define EMMC_BOOTP_USER		0x38
#define EMMC_BOOTP_BP_1		0x8
#define EMMC_BOOTP_BP_2		0x10

#define EMMC_BOOT_1BIT		8
#define EMMC_BOOT_4BIT		9
#define EMMC_BOOT_8BIT		10


#if defined(CONFIG_EMMC_ACCPART_USER)
#define EMMC_ACCESS_PART	EMMC_ACCP_USER
#elif defined(CONFIG_EMMC_ACCPART_BP1)
#define EMMC_ACCESS_PART	EMMC_ACCP_BP_1
#else
#define EMMC_ACCESS_PART	EMMC_ACCP_BP_2
#endif

#if defined(CONFIG_EMMC_BOOTPART_USER)
#define EMMC_BOOT_PART		EMMC_BOOTP_USER
#elif defined(CONFIG_EMMC_BOOTPART_BP1)
#define EMMC_BOOT_PART		EMMC_BOOTP_BP_1
#else
#define EMMC_BOOT_PART		EMMC_BOOTP_BP_2
#endif

#if defined(CONFIG_EMMC_BOOT_1BIT)
#define EMMC_BOOT_BUS	EMMC_BOOT_1BIT
#elif defined(CONFIG_EMMC_BOOT_4BIT)
#define EMMC_BOOT_BUS	EMMC_BOOT_4BIT
#else
#define EMMC_BOOT_BUS	EMMC_BOOT_8BIT
#endif

#if defined(ENABLE_EMMC_BOOT)
static int sdmmc_set_extcsd(int slot, u8 bootp, u8 accp, u8 bbus_width)
{
	u8 cfg = 0, bus = 0;
	u32 arg1, arg2;
	int rval;

	cfg |= (bootp | accp);
	arg1 = ((ACCESS_WRITE_BYTE & 0x3) << 24) |
		(EMMC_BOOT_CONFIG << 16)	 |
		(cfg << 8);

	/* Set the boot config to EMMC device */
	rval = sdmmc_command(sd_cmd_6, arg1, 0x3000);
	if (rval != SDMMC_ERR_NONE) {
		putstr("Set EMMC boot config failed\r\n");
		return -1;
	}

	bus = bbus_width;
	arg2 = ((ACCESS_WRITE_BYTE & 0x3) << 24) |
		(EMMC_BOOT_BUS_WIDTH << 16)	 |
		(bus << 8);

	/* Set the boot bus width to EMMC device */
	rval = sdmmc_command(sd_cmd_6, arg2, 0x3000);
	if (rval != SDMMC_ERR_NONE) {
		putstr("Set EMMC boot bus failed\r\n");
		return -1;
	}

#ifdef CONFIG_EMMC_HW_RESET_PERM_ENABLED
	arg1 = ((ACCESS_WRITE_BYTE & 0x3) << 24) |
		(EMMC_HW_RESET << 16) |
		(1 << 8);
	rval = sdmmc_command(sd_cmd_6, arg1, 0x3000);
	if (rval != SDMMC_ERR_NONE) {
		putstr("Enable EMMC hw reset function failed\r\n");
		return -1;
	}
#endif

#ifdef CONFIG_EMMC_HW_RESET_PERM_DISABLED
	arg1 = ((ACCESS_WRITE_BYTE & 0x3) << 24) |
		(EMMC_HW_RESET << 16) |
		(2 << 8);
	rval = sdmmc_command(sd_cmd_6, arg1, 0x3000);
	if (rval != SDMMC_ERR_NONE) {
		putstr("Enable EMMC hw reset function failed\r\n");
		return -1;
	}
#endif

	return 0;
}
#endif

int sdmmc_prog_bootp(void)
{
#ifdef ENABLE_EMMC_BOOT
	u8 accp = (EMMC_BOOT_PART == EMMC_BOOTP_USER) ? (EMMC_ACCP_USER) :
		  (EMMC_BOOT_PART == EMMC_BOOTP_BP_1) ? (EMMC_ACCP_BP_1) :
		  (EMMC_ACCP_BP_2);

	u8 buf[512];

	/* do dummy read to make sure DAT line is ok. */
	sdmmc_read_sector(0, 1, (u32 *) buf);

	return sdmmc_set_extcsd(FIRMWARE_CONTAINER, EMMC_BOOT_PART,
				accp, EMMC_BOOT_BUS);
#else
	return 0;
#endif
}

int sdmmc_prog_accp(void)
{
#ifdef ENABLE_EMMC_BOOT
	return sdmmc_set_extcsd(FIRMWARE_CONTAINER, EMMC_BOOT_PART,
				EMMC_ACCESS_PART, EMMC_BOOT_BUS);
#else
	return 0;
#endif
}
