/**
 * system/src/bld/linux.h
 *
 * Boot loader function prototypes.
 *
 * History:
 *    2010/06/09 - [Anthony Ginger] created file
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __LINUX_H__
#define __LINUX_H__

/* ==========================================================================*/
#define SIZE_1MB				(1024 * 1024)
#define MEM_SIZE_1MB_MASK			(SIZE_1MB - 1)

#define DEFAULT_SSS_START			(0xC00F7000)
#define DEFAULT_SSS_MAGIC0			(0x19790110)
#define DEFAULT_SSS_MAGIC1			(0x19450107)
#define DEFAULT_SSS_MAGIC2			(0x19531110)

#define AMBARELLA_BOARD_TYPE_AUTO		(0)
#define AMBARELLA_BOARD_TYPE_BUB		(1)
#define AMBARELLA_BOARD_TYPE_EVK		(2)
#define AMBARELLA_BOARD_TYPE_IPCAM		(3)
#define AMBARELLA_BOARD_TYPE_VENDOR		(4)
#define AMBARELLA_BOARD_TYPE_ATB		(5)

#define AMBARELLA_BOARD_CHIP_AUTO		(0)

#define AMBARELLA_BOARD_REV_AUTO		(0)

#define AMBARELLA_BOARD_VERSION(c,t,r)		(((c) << 16) + ((t) << 12) + (r))
#define AMBARELLA_BOARD_CHIP(v)			(((v) >> 16) & 0xFFFF)
#define AMBARELLA_BOARD_TYPE(v)			(((v) >> 12) & 0xF)
#define AMBARELLA_BOARD_REV(v)			(((v) >> 0) & 0xFFF)

#define CORTEX_BST_MAGIC			(0xffaa5500)
#define CORTEX_BST_INVALID			(0xdeadbeaf)
#define CORTEX_BST_START_COUNTER		(0xffffffff)
#define CORTEX_BST_WAIT_LIMIT			(0x00010000)
#define AXI_CORTEX_RESET(x)			((x) << 5)
#define AXI_CORTEX_CLOCK(x)			((x) << 8)

#define CORTEX_BOOT_ADDRESS			(0x00000000)
#define CORTEX_BOOT_STRAP_ALIGN			(32)
#if ((CHIP_REV == S2) && defined(AMBOOT_DEV_BOOT_CORTEX))
#define CORTEX_TO_ARM11(x)			((x) + 0xC0000000)
#define ARM11_TO_CORTEX(x)			((x) - 0xC0000000)
#else
#define CORTEX_TO_ARM11(x)			(x)
#define ARM11_TO_CORTEX(x)			(x)
#endif

#define DEFAULT_BAPI_AOSS_PART			PART_SWP
#define DEFAULT_BAPI_OFFSET			(0x000F2000)
#define DEFAULT_BAPI_BACKUP_OFFSET		(0x000F1000)
#define DEFAULT_BAPI_NAND_OFFSET		(8)
#define DEFAULT_BAPI_SM_OFFSET			(8)

#define DEFAULT_BAPI_TAG_MAGIC			(0x19450107)
#define DEFAULT_BAPI_MAGIC			(0x19790110)
#define DEFAULT_BAPI_VERSION			(0x00000001)
#define DEFAULT_BAPI_SIZE			(4096)
#define DEFAULT_BAPI_AOSS_SIZE			(1024)

#define DEFAULT_BAPI_AOSS_MAGIC			(0x19531110)

#define DEFAULT_BAPI_REBOOT_MAGIC		(0x4a32e9b0)
#define AMBARELLA_BAPI_CMD_REBOOT_NORMAL	(0xdeadbeaf)
#define AMBARELLA_BAPI_CMD_REBOOT_RECOVERY	(0x5555aaaa)
#define AMBARELLA_BAPI_CMD_REBOOT_FASTBOOT	(0x555aaaa5)
#define AMBARELLA_BAPI_CMD_REBOOT_SELFREFERESH	(0x55aaaa55)
#define AMBARELLA_BAPI_CMD_REBOOT_HIBERNATE	(0x5aaaa555)

#define AMBARELLA_BAPI_REBOOT_HIBERNATE		(0x1 << 0)
#define AMBARELLA_BAPI_REBOOT_SELFREFERESH	(0x1 << 1)

