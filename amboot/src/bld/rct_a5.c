/**
 * system/src/bld/rct_a5.c
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
        /* Core pll force lock */
        writel(PLL_CORE_CTRL_REG, readl(PLL_CORE_CTRL_REG) | 0x00100000);

	/* PLL CTRL3 */
	/* Core */
	writel(0x70170104, 0x00068300);
	/* DDR */
	writel(0x70170114, 0x00068300);

	/* PLL CTRL2 */
	/* Core */
	writel(0x70170100, 0x3f710000);
	/* DDR */
	writel(0x70170110, 0x3f710000);
	/* ADC module power down */
	writel(0x70170198, (readl(0x70170198) | 0x00080000));

	/* Margin registers to have correct speed selection of vDSP' SRAM. */
	writel(0x70150108, 0x01000000);
	writel(0x70160100, 0x01000000);
	
	/* Clear the bit-0 to ensure the other reset triggers will not be masked 
	   after warm reset */
	writel(SOFT_RESET_REG, readl(SOFT_RESET_REG) & (~0x01));
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

#if 0	/* A5-A0 */
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	u32 c = readl(PLL_CORE_CTRL_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);
#else	/* A5-A1 */
	/* 0, 3 and 6 bit are reserved */
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	/* 12 ~ 19 bit are reserved */
	u32 c = (pll_core_ctrl & 0xfff00fff);
#endif	/* A5-Ax */

	u32 intprog;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;

	/*
	 * PLL_FREQ_HZ * intprog * sdiv /sout / 2
	 * sdiv = 1
	 * sout = 2
	 */
	return PLL_FREQ_HZ * intprog / 2 / 2;
#endif
}

u32 get_core_bus_freq_hz(void)
{
	u32 val = readl(PLL_CORE_CTRL_REG);

	return _get_core_bus_freq_hz(val);
}

static u32 get_idsp_dram_pll_freq(u32 z, u32 f, u32 c, u32 s)
{
	u32 intprog, sout, sdiv, p, n;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);
	p = SCALER_PRE_P(s);
	n = SCALER_PRE_N(s);

	if (p != 1 && p != 2 && p != 3 && p != 5 && p != 7 &&
	    p != 9 && p != 11 && p != 13 && p != 17 && p != 19 &&
	    p != 23 && p != 29 && p != 32)
		p = 2;

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;
	sdiv++;
	sout++;

	return PLL_FREQ_HZ * intprog * sdiv / sout / p / n / 2;

}

