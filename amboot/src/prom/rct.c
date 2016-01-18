/**
 * system/src/prom/rct.c
 *
 * History:
 *    2008/09/30 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define __AMBPROM_IMPL__
#include "prom.h"
#include <hal/hal.h>

#ifdef __DEBUG_BUILD__
#define DEBUG_MSG uart_putstr
#else
#define DEBUG_MSG(...)
#endif

#if (CHIP_REV == A5S)
/* Turn off mini hal to reduce code size. */
#undef	USE_MINI_HAL
#else
#undef	USE_MINI_HAL
#endif

#if (CHIP_REV == A5S)
#include "a5s_spiboot_param.h"
void* memset(void*, int, unsigned int);

void rct_fix_spiboot(void)
{
	amb_dram_parameters_t param;
	memset(&param, 0x0, sizeof(amb_dram_parameters_t));
	param.dram_dqs_sync = DRAM_DQS_SYNC;
	param.dram_pad_zctl = DRAM_PAD_ZCTL;
	param.dll0 = DRAM_DLL0;
	param.dll1 = DRAM_DLL1;
	param.delay = 0;
	amb_mini_fix_spiboot(&param);
}
#else
void rct_fix_spiboot(void){}
#endif

static u32 get_sm_boot_device(void)
{
	u32 rval = 0x0;

#if defined(FIRMWARE_CONTAINER_TYPE)
#if (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_SD)
		rval |= BOOT_FROM_SD;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_SDHC)
		rval |= BOOT_FROM_SDHC;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_MMC)
		rval |= BOOT_FROM_MMC;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_MOVINAND)
		rval |= BOOT_FROM_MOVINAND;
#elif (FIRMWARE_CONTAINER_TYPE == SDMMC_TYPE_AUTO)
		/* rval |= BOOT_FROM_SDMMC; */
#endif
#endif
		return rval;
}

#ifdef	USE_MINI_HAL

/* This function return the device boot from in different boot mode. */
u32 rct_boot_from(void)
{
#ifdef	RCT_BOOT_FROM
	u32 rval = 0x0;

	/* The device boot from is specified by user. */
	rval = RCT_BOOT_FROM;
#else
	u32 rval = 0x0;
	u32 sm;
	amb_boot_type_t type;

	type = amb_mini_get_boot_type();

	if (type == AMB_NAND_BOOT) {
		rval |= BOOT_FROM_NAND;
	} else if (type == AMB_NOR_BOOT) {
		rval |= BOOT_FROM_NOR;
	} else if (type == AMB_SD_BOOT) {
		rval |= get_sm_boot_device();

	} else if (type == AMB_SSI_BOOT || type == AMB_XIP_BOOT) {
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();

	} else if (type == AMB_USB_BOOT) {
		sm = get_sm_boot_device();
		if (sm) {
			rval |= sm;
		} else {
			rval |= BOOT_FROM_NAND;
		}
	} else if (type == AMB_HIF_BOOT) {
		rval |= BOOT_FROM_HIF;
	}
#endif

	return rval;
}

u32 get_core_bus_freq_hz(void)
{
	return (u32) amb_mini_get_core_clock_frequency();
}

void rct_set_sd_pll(u32 freq_hz)
{
	if (amb_mini_set_sd_clock_frequency(48000000) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_mini_set_sd_clock_frequency() failed\r\n");
	}
}

void rct_switch_core_freq(void)
{
	/* No support in mini hal. */
}

#else

/*
 *	A6	SD_BOOT[11]	NAND_READ_CONFIRM[6]	2K[5]
 *	A2S/M	SD_BOOT[11]	RDY_PL[13]		HIF type[12]
 *----------------------------------------------------------------------
 *	SD		1		1		0
 *	SDHC		1		0		1
 *	MMC		1		0		0
 *	MoviNAND	1		1		1
 */

#if (CHIP_REV == A5S)
#define PLL_FREQ_HZ	24000000
#else
#define PLL_FREQ_HZ	REF_CLK_FREQ
#endif

u32 rct_boot_from(void)
{
	u32 rval = 0x0;
	u32 val = readl(SYS_CONFIG_REG);

	if ((val & SYS_CONFIG_FLASH_BOOT) != 0x0) {
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
	}

#if ((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
     (CHIP_REV == A2K)) || (CHIP_REV == A5S) || (CHIP_REV == A7) || \
     (CHIP_REV == A5L) || (CHIP_REV == I1)

	if ((val & SYS_CONFIG_SPI_BOOT) != 0x0) {
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();
	}

	if (((val & SYS_CONFIG_FLASH_BOOT) == 0x0) &&
	    ((val & SYS_CONFIG_SPI_BOOT) == 0x0)) {
		/* USB boot */

		/* Force enabling flash access on USB boot */
#if defined(FIRMWARE_CONTAINER)
		rval |= BOOT_FROM_SPI;
		rval |= get_sm_boot_device();
#else
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
#endif	/* FIRMWARE_CONTAINER */
	}