#ifdef CONFIG_KERNEL_DUAL_CPU
#define BOOT_DUAL_KERNEL_RAM_SIZE_CORTEX		(KERNEL_ARM_DUAL_KERNEL_RAM_START - KERNEL_RAM_START)
#endif

/* ==========================================================================*/
#ifndef __ASM__

__BEGIN_C_PROTO__

/* ==========================================================================*/
enum ambarella_bapi_cmd_e {
	AMBARELLA_BAPI_CMD_INIT			= 0x0000,

	AMBARELLA_BAPI_CMD_AOSS_INIT		= 0x1000,
	AMBARELLA_BAPI_CMD_AOSS_COPY_PAGE	= 0x1001,
	AMBARELLA_BAPI_CMD_AOSS_SAVE		= 0x1002,

	AMBARELLA_BAPI_CMD_SET_REBOOT_INFO	= 0x2000,
	AMBARELLA_BAPI_CMD_CHECK_REBOOT		= 0x2001,

	AMBARELLA_BAPI_CMD_UPDATE_FB_INFO	= 0x3000,
};

struct ambarella_bapi_aoss_page_info_s {
	u32					src;
	u32					dst;
	u32					size;
};

struct ambarella_bapi_aoss_s {
	u32					fn_pri[256 - 4];
	u32					magic;
	u32					total_pages;
	u32					copy_pages;
	u32					page_info;
};

struct ambarella_bapi_reboot_info_s {
	u32					magic;
	u32					mode;
	u32					flag;
	u32					rev;
};

struct ambarella_bapi_fb_info_s {
	int					xres;
	int					yres;
	int					xvirtual;
	int					yvirtual;
	int					format;
	u32					fb_start;
	u32					fb_length;
	u32					bits_per_pixel;
};

struct ambarella_bapi_s {
	u32					magic;
	u32					version;
	int					size;
	u32					crc;
	u32					mode;
	u32					block_dev;
	u32					block_start;
	u32					block_num;
	u32					rev0[64 - 8];
	struct ambarella_bapi_reboot_info_s	reboot_info;
	u32					fb_start;
	u32					fb_length;
	struct ambarella_bapi_fb_info_s		fb0_info;
	struct ambarella_bapi_fb_info_s		fb1_info;
	u32					rev1[64 - 4 - 8 - 8 - 2];
	u32					debug[128];
	u32					rev2[1024 - 128 - 128 - 256];
	struct ambarella_bapi_aoss_s		aoss_info;
};

struct ambarella_bapi_tag_s {
	u32					magic;
	u32					pbapi_info;
};

/* ==========================================================================*/
#define BUG_ON(x) {							\
		if (x) {						\
			putstr("BUG_ON(");				\
			putstr(__FILE__);				\
			putstr(":");					\
			putdec(__LINE__);				\
			putstr(")");					\
			for (;;);					\
		}							\
	}

/* ==========================================================================*/
extern void cmd_cortex_pre_init(int verbose,
	u32 ctrl, u32 ctrl2, u32 ctrl3, u32 frac);
extern int cmd_cortex_boot(const char *cmdline, int verbose);
extern int cmd_cortex_bios(const char *cmdline, int verbose);
extern int cmd_cortex_boot_net(const char *cmdline, int verbose);
extern int cmd_cortex_resume(u32 code_address, u32 data_address, int verbose);

extern int cmd_bapi_init(int verbose);
extern void cmd_bapi_aoss_resume_boot(boot_fn_t fn, int verbose,
	int argc, char *argv[]);
extern void cmd_bapi_aoss_boot_incoming(int verbose);
extern int cmd_bapi_reboot_set(int verbose, u32 mode);
extern int cmd_bapi_reboot_get(int verbose, u32 *mode);
extern int cmd_bapi_set_fb_info(int verbose, u32 id, int xres, int yres,
	int xvirtual, int yvirtual, int format, u32 bits_per_pixel);
extern u32 cmd_bapi_get_splash_fb_start(void);
extern u32 cmd_bapi_get_splash_fb_length(void);

