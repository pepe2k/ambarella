/**
 * system/src/bld/bldfunc.h
 *
 * Boot loader function prototypes.
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __BLDFUNC_H__
#define __BLDFUNC_H__

#include <basedef.h>
#include <config.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <hal/hal.h>

#include <fio/sdboot.h>

#if defined(AMBOOT_DEFAULT_LINUX_MACHINE_ID)
#define AMBARELLA_LINUX_MACHINE_ID		(AMBOOT_DEFAULT_LINUX_MACHINE_ID)
#else
#define AMBARELLA_LINUX_MACHINE_ID		(1223)
#endif

#if (CHIP_REV == A2S) && defined(FIRMWARE_CONTAINER)
#define RTPLL_LOCK_ICACHE		1
#endif

/* Enable this macro to change core/idsp frequence gradually. */
/* #define USE_RTPLL_CHANGE	1 */


#ifndef __ASM__

__BEGIN_C_PROTO__

#ifndef K_ASSERT
#ifdef __DEBUG_BUILD__
#define K_ASSERT(x) {							\
		if (!(x)) {						\
			putstr("Assertion failed!(");			\
			putstr(__FILE__);				\
			putstr(":");					\
			putdec(__LINE__);				\
			putstr(")");					\
			for (;;);					\
		}							\
	}
#else
#define K_ASSERT(x) {							\
		if (!(x)) {						\
			for (;;);					\
		}							\
	}
#endif
#endif


/* IRQ functions */
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void vic_init(void);
#define VIRQ_RISING_EDGE	0
#define VIRQ_FALLING_EDGE	1
#define VIRQ_BOTH_EDGES		2
#define VIRQ_LEVEL_LOW		3
#define VIRQ_LEVEL_HIGH		4
extern void vic_set_type(u32 line, u32 type);
extern void vic_enable(u32 line);
extern void vic_disable(u32 line);
extern void vic_ackint(u32 line);
extern void vic_sw_set(u32 line);
extern void vic_sw_clr(u32 line);

/* PLL and clock generator functions */
extern void pll_check(void);

/* Core functions */
extern void setup_pagetbl(void);
extern void enable_mmu(void);
extern void disable_mmu(void);
extern void _enable_icache(void);
extern void _disable_icache(void);
extern void _enable_dcache(void);
extern void _disable_dcache(void);
extern void _flush_i_cache(void);
extern void _flush_d_cache(void);
extern void _clean_d_cache(void);
extern void _clean_flush_d_cache(void);
extern void _clean_flush_all_cache(void);
extern void _drain_write_buffer(void);
extern void flush_all_cache_region(void *addr, unsigned int size);
extern void clean_flush_all_cache_region(void *addr, unsigned int size);
extern void flush_i_cache_region(void *addr, unsigned int size);
extern void flush_d_cache_region(void *addr, unsigned int size);
extern void clean_d_cache_region(void *addr, unsigned int size);
extern void clean_flush_d_cache_region(void *addr, unsigned int size);
extern void drain_write_buffer(u32 addr);
extern void clean_d_cache(void *addr, unsigned int size);
extern void flush_d_cache(void *addr, unsigned int size);
extern void clean_flush_d_cache(void *addr, unsigned int size);
extern int lock_i_cache_region(void *addr, unsigned int size);
extern void unlock_i_cache_ways(unsigned int ways);
extern int pli_cache_region(void *addr, unsigned int size);

/* GPIO functions */
extern void gpio_init(void);
extern void gpio_config_hw(int gpio);
extern void gpio_config_sw_in(int gpio);
extern void gpio_config_sw_out(int gpio);
extern void gpio_set(int gpio);
extern void gpio_clr(int gpio);
extern int gpio_get(int gpio);

/* Util functions */
extern char *strcat(register char *src, register const char *append);
extern char *strcpy(char *dst, const char *src);

