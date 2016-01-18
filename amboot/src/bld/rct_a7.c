/**
 * system/src/bld/rct_a7.c
 *
 * History:
 *    2009/10/14 - [Allen Wang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <hal/hal.h>
#include <board.h>

#ifdef ENABLE_DEBUG_MSG_RCT
#define DEBUG_MSG	uart_putstr
#else
#define DEBUG_MSG(...)
#endif

#ifdef ENABLE_DEBUG_MSG_HAL
#define DEBUG_MSG_HAL	uart_putstr
#else
#define DEBUG_MSG_HAL(...)
#endif

/* These options are for debug purpose. */
//#define HARD_CODE_CORE_FREQ	120000000
//#define USE_DIRECT_RCT_PROGRAM	1

#ifdef USE_DIRECT_RCT_PROGRAM

amb_clock_frequency_t rct_get_core_clock_frequency (void);
#define PLL_FREQ_HZ	24000000

static u32 get_core_idsp_pll_freq(u32 z, u32 f, u32 c, u32 s)
{
	u32 intprog, sout, sdiv, p;

	f = PLL_FRAC_VAL(f);
	intprog = PLL_CTRL_INTPROG(c);
	sout = PLL_CTRL_SOUT(c);
	sdiv = PLL_CTRL_SDIV(c);
	p = SCALER_POST(s);

	if (f != 0) {
		/* If fraction part, return 0 to be a warning. */
		putstr("fraction is zero\r\n");
		return 0;
	}

	intprog += 1 + z;
	sdiv++;
	sout++;

	return PLL_FREQ_HZ * intprog * sdiv / sout / p / 2;

}

static amb_clock_frequency_t rct_get_apb_clock_frequency (void)
{
	return rct_get_core_clock_frequency() / 2;
}

amb_hal_success_t rct_set_sd_clock_frequency (amb_clock_frequency_t freq_hz)
{
#define DUTY_CYCLE_CONTRL_ENABLE	0x01000000 /* Duty cycle correction */
	u32 scaler;
	u32 core_freq;

	K_ASSERT(freq_hz != 0);

	/* Scaler = core_freq *2 / desired_freq */
	core_freq = rct_get_core_clock_frequency();
	scaler = ((core_freq << 1) / freq_hz) + 1;

	/* Sdclk = core_freq * 2 / Int_div */
	/* For example: Sdclk = 108 * 2 / 5 = 43.2 Mhz */
	/* For example: Sdclk = 121.5 * 2 / 5 = 48.6 Mhz */
	writel(SCALER_SD48_REG,
		(readl(SCALER_SD48_REG) & 0xffff0000) |
		(DUTY_CYCLE_CONTRL_ENABLE | scaler));

	return AMB_HAL_SUCCESS;
}

amb_clock_frequency_t rct_get_sd_clock_frequency (void)
{
	u32 scaler = readl(SCALER_SD48_REG) & SD48_INTEGER_DIV;
	return (rct_get_core_clock_frequency() * 2 / scaler);
}

amb_hal_success_t rct_set_uart_clock_frequency (amb_clock_frequency_t freq_hz)
{
	u32 d;

	d = PLL_FREQ_HZ / freq_hz;
	writel(CG_UART_REG, d);

	return AMB_HAL_SUCCESS;
}

amb_clock_frequency_t rct_get_uart_clock_frequency (void)
{
	return (PLL_FREQ_HZ / readl(CG_UART_REG));
}

amb_hal_success_t rct_set_ssi_clock_frequency (amb_clock_frequency_t freq_hz)
{
	writel(CG_SSI_REG, rct_get_apb_clock_frequency() / 13500000);
	return AMB_HAL_SUCCESS;
}

amb_clock_frequency_t rct_get_ssi_clock_frequency (void)
{
	return (get_apb_bus_freq_hz() / readl(CG_SSI_REG));
}

amb_hal_success_t rct_set_ssi2_clock_frequency (amb_clock_frequency_t freq_hz)
{
	/* 25 bits, bit-24 = 1 enable ssi2_clk */
	writel(CG_SSI2_REG,
	       ((0x1 << 24)  |
		(rct_get_apb_clock_frequency() / 13500000)));

	return AMB_HAL_SUCCESS;
}

amb_clock_frequency_t rct_get_ssi2_clock_frequency (void)
{
	return (rct_get_apb_clock_frequency() / (readl(CG_SSI2_REG) & 0xffffff));
}

