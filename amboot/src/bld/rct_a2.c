/**
 * system/src/bld/rct_a2.c
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

u32 _get_core_bus_freq_hz(u32 pll_core_ctrl)
{
#if     defined(__FPGA__)
	return 48000000;
#else
	u32 n = pll_core_ctrl;
	u32 x = (n & 0x00030000) >> 16;
	u32 y = (n & 0x0000c000) >> 8;
	u32 m = readl(SCALER_CORE_PRE_REG);

	n = (n & 0xff000000) >> 24;

	switch (x) {
	case 0x0: x = 8; break;
	case 0x1: x = 4; break;
	case 0x2: x = 2; break;
	case 0x3: x = 1; break;
	}

	switch (y) {
	case 0x00: y = 8; break;
	case 0x40: y = 4; break;
	case 0x80: y = 2; break;
	case 0xc0: y = 1; break;
	}

	return PLL_FREQ_HZ * n / 2 / x * y / m;
#endif
}

u32 get_core_bus_freq_hz(void)
{
	u32 pll_core_ctrl = readl(PLL_CORE_CTRL_REG);

	return _get_core_bus_freq_hz(pll_core_ctrl);
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
	return PLL_FREQ_HZ / readb(SCALER_ADC_REG);
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

#if     defined(A2_METAL_REV_A1)
	/* A2_A1 chip uses apb bus clock frequency */
     	return (get_apb_bus_freq_hz() / readl(CG_HOST_REG));
#else
	return PLL_FREQ_HZ / readl(CG_SSI_REG);
#endif
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

void rct_set_ssi_pll(void)
{
#define SPI_CLK_DIV 	0x2

	writel(CG_SSI_REG, SPI_CLK_DIV);

#if	defined(A2_METAL_REV_A1)
	writel(CG_HOST_REG, get_apb_bus_freq_hz() / 13500000);
#endif
}

u32 get_host_freq_hz(void)
{
#if	defined(__FPGA__)
	return get_apb_bus_freq_hz();
#else
	return (get_apb_bus_freq_hz() / readl(CG_HOST_REG));
#endif
}

u32 get_sd_freq_hz(void)
{
#if	defined(__FPGA__)
	return 24000000;
#else
	u32 scaler = readl(SCALER_SD48_REG) & SD48_INTEGER_DIV;
	return (get_core_bus_freq_hz() * 2 / scaler);
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

	if ((val & SYS_CONFIG_FLASH_BOOT) != 0x0) {
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
	} else {
		/* USB boot */

		/* Force enabling flash access on USB boot */
		if ((val & SYS_CONFIG_BOOTMEDIA) != 0x0)
			rval |= BOOT_FROM_NOR;
		else
			rval |= BOOT_FROM_NAND;
	}

	if ((val & SYS_CONFIG_BOOT_BYPASS) != 0x0) {
		rval |= BOOT_FROM_BYPASS;
	}

	return rval;
}

int rct_is_cf_trueide(void)
{
	return 0;
}

int rct_is_eth_enabled(void)
{
	return (readl(SYS_CONFIG_REG) & SYS_CONFIG_ENET_SEL) != 0x0;
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


#ifdef USE_RTPLL_CHANGE

#define do_some_delay()		for (i = 0; i < 50000; i++);

u32 get_expect_core_freq(void)
{
	u32 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	u32 pll_core_ctrl;

	if (expect_pll_core_val == 0xffff)
		return 0;

	pll_core_ctrl = readl(PLL_CORE_CTRL_REG);
	if (pll_core_ctrl != expect_pll_core_val) {
		return _get_core_bus_freq_hz(expect_pll_core_val);
	} else {
		return 0;
	}
}

u32 _rct_switch_core_freq(u32 val)
{
	u32 freq = val;
	int dir = 0;
	int i;

	freq = get_core_bus_freq_hz();
	if (val == freq)
		goto done;
	else if (val > freq)
		dir = 1;
	else
		dir = 0;

#ifdef CORE_PLL_FINE_ADJUST
	if (val == 183000000 || val == 189000000 ||
	    val == 216000000 || val == 243000000) {
		freq = readl(PLL_CORE_CTRL_REG) & 0xfff00000;
		writel(PLL_CORE_CTRL_REG, freq | CORE_PLL_FINE_ADJUST);
	}
#endif

	do {
		freq = readl(PLL_CORE_CTRL_REG) & 0xfff00000;

		if (dir) {
			freq += 0x01000000;
		} else {
			freq -= 0x01000000;
		}

		freq |= (readl(PLL_CORE_CTRL_REG) & 0x000fffff);
		writel(PLL_CORE_CTRL_REG, freq);

		do_some_delay();
		freq = get_core_bus_freq_hz();
	} while ((dir == 1 && freq < val) ||
		 (dir == 0 && freq > val));

done:
	return freq;
}

void rct_switch_core_freq(void)
{
	u32 val = get_expect_core_freq();

	if (val == 0)
		return;

	_rct_switch_core_freq(val);
}
#else
void rct_switch_core_freq(void)
{
	register u32 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	register u32 pll_core_ctrl;

	if (expect_pll_core_val == 0xffff)
		return;

	pll_core_ctrl = readl(PLL_CORE_CTRL_REG);
	if (pll_core_ctrl != expect_pll_core_val) {
		writel(PLL_CORE_CTRL_REG, expect_pll_core_val);
	}
}
#endif

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
#else
	/* Program UART RCT divider value to generate lower clock */
	writel(CG_UART_REG, 0x8);
#endif

}

void rct_set_sd_pll(u32 freq_hz)
{
	register u32 clk;

	/* Program the SD clock generator */
	clk = get_core_bus_freq_hz();

	if (clk >= 270000000) {
		/* Core == { 273MHz, 283MHz } */
		writeb(SCALER_SD48_REG, 0xc);
	} else if (clk >= 243000000) {
		/* Core == { 243MHz, 256Mhz } */
		writeb(SCALER_SD48_REG, 0xb);
	} else if (clk >= 230000000) {
		/* Core == 230MHz */
		writeb(SCALER_SD48_REG, 0xa);
	} else if (clk >= 216000000) {
		/* Core == 216MHz */
		writeb(SCALER_SD48_REG, 0x9);
	} else if (clk >= 182250000) {
		/* Core == 182.25MHz */
		writeb(SCALER_SD48_REG, 0x8);
	} else {
		/* Core below or equal to 135MHz */
		writeb(SCALER_SD48_REG, 0x6);
	}
}

void rct_set_ir_pll(void)
{
	writew(CG_IR_REG, 0x800);
}

void rct_usb_change (void)
{

}

void rct_set_usb_ext_clk(void)
{
}

void rct_set_usb_int_clk(void)
{
}

void rct_set_usb_debounce(void)
{
}

void rct_enable_usb(void)
{
	writel(PLL_USB_CTRL_REG, 0x1003FA0A); /* better jitter */
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 0x2);
}

void rct_suspend_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x02);
}

void rct_x_usb_clksrc(void)
{

}

void rct_usb_reset(void)
{
}