extern void *memcpy(void *dst, const void *src, unsigned int n);
extern void memzero(void *dst, unsigned int n);
extern int  memcmp(const void *dst, const void *src, unsigned int n);
extern int memcmp_oob(const void *dst, const void *src, u32 main_size, u32 spare_size);
extern void *memset(void *s, int c, unsigned int n);
extern u32  crc32(const void *buf, unsigned int size);
extern void led_on(void);
extern void led_off(void);
extern int check_mem_access(u32 start_addr, u32 end_addr);
extern int flash_timing(int minmax, int val);

#define FLASH_TIMING_MIN(x) flash_timing(0, x)
#define FLASH_TIMING_MAX(x) flash_timing(1, x)

/* Firmware program functions */
extern int flprog_validate_image(u8 *image, unsigned int len);

extern int flprog_get_part_table(flpart_table_t *table);
extern int flprog_set_part_table(flpart_table_t *table);
extern int flprog_get_meta(flpart_meta_t * table);
extern int flprog_set_meta(void);

extern void get_part_size(int *part_size);
extern int flprog_update_part_info_from_meta();

/* NAND functions */
extern int nand_init(void);
extern int nand_is_init(void);
extern int nand_reset(void);
extern int nand_mark_bad_block(u32 block);
extern int nand_is_bad_block(u32 block);
extern int nand_read_data(u8 *dst, u8 *src, int len);
extern int nand_read_pages(u32 block, u32 page, u32 pages, const u8 *main_buf,
					const u8 *spare_buf, u32 enable_ecc);
extern int nand_prog_pages(u32 block, u32 page, u32 pages, const u8 *main_buf,
					const u8 *spare_buf);
extern int nand_prog_pages_noecc(u32 block, u32 page, u32 pages, const u8 *buf);
extern int nand_read_spare(u32 block, u32 page, u32 pages, const u8 *buf);
extern int nand_prog_spare(u32 block, u32 page, u32 pages, const u8 *buf);
extern int nand_erase_block(u32 block);
extern int nand_update_bb_info(void);

#if defined(NAND_SUPPORT_BBT)
extern int nand_scan_bbt(int verbose);
extern int nand_update_bbt(u32 bblock, u32 gblock);
extern int nand_erase_bbt(void);
extern int nand_isbad_bbt(u32 block);
extern int nand_show_bbt(void);
extern int nand_has_bbt(void);
#endif

/* ONENAND functions */
extern int onenand_init(void);
extern int onenand_reset(void);
extern int onenand_is_bad_block(u32 block);
extern int onenand_read_data(u8 *dst, u8 *src, int len);
extern int onenand_read_pages(u32 block, u32 page, u32 pages, const u8 *buf);
extern int onenand_prog_pages(u32 block, u32 page, u32 pages, const u8 *buf);
extern int onenand_read_spare(u32 block, u32 page, u32 pages, const u8 *buf);
extern int onenand_prog_spare(u32 block, u32 page, u32 pages, const u8 *buf);
extern int onenand_erase_block(u32 block);
extern int onenand_read_reg(u8 bank, u16 addr, u16 * single_r_data);
extern int onenand_write_reg(u8 bank, u16 addr, const u16 single_w_data);
extern int onenand_update_bb_info(void);

/* NOR functions */
extern int nor_init(void);
extern int nor_reset(void);
extern int nor_read_data(u8 *dst, u8 *src, int len);
extern int nor_read_sector(u32 sector, u8 *buf, int len);
extern int nor_prog_sector(u32 sector, u8 *buf, int len);
extern int nor_erase_sector(u32 sector);
extern int nor_lock(u32 sector);
extern int nor_unlock(u32 sector);
extern int nor_lock_down(u32 sector);
extern int nor_read_cfi(u8 bank, u32 *cfi);
extern int nor_read_id(u8 bank, u32 *id);