u32 get_ddr_freq_hz(void)
{
	u32 z = (readl(PLL_DDR_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_DDR_FRAC_REG);
	u32 c = readl(PLL_DDR_CTRL_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);

	return get_idsp_dram_pll_freq(z, f, c, s);
}

u32 _get_idsp_freq_hz(u32 c)
{
	u32 z = (readl(PLL_IDSP_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_IDSP_FRAC_REG);
	u32 s = readl(SCALER_CORE_PRE_REG);

	return get_idsp_dram_pll_freq(z, f, c, s);
}

u32 get_idsp_freq_hz(void)
{
	u32 c = readl(PLL_IDSP_CTRL_REG);
	return _get_idsp_freq_hz(c);
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
	if ((readl(ADC16_CTRL_REG) & 0x1) == 0) {
		return PLL_FREQ_HZ / readl(SCALER_ADC_REG);
	} else {
		return get_audio_freq_hz();
	}
#endif
}

void rct_set_adc_clk_src(int src)
{
	writel(ADC16_CTRL_REG, (src & 0x01));
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
	return (get_apb_bus_freq_hz() / readl(CG_SSI_REG));
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

	if (expect_pll_core_val == 0xffffffff)
		return 0;

	pll_core_ctrl = (readl(PLL_CORE_CTRL_REG) & ~0x1);
	if (pll_core_ctrl != (expect_pll_core_val & ~0x1)) {
		return _get_core_bus_freq_hz(expect_pll_core_val);
	} else {
		return 0;
	}
}

u32 get_expect_idsp_freq(void)
{
	u32 expect_pll_idsp_val = EXPECT_PLL_IDSP_VAL;
	u32 pll_idsp_ctrl;

	if (expect_pll_idsp_val == 0xffffffff)
		return 0;

	pll_idsp_ctrl = (readl(PLL_IDSP_CTRL_REG) & ~0x1);
	if (pll_idsp_ctrl != (expect_pll_idsp_val & ~0x1)) {
		return _get_idsp_freq_hz(expect_pll_idsp_val);
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

	if (val == 0)
		return;

	_rct_switch_core_freq(val);
}

u32 _rct_switch_idsp_freq(u32 val)
{
	int i;
	u32 reg, freq;
	u32 intprog, sdiv, sout, valwe;

	freq = get_idsp_freq_hz();

	if (val > freq) {
		do {
			reg = readl(PLL_IDSP_CTRL_REG);
			intprog = PLL_CTRL_INTPROG(reg);
			sout = PLL_CTRL_SOUT(reg);
			sdiv = PLL_CTRL_SDIV(reg);
			valwe = PLL_CTRL_VALWE(reg);

			intprog++;

			reg = PLL_CTRL_VAL(intprog, sout, sdiv, valwe);

			writel(PLL_IDSP_CTRL_REG, (reg & 0xfffffffe));

			/* PLL write enable */
			writel(PLL_IDSP_CTRL_REG, (reg | 0x00000001));

			while ((readl(PLL_LOCK_REG) & PLL_LOCK_IDSP) !=
					PLL_LOCK_IDSP);

			do_some_delay();

			freq = get_idsp_freq_hz();
		} while (freq < val);
	} else if (val < freq) {
		do {
			reg = readl(PLL_IDSP_CTRL_REG);
			intprog = PLL_CTRL_INTPROG(reg);
			sout = PLL_CTRL_SOUT(reg);
			sdiv = PLL_CTRL_SDIV(reg);
			valwe = PLL_CTRL_VALWE(reg);

			intprog--;

			reg = PLL_CTRL_VAL(intprog, sout, sdiv, valwe);

			writel(PLL_IDSP_CTRL_REG, (reg & 0xfffffffe));

			/* PLL write enable */
			writel(PLL_IDSP_CTRL_REG, (reg | 0x00000001));

			while ((readl(PLL_LOCK_REG) & PLL_LOCK_IDSP) !=
					PLL_LOCK_IDSP);

			do_some_delay();

			freq = get_idsp_freq_hz();
		} while (freq > val);
	}

	return freq;
}

void rct_switch_idsp_freq(void)
{
	u32 val = get_expect_idsp_freq();

	if (val == 0)
		return;

	_rct_switch_idsp_freq(val);
}

#else

void rct_switch_core_freq(void)
{
	register u32 expect_pll_core_val = EXPECT_PLL_CORE_VAL;
	register u32 pll_core_ctrl;

	if (expect_pll_core_val == 0xffffffff)
		return;

	pll_core_ctrl = (readl(PLL_CORE_CTRL_REG) & ~0x1);
	if (pll_core_ctrl != (expect_pll_core_val & ~0x1)) {
		writel(PLL_CORE_CTRL_REG, (expect_pll_core_val & ~0x1));
		writel(PLL_CORE_CTRL_REG, (expect_pll_core_val | 0x1));
	}
}

void rct_switch_idsp_freq(void)
{
	register u32 expect_pll_idsp_val = EXPECT_PLL_IDSP_VAL;
	register u32 pll_idsp_ctrl;

	if (expect_pll_idsp_val == 0xffffffff)
		return;

	pll_idsp_ctrl = (readl(PLL_IDSP_CTRL_REG) & ~0x1);
	if (pll_idsp_ctrl != (expect_pll_idsp_val & ~0x1)) {
		writel(PLL_IDSP_CTRL_REG, (expect_pll_idsp_val & ~0x1));
		writel(PLL_IDSP_CTRL_REG, (expect_pll_idsp_val | 0x1));
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
#define DUTY_CYCLE_CONTRL_ENABLE	0x01000000 /* Duty cycle correction */
	u32 scaler;
	u32 core_freq;

	K_ASSERT(freq_hz != 0);

	/* Scaler = core_freq *2 / desired_freq */
	core_freq = get_core_bus_freq_hz();
	scaler = ((core_freq << 1) / freq_hz) + 1;

	/* Sdclk = core_freq * 2 / Int_div */
	/* For example: Sdclk = 108 * 2 / 5 = 43.2 Mhz */
	/* For example: Sdclk = 121.5 * 2 / 5 = 48.6 Mhz */
	writel(SCALER_SD48_REG,
		(readl(SCALER_SD48_REG) & 0xffff0000) |
		(DUTY_CYCLE_CONTRL_ENABLE | scaler));
}

void rct_set_ir_pll(void)
{
	writew(CG_IR_REG, 0x800);
}

void rct_set_ssi_pll(void)
{
	writel(CG_SSI_REG, get_apb_bus_freq_hz() / 13500000);
}

u32 get_ssi2_freq_hz(void)
{
	return (get_apb_bus_freq_hz() / (readl(CG_SSI2_REG) & 0xffffff));
}

void rct_set_ssi2_pll(void)
{
	/* 25 bits, bit-24 = 1 enable ssi2_clk */
	writel(CG_SSI2_REG,
	       ((0x1 << 24)  | (get_apb_bus_freq_hz() / 13500000)));
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
	writel(PLL_USB_CTRL_REG, 0x07081100);
	writel(PLL_USB_CTRL_REG, 0x07081101);
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 0x6);
}

void rct_suspend_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x06);
}

void rct_x_usb_clksrc(void)
{

}

void rct_usb_reset(void)
{
}
