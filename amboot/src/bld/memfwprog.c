/**
 * system/src/bld/memfwprog.c
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
 *    2007/10/11 - [Charles Chiou] added PBA partition
 *    2007/12/30 - [Charles Chiou] added memfwprog process result table
 *    2008/11/18 - [Charles Chiou] added HAL and SEC partitions
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <fio/firmfl_api.h>

static const char *MEMFWPROG_LOGO =
	"\r\n" \
	"------------------------------------------------------\r\n" \
	"In-memory Firmware Flash Programming Utility (by C.C.)\r\n" \
	"Ambarella(R) Copyright (C) 2004-2008\r\n"		     \
	"------------------------------------------------------\r\n";

extern u32 firmware_start;
extern u32 begin_bst_image;
extern u32 end_bst_image;
extern u32 begin_bld_image;
extern u32 end_bld_image;
extern u32 begin_hal_image;
extern u32 end_hal_image;
extern u32 begin_pba_image;
extern u32 end_pba_image;
extern u32 begin_kernel_image;
extern u32 end_kernel_image;
extern u32 begin_secondary_image;
extern u32 end_secondary_image;
extern u32 begin_backup_image;
extern u32 end_backup_image;
extern u32 begin_ramdisk_image;
extern u32 end_ramdisk_image;
extern u32 begin_romfs_image;
extern u32 end_romfs_image;
extern u32 begin_dsp_image;
extern u32 end_dsp_image;
extern u32 begin_lnx_image;
extern u32 end_lnx_image;
extern u32 begin_swp_image;
extern u32 end_swp_image;
extern u32 begin_add_image;
extern u32 end_add_image;
extern u32 begin_adc_image;
extern u32 end_adc_image;

extern void output_header(partimg_header_t *header, u32 len);
extern void output_failure(int errcode);
extern void output_report(const char *name, u32 flag);
extern int create_parameter(int target, struct sdmmc_header_s *param);
extern int flprog_prog(int pid, u8 *image, unsigned int len);

extern void hook_pre_memfwprog(fwprog_cmd_t *) __attribute__ ((weak));
extern void hook_post_memfwprog(fwprog_cmd_t *) __attribute__ ((weak));

extern unsigned int __memfwprog_result;
extern unsigned int __memfwprog_command;

#if defined(FIRMWARE_CONTAINER)
static int sm_slot = FIRMWARE_CONTAINER;
#elif  defined(SM_STG2_SLOT)
static int sm_slot = SM_STG2_SLOT;
#endif

#ifndef BUILD_AMBPROM
static u32 get_bst_id(void)
{
	u32 bst_id = 0;

#if defined(BST_IMG_ID)
	return BST_IMG_ID;
#endif

#if defined(BST_SEL0_GPIO)
	gpio_config_sw_in(BST_SEL0_GPIO);
	bst_id = gpio_get(BST_SEL0_GPIO);
#endif

#if defined(BST_SEL1_GPIO)
	gpio_config_sw_in(BST_SEL1_GPIO);
	bst_id |= gpio_get(BST_SEL1_GPIO) << 1;
#endif

#if defined(BST_SEL0_GPIO) || defined(BST_SEL1_GPIO)
	uart_putstr("BST ID = ");
	uart_putdec(bst_id);
	uart_putstr("\r\n");
#endif

	return bst_id;
}
#endif

static int select_bst_fw(u8 *img_in, int len_in, u8 **img_out, int *len_out)
{
#ifndef BUILD_AMBPROM
	int bst_id, fw_size;

	bst_id = get_bst_id();
	fw_size = AMBOOT_BST_FIXED_SIZE + sizeof(partimg_header_t);

	/* Sanity check */
	if (len_in % fw_size != 0 || (bst_id + 1) * fw_size > len_in ) {
		uart_putstr("invalid bst_id or firmware length: ");
		uart_putdec(bst_id);
		uart_putstr(", ");
		uart_putdec(len_in);
		uart_putstr("(");
		uart_putdec(fw_size);
		uart_putstr(")\r\n");
		return FLPROG_ERR_LENGTH;
	}

	*img_out = img_in + fw_size * bst_id;
	*len_out = fw_size;

#else
	*img_out = img_in;
	*len_out = len_in;
#endif
	return 0;
}

int main(void)
{
	int rval = 0;
	u8 code;
	u8 *image;
	int part_len[HAS_IMG_PARTS];
	int begin_image[HAS_IMG_PARTS];
	u8 buffer[512];
	unsigned int boot_from = rct_boot_from();
	int i;
	fwprog_result_t *result = (fwprog_result_t *) &__memfwprog_result;

	/* RCT/PLL setup */
	clock_source_select(0x0);

	enable_fio_dma();
	rct_reset_fio();
	fio_exit_random_mode();

	/* Initialize the UART */
	uart_init();

	uart_putstr("\x1b[4l");	/* Set terminal to replacement mode */
	uart_putstr("\r\n");	/* First, output a blank line to UART */

	putstr(MEMFWPROG_LOGO);

	/* Initial firmware information */
	set_part_dev();

	/* initial boot device */
	if ((boot_from & BOOT_FROM_SPI) == BOOT_FROM_SPI)
		spi_init();

#if (defined(ENABLE_SD) && (defined(FIRMWARE_CONTAINER) || defined(SM_STG2_SLOT)))
		sm_dev_init(sm_slot);
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
		nand_init();
		nand_reset();
#if defined(NAND_SUPPORT_BBT)
		nand_scan_bbt(0);
#endif
		nand_update_bb_info();
	}
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		onenand_init();
		onenand_reset();
		onenand_update_bb_info();
	}
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		nor_init();
		nor_reset();
	}
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	if ((boot_from & BOOT_FROM_SNOR) == BOOT_FROM_SNOR) {
		snor_init();
		snor_reset();
	}
