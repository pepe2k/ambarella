/**
 * system/src/bld/main.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <hal/hal.h>

#define putstr uart_putstr

extern void get_stepping_info(int *chip, int *major, int *minor);

const char *AMBOOT_LOGO =						\
	"\r\n"								\
	"             ___  ___  _________                _   \r\n"	\
	"            / _ \\ |  \\/  || ___ \\              | |  \r\n"	\
	"           / /_\\ \\| .  . || |_/ /  ___    ___  | |_ \r\n"	\
	"           |  _  || |\\/| || ___ \\ / _ \\  / _ \\ | __|\r\n"	\
	"           | | | || |  | || |_/ /| (_) || (_) || |_ \r\n"	\
	"           \\_| |_/\\_|  |_/\\____/  \\___/  \\___/  \\__|\r\n" \
	"----------------------------------------------------------\r\n" \
	"Amboot(R) Ambarella(R) Copyright (C) 2004-2007\r\n";

const u32 hotboot_valid = 0x0;
const u32 hotboot_pattern = 0x0;

/**
 * The C entry point of the bootloader.
 */
int main(void)
{
	unsigned int boot_from = rct_boot_from();

#if (defined(ENABLE_FLASH) || defined(FIRMWARE_CONTAINER) && \
     (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE)))
	flpart_table_t ptb;
	int escape = 0;
#endif
	int h, l;
	char cmd[MAX_CMDLINE_LEN];
	int bldbsp_rval = 0;
#if defined(USE_HAL)
	u32 hal_version;
#endif

	/* RCT/PLL setup */
	rct_pll_init();
	rct_switch_core_freq();

	/* RCT/PLL  setup */
	enable_fio_dma();
	rct_reset_fio();
	fio_exit_random_mode();

	/* Initialize various peripherals used in AMBoot */
	vic_init();
	commands_init();
	uart_init();
	putstr("\x1b[4l");	/* Set terminal to replacement mode */
	putstr("\r\n");		/* First, output a blank line to UART */

	/* Initial firmware information */
	set_part_dev();

	/* initial boot device */
#if (defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER))
	sm_dev_init(FIRMWARE_CONTAINER);
#endif
	if ((boot_from & BOOT_FROM_NAND) == BOOT_FROM_NAND) {
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
		nand_init();
		nand_reset();
#if defined(NAND_SUPPORT_BBT)
		nand_scan_bbt(0);
#endif
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_ONENAND_NONE))
	} else if ((boot_from & BOOT_FROM_ONENAND) == BOOT_FROM_ONENAND) {
		onenand_init();
		onenand_reset();
#endif

#if (defined(ENABLE_FLASH) && !defined(CONFIG_NOR_NONE))
	} else if ((boot_from & BOOT_FROM_NOR) == BOOT_FROM_NOR) {
		nor_init();
		nor_reset();
#endif
#if (defined(ENABLE_FLASH) && !defined(CONFIG_SNOR_NONE))
	} else if ((boot_from & BOOT_FROM_SNOR) == BOOT_FROM_SNOR) {
		snor_init();
		snor_reset();
#endif
	} else if (((boot_from & BOOT_FROM_BYPASS) == BOOT_FROM_BYPASS)
		   ||((boot_from & BOOT_FROM_SDMMC) != 0x0)) {
		/* Boot bypass, go ahead */
	} else {
		uart_putstr("Unknow boot device\r\n");
		return -1;
	}

	/* Sanitize ptb.dev if needed */
#if (defined(ENABLE_FLASH) || defined(FIRMWARE_CONTAINER) && \
     (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE)))
	if (flprog_update_part_info_from_meta() >= 0 &&
	    flprog_get_part_table(&ptb) >= 0 &&
	    (ptb.dev.magic != FLPART_MAGIC)) {
		memzero(&ptb.dev, sizeof(ptb.dev));
#if defined(AMBOOT_DEV_AUTO_BOOT)
		ptb.dev.auto_boot = 1;
#endif
#if defined(AMBOOT_DEV_CMDLINE)
		strncpy(ptb.dev.cmdline, AMBOOT_DEV_CMDLINE,
			sizeof(ptb.dev.cmdline) - 1);
#endif
#if defined(SHOW_AMBOOT_SPLASH)
		ptb.dev.splash_id = 0;
#endif
#if defined(AMBOOT_DEFAULT_SN)
		strncpy(ptb.dev.sn, AMBOOT_DEFAULT_SN, sizeof(ptb.dev.sn) - 1);
#endif
		ptb.dev.magic = FLPART_MAGIC;

		if (flprog_set_part_table(&ptb) >= 0) {
			putstr("sanitized ptb.dev\r\n");
		}
	}

	/* Alway attempt to load HAL and set it up! */
	hal_init();
	set_dram_arbitration();