/* SNOR functions */
extern u32 snor_blocks_to_len(u32 block, u32 blocks);
extern u32 snor_len_to_blocks(u32 start_block, u32 len);
extern u32 snor_get_addr(u32 block ,u32 offset);
extern void snor_get_block_offset(u32 addr, u32 *block, u32 *offset);
extern int snor_init(void);
extern int snor_reset(void);
extern int snor_read(u32 addr, u32 len, u8 *buf);
extern int snor_prog(u32 addr, u32 len, const u8 *buf);
extern int snor_read_block(u32 block, u32 blocks, u8 *buf);
extern int snor_prog_block(u32 block, u32 blocks, u8 *buf);
extern int snor_erase_block(u32 block);
extern int snor_erase_chip(u8 bank);
extern int snor_cfi_query(u8 bank);
extern int snor_autosel(u8 bank, u32 addr, u16 *buf);

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

/* sector media */
extern int sm_dev_init(int slot);
extern int sm_read_data(unsigned char *dst, unsigned char *src, int len);
extern int sm_read_sector(int sector, unsigned char *buf, int len);
extern int sm_write_sector(int sector, unsigned char *buf, int len);
extern int sm_erase_sector(int sector, int sectors);
extern int create_parameter(int target, struct sdmmc_header_s *param);
extern int sm_is_init(int id);

/* SD/MMC functions */
extern int sdmmc_init(int slot, int device_type);
#define SDMMC_TYPE_AUTO		0x0
#define SDMMC_TYPE_SD		0x1
#define SDMMC_TYPE_SDHC		0x2
#define SDMMC_TYPE_MMC		0x3
#define SDMMC_TYPE_MOVINAND	0x4

extern int sdmmc_command(int command, int argument, int timeout);
extern int sdmmc_read_sector(int sector, int sectors, unsigned int *target);
extern int sdmmc_write_sector(int sector, int sectors, unsigned int *image);
extern int sdmmc_erase_sector(int sector, int sectors);
extern u32 sdmmc_get_total_sectors(void);
extern int sdmmc_prog_bootp(void);
extern int sdmmc_prog_accp(void);

/* DSP */
extern int set_dsp_buf_memory(u32 addr, u32 size);
extern int load_dsp_ucode_data(u32 device, u32 addr);
extern int move_dsp_ucode_data(u32 addr);
extern int move_aorc_ucode(void);
#if defined(SHOW_AMBOOT_SPLASH)
extern int dsp_vout_init(void);
extern int dsp_vout_set(int chan, int res);
#endif

/***************************************************/
/* Functions for getting/setting system properties */
/* and anything else that involves the RCT module. */
/***************************************************/
extern u32 get_apb_bus_freq_hz(void);
extern u32 get_ahb_bus_freq_hz(void);
extern u32 get_core_bus_freq_hz(void);
extern u32 get_arm_bus_freq_hz(void);
extern u32 get_ddr_freq_hz(void);
extern u32 get_idsp_freq_hz(void);
extern u32 get_adc_freq_hz(void);
extern u32 get_uart_freq_hz(void);
extern u32 get_ssi_freq_hz(void);
extern u32 get_ssi2_freq_hz(void);
extern u32 get_motor_freq_hz(void);
extern u32 get_ir_freq_hz(void);
extern u32 get_host_freq_hz(void);
extern u32 get_sd_freq_hz(void);

extern u32 rct_boot_from(void);
#define BOOT_FROM_BYPASS	0x00008000
#define BOOT_FROM_HIF		0x00004000
#define BOOT_FROM_NAND		0x00000001
#define BOOT_FROM_NOR		0x00000002
#define BOOT_FROM_ONENAND	0x00000100
#define BOOT_FROM_SNOR		0x00000200
#define BOOT_FROM_FLASH		(BOOT_FROM_NAND | BOOT_FROM_NOR | \
				 BOOT_FROM_ONENAND | BOOT_FROM_SNOR)
#define BOOT_FROM_NOR_FLASH	(BOOT_FROM_NOR | BOOT_FROM_SNOR)
#define BOOT_FROM_NAND_FLASH	(BOOT_FROM_NAND | BOOT_FROM_ONENAND)

