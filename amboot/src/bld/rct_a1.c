/**
 * system/src/bld/rct_a1.c
 *
 * History:
 *    2005/07/25 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

#define PLL_FREQ_HZ	REF_CLK_FREQ

void rct_pll_init(void)
{

}

u32 get_apb_bus_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	return get_core_bus_freq_hz() >> 1;
#endif
}

u32 get_ahb_bus_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	return get_core_bus_freq_hz();
#endif
}

#if !defined(__FPGA__)

/**
 * Map of core register PLL value to core frequency.
 */
struct val_to_freq_s {
	u32 rctv;
	u32 freq;
};

#endif  /* __FPGA__ */

u32 get_core_bus_freq_hz(void)
{
#if     defined(__FPGA__)
	return 48000000;
#else
	u32 fcore, val;
	u32 nr, nf, od, no;

	val = readl(PLL_CORE_REG);

	od = (val & 0x0002) >> 1;
	no = od + 1;
	nf = (3 - no) * (((val & 0x00fc) >> 2) + 2);
	nr = ((val & 0x0f00) >> 8) + 1;
	fcore = PLL_FREQ_HZ * nf / nr / 2;

	return fcore;
#endif
}

u32 get_ddr_freq_hz(void)
{
	return get_core_bus_freq_hz();
}

u32 get_idsp_freq_hz(void)
{
	return get_core_bus_freq_hz();
}

static u32 aud_sysclk = 12288000;

u32 get_audio_freq_hz(void)
{
	return aud_sysclk; /* FIXME */
}

u32 get_adc_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readb(CG_ADC_REG);
#endif
}

void rct_set_adc_clk_src(int src)
{
}

u32 get_uart_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_UART_REG);
#endif
}

u32 get_ssi_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_SSI_REG);
#endif
}

u32 get_motor_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_MOTOR_REG);
#endif
}

u32 get_ir_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_IR_REG);
#endif
}

u32 get_host_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return PLL_FREQ_HZ / readl(CG_HOST_REG);
#endif
}

u32 get_sd_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	u32 cg_sd = readl(CG_SD_REG);
	return 2 * (get_core_bus_freq_hz() / (cg_sd + 2));
#endif
}

void get_stepping_info(int *chip, int *major, int *minor)
{
#if	!defined(__FPGA__)
	u32 val = readl(HOST_AHB_EARLY_TERMINATION_INFO);
	writel(HOST_AHB_EARLY_TERMINATION_INFO, 0x0);

	if (val == readl(HOST_AHB_EARLY_TERMINATION_INFO)) {
		*chip  = (val         & 0xff);
		*major = ((val >>  8) & 0xff);
		*minor = ((val >> 16) & 0xff);
	} else {
		writel(HOST_AHB_EARLY_TERMINATION_INFO, val);
		*chip  = 0x1;
		*major = 0x0;
		*minor = 0x0;
	}
#else
	*chip  = 0x0;
	*major = 0x0;
	*minor = 0x0;
#endif
}

u32 rct_boot_from(void)
{
	u32 rval = 0x0;
	u32 val = readl(SYS_CONFIG_REG);

	if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
		rval |= BOOT_FROM_NOR;
	else
		rval |= BOOT_FROM_NAND;

	if ((val & SYS_CONFIG_BOOT_BYPASS) != 0x0) {
		rval |= BOOT_FROM_BYPASS;
	}

	return rval;
}

int rct_is_cf_trueide(void)
{
	return (readl(SYS_CONFIG_REG) & SYS_CONFIG_NAND_CF_TRUE_IDE) != 0x0;
}

int rct_is_eth_enabled(void)
{
	return 0;
}

void rct_reset_chip(void)
{

#if	defined(__FPGA__)
	/* Do nothing... NOT supported! */
#else
	writel(SOFT_RESET_REG, 0x0);
	writel(SOFT_RESET_REG, 0x1);
#endif

}

