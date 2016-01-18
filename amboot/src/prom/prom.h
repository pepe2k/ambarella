/**
 * system/src/prom/prom.h
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __PROM_H__
#define __PROM_H__

#include <basedef.h>
#include <config.h>
#include <ambhw.h>

#define AMBPROM_STACK_SIZE	0x00002000
#define AMBPROM_RAM_START	(DRAM_START_ADDR + 0xc0000)

#ifndef K_ASSERT
#define K_ASSERT(x) {							\
		if (!x)							\
			for (;;);					\
	}
#endif

#ifdef __AMBPROM_IMPL__
#ifndef __ASM__

#ifdef __cplusplus
extern "C" {
#endif

#if (CHIP_REV == A2S)
	/* DDR and CORE frequency are come from the same pll
	 * and change core frequency
	 * should be lock in icache and done in bld. */
#define PROM_NO_SWITCH_CORE_FREQ

	/* This value do not use in A2S SPI boot rom. */
	/* Set this value manual in prom loader.      */
#define DDR_BIAS_CURRENT	0x5ef2
#endif
/* ------------------------------------------------------------------------- */

/* The following are 'slot' ID definitions. */
#define SCARDMGR_SLOT_FL	0	/**< Flash (NOR/NAND) */
#define SCARDMGR_SLOT_XD	1	/**< XD */
#define SCARDMGR_SLOT_CF	2	/**< Compact flash (master) */
#define SCARDMGR_SLOT_SD	3	/**< First SD/MMC controller */
#define SCARDMGR_SLOT_SD2	4	/**< Second SD/MMC controller */
#define SCARDMGR_SLOT_RD	5	/**< Ramdisk */
#define SCARDMGR_SLOT_FL2	6	/**< Flash (NOR/NAND) */
#define SCARDMGR_SLOT_CF2	7	/**< Compact flash (slave) */
#define SCARDMGR_SLOT_MS	8	/**< Memory stick AHB */
#define SCARDMGR_SLOT_CFMS	9	/**< Memory stick (master) */
#define SCARDMGR_SLOT_CFMS2	10	/**< Memory stick (slave) */
#define SCARDMGR_SLOT_SDIO	11	/**< First SD/SDIO (slave) */
#define SCARDMGR_SLOT_RF	25	/**< ROMFS: (Z) drive */

extern u32 rct_boot_from(void);
#define BOOT_FROM_BYPASS	0x00008000
#define BOOT_FROM_HIF		0x00004000
#define BOOT_FROM_NAND		0x00000001
#define BOOT_FROM_NOR		0x00000002
#define BOOT_FROM_ONENAND	0x00000100
#define BOOT_FROM_SNOR		0x00000200
#define BOOT_FROM_FLASH		(BOOT_FROM_NAND | BOOT_FROM_NOR | \
				 BOOT_FROM_ONENAND | BOOT_FROM_SNOR)
#define BOOT_FROM_SPI		0x00000004
#define BOOT_FROM_SD		0x00000010
#define BOOT_FROM_SDHC		0x00000020
#define BOOT_FROM_MMC		0x00000040
#define BOOT_FROM_MOVINAND	0x00000080
#define BOOT_FROM_SDMMC		(BOOT_FROM_SD	| BOOT_FROM_SDHC	| \
				 BOOT_FROM_MMC	| BOOT_FROM_MOVINAND)

/* SD/MMC functions */
#define SDMMC_TYPE_AUTO		0x0
#define SDMMC_TYPE_SD		0x1
#define SDMMC_TYPE_SDHC		0x2
#define SDMMC_TYPE_MMC		0x3
#define SDMMC_TYPE_MOVINAND	0x4

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD) && defined (SDMMC_1_PWRCYC_GPIO)
#define	SM_PWRCYC_GPIO	SDMMC_1_PWRCYC_GPIO
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2) && defined (SDMMC_2_PWRCYC_GPIO)
#define	SM_PWRCYC_GPIO	SDMMC_2_PWRCYC_GPIO
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO) \
	&& defined (SDMMC_SDIO_PWRCYC_GPIO) && defined (SDMMC_SDIO_CD_GPIO)
#define	SM_PWRCYC_GPIO	SDMMC_SDIO_PWRCYC_GPIO
#define	SM_CD_GPIO	SDMMC_SDIO_CD_GPIO
#endif

extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void rct_switch_core_freq(void);
extern void rct_fix_spiboot(void);
extern void rct_set_sd_pll(u32 freq_hz);
extern void rct_set_sdxc_pll(u32 freq_hz);

#define SDMMC_FREQ_48MHZ 48000000

#ifdef __DEBUG_BUILD__
/* UART function */
extern void uart_init(void);
extern void uart_putchar(char c);
extern void uart_getchar(char *c);
extern void uart_puthex(u32 h);
extern void uart_putbyte(u32 h);
extern void uart_putdec(u32 d);
extern int uart_putstr(const char *str);
extern int uart_poll(void);
extern int uart_read(void);
extern void uart_flush_input(void);
extern int uart_wait_escape(void);
extern int uart_getstr(char *str, int n);
extern int uart_getcmd(char *cmd, int n);
extern int uart_getblock(char *buf, int n);
#endif

/* GPIO functions */
extern void gpio_init(void);
extern void gpio_config_hw(int gpio);
extern void gpio_config_sw_in(int gpio);
extern void gpio_config_sw_out(int gpio);
extern void gpio_set(int gpio);
extern void gpio_clr(int gpio);
extern int gpio_get(int gpio);

#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD)  || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2) || \
    (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SDIO)
extern int sdmmc_init(int slot, u32 type);
extern int sdmmc_boot(void);
#elif (FIRMWARE_CONTAINER == SCARDMGR_SLOT_CF)
#endif

#ifdef __cplusplus
}
#endif

#endif	/* __ASM__ */
#endif	/* __AMBPROM_IMPL__ */

#endif	/* __PROM_H__ */