#define BOOT_FROM_SPI		0x00000004
#define BOOT_FROM_SD		0x00000010
#define BOOT_FROM_SDHC		0x00000020
#define BOOT_FROM_MMC		0x00000040
#define BOOT_FROM_MOVINAND	0x00000080
#define BOOT_FROM_SDMMC		(BOOT_FROM_SD	| BOOT_FROM_SDHC	| \
				 BOOT_FROM_MMC	| BOOT_FROM_MOVINAND	| \
				 BOOT_FROM_EMMC)

#define BOOT_FROM_EMMC		0x00010000

extern void rct_pll_init(void);
extern int rct_is_cf_trueide(void);
extern int rct_is_eth_enabled(void);
extern void rct_reset_chip(void);
extern void rct_reset_fio(void);
extern void rct_switch_core_freq(void);
extern void rct_switch_idsp_freq(void);
extern void rct_set_uart_pll(void);
extern void rct_set_ssi_pll(void);
extern void rct_set_ssi2_pll(void);

extern void rct_set_sd_pll(u32 freq_hz);
extern void rct_set_sdxc_pll(u32 freq_hz);

extern void rct_enable_usb(void);
extern void rct_suspend_usb(void);
extern void rct_x_usb_clksrc(void);
extern void rct_usb_reset(void);
extern void fio_exit_random_mode(void);
extern void enable_fio_dma(void);
extern void clock_source_select(int src);

/* Decompression functions */
extern u32 decompress(const char *name,
		      u32 input_start,
		      u32 input_end,
		      u32 output_start,
		      u32 free_mem_ptr_p,
		      u32 free_mem_ptr_end_p);

/* Romfs functions */
extern int romfs_init(void);
extern int romfs_is_init(void);
extern int romfile_size_name(const char *file);
extern int romfile_name_load(const char *file, u8 *ptr, unsigned int len,
		             unsigned int fpos);

/* String utilities */
extern int  strlen(const char *s);
extern int  strcmp(const char *s1, const char *s2);
extern int  strncmp(const char *s1, const char *s2, unsigned int maxlen);
extern char *strncpy(char *dest, const char *src, unsigned int n);
extern char *strlcpy(char *dest, const char *src, unsigned int n);
extern int  strtou32(const char *str, u32 *value);
extern char *strfromargv(char *s, unsigned int slen, int argc, char **argv);
extern char *strtok(char *str, const char *token);
extern int str_to_ipaddr(const char *, u32 *);
extern int str_to_hwaddr(const char *, u8 *);

/* Timer functions */
#define TIMER1_ID	1
#define TIMER2_ID	2
#define TIMER3_ID	3
#define TIMER8_ID	8

extern void timer_init(void);
extern void timer_reset_count(int tmr_id);
extern u32  timer_get_count(int tmr_id);
extern void timer_enable(int tmr_id);
extern void timer_disable(int tmr_id);
extern u32 timer_get_tick(int tmr_id);
extern u32 timer_tick2ms(u32 s_tck, u32 e_tck);
extern void timer_dly_ms(int tmr_id, u32 dly_tim);

/* UART functions */
extern void uart_init(void);
extern void uart_putchar(char c);
extern void uart_getchar(char *c);
extern void uart_puthex(u32 h);
extern void uart_putbyte(u32 h);
extern void uart_putdec(u32 d);
extern int  uart_putstr(const char *str);
extern int  uart_poll(void);
extern int  uart_read(void);
extern void uart_flush_input(void);
extern int  uart_wait_escape(int time);
extern int  uart_getstr(char *str, int n, int timeout);
extern int  uart_getcmd(char *cmd, int n, int timeout);
extern int  uart_getblock(char *buf, int n, int timeout);
extern void uart1_init(void);
extern int  uart1_putstr(const char *str);

/* SPI functions */
extern void spi_init(void);
extern void spi_config(u8 spi_id, u8 spi_mode, u8 fs, u32 baud_rate);
extern int spi_write(u8 spi_id, u16 *buffer, u16 n_size);
extern int spi_read(u8 spi_id, u16 *buffer, u16 n_size);
extern int spi_write_read(u8 spi_id, u16 *w_buffer, u16 *r_buffer, u16 w_size,
			  u16 r_size);