void rct_reset_fio(void)
{
	volatile int c;

	writel(FIO_RESET_REG,
	       FIO_RESET_FIO_RST |
	       FIO_RESET_CF_RST  |
	       FIO_RESET_XD_RST  |
	       FIO_RESET_FLASH_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
}

#define FLUSH_WITH_NOP()					\
	for (i = 0; i < 100; i++)				\
		__asm__ __volatile__ ("nop");

void rct_switch_core_freq(void)
{
	register u16 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	register u16 pll_core;
	int i;

#ifdef __BSP_A1GUCLB__
	int val;

	/* The A1GUCLB board has peculiar settings for determining the */
	/* CORE PLL... All other configurations should *NOT* do what's */
	/* being done here, and should instead use the menuconfig      */
	/* options (FIX_CORE_xxx) to determine the PLL the normal way! */
	gpio_config_sw_in(GPIO(0));
	gpio_config_sw_in(GPIO(1));
	gpio_config_sw_out(GPIO(2));
	gpio_config_sw_out(GPIO(3));
	gpio_config_sw_out(GPIO(4));
	gpio_config_sw_out(GPIO(5));
	gpio_config_sw_out(GPIO(6));
	gpio_config_sw_in(GPIO(7));
	gpio_config_sw_in(GPIO(8));
	gpio_config_sw_in(GPIO(9));
	gpio_clr(GPIO(2));
	gpio_clr(GPIO(3));
	gpio_clr(GPIO(4));
	gpio_clr(GPIO(5));
	gpio_clr(GPIO(6));

	val = readl(GPIO0_DATA_REG) & 0x3;
	if ((readl(GPIO0_DATA_REG) >> 7) & 0x1)
		val |= 0x4;

	if ((readl(GPIO0_DATA_REG) >> 8) & 0x1)
		val |= 0x8;

	if ((readl(GPIO0_DATA_REG) >> 9) & 0x1)
		val |= 0x10;

	switch (val) {
	case 1:  /* yuv and raw2jpg 243MHz */
	case 5:  /* adc 243MHz */
	case 8:  /* yuv 243MHz */
	case 9:  /* raw2jpg 243MHz*/
	case 12: /* dram test 243MHz */
	case 13: /* smem test 243MHz */
	case 15: /* H264 480P decode 243MHz */
		expect_pll_core_val = PLL_CORE_243MHZ_VAL;
		break;
	default:
		expect_pll_core_val = PLL_CORE_216MHZ_VAL;
		break;
	}
#endif  /* __BSP_A1GUCLB__ */


	if (expect_pll_core_val == 0xffff)
		goto core_freq_done;

	if (readl(CG_SETTING_CHANGED_REG) != 0x0)
		goto core_freq_done;

	pll_core = readw(PLL_CORE_REG);
	if (pll_core != expect_pll_core_val) {
		writew(PLL_CORE_REG, expect_pll_core_val);
		FLUSH_WITH_NOP();

		writel(CG_SETTING_CHANGED_REG,
		       CG_SETTING_CHANGED_CORE_CLK |
		       CG_SETTING_CHANGED_CORE_PLL);

		/* The chip should reset at this point */
		i = 0x0;
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c0, 4" : : "r" (i));
	}

core_freq_done:
	/* We're here when the core clock has been fixed to the desired */
	/* frequency and possible chip reset has been completed. */

	return;
}

void rct_switch_idsp_freq(void)
{

}

#if	!defined(PRK_UART_115200) && \
	!defined(PRK_UART_57600) && \
	!defined(PRK_UART_38400) && \
	!defined(PRK_UART_19200)
#define PRK_UART_115200
#endif

void rct_set_uart_pll(void)
{

#if	defined(PRK_UART_38400) || \
	defined(PRK_UART_57600) || \
	defined(PRK_UART_115200)
	/* Program UART RCT divider value to generate higher clock */
	writel(CG_UART_REG, 0x2);
	writew(CG_SETTING_CHANGED_REG, CG_SETTING_CHANGED_UART);
	writew(CG_SETTING_CHANGED_REG, 0x0);
#else
	/* Program UART RCT divider value to generate lower clock */
	writel(CG_UART_REG, 0x8);
	writew(CG_SETTING_CHANGED_REG, CG_SETTING_CHANGED_UART);
	writew(CG_SETTING_CHANGED_REG, 0x0);
#endif

}

void rct_set_sd_pll(u32 freq_hz)
{
	register u32 clk;

	/* Program the SD clock generator */
	clk = get_core_bus_freq_hz();

	/* Enable sd pll */
	writel(SD_CLK_EN_REG, 0x1);

	if (clk > 216000000) {
		/* Any core rate greater than 216MHz */
		writeb(CG_SD_REG, 7);
	} else if (clk > 182250000) {
		/* Core == 216 MHz */
		writeb(CG_SD_REG, 7);
	} else if (clk > 162000000) {
		/* Core == 182.25 MHz */
		writeb(CG_SD_REG, 6);
	} else if (clk > 135000000) {
		/* Core == 162 MHz */
		writeb(CG_SD_REG, 5);
	} else {
		/* Core == 135 MHz */
		writeb(CG_SD_REG, 3);
	}

	/* Keep the SPI clock the same. */
	writew(CG_SSI_REG, readw(CG_SSI_REG));
	/* Make SD colck setting to take effect. */
	writew(CG_SETTING_CHANGED_REG, 0x0020);
	writew(CG_SETTING_CHANGED_REG, 0x0000);
}

void rct_set_ir_pll(void)
{
}

void rct_set_ssi_pll(void)
{
	writel(CG_SSI_REG, 0x2);
	writeb(CG_SD_REG, readb(CG_SD_REG));
	writew(CG_SETTING_CHANGED_REG, 0x0020);
	writew(CG_SETTING_CHANGED_REG, 0x0000);
}

void rct_usb_change (void)
{
	register u32 value;

	value = readl(CG_SETTING_CHANGED_REG);
	writel(CG_SETTING_CHANGED_REG,value & ~(0x404));
	writel(CG_SETTING_CHANGED_REG,value | (0x404));
}

void rct_set_usb_ext_clk(void)
{
	/* changed to ext clock */
	writel(USB_CLK_REG, 0x17);
	writel(ANA_PWR_REG, 0x02);
	rct_usb_change ();
}

void rct_set_usb_int_clk(void)
{
	writel(PLL_SO_REG, 0x3a);
	writel(CG_SO_REG,0x25);
	writel(ANA_PWR_REG,0x2);
	rct_usb_change ();
	writel(USB_CLK_REG,0x14);
}

void rct_set_usb_debounce(void)
{
	writel(CLK_DEBOUNCE_REG,0x1);
}

void rct_enable_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | USB_ON_BIT);
}

void rct_suspend_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x02);
}


void rct_set_usb_pll_on(void)
{
	int i;

	writel(PLL_SO_REG, 0x3a);
	writel(CG_SO_REG,0x25);
	rct_usb_change ();

	/* Wait for PLL on */
	for (i = 0; i < 0x10000; i++);
}

void rct_set_usb_pll_off(void)
{
	writel(PLL_SO_REG, 0x1);
	writel(CG_SO_REG,0x25);
	rct_usb_change ();
}

void rct_set_usb_phy_pll_on(void)
{
	int i;

	writel(USB_CLK_REG, readl(USB_CLK_REG) & ~0x20);

	/* Wait for PLL on */
	for (i = 0; i < 0x10000; i++);
}

void rct_set_usb_phy_pll_off(void)
{
	writel(USB_CLK_REG, readl(USB_CLK_REG) | 0x20);
}

void rct_x_usb_clksrc(void)
{

}

void rct_usb_reset(void)
{
}