amb_clock_frequency_t rct_get_core_clock_frequency (void)
{
	u32 z = (readl(PLL_CORE_CTRL3_REG) & 0x40) >> 6;
	u32 f = readl(PLL_CORE_FRAC_REG);
	u32 c = readl(PLL_CORE_CTRL_REG);
	u32 p = readl(SCALER_CORE_POST_REG);

	return get_core_idsp_pll_freq(z, f, c, p);
}

amb_hal_success_t rct_reset_all (void)
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

	return AMB_HAL_SUCCESS;
}

amb_boot_type_t rct_get_boot_type (void)
{
	/* FIXME: Hard code to NAND boot becuase we are debugging this. */
	return AMB_NAND_BOOT;
}

amb_system_configuration_t rct_get_system_configuration (void)
{
	/* FIXME: Just return 0 for NAND flash. */
	return 0;
}

#else /* USE_DIRECT_RCT_PROGRAM */

amb_hal_success_t rct_set_sd_clock_frequency (amb_clock_frequency_t freq_hz)
{
	return amb_mini_set_sd_clock_frequency(freq_hz);
}

amb_clock_frequency_t rct_get_sd_clock_frequency (void)
{
	return amb_mini_get_sd_clock_frequency();
}

amb_hal_success_t rct_set_uart_clock_frequency (amb_clock_frequency_t freq_hz)
{
	return amb_mini_set_uart_clock_frequency(freq_hz);
}

amb_clock_frequency_t rct_get_uart_clock_frequency (void)
{
	return amb_mini_get_uart_clock_frequency();
}

amb_hal_success_t rct_set_ssi_clock_frequency (amb_clock_frequency_t freq_hz)
{
	return amb_mini_set_ssi_clock_frequency(freq_hz);
}

amb_clock_frequency_t rct_get_ssi_clock_frequency (void)
{
	return amb_mini_get_ssi_clock_frequency();
}

amb_hal_success_t rct_set_ssi2_clock_frequency (amb_clock_frequency_t freq_hz)
{
	return amb_mini_set_ssi2_clock_frequency(freq_hz);
}

amb_clock_frequency_t rct_get_ssi2_clock_frequency (void)
{
	return amb_mini_get_ssi2_clock_frequency();
}

amb_clock_frequency_t rct_get_core_clock_frequency (void)
{
	return amb_mini_get_core_clock_frequency();
}

amb_hal_success_t rct_reset_all (void)
{
#if 1
	return amb_mini_reset_all();
#else
	volatile int c;

	writel(FIO_RESET_REG,
	       FIO_RESET_FIO_RST|
	       FIO_RESET_CF_RST |
	       FIO_RESET_XD_RST |
	       FIO_RESET_FLASH_RST);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */
	writel(FIO_RESET_REG, 0x0);
	for (c = 0; c < 0xffff; c++);	/* Small busy-wait loop */

	return AMB_HAL_SUCCESS;
#endif
}

amb_boot_type_t rct_get_boot_type (void)
{
	return amb_mini_get_boot_type();
}

amb_system_configuration_t rct_get_system_configuration (void)
{
	return amb_mini_get_system_configuration();
}
#endif /* USE_DIRECT_RCT_PROGRAM */

/*----------------------------------------------------------------------------*/

void rct_pll_init(void)
{
	writel(HDMI_PHY_CTRL_REG,
	       (HDMI_PHY_CTRL_HDMI_PHY_ACTIVE_MODE |
	       HDMI_PHY_CTRL_RESET_HDMI_PHY));

	/* ADC module power down */
	writel(ADC16_CTRL_REG, 0x2);
}

u32 get_apb_bus_freq_hz(void)
{
	return get_ahb_bus_freq_hz() >> 1;
}

u32 get_ahb_bus_freq_hz(void)
{
	return get_core_bus_freq_hz() >> (readl(CORE_CLK_RATIO_1X_REG) ^ 0x1);
}

/* core is vdsp frequency, not arm11 frequency. */
u32 get_core_bus_freq_hz(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_core_clock_frequency()\r\n");
#ifdef HARD_CODE_CORE_FREQ
		return HARD_CODE_CORE_FREQ;
#else
		return (u32) amb_get_core_clock_frequency(HAL_BASE_VP);
#endif
	} else {
		return (u32) rct_get_core_clock_frequency();
	}
}