extern void spi2_config(u8 spi_id, u8 spi_mode, u8 cfs_dfs, u32 baud_rate);
extern int spi2_write(u8 spi_id, u16 *buffer, u16 n_size);
extern int spi2_read(u8 spi_id, u16 *buffer, u16 n_size);
extern int spi2_write_read(u8 spi_id, u16 *w_buffer,  u16 *r_buffer, u16 w_size,
			   u16 r_size);
#if defined(BOARD_SUPPORT_SPI_PMIC_WM831X)
extern void spi_pmic_wm831x_config(void);
extern int spi_pmic_wm831x_reg_read(u16 reg_addr, u16 *reg_value);
extern int spi_pmic_wm831x_reg_write(u16 reg_addr, u16 reg_value);
extern void amboot_wm831x_setreg(u16 reg, u16 val, u16 mask, u16 shift);
extern int amboot_wm831x_reg_unlock(void);
extern void amboot_wm831x_reg_lock(void);
extern void amboot_wm831x_set_slp_slot(u16 reg, u16 slot);
extern int amboot_wm831x_auxadc_read(int input);
#define WM831X_SLP_SLOT_MASK			((u16)(~0xE000))
#define WM831X_SLP_SLOT_SHIFT			(13)
#define WM831X_POWER_OFF_MASK		((u16)(~0xEC8F))
#define WM831X_POWER_OFF_SHIFT		(0)
#define WM831X_POWER_SLP_MASK		((u16)(~0x040F))
#define WM831X_POWER_SLP_SHIFT		(0)
#endif

#if defined(BOARD_SUPPORT_I2C_PMIC_TPS6586XX)
extern void idc_init(void);
extern void idc_wr_tps6586xx(u16 regaddr, u16 pdata);
extern void idc_rd_tps6586xx(u16 regaddr, u16 *pdata);
#endif

#if defined(AMBARELLA_CRYPTO_REMAP)
extern int crypto_remap(void);
#endif


#define putchar	uart_putchar
#define puthex	uart_puthex
#define putbyte uart_putbyte
#define putdec	uart_putdec
#define putstr	uart_putstr

/* Downloader utility functions */
extern int  xmodem_recv(char *buf, int buf_len);

/* Command functions */

#define CMDHIST_KEY_UP	'\025'
#define CMDHIST_KEY_DN	'\004'

extern int cmdhist_push(const char *cmd);
extern int cmdhist_next(char **cmd);
extern int cmdhist_prev(char **cmd);
extern int cmdhist_reset(void);

#define COMMAND_MAGIC	0x436d6420

typedef int(*cmdfunc_t)(int, char **);

typedef struct cmdlist_s
{
	u32	magic;
	char	*name;
	char	*help;
	cmdfunc_t	fn;
	struct cmdlist_s	*next;
} cmdlist_t;

#define __COMMAND __attribute__((unused, __section__(".cmdlist")))

#define __CMDLIST(f, nm, hlp)				\
	cmdlist_t __COMMAND##f __COMMAND = {		\
		magic:	COMMAND_MAGIC,			\
		name:	nm,				\
		help:	hlp,				\
		fn:	f				\
	}

extern cmdlist_t *G_commands;

extern void commands_init(void);
extern int  parse_command(char *cmdline);

#define MAX_CMDLINE_LEN	1024	/* atag cmdling */
#define MAX_CMD_ARGS	32
#define MAX_ARG_LEN	64


