/**
 * system/src/bld/cmd_cortex.c
 *
 * History:
 *    2010/08/25 - [Anthony Ginger] created file
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
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

#include <atag.h>

extern int fwprog_load_partition(int dev, int part_id, const flpart_t *part,
	int verbose, int override);
extern void cortex_bst_entry(void);
extern void cortex_bst_head(void);
extern void cortex_bst_end(void);
extern int nand_load(u32 sblk, u32 nblk, u32 mem_addr, u32 img_len);

#define PROCESSOR_START_0	(0)
#define PROCESSOR_START_1	(1)
#define PROCESSOR_START_2	(2)
#define PROCESSOR_START_3	(3)

#define PROCESSOR_STATUS_0	(4)
#define PROCESSOR_STATUS_1	(5)
#define PROCESSOR_STATUS_2	(6)
#define PROCESSOR_STATUS_3	(7)

#define ICDISER0_MASK		(8)
#define INTDIS_LOCKER		(9)
#define INTDIS_STATUS		(10)

#define MACHINE_ID		(11)
#define ATAG_DATA		(12)

#define PROCESSOR0_R0		(15)

#define BOOT_BACKUP_SIZE	(3)

#define CORTEX_BAPI_IRQ		SOFTWARE_INT_VEC(0)

static u32 *pcortex_start_add = NULL;
static u32 cortex_boot_add = 0;
static u32 *pcortex_boot_head_add = NULL;
static u32 cortex_boot_size = 0;
static u32 cortex_head_size = 0;
static u32 boot_backuped = 0;
static u32 boot_backup[BOOT_BACKUP_SIZE];
static u32 cortex_pre_inited = 0;

static int cmd_cortex_load_bootstrap(u32 start_address, u32 code_address,
	int verbose, u32 r0,
	int setup_atag, const char *cmdline,
	u32 initrd2_start, u32 initrd2_size)
{
	int					rval = 0;
	int					i;

	pcortex_start_add = (u32 *)CORTEX_TO_ARM11(start_address);
	cortex_boot_add = ARM11_TO_CORTEX((u32)cortex_bst_entry);
	pcortex_boot_head_add = (u32 *)cortex_bst_head;
	cortex_boot_size = (u32)(cortex_bst_end - cortex_bst_entry);
	cortex_head_size = (u32)(cortex_bst_end - cortex_bst_head);

	if (boot_backuped == 0) {
		for (i = 0; i < BOOT_BACKUP_SIZE; i++)
			boot_backup[i] = pcortex_start_add[i];
		boot_backuped = 1;
	}

	pcortex_start_add[0] = 0xe59f0000;		//ldr	r0, [pc, #0]
	pcortex_start_add[1] = 0xe12fff10;		//bx	r0
	pcortex_start_add[2] = cortex_boot_add;
	clean_d_cache(pcortex_start_add, (BOOT_BACKUP_SIZE * sizeof(u32)));

	pcortex_boot_head_add[PROCESSOR_START_0] =
		ARM11_TO_CORTEX(code_address);
	pcortex_boot_head_add[PROCESSOR_STATUS_0] = CORTEX_BST_START_COUNTER;
	pcortex_boot_head_add[MACHINE_ID] = AMBARELLA_LINUX_MACHINE_ID;
	if (verbose) {
		putstr("loading jmp: 0x");
		puthex(start_address);
		putstr(" size: 0x");
		puthex(BOOT_BACKUP_SIZE);
		putstr("\r\n");
		putstr("BST: 0x");
		puthex(pcortex_start_add[2]);
		putstr(" size: 0x");
		puthex(cortex_boot_size);
		putstr("\r\n");
		putstr("HEAD: 0x");
		puthex((u32)pcortex_boot_head_add);
		putstr(" size: 0x");
		puthex((u32)cortex_head_size);
		putstr("\r\n");
	}

	if (setup_atag) {
		setup_tags((void *)code_address, cmdline, DRAM_START_ADDR,
			DRAM_SIZE, initrd2_start, initrd2_size, verbose);
		pcortex_boot_head_add[ATAG_DATA] =
			ARM11_TO_CORTEX((u32)atag_data);
	}
	if (verbose) {
		putstr("PROCESSOR_START_0: 0x");
		puthex(pcortex_boot_head_add[PROCESSOR_START_0]);
		putstr(" MACHINE_ID: 0x");
		puthex(pcortex_boot_head_add[MACHINE_ID]);
		putstr(" ATAG_DATA: 0x");
		puthex(pcortex_boot_head_add[ATAG_DATA]);
		putstr("\r\n");
	}

	if (r0) {
		pcortex_boot_head_add[PROCESSOR0_R0] = r0;
		if (verbose) {
			putstr("PROCESSOR0_R0: 0x");
			puthex(pcortex_boot_head_add[PROCESSOR0_R0]);
			putstr("\r\n");
		}
	}

	clean_d_cache(pcortex_boot_head_add, cortex_head_size);

	return rval;
}

static int cmd_cortex_load(u32 start_address,
	const char *cmdline, int bios, int verbose)
{
	int					rval = 0;
	flpart_table_t				ptb;
	u32					rmd_start = 0;
	u32					rmd_size = 0;

	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto cmd_cortex_load_exit;

	if (bios) {
		rval = fwprog_load_partition(g_part_dev[PART_PBA], PART_PBA,
			&ptb.part[PART_PBA], verbose, 0);
		if (rval < 0)
			goto cmd_cortex_load_exit;
		rval = cmd_cortex_load_bootstrap(start_address,
			ptb.part[PART_PBA].mem_addr, verbose, 0, 1,
			cmdline, rmd_start, rmd_size);
	} else {
		rval = fwprog_load_partition(g_part_dev[PART_SEC], PART_SEC,
			&ptb.part[PART_SEC], verbose, 0);
		if (rval < 0)
			goto cmd_cortex_load_exit;

		rval = fwprog_load_partition(g_part_dev[PART_RMD], PART_RMD,
			&ptb.part[PART_RMD], verbose, 0);
		if (rval >= 0x0) {
			rmd_start = ptb.part[PART_RMD].mem_addr;
			if (rval == 0x0)
				rmd_size  = ptb.part[PART_RMD].img_len;
			else
				rmd_size  = rval;
		}
		rval = cmd_cortex_load_bootstrap(start_address,
			ptb.part[PART_SEC].mem_addr, verbose, 0, 1,
			((cmdline == NULL) ? ptb.dev.cmdline : cmdline),
			rmd_start, rmd_size);
	}

cmd_cortex_load_exit:
	return rval;
}

static int cmd_cortex_load_net(u32 start_address,
	const char *cmdline, int verbose)
{
	int rval = 0;
#if (ETH_INSTANCES >= 1)
	flpart_table_t ptb;
	u32 rmd_start = 0;
	u32 rmd_size = 0;

	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		goto cmd_cortex_load_net_exit;
	if (ptb.dev.tftpd == 0x0) {
		if (verbose)
			putstr("Invalid: tftpd!\r\n");
		rval = -5;
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_net_wait_ready(DEFAULT_NET_WAIT_TMO);
	if (rval < 0) {
		if (verbose)
			putstr("link down ...\r\n");
		bld_net_down();
		bld_net_init(verbose);
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_udp_bind(ptb.dev.tftpd, 0);
	if (rval < 0) {
		if (verbose)
			putstr("Bind UDP fail ...\r\n");
		goto cmd_cortex_load_net_exit;
	}

	rval = bld_tftp_load(ptb.dev.rmd_addr, ptb.dev.rmd_file, verbose);
	if (rval >= 0) {
		rmd_start = ptb.dev.rmd_addr;
		rmd_size  = rval;
	}

	rval = bld_tftp_load(ptb.dev.pri_addr, ptb.dev.pri_file, verbose);
	if (rval >= 0) {
		rval = cmd_cortex_load_bootstrap(start_address,
			ptb.dev.pri_addr, verbose, 0, 1,
			((cmdline == NULL) ? ptb.dev.cmdline : cmdline),
			rmd_start, rmd_size);
	}

	bld_udp_close();

cmd_cortex_load_net_exit:
#endif
	return rval;
}

void cmd_cortex_pre_init(int verbose,
	u32 ctrl, u32 ctrl2, u32 ctrl3, u32 frac)
{
	u32 cortex_ctl;

	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_RESET(0x3);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	timer_dly_ms(TIMER3_ID, 1);
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl |= AXI_CORTEX_CLOCK(0x3);
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	timer_dly_ms(TIMER3_ID, 1);
	if (ctrl) {
		ctrl &= ~(0x00200000);
		writel(PLL_CORTEX_FRAC_REG, frac);
		timer_dly_ms(TIMER3_ID, 1);
		writel(PLL_CORTEX_CTRL_REG, ctrl);
		timer_dly_ms(TIMER3_ID, 1);
		writel(PLL_CORTEX_CTRL_REG, (ctrl | 0x01));
		timer_dly_ms(TIMER3_ID, 1);
		writel(PLL_CORTEX_CTRL_REG, ctrl);
		timer_dly_ms(TIMER3_ID, 1);
		writel(PLL_CORTEX_CTRL2_REG, ctrl2);
		timer_dly_ms(TIMER3_ID, 1);
		writel(PLL_CORTEX_CTRL3_REG, ctrl3);
		timer_dly_ms(TIMER3_ID, 1);
	} else {
		ctrl = readl(PLL_CORTEX_CTRL_REG);
		if (ctrl & (0x00200000)) {
			ctrl &= ~(0x00200000);
			writel(PLL_CORTEX_CTRL_REG, ctrl);
			timer_dly_ms(TIMER3_ID, 1);
			writel(PLL_CORTEX_CTRL_REG, (ctrl | 0x01));
			timer_dly_ms(TIMER3_ID, 1);
			writel(PLL_CORTEX_CTRL_REG, ctrl);
			timer_dly_ms(TIMER3_ID, 1);
		}
	}
	cortex_ctl = readl(AHB_SECURE_REG(0x04));
	cortex_ctl &= ~(AXI_CORTEX_RESET(0x3));
	writel(AHB_SECURE_REG(0x04), cortex_ctl);
	timer_dly_ms(TIMER3_ID, 1);

	cortex_pre_inited = 1;
}

static int cmd_cortex_init(int verbose)
{
	u32 cortex_ctl;
	int i, j;
#ifdef CONFIG_KERNEL_DUAL_CPU
	int rval=0;
	flpart_table_t ptb;
	u32 arm_kernel_jump_addr = 0;
	u32 rmd_start = RMD_ARM_DUAL_KERNEL_RAM_START;
	u32 rmd_size = 0;
#endif

	if ((cortex_boot_add == 0) || (cortex_boot_size == 0)) {
		if (verbose)
			putstr("Skip bad bootstrap!\r\n");
		return -1;
	}

#ifdef CONFIG_KERNEL_DUAL_CPU
	rval = flprog_get_part_table(&ptb);
	if (rval < 0)
		putstr("get part table failed!\r\n");

	rval = fwprog_load_partition(g_part_dev[PART_PRI], PART_PRI,
		&ptb.part[PART_PRI], verbose, 0);
	arm_kernel_jump_addr = ptb.part[PART_PRI].mem_addr;
	if (rval < 0)
		putstr("get pri_partition image failed!\r\n");

	rval = fwprog_load_partition(g_part_dev[PART_RMD], PART_RMD,
		&ptb.part[PART_RMD], verbose, 0);
	if (rval < 0)
		putstr("get rmd_partition image failed!\r\n");
	rmd_size = (ptb.part[PART_RMD].img_len);
	memcpy((void *)rmd_start,(void *)(ptb.part[PART_RMD].mem_addr),rmd_size);
#endif

#if (ETH_INSTANCES >= 1)
	bld_net_down();
#endif
	vic_init();
	vic_sw_clr(CORTEX_BAPI_IRQ);
	disable_interrupts();

	if (amboot_bsp_cortex_init_pre != NULL) {
		amboot_bsp_cortex_init_pre();
	} else {
		if (cortex_pre_inited == 0) {
			cmd_cortex_pre_init(verbose, 0x00000000,
				0x00000000, 0x00000000, 0x00000000);
		}
	}
	_drain_write_buffer();
	_clean_flush_all_cache();

	if (amboot_bsp_cortex_init_boot != NULL) {
		amboot_bsp_cortex_init_boot();
	} else {
		cortex_ctl = readl(AHB_SECURE_REG(0x04));
		cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x3));
		writel(AHB_SECURE_REG(0x04), cortex_ctl);
	}

	i = 0;
	while (1) {
		if (i > CORTEX_BST_WAIT_LIMIT) {
			i = 0;
			cortex_ctl = readl(AHB_SECURE_REG(0x04));
			if (verbose) {
				putstr("Timeout: Reset Core0 [0x");
				puthex(cortex_ctl);
				putstr("]...\r\n");
				uart1_init();
				uart1_putstr("cortex boot timeout\r\n");
			}
			cortex_ctl |= AXI_CORTEX_RESET(0x3);
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl |= AXI_CORTEX_CLOCK(0x3);
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl &= ~(AXI_CORTEX_RESET(0x3));
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
			for (j = 0; j < 100; j++) {
				cortex_ctl = readl(AHB_SECURE_REG(0x04));
			}
			cortex_ctl &= ~(AXI_CORTEX_CLOCK(0x3));
			writel(AHB_SECURE_REG(0x04), cortex_ctl);
		}
		flush_d_cache(pcortex_boot_head_add, cortex_head_size);
		if (pcortex_boot_head_add[PROCESSOR_STATUS_0] == 0)
			break;
		i++;
	}

#ifdef CONFIG_KERNEL_DUAL_CPU
	timer_dly_ms(TIMER3_ID, 2000);
	setup_arm_tags((void *)arm_kernel_jump_addr, "console=ttyS1", DRAM_START_ADDR, DRAM_SIZE, rmd_start, rmd_size, 1);
	jump_to_kernel((void *)arm_kernel_jump_addr);
#endif

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	vic_set_type(CORTEX_BAPI_IRQ, VIRQ_LEVEL_HIGH);
	vic_enable(CORTEX_BAPI_IRQ);
	if (amboot_bsp_cortex_init_post != NULL) {
		amboot_bsp_cortex_init_post();
	}
	_drain_write_buffer();
	__asm__ __volatile__ ("mov	r0, #0" : : : "r0");
	__asm__ __volatile__ ("mcr	p15, 0, r0, c7, c0, 4" : : : "r0");
	vic_sw_clr(CORTEX_BAPI_IRQ);
	vic_disable(CORTEX_BAPI_IRQ);
	_flush_d_cache();
#else
	__asm__ __volatile__ ("bkpt");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
	__asm__ __volatile__ ("nop");
#endif

	if (boot_backuped == 1) {
		for (i = 0; i < BOOT_BACKUP_SIZE; i++) {
			pcortex_start_add[i] = boot_backup[i];
		}
		clean_d_cache(pcortex_start_add,
			(BOOT_BACKUP_SIZE * sizeof(u32)));
		_flush_i_cache();
	}

#if defined(CONFIG_AMBOOT_BAPI_SUPPORT)
	cmd_bapi_aoss_boot_incoming(verbose);
#else
	amboot_basic_reinit(verbose);
	enable_interrupts();
#endif

	return 0;
}

void ambarella_cortex_atag_entry(int verbose)
{
	if (cortex_boot_size != 0) {
		params->hdr.tag = ATAG_AMBARELLA_BST;
		params->hdr.size = tag_size(tag_mem32);
		params->u.mem.start = cortex_boot_add;
		params->u.mem.size = cortex_boot_size;
		params = tag_next(params);
		if (verbose) {
			putstr("Cortex BST: 0x");
			puthex(cortex_boot_add);
			putstr(" size: 0x");
			puthex(cortex_boot_size);
			putstr("\r\n");
		}
	}
}

int cmd_cortex_boot(const char *cmdline, int verbose)
{
	int					rval = 0;

	rval = cmd_cortex_load(CORTEX_BOOT_ADDRESS, cmdline, 0, verbose);
	if (rval)
		goto cmd_cortex_boot_exit;

	rval = cmd_cortex_init(verbose);

cmd_cortex_boot_exit:
	return rval;
}

int cmd_cortex_bios(const char *cmdline, int verbose)
{
	int					rval = 0;

	rval = cmd_cortex_load(CORTEX_BOOT_ADDRESS, cmdline, 1, verbose);
	if (rval)
		goto cmd_cortex_boot_exit;

	rval = cmd_cortex_init(verbose);

cmd_cortex_boot_exit:
	return rval;
}

int cmd_cortex_boot_net(const char *cmdline, int verbose)
{
	int					rval = 0;

	rval = cmd_cortex_load_net(CORTEX_BOOT_ADDRESS, cmdline, verbose);
	if (rval)
		goto cmd_cortex_boot_net_exit;

	rval = cmd_cortex_init(verbose);

cmd_cortex_boot_net_exit:
	return rval;
}

int cmd_cortex_resume(u32 code_address, u32 data_address, int verbose)
{
	int					rval = 0;

	rval = cmd_cortex_load_bootstrap(CORTEX_BOOT_ADDRESS, code_address,
		verbose, data_address, 0, NULL, 0, 0);
	if (rval)
		goto cmd_cortex_boot_exit;

	rval = cmd_cortex_init(verbose);

cmd_cortex_boot_exit:
	return rval;
}

static int cmd_cortex(int argc, char *argv[])
{
	char					*pcmdline = NULL;
	char					cmdline[MAX_CMDLINE_LEN];
	int					valid_cmd = 0;

	if (argc > 1) {
		if (strcmp(argv[1], "fload") == 0) {
			valid_cmd = 1;
			cmd_cortex_load(CORTEX_BOOT_ADDRESS, pcmdline, 0, 1);
		} else
		if (strcmp(argv[1], "nload") == 0) {
			valid_cmd = 1;
			cmd_cortex_load_net(CORTEX_BOOT_ADDRESS, pcmdline, 1);
		} else
		if (strcmp(argv[1], "init") == 0) {
			valid_cmd = 1;
			cmd_cortex_init(1);
		} else
		if (strcmp(argv[1], "fboot") == 0) {
			if (argc >= 3) {
				strfromargv(cmdline, sizeof(cmdline),
					(argc - 2), &argv[2]);
				pcmdline = cmdline;
			}
			valid_cmd = 1;
			cmd_cortex_boot(pcmdline, 1);
		} else
		if (strcmp(argv[1], "nboot") == 0) {
			if (argc >= 3) {
				strfromargv(cmdline, sizeof(cmdline),
					(argc - 2), &argv[2]);
				pcmdline = cmdline;
			}
			valid_cmd = 1;
			cmd_cortex_boot_net(pcmdline, 1);
		}
	}

	if (valid_cmd == 0) {
		putstr("Type 'help cortex' for help\r\n");
		return -8;
	}

	return 0;
}

static char help_cortex[] =
	"\r\ncortex [f/n][boot/load] <cmdline>\r\n"
	"cortex init \r\n"
	"cortex status \r\n"
	"\r\n";

__CMDLIST(cmd_cortex, "cortex", help_cortex);