/* arm clock is 2x or 4x times of idsp. */
u32 get_arm_bus_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_arm_clock_frequency()\r\n");
	return (u32) amb_get_arm_clock_frequency(HAL_BASE_VP);
}

u32 get_ddr_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_ddr_clock_frequency()\r\n");
	return (u32) amb_get_ddr_clock_frequency(HAL_BASE_VP);
}

u32 get_idsp_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_idsp_clock_frequency()\r\n");
	return (u32) amb_get_idsp_clock_frequency(HAL_BASE_VP);
}

u32 get_audio_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_audio_clock_frequency()\r\n");
	return (u32) amb_get_audio_clock_frequency(HAL_BASE_VP);
}

u32 get_adc_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_adc_clock_frequency()\r\n");
	return (u32) amb_get_adc_clock_frequency(HAL_BASE_VP);
}

void rct_set_adc_clk_src(int src)
{
	/* FIXME: replace with HAL call */
}

u32 get_uart_freq_hz(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_uart_clock_frequency()\r\n");
		return (u32) amb_get_uart_clock_frequency(HAL_BASE_VP);
	} else {
		return (u32) rct_get_uart_clock_frequency();
	}
}

u32 get_ssi_freq_hz(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_ssi_clock_frequency()\r\n");
		return (u32) amb_get_ssi_clock_frequency(HAL_BASE_VP);
	} else {
		return (u32) rct_get_ssi_clock_frequency();
	}
}

u32 get_motor_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_motor_clock_frequency()\r\n");
	return (u32) amb_get_motor_clock_frequency(HAL_BASE_VP);
}

u32 get_ir_freq_hz(void)
{
	DEBUG_MSG_HAL("amb_get_ir_clock_frequency()\r\n");
	return (u32) amb_get_ir_clock_frequency(HAL_BASE_VP);
}

u32 get_host_freq_hz(void)
{
	//DEBUG_MSG_HAL("amb_get_host_clock_frequency()\r\n");
	//return (u32) amb_get_host_clock_frequency(HAL_BASE_VP);
	return 0;
}

u32 get_sd_freq_hz(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_sd_clock_frequency()\r\n");
		return (u32) amb_get_sd_clock_frequency(HAL_BASE_VP);
	} else {
		return (u32) rct_get_sd_clock_frequency();
	}
}

void get_stepping_info(int *chip, int *major, int *minor)
{
	*chip  = 0x7;
	*major = 0x0;
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
#endif /* FIRMWARE_CONTAINER_TYPE */
		return rval;
}

/* This function return the device boot from in different boot mode. */
u32 rct_boot_from(void)
{
	u32 rval = 0x0;
	u32 sm;
	amb_boot_type_t type;

#ifdef	RCT_BOOT_FROM
	/* The device boot from is specified by user. */
	rval = RCT_BOOT_FROM;
	return rval;
#endif

	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_boot_type()\r\n");
		type = amb_get_boot_type(HAL_BASE_VP);
	} else {
		type = rct_get_boot_type();
	}

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
			/* FIXME: */
			rval |= BOOT_FROM_NAND;
		}
	} else if (type == AMB_HIF_BOOT) {
		rval |= BOOT_FROM_HIF;
	}

	return rval;
}

int rct_is_cf_trueide(void)
{
	return 0;
}

int rct_is_eth_enabled(void)
{
	amb_system_configuration_t cfg;

	DEBUG_MSG_HAL("amb_get_system_configuration()\r\n");
	cfg = amb_get_system_configuration(HAL_BASE_VP);
	if (cfg & AMB_SYSTEM_CONFIGURATION_ETHERNET_SELECTED)
		return 1;
	else
		return 0;
}

void rct_reset_chip(void)
{
	DEBUG_MSG_HAL("amb_reset_chip()\r\n");
	if (amb_reset_chip(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_reset_chip() failed\r\n");
	}
}

void rct_reset_fio(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_reset_fio()\r\n");
		if (amb_reset_fio(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("amb_reset_fio() failed\r\n");
		}
	} else {
		rct_reset_all();
	}
}

void rct_switch_core_freq(void)
{
	/* Not support in A5S. */
}

void rct_set_uart_pll(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_set_uart_clock_frequency()\r\n");
		if (amb_set_uart_clock_frequency(HAL_BASE_VP,
					1800000) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("amb_set_uart_clock_frequency() failed\r\n");
		}
	} else {
		if (rct_set_uart_clock_frequency(1800000) !=
							AMB_HAL_SUCCESS) {
			DEBUG_MSG("rct_set_uart_clock_frequency() failed\r\n");
		}
	}
}