/* USB functions */
extern void enable_usb_reg(void);
extern u32  usb_download(void *addr, int exec, int flag);
extern int  check_connected(void);
extern int  usb_check_connected(void);
extern void usb_boot(u8 usbdl_mode);
extern void usb_disconnect(void);
/* Flags used by usb_boot() */
#define USB_DL_NORMAL		0x01
#define USB_DL_DIRECT_USB	0x02
#define USB_MODE_DEFAULT	USB_DL_DIRECT_USB
/* Flags used by usb_download() */
#define USB_FLAG_FW_PROG	0x0001
#define USB_FLAG_KERNEL		0x0002
#define USB_FLAG_MEMORY		0x0004
#define USB_FLAG_UPLOAD		0x0010
#define USB_FLAG_TEST_DOWNLOAD	0x0100
#define USB_FLAG_TEST_PLL	0x0200
#define USB_FLAG_TEST_MASK	(USB_FLAG_TEST_DOWNLOAD | USB_FLAG_TEST_PLL)

/* Network device(s) functions */
#if (ETH_INSTANCES >= 1)
#include <bldnet.h>
#endif

/* SATA functions */
#if (CHIP_REV == I1)
extern void sata_phy_init(void);
#endif

/*********************************/
/* Booting and loading functions */
/*********************************/
extern const u32 hotboot_valid;
extern const u32 hotboot_pattern;
typedef int (*boot_fn_t)(const char *, int);
extern int bios(const char *, int);
extern int boot(const char *, int);
extern int netboot(const char *, int);
extern void setup_tags(void *, const char *, u32, u32, u32, u32, int);
#ifdef CONFIG_KERNEL_DUAL_CPU
extern void setup_arm_tags(void *, const char *, u32, u32, u32, u32, int);
#endif
extern int setup_extra_cmdline(const char *);
extern void jump_to_kernel(void *);
#ifdef ENABLE_AMBOOT_TEST_REBOOT
extern void test_reboot(void);
#endif

extern unsigned int __bld_hugebuf;
extern unsigned int g_hugebuf_addr;
/****************************************************************************/
/* BSP specific function should implement the 'amboot_bsp_entry()' function */
/****************************************************************************/
/**
 * The amboot_bsp_entry function is a call-out function from the main()
 * function of the BLD (2nd-stage boot-loader of AMBoot). It is a weak
 * symbol - meaning that the symbol is optional. If the symbol is supplied
 * by the user in an external library, then it is called out to perform
 * board-specific initializations.
 *
 * If the return value is exactly '1', then the boot-loader will attempt to
 * load the PBA_CODE.
 */
extern int amboot_bsp_entry(void) __attribute__ ((weak));

__END_C_PROTO__

#endif  /* !__ASM__ */

#if defined(__ASM__)

#ifndef __ALIGN
#define __ALIGN		.align 4,0x90
#define __ALIGN_STR	".align 4,0x90"
#endif

#define ALIGN __ALIGN
#define ALIGN_STR __ALIGN_STR

#ifndef ENTRY
#define ENTRY(name) \
  .globl name; \
  ALIGN; \
  name:
#endif

#ifndef END
#define END(name) \
  .size name, .-name
#endif

#if 0
#ifndef ENDPROC
#define ENDPROC(name) \
  .type name, @function; \
  END(name)
#endif
#else
#ifndef ENDPROC
#define ENDPROC(name) \
  END(name)
#endif
#endif

/*
 * Endian independent macros for shifting bytes within registers.
 */
#ifndef __ARMEB__
#define pull            lsr
#define push            lsl
#define get_byte_0      lsl #0
#define get_byte_1	lsr #8
#define get_byte_2	lsr #16
#define get_byte_3	lsr #24
#define put_byte_0      lsl #0
#define put_byte_1	lsl #8
#define put_byte_2	lsl #16
#define put_byte_3	lsl #24
#else
#define pull            lsl
#define push            lsr
#define get_byte_0	lsr #24
#define get_byte_1	lsr #16
#define get_byte_2	lsr #8
#define get_byte_3      lsl #0
#define put_byte_0	lsl #24
#define put_byte_1	lsl #16
#define put_byte_2	lsl #8
#define put_byte_3      lsl #0
#endif

#define PLD(code...)
#define CALGN(code...)

#endif /* __ASM__ */

#if defined(AMBARELLA_LINUX_LAYOUT)
#include <linux.h>
#endif

#endif

