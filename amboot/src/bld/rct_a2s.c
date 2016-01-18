/**
 * system/src/bld/rct_a2s.c
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
#define do_some_delay()	{int i; for (i = 0; i < 0x50000; i++);}

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
	writel(SCALER_ADC_REG, 0x00010006);
	
	/* Clear the bit-0 to ensure the other reset triggers will not be masked 
	   after warm reset */
	writel(SOFT_RESET_REG, readl(SOFT_RESET_REG) & (~0x01));

	writel(HDMI_PHY_CTRL_REG,
	       (HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
	       HDMI_PHY_CTRL_RESET_HDMI_PHY));
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

static u32 get_core_pll_freq(u32 z, u32 f, u32 c)
{
	u32 intprog, sout, sdiv;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		return 0;
	}

	intprog += 1 + z;
	sdiv++;
	sout++;

	return PLL_FREQ_HZ * intprog * sdiv / sout / 2;
}

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
	*chip  = 0x2;
	*major = 0x1;
	*minor = 0x0;
}

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
		rval |= BOOT_FROM_SDMMC;
#endif
#endif
		return rval;
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
	}

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

static void set_ddr_bias_current(void)
{
	u32 boot_from = rct_boot_from();

	if (boot_from & BOOT_FROM_SPI) {
		writel(DLL_CTRL_SEL_REG, RCT_DLL_CTRL_SEL_VAL);
	}
}

#if defined(RTPLL_LOCK_ICACHE)
extern int lock_i_cache_region(void *addr, unsigned int size);
extern void unlock_i_cache_ways(unsigned int ways);

extern void rtpll_set_core_pll(void);
extern void *rtpll_core_pll_val;
extern void *rtpll_text_begin;
extern void *rtpll_text_end;

void enable_mmu(void)
{
	register u32 x;

	/* Read control register */
	__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));

	/* Make sure MMU is in disabled state first */
	x &= ~(0x1);
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));

	/* Flush all cache */
	x = 0x0;
	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 0": : "r" (x));

	/* Flush all TLB */
	x = 0x0;
	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c6, 0": : "r" (x));

	/* Set DOM for client */
	x = 0x1;
	__asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0": : "r" (x));

	/* Read control register */
	__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));

	/* Enable MMU */
	x |= 0x1;
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));
}

void disable_mmu(void)
{
	register u32 x;

	/* Read control register */
	__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));

	/* Disable MMU */
	x &= ~(0x1);
	__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));
}


void set_core_pll_in_icache(u32 val)
{
	u32 ways;

	_clean_flush_all_cache();
 	_disable_icache();
	_disable_dcache();
	disable_mmu();
	writel((u32) &rtpll_core_pll_val, (val & 0xfffffffe));
	enable_mmu();
 	_enable_icache();
	_enable_dcache();

	ways = lock_i_cache_region((void *) &rtpll_text_begin,
				   (u32) &rtpll_text_end -
				   (u32) &rtpll_text_begin);
	rtpll_set_core_pll();
	unlock_i_cache_ways(ways);
}
#endif

void set_core_pll(u32 val)
{
#if defined(RTPLL_LOCK_ICACHE)
	set_core_pll_in_icache(val);
	while ((readl(PLL_LOCK_REG) & PLL_LOCK_CORE) != PLL_LOCK_CORE);
#else
	writel(PLL_CORE_CTRL_REG, (val & 0xfffffffe));
	writel(PLL_CORE_CTRL_REG, (val | 0x00000001));

	while ((readl(PLL_LOCK_REG) & PLL_LOCK_CORE) != PLL_LOCK_CORE);
#endif
}

#if	defined(USE_RTPLL_CHANGE)
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
			set_core_pll(reg);
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
			set_core_pll(reg);
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
	set_ddr_bias_current();
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
		set_core_pll(expect_pll_core_val);
	}
	set_ddr_bias_current();
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

void rct_set_sdxc_pll(u32 freq_hz)
{
	return;
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
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) | 0x6);
}

void rct_suspend_usb(void)
{
	writel(ANA_PWR_REG, readl(ANA_PWR_REG) & ~0x06);
}

void rct_x_usb_clksrc(void)
{
	register u32 clk;
	int val;
#if 0
	val = *(volatile int *)(0x7017007c);
	putstr("original SCALER_USB_REG value = 0x");
	puthex(val);
	putstr("\r\n");
	val = *(volatile int *)(0x70170088);
	putstr("original USB_REFCLK_REG value = 0x");
	puthex(val);
	putstr("\r\n");
#endif
	/* Program the USB phy clock generator */
	clk = get_core_bus_freq_hz();

	if (clk == 216000000) {
		/* Core == 216MHz, */
		writel(USB_REFCLK_REG,readl(USB_REFCLK_REG) | 0x01 );
		writel(SCALER_USB_REG,
			(readl(SCALER_USB_REG) & 0xffe0ff00) |
			 0x00100012
			);
	}
	else if (clk == 162000000){
		/* Core == 162MHz, */
		writel(USB_REFCLK_REG,readl(USB_REFCLK_REG) | 0x01 );
		writel(SCALER_USB_REG,
			(readl(SCALER_USB_REG) & 0xffe0ff00) |
			 0x0001001b
			);
	}
	else
	{
		/* set default Core == 216MHz, */
		*(volatile int *)(0x7017007c) = 0x00100012; /* set to 12MHz */
		val = *(volatile int *)(0x70170088);
		val = val & 0xfffffff0;
		val = val | 0x1;	/* set to 12MHz */
		*(volatile int *)(0x70170088) = val;
	}
#if 0
	val = *(volatile int *)(0x7017007c);
	putstr("now SCALER_USB_REG value = 0x");
	puthex(val);
	putstr("\r\n");
	val = *(volatile int *)(0x70170088);
	putstr("now USB_REFCLK_REG value = 0x");
	puthex(val);
	putstr("\r\n");
#endif

}

void rct_usb_reset(void)
{
}
