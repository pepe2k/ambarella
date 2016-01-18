/**
 * system/src/bld/devfw.lds.cpp
 *
 * Note: This linker script should be preprocessed before used against a
 *	 specific configuration of 'devfw' code.
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *    2010/12/23 - [Cao Rongrong] added SWP, ADD and ADC partitions
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

#ifdef BUILD_AMBPROM
#define BST_IMAGE	.temp/prom*.fw
#else
#define BST_IMAGE	.temp/amboot_bst*.fw
#endif

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SECTIONS
{
	. = AMBOOT_BLD_RAM_START + 0x00200000;

	.text : {
		. = ALIGN(4);
		__BEGIN_FIRMWARE_IMAGE__ = ABSOLUTE(.);
		.temp/header_*.fw (.text)

#if defined(__LINK_AMBOOT__)
		/* Payload of BST */
		. = ALIGN(2048);
		__BEGIN_AMBOOT_BST_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		BST_IMAGE (.text)
		__END_AMBOOT_BST_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#if defined(__LINK_BLD__)
		/* Payload of BLD */
		. = ALIGN(2048);
		__BEGIN_AMBOOT_BLD_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/amboot_bld*.fw (.text)
		__END_AMBOOT_BLD_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_HAL__
		/* Payload of HAL */
		. = ALIGN(2048);
		__BEGIN_HAL_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/hal.fw (.text)
		__END_HAL_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_PBA__
		/* Payload of PBA */
		. = ALIGN(2048);
		__BEGIN_PBA_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/pba.fw (.text)
		__END_PBA_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_KERNEL__
		/* Payload of Kernel */
		. = ALIGN(2048);
		__BEGIN_KERNEL_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/kernel.fw (.text)
		__END_KERNEL_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_SECONDARY__
		/* Payload of Secondary */
		. = ALIGN(2048);
		__BEGIN_SECONDARY_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/secondary.fw (.text)
		__END_SECONDARY_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_BACKUP__
		/* Payload of Backup */
		. = ALIGN(2048);
		__BEGIN_BACKUP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/backup.fw (.text)
		__END_BACKUP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_RAMDISK__
		/* Payload of RAMDISK */
		. = ALIGN(2048);
		__BEGIN_RAMDISK_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/ramdisk.fw (.text)
		__END_RAMDISK_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_ROMFS__
		/* Payload of ROMFS */
		. = ALIGN(2048);
		__BEGIN_ROMFS_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/romfs.fw (.text)
		__END_ROMFS_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_DSP__
		/* Payload of DSP */
		. = ALIGN(2048);
		__BEGIN_DSP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/dsp.fw (.text)
		__END_DSP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_LNX__
		/* Payload of LNX */
		. = ALIGN(2048);
		__BEGIN_LNX_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/lnx.fw (.text)
		__END_LNX_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_SWP__
		/* Payload of SWP */
		. = ALIGN(2048);
		__BEGIN_SWP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/swp.fw (.text)
		__END_SWP_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_ADD__
		/* Payload of ADD */
		. = ALIGN(2048);
		__BEGIN_ADD_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/add.fw (.text)
		__END_ADD_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif

#ifdef __LINK_ADC__
		/* Payload of ADC */
		. = ALIGN(2048);
		__BEGIN_ADC_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
		.temp/adc.fw (.text)
		__END_ADC_IMAGE__ = . - __BEGIN_FIRMWARE_IMAGE__;
#endif
		. = ALIGN(2048);
		__END_FIRMWARE_IMAGE__ = ABSOLUTE(.);
	}

	.trash : {
		* (*)
	}
}