#if defined(AMBOOT_BUILD_DIAG)
#if defined(AMBARELLA_CRYPTO_REMAP)
	crypto_remap();
#endif
#endif

	/* Functions use HAL APIs should be called after hal_init(). */
	timer_init();
	rct_x_usb_clksrc();
	if (amboot_bsp_hw_init != NULL) {
		amboot_bsp_hw_init();
	}

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	cmd_bapi_init(0);
#endif

#if defined(AMBOOT_DEV_USBDL_MODE)
	/* Force USB download mode by config */
	ptb.dev.usbdl_mode = USB_MODE_DEFAULT;
#else
#if defined(AMBARELLA_LINUX_LAYOUT)
	if ((ptb.dev.usbdl_mode == 0) && (amboot_bsp_check_usbmode != NULL)) {
		ptb.dev.usbdl_mode = amboot_bsp_check_usbmode();
	}
#endif
#endif
	/* Check for a 'enter' key on UART and halt auto_boot, usbdl_mode */
	if (ptb.dev.auto_boot || ptb.dev.usbdl_mode)
		escape = uart_wait_escape(50);

	/* If automatic USB download mode is enabled, */
	/* then enter special USB download mode */
	if (escape == 0 && ptb.dev.usbdl_mode) {
		usb_boot(ptb.dev.usbdl_mode);
	} else if (escape == 0) {
		/* Suspend USB Phy for Kernel to save power except 'escape'.
		   In 'escape', seems invoke this too early makes suspend USB by HAL fail.
		   So we skip it because 'escape' should be special use case. */
		usb_disconnect();
	}

#if defined(SHOW_AMBOOT_SPLASH)
	if (escape == 0 && ptb.dev.auto_boot) {
		extern void splash_logo(u32 splash_id);
		splash_logo(ptb.dev.splash_id);
	}
#endif

	if (amboot_bsp_system_init != NULL) {
		amboot_bsp_system_init();
	}

	/* If automatic boot is enabled, attempt to boot from flash */
	if (escape == 0 && ptb.dev.auto_boot) {
		/* Call out to BSP supplied entry point (if exists) */
		if (amboot_bsp_entry != NULL) {
			bldbsp_rval = amboot_bsp_entry();
		}

		if (bldbsp_rval == 1) {
#if defined(AMBOOT_DEV_BOOT_CORTEX)
			cmd_cortex_bios(NULL, 0);
#else
			bios(NULL, 0);  /* BIOS without cmdline*/
#endif
		} else if (bldbsp_rval == 2) {
#if defined(AMBOOT_DEV_BOOT_CORTEX)
			cmd_cortex_bios(ptb.dev.cmdline, 0);
#else
			bios(ptb.dev.cmdline, 0);  /* Auto BIOS */
#endif
		} else {
#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
#if defined(AMBOOT_DEV_BOOT_CORTEX)
			cmd_bapi_aoss_resume_boot(cmd_cortex_boot, 0, 0, NULL);
#else
			cmd_bapi_aoss_resume_boot(boot, 0, 0, NULL);
#endif
#else
#if defined(AMBOOT_DEV_BOOT_CORTEX)
			cmd_cortex_boot(ptb.dev.cmdline, 0);
#else
			boot(ptb.dev.cmdline, 0);  /* Auto boot */
#endif
#endif
		}
	}
#else /* !CONFIG_NAND_NONE || !CONFIG_NOR_NONE */