void rct_set_sd_pll(u32 freq_hz)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_set_sd_clock_frequency()\r\n");
		if (amb_set_sd_clock_frequency(HAL_BASE_VP,
					freq_hz) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("amb_set_sd_clock_frequency() failed\r\n");
		}
	} else {
		if (rct_set_sd_clock_frequency(freq_hz) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("rct_set_sd_clock_frequency() failed\r\n");
		}
	}
}


void rct_set_sdxc_pll(u32 freq_hz)
{

}

void rct_set_ir_pll(void)
{
	DEBUG_MSG_HAL("amb_set_ir_clock_frequency()\r\n");
	if (amb_set_ir_clock_frequency(HAL_BASE_VP,
				13000) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_ir_clock_frequency() failed\r\n");
	}
}

void rct_set_ssi_pll(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_set_ssi_clock_frequency()\r\n");
		if (amb_set_ssi_clock_frequency(HAL_BASE_VP,
					13500000) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("amb_set_ssi_clock_frequency() failed\r\n");
		}
	} else {
		if (rct_set_ssi_clock_frequency(13500000) !=
							AMB_HAL_SUCCESS) {
			DEBUG_MSG("rct_set_ssi_clock_frequency() failed\r\n");
		}
	}
}

u32 get_ssi2_freq_hz(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_get_ssi2_clock_frequency()\r\n");
		return (u32) amb_get_ssi2_clock_frequency(HAL_BASE_VP);
	} else {
		return (u32) rct_get_ssi2_clock_frequency();
	}
}

void rct_set_ssi2_pll(void)
{
	if (IS_HAL_INIT()) {
		DEBUG_MSG_HAL("amb_set_ssi2_clock_frequency()\r\n");
		if (amb_set_ssi2_clock_frequency(HAL_BASE_VP,
					13500000) != AMB_HAL_SUCCESS) {
			DEBUG_MSG("amb_set_ssi2_clock_frequency() failed\r\n");
		}
	} else {
		if (rct_set_ssi2_clock_frequency(13500000) !=
							AMB_HAL_SUCCESS) {
			DEBUG_MSG("rct_set_ssi2_clock_frequency() failed\r\n");
		}
	}
}

void rct_usb_change (void)
{
	/* FIXME: replace with HAL call */
}

void rct_set_usb_ext_clk(void)
{
	DEBUG_MSG_HAL("amb_set_usb_clock_source()\r\n");
	if (amb_set_usb_clock_source(HAL_BASE_VP, AMB_USB_CLK_EXT_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_clock_source() failed\r\n");
	}
}

void rct_set_usb_int_clk(void)
{
	/* FIXME, Set to internal 48MHz by default */
	DEBUG_MSG_HAL("amb_set_usb_clock_source()\r\n");
	if (amb_set_usb_clock_source(HAL_BASE_VP, AMB_USB_CLK_CORE_48MHZ) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_clock_source() failed\r\n");
	}
}

void rct_set_usb_debounce(void)
{
	/* FIXME: replace with HAL call */
}

void rct_enable_usb(void)
{
	DEBUG_MSG_HAL("amb_set_usb_interface_state()\r\n");
	if (amb_set_usb_interface_state(HAL_BASE_VP, AMB_USB_ON) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_interface_state() failed\r\n");
	}
}

void rct_suspend_usb(void)
{
	DEBUG_MSG_HAL("amb_set_usb_interface_state()\r\n");
	if (amb_set_usb_interface_state(HAL_BASE_VP, AMB_USB_SUSPEND) !=
							AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_set_usb_interface_state() failed\r\n");
	}
}

void rct_x_usb_clksrc(void)
{
	/* FIXME: replace with HAL call */
}

void rct_usb_reset(void)
{
	DEBUG_MSG_HAL("amb_usb_subsystem_soft_reset()\r\n");
	if (amb_usb_subsystem_soft_reset(HAL_BASE_VP) != AMB_HAL_SUCCESS) {
		DEBUG_MSG("amb_usb_subsystem_soft_reset() failed\r\n");
	}
}