#elif (CHIP_REV == A6)

	if ((val & SYS_CONFIG_SPI_BOOT) != 0x0) {
		/* SPI boot */
		/* NOTE : need to synchronize with AMBProm code,
			   if AMBProm code change */
		if (((val & SYS_CONFIG_FLASH_BOOT) == 0x0) &&
		    ((val & SYS_CONFIG_SD_BOOT) == 0x0)) {
		    	rval |= BOOT_FROM_SPI;
		    	rval |= get_sm_boot_device();

		/* SD boot */
		} else if (((val & SYS_CONFIG_FLASH_BOOT) == 0x0) &&
			   ((val & SYS_CONFIG_SD_BOOT) != 0x0)) {
		    	rval |= get_sm_boot_device();

		/* SPIB boot */
		/* NOTE : need to synchronize with AMBProm code,
			   if AMBProm code change */
		} else if ((val & SYS_CONFIG_FLASH_BOOT) != 0x0) {
		    	rval |= get_sm_boot_device();
		}
	}

#endif

	if ((val & SYS_CONFIG_BOOT_BYPASS) != 0x0) {
		rval |= BOOT_FROM_BYPASS;
	}

	return rval;
}

static u32 get_core_pll_freq(u32 z, u32 f, u32 c)
{
        u32 intprog, sout, sdiv, p, n;

        f = PLL_FRAC_VAL(f);
        intprog = PLL_CTRL_INTPROG(c);
        sout = PLL_CTRL_SOUT(c);
        sdiv = PLL_CTRL_SDIV(c);
#if ((CHIP_REV == A2S) || (CHIP_REV == A2M) || (CHIP_REV == A2Q) || \
     (CHIP_REV == A2K))
	p = n = 1;
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
      (CHIP_REV == I1)
	{
		u32 s = readl(SCALER_CORE_POST_REG);
		p = SCALER_POST(s);
		n = 1;
	}
#else
	/* A6 and later chips. */
	{
		u32 s = readl(SCALER_CORE_PRE_REG);
        	p = SCALER_PRE_P(s);
        	n = SCALER_PRE_N(s);

        	if (p != 1 && p != 2 && p != 3 && p != 5 && p != 7 &&
        	    p != 9 && p != 11 && p != 13 && p != 17 && p != 19 &&
        	    p != 23 && p != 29 && p != 32)
        	        p = 2;
	}
#endif

        if (f != 0) {
                /* If fraction part, return 0 to be a warning. */
                return 0;
        }

        intprog += 1 + z;
        sdiv++;
        sout++;

        return PLL_FREQ_HZ * intprog * sdiv / sout / p / n / 2;
}

#if (CHIP_REV == A5L)
static u32 _get_core_pll_out_freq_hz(void)
{
 	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
 	u32 f = readl(PLL_CORE_FRAC_REG);
	u32 c = readl(PLL_CORE_CTRL_REG);

	return get_core_pll_freq(z, f, c);
}
#endif

u32 _get_core_bus_freq_hz(u32 pll_core_ctrl)
{
#if     defined(__FPGA__)
	return 48000000;
#else
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);

	return get_core_pll_freq(z, f, pll_core_ctrl);
#endif
}

u32 get_core_bus_freq_hz(void)
{
	u32 pll_core_ctrl = readl(PLL_CORE_CTRL_REG);

	return _get_core_bus_freq_hz(pll_core_ctrl);
}

void rct_set_sd_pll(u32 freq_hz)
{
#define DUTY_CYCLE_CONTRL_ENABLE	0x01000000 /* Duty cycle correction */
	u32 scaler;
	u32 core_freq;

	K_ASSERT(freq_hz != 0);

#if (CHIP_REV == A5L)
	/* Program the SD clock generator */
	core_freq = _get_core_pll_out_freq_hz();
	scaler = (core_freq % freq_hz) ? (core_freq / freq_hz) + 1 : (core_freq / freq_hz);
#else
	/* Scaler = core_freq *2 / desired_freq */
	core_freq = get_core_bus_freq_hz();
	scaler = ((core_freq << 1) / freq_hz) + 1;
#endif
	/* Sdclk = core_freq * 2 / Int_div */
	/* For example: Sdclk = 108 * 2 / 5 = 43.2 Mhz */
	/* For example: Sdclk = 121.5 * 2 / 5 = 48.6 Mhz */
	writel(SCALER_SD48_REG,
		(readl(SCALER_SD48_REG) & 0xffff0000) |
		(DUTY_CYCLE_CONTRL_ENABLE | scaler));
}