#if (AMBARELLA_PPM_SIZE > 0)
#if (AMBARELLA_PPM_SIZE <= SIZE_1MB)
#undef AMBARELLA_PPM_SIZE
#define AMBARELLA_PPM_SIZE	SIZE_1MB
#warning "Change AMBARELLA_PPM_SIZE to SIZE_1MB"
#endif
#if (AMBARELLA_PPM_SIZE & MEM_SIZE_1MB_MASK)
#error "AMBARELLA_PPM_SIZE must aligned to SIZE_1MB"
#endif
#endif

static inline void ambarella_linux_mem_layout(u32 *kernelp, u32 *kernels,
	u32 *bsbp, u32 *bsbs, u32 *dspp, u32 *dsps)
{
	*bsbp = BSB_RAM_START & (~MEM_SIZE_1MB_MASK);
	*dspp = IDSP_RAM_START & (~MEM_SIZE_1MB_MASK);
	*kernelp = DRAM_START_ADDR + AMBARELLA_PPM_SIZE;
#ifdef CONFIG_KERNEL_DUAL_CPU
	*kernels = BOOT_DUAL_KERNEL_RAM_SIZE_CORTEX;
#else
	*kernels = *bsbp - *kernelp;
#endif
	*bsbs = (*dspp - *bsbp);
	*dsps = (DRAM_SIZE - (*dspp - DRAM_START_ADDR));
}

#ifdef CONFIG_KERNEL_DUAL_CPU
static inline void ambarella_linux_arm_mem_layout(u32 *kernelp, u32 *kernels,
	u32 *bsbp, u32 *bsbs, u32 *dspp, u32 *dsps)
{
	*bsbp = BSB_RAM_START & (~MEM_SIZE_1MB_MASK);
	*dspp = IDSP_RAM_START & (~MEM_SIZE_1MB_MASK);
	*kernelp = DRAM_START_ADDR + AMBARELLA_PPM_SIZE + BOOT_DUAL_KERNEL_RAM_SIZE_CORTEX;
	*kernels = *bsbp - *kernelp;
	*bsbs = (*dspp - *bsbp);
	*dsps = (DRAM_SIZE - (*dspp - DRAM_START_ADDR));
}
#endif

static inline void amboot_basic_reinit(int verbose)
{
#if (USE_HAL == 1)
#if (PHY_BUS_MAP_TYPE >= 1)
	amb_set_peripherals_base_address((void *)HAL_BASE_VIRT,
		(void *)APB_BASE, (void *)AHB_BASE, (void *)DRAM_VIRT_BASE);
#else
	amb_set_peripherals_base_address((void *)HAL_BASE_VIRT,
		(void *)APB_BASE, (void *)AHB_BASE);
#endif
#endif
	vic_init();
	gpio_init();
	uart_init();
	timer_init();
#if (defined(ENABLE_SD) && defined(FIRMWARE_CONTAINER))
	sm_dev_init(FIRMWARE_CONTAINER);
#endif
#if (defined(ENABLE_FLASH) && !defined(CONFIG_NAND_NONE))
	nand_init();
	nand_reset();
#endif
}

#if (__GNUC__ == 3)
extern int amboot_bsp_self_refresh_enter(void) __attribute__ ((weak));
#else
extern int amboot_bsp_self_refresh_enter(void) __attribute__ ((weak, noinline, aligned(32)));
#endif
extern int amboot_bsp_self_refresh_exit(void) __attribute__ ((weak));
extern int amboot_bsp_self_refresh_check_valid(void) __attribute__ ((weak));
extern int amboot_bsp_power_off(void) __attribute__ ((weak));
extern int amboot_bsp_hw_init(void) __attribute__ ((weak));
extern int amboot_bsp_system_init(void) __attribute__ ((weak));
extern int amboot_bsp_check_usbmode(void) __attribute__ ((weak));
extern int amboot_bsp_netphy_init(void) __attribute__ ((weak));
extern int amboot_bsp_cortex_init_pre(void) __attribute__ ((weak));
extern int amboot_bsp_cortex_init_boot(void) __attribute__ ((weak));
extern int amboot_bsp_cortex_init_post(void) __attribute__ ((weak));

__END_C_PROTO__

#endif  /* !__ASM__ */

#endif  /* __LINUX_H__ */