#if defined(AMBOOT_DEV_USBDL_MODE)
	/* If no NAND and NOR, try booting from USB */
	if (usb_check_connected()) {
		usb_boot(USB_MODE_DEFAULT);
	} else {
		usb_disconnect();
	}
#endif

#endif /* !CONFIG_NAND_NONE || !CONFIG_NOR_NONE */

#if (ETH_INSTANCES >= 1)
	if (amboot_bsp_netphy_init != NULL) {
		amboot_bsp_netphy_init();
	}

	bld_net_init(0);

#if (defined(ENABLE_FLASH) && \
     (!defined(CONFIG_NAND_NONE) || !defined(CONFIG_NOR_NONE)))
	/* Try booting from the network */
	if (escape == 0 && ptb.dev.auto_dl) {
		putstr("auto-boot from network\r\n");
		netboot(ptb.dev.cmdline, 0);
	}
#endif
#endif  /* ETH_INSTANCES == 1 */

	putstr(AMBOOT_LOGO);  /* Dump the logo */
#if defined(USE_HAL)
	/* DUMP BST and HAL Version */
	putstr("BST (");
	putdec(readl(g_haladdr + readl(g_haladdr + 0xc)));
	putstr("), HAL (");
	if (amb_get_version(HAL_BASE_VP, &hal_version) != -1) {
		putdec(hal_version);
	} else {
		putstr("0");
	}
	putstr(")\r\n");
	putstr("SYS (");
	puthex(amb_get_system_configuration(HAL_BASE_VP));
	putstr(")\r\n");
#endif

#if	(RCT_ARM_CLK_SRC_IDSP == 1 || RCT_ARM_CLK_SRC_CORE_DDR == 1)
	putstr("Arm freq: ");
	putdec(get_arm_bus_freq_hz());
	putstr("\r\n");
	putstr("iDSP freq: ");
	putdec(get_idsp_freq_hz());
	putstr("\r\n");
#endif
	putstr("Core freq: ");
	putdec(get_core_bus_freq_hz());
	putstr("\r\n");
	putstr("Dram freq: ");
	putdec(get_ddr_freq_hz());
	putstr("\r\n");
	putstr("AHB freq: ");
	putdec(get_ahb_bus_freq_hz());
	putstr("\r\n");
	putstr("APB freq: ");
	putdec(get_apb_bus_freq_hz());
	putstr("\r\n");
	putstr("UART freq: ");
	putdec(get_uart_freq_hz());
	putstr("\r\n");
	putstr("SD freq: ");
	putdec(get_sd_freq_hz());
	putstr("\r\n");
#if     (CHIP_REV == I1)
	putstr("Cortex freq: ");
	putdec(amb_get_cortex_clock_frequency(HAL_BASE_VP)/100);
	putstr("00\r\n");
	putstr("AXI freq: ");
	putdec(amb_get_axi_clock_frequency(HAL_BASE_VP)/100);
	putstr("00\r\n");
	putstr("DDD freq: ");
	putdec(amb_get_3d_clock_frequency(HAL_BASE_VP));
	putstr("\r\n");
	putstr("GTX freq: ");
	putdec(amb_get_gtx_clock_frequency(HAL_BASE_VP));
	putstr("\r\n");
	putstr("SDXC freq: ");
	putdec(amb_get_sdxc_clock_frequency(HAL_BASE_VP));
	putstr("\r\n");
#endif
#if     (CHIP_REV == S2)
	putstr("Cortex freq: ");
	putdec(amb_get_cortex_clock_frequency(HAL_BASE_VP)/100);
	putstr("00\r\n");
	putstr("GTX freq: ");
	putdec(amb_get_gtx_clock_frequency(HAL_BASE_VP));
	putstr("\r\n");
	putstr("SDXC freq: ");
	putdec(amb_get_sdio_clock_frequency(HAL_BASE_VP));
	putstr("\r\n");
#endif

#ifdef ENABLE_AMBOOT_TEST_REBOOT
	test_reboot();
#endif

	/************************/
	/* Endless command loop */
	/************************/
	for (h = 0; ; h++) {
		uart_putstr("amboot> ");
		l = uart_getcmd(cmd, sizeof(cmd), 0);
		if (l > 0) {
			parse_command(cmd);
		}
	}

	return 0;
}