void rct_set_sdxc_pll(u32 freq_hz)
{
#if (FIRMWARE_CONTAINER == SCARDMGR_SLOT_SD2)
		if (amb_mini_set_sdxc_clock_frequency(freq_hz) != AMB_HAL_SUCCESS) {
				DEBUG_MSG("rct_set_sd_clock_frequency() failed\r\n");
		}
#endif
}

#ifdef USE_RTPLL_CHANGE

#define do_some_delay()		for (i = 0; i < 50000; i++);

u32 get_expect_core_freq(void)
{
	u32 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	u32 pll_core_ctrl;

	if (expect_pll_core_val == 0xffffffff)
		return 0;

	pll_core_ctrl = (readl(PLL_CORE_CTRL_REG) & ~0x1);
	if (pll_core_ctrl != (expect_pll_core_val & ~0x1)) {
		return _get_core_bus_freq_hz(expect_pll_core_val);
	} else {
		return 0;
	}
}
u32 _rct_switch_core_freq(u32 val)
{
	int i;
	u32 reg, freq;
	u32 intprog, sdiv, sout, valwe;

	freq = get_core_bus_freq_hz();

	if (val > freq) {
		do {
			reg = readl(PLL_CORE_CTRL_REG);
			intprog = PLL_CTRL_INTPROG(reg);
			sout = PLL_CTRL_SOUT(reg);
			sdiv = PLL_CTRL_SDIV(reg);
			valwe = PLL_CTRL_VALWE(reg);

			intprog++;

			reg = PLL_CTRL_VAL(intprog, sout, sdiv, valwe);
			writel(PLL_CORE_CTRL_REG, (reg & 0xfffffffe));

			/* PLL write enable */
			writel(PLL_CORE_CTRL_REG, (reg | 0x00000001));

			while ((readl(PLL_LOCK_REG) & PLL_LOCK_CORE) !=
					PLL_LOCK_CORE);

			do_some_delay();

			freq = get_core_bus_freq_hz();
		} while (freq < val);
	} else if (val < freq) {
		do {
			reg = readl(PLL_CORE_CTRL_REG);
			intprog = PLL_CTRL_INTPROG(reg);
			sout = PLL_CTRL_SOUT(reg);
			sdiv = PLL_CTRL_SDIV(reg);
			valwe = PLL_CTRL_VALWE(reg);

			intprog--;

			reg = PLL_CTRL_VAL(intprog, sout, sdiv, valwe);
			writel(PLL_CORE_CTRL_REG, (reg & 0xfffffffe));

			/* PLL write enable */
			writel(PLL_CORE_CTRL_REG, (reg | 0x00000001));

			while ((readl(PLL_LOCK_REG) & PLL_LOCK_CORE) !=
					PLL_LOCK_CORE);

			do_some_delay();

			freq = get_core_bus_freq_hz();
		} while (freq > val);
	}

	return freq;
}

void rct_switch_core_freq(void)
{
	u32 val = get_expect_core_freq();
#ifdef	DDR_BIAS_CURRENT
	writel(DLL_CTRL_SEL_REG, DDR_BIAS_CURRENT);
#endif
	if (val == 0)
		return;

#ifndef	PROM_NO_SWITCH_CORE_FREQ
	_rct_switch_core_freq(val);
#endif
}
#else
void rct_switch_core_freq(void)
{
	u32 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	u32 pll_core_ctrl;

#ifdef	DDR_BIAS_CURRENT
	writel(DLL_CTRL_SEL_REG, DDR_BIAS_CURRENT);
#endif
	if (expect_pll_core_val == 0xffffffff)
		return;

	pll_core_ctrl = (readl(PLL_CORE_CTRL_REG) & ~0x1);
#ifndef	PROM_NO_SWITCH_CORE_FREQ
	if (pll_core_ctrl != (expect_pll_core_val & ~0x1)) {
		writel(PLL_CORE_CTRL_REG, (expect_pll_core_val & ~0x1));
		writel(PLL_CORE_CTRL_REG, (expect_pll_core_val | 0x1));
	}
#endif /* PROM_NO_SWITCH_CORE_FREQ */
}
#endif
#endif /* USE_MINI_HAL */