#endif

	if (hook_pre_memfwprog != 0x0) {
		fwprog_cmd_t *fwprog_cmd;
		fwprog_cmd = (fwprog_cmd_t *) &__memfwprog_command;
		putstr("\r\nhook_pre_memfwprog\r\n");
		hook_pre_memfwprog(fwprog_cmd);
	}

	memzero(buffer, sizeof(buffer));
	memzero(result, sizeof(*result));
	result->magic = FWPROG_RESULT_MAGIC;

	/* Calculate the firmware payload offsets of images */
	begin_image[PART_BST] = begin_bst_image;
	begin_image[PART_PTB] = 0;
	begin_image[PART_BLD] = begin_bld_image;
	begin_image[PART_HAL] = begin_hal_image;
	begin_image[PART_PBA] = begin_pba_image;
	begin_image[PART_PRI] = begin_kernel_image;
	begin_image[PART_SEC] = begin_secondary_image;
	begin_image[PART_BAK] = begin_backup_image;
	begin_image[PART_RMD] = begin_ramdisk_image;
	begin_image[PART_ROM] = begin_romfs_image;
	begin_image[PART_DSP] = begin_dsp_image;
	begin_image[PART_LNX] = begin_lnx_image;
	begin_image[PART_SWP] = begin_swp_image;
	begin_image[PART_ADD] = begin_add_image;
	begin_image[PART_ADC] = begin_adc_image;

	part_len[PART_BST] = end_bst_image -     begin_bst_image;
	part_len[PART_PTB] = 0;
	part_len[PART_BLD] = end_bld_image -     begin_bld_image;
	part_len[PART_HAL] = end_hal_image -     begin_hal_image;
	part_len[PART_PBA] = end_pba_image -     begin_pba_image;
	part_len[PART_PRI] = end_kernel_image -  begin_kernel_image;
	part_len[PART_SEC] = end_secondary_image -  begin_secondary_image;
	part_len[PART_BAK] = end_backup_image -  begin_backup_image;
	part_len[PART_RMD] = end_ramdisk_image - begin_ramdisk_image;
	part_len[PART_ROM] = end_romfs_image -   begin_romfs_image;
	part_len[PART_DSP] = end_dsp_image -     begin_dsp_image;
	part_len[PART_LNX] = end_lnx_image -     begin_lnx_image;
	part_len[PART_SWP] = end_swp_image -     begin_swp_image;
	part_len[PART_ADD] = end_add_image -     begin_add_image;
	part_len[PART_ADC] = end_adc_image -     begin_adc_image;

	i = PART_BST;
	if (part_len[i] > 0) {
		putstr(g_part_str[i]);
		putstr(" code found in firmware!\r\n");
		if ((boot_from & BOOT_FROM_FLASH) ||
		    (boot_from & BOOT_FROM_SPI) ||
		    (boot_from & BOOT_FROM_EMMC)) {
			image = (u8 *) firmware_start + begin_image[i];

			rval = select_bst_fw(image, part_len[i], &image, &part_len[i]);
			if (rval < 0)
				goto done;

			output_header((partimg_header_t *)image, part_len[i]);

			rval = flprog_prog(PART_BST, image, part_len[i]);
			if (rval == 0 && (boot_from & BOOT_FROM_SPI))
				goto cre_param;
			else
				goto done;
		} else if ((boot_from & BOOT_FROM_SDMMC) != 0x0) {
cre_param:
			/* save parameters to sector 1 */
			rval = create_parameter(PART_BLD,
					(struct sdmmc_header_s *)buffer);
			putstr("program sm boot parameters\r\n");
			rval = sm_write_sector(1, buffer, sizeof(buffer));
			if (rval == 0) {
				putstr("done\r\n");
			} else {
				putstr("failed\r\n");
			}
			goto done;
		} else {
			rval = -1;
		}
done:
		code = rval < 0 ? -rval : rval;
		result->flag[i] = FWPROG_RESULT_MAKE(code, part_len[i]);
		if (rval == FLPROG_ERR_PROG_IMG)
			result->bad_blk_info |= BST_BAD_BLK_OVER;
		output_failure(rval);
	}

	for (i = PART_BLD; i < HAS_IMG_PARTS; i++) {
		if (part_len[i] > 0) {
			putstr("\r\n");
			putstr(g_part_str[i]);
			putstr(" code found in firmware!\r\n");

			image = (u8 *)(firmware_start + begin_image[i]);
			output_header((partimg_header_t *)image, part_len[i]);
			rval = flprog_prog(i, image, part_len[i]);
			code = rval < 0 ? -rval : rval;
			result->flag[i] =
				FWPROG_RESULT_MAKE(code, part_len[i]);
			if (rval == FLPROG_ERR_PROG_IMG)
				result->bad_blk_info |= (0x1 << i);
			output_failure(rval);
		}
	}

	putstr("\r\n------ Report ------\r\n");
	for (i = 0; i < HAS_IMG_PARTS; i++) {
		if (i == PART_PTB)
			continue;
		output_report(g_part_str[i], result->flag[i]);
	}

	if (hook_post_memfwprog != 0x0) {
		fwprog_cmd_t *fwprog_cmd;
		fwprog_cmd = (fwprog_cmd_t *) &__memfwprog_command;
		putstr("\r\nhook_post_memfwprog\r\n");
		hook_post_memfwprog(fwprog_cmd);
	}

	putstr("\r\n\t- Program Terminated -\r\n");

	return 0;
}
