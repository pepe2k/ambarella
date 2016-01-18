/**
 * boards/s2ipcam/bsp/bsp.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>
#include <peripheral/idc.h>
#include <vout.h>

#define ISL12022M_I2C_ADDR	0xde
#define PCA9539_I2C_ADDR	0xe8

static void isl12022m_init()
{
	u16 val;

	/* disable frequency output */
	idc3_read(IDC2, ISL12022M_I2C_ADDR+1, 0x08, &val, 1);
	val &= 0xf0;
	idc3_write(IDC2, ISL12022M_I2C_ADDR, 0x08, &val, 1);
}

static void pca9539_set(u32 id)
{
	u16					val;

	if (id >=16)
		return;

	idc3_read(IDC2, PCA9539_I2C_ADDR+1, 0x06, &val, 1);
	val &= ~(0x1 << id);
	idc3_write(IDC2, PCA9539_I2C_ADDR, 0x06, &val, 1);

	idc3_read(IDC2, PCA9539_I2C_ADDR+1, 0x02, &val, 1);
	val |= (0x1 << id);
	idc3_write(IDC2, PCA9539_I2C_ADDR, 0x02, &val, 1);
}

static void pca9539_clr(u32 id)
{
	u16					val;

	if (id >=16)
		return;

	idc3_read(IDC2, PCA9539_I2C_ADDR+1, 0x06, &val, 1);
	val &= ~(0x1 << id);
	idc3_write(IDC2, PCA9539_I2C_ADDR, 0x06, &val, 1);

	idc3_read(IDC2, PCA9539_I2C_ADDR+1, 0x02, &val, 1);
	val &= ~(0x1 << id);
	idc3_write(IDC2, PCA9539_I2C_ADDR, 0x02, &val, 1);
}

int amboot_bsp_netphy_init(void)
{
	pca9539_set(2);
	timer_dly_ms(TIMER3_ID, 200);

	return 0;
}

/* ==========================================================================*/
int amboot_bsp_hw_init(void)
{
	int					retval = 0;


	writel(MS_DELAY_CTRL_REG, 0x00000000);
	writel(AHB_MISC_EN_REG, 0x50);
	amb_set_smioa_ioctrl_drive_strength(HAL_BASE_VP,
		AMB_IOCTRL_DRIVE_STRENGTH_12MA);
	amb_set_smiob_ioctrl_drive_strength(HAL_BASE_VP,
		AMB_IOCTRL_DRIVE_STRENGTH_12MA);
	amb_set_smioc_ioctrl_drive_strength(HAL_BASE_VP,
		AMB_IOCTRL_DRIVE_STRENGTH_12MA);
	amb_set_smiod_ioctrl_drive_strength(HAL_BASE_VP,
		AMB_IOCTRL_DRIVE_STRENGTH_12MA);

#if defined(CONFIG_S2IPCAM_PHY_CLK_INTERNAL)
	writel(RCT_REG(0x2CC), 0x01);	// phy clcok from internal, 100MHz.
#else
	writel(RCT_REG(0x2CC), 0x00); // phy clcok from external.
#endif

#if defined(AMBOOT_DEV_BOOT_CORTEX)
	cmd_cortex_pre_init(0, 0x00000000,
		0x00000000, 0x00000000, 0x00000000);
#endif

	idc_init();
	isl12022m_init();
#ifdef ENABLE_EMMC_BOOT
	pca9539_set(0);
#else
	pca9539_clr(0);
#endif
	pca9539_clr(1);

	return retval;
}

#ifdef SHOW_AMBOOT_SPLASH

#ifdef CONFIG_LCD_PANEL_TD043
#define TD043_BKL_ON       GPIO(16)
static void td043_lcd_config_wvga()
{
  gpio_config_sw_out(TD043_BKL_ON);
  gpio_set(TD043_BKL_ON);
}

static int td043_set_vout_ctrl_wvga(lcd_dev_vout_cfg_t *vout_cfg)
{
  /* Fill the Sync Mode and Color space */
  vout_cfg->lcd_sync    	= INPUT_FORMAT_601;
  vout_cfg->color_space		= CS_RGB;

  /* Fill the HVSync polarity */
  vout_cfg->hs_polarity		= ACTIVE_LO;
  vout_cfg->vs_polarity		= ACTIVE_LO;
  vout_cfg->de_polarity		= ACTIVE_HI;

  /* Fill the number of VD_CLK per H-Sync, number of H-Sync's per field */
  vout_cfg->ftime_hs		= 940;
  vout_cfg->ftime_vs_top	= 504;
  vout_cfg->ftime_vs_bot	= 504;

  /* HSync start and end */
  vout_cfg->hs_start		= 0;
  vout_cfg->hs_end		= 2;

  /* VSync start and end, use two dimension(x,y) to calculate */
  vout_cfg->vs_start_row_top	= 0;
  vout_cfg->vs_start_row_bot	= 0;
  vout_cfg->vs_end_row_top	= 2;
  vout_cfg->vs_end_row_bot	= 2;

  vout_cfg->vs_start_col_top	= 0;
  vout_cfg->vs_start_col_bot	= 0;
  vout_cfg->vs_end_col_top	= 0;
  vout_cfg->vs_end_col_bot	= 0;

  /* Active start and end, use two dimension(x,y) to calculate */
  vout_cfg->act_start_row_top	= 20;
  vout_cfg->act_start_row_bot   = 20;
  vout_cfg->act_end_row_top	= 499;
  vout_cfg->act_end_row_bot	= 499;

  vout_cfg->act_start_col_top   = 120;
  vout_cfg->act_start_col_bot   = 120;
  vout_cfg->act_end_col_top	= 919;
  vout_cfg->act_end_col_bot	= 919;

  /* Set default color */
  vout_cfg->bg_color		= BG_BLACK;

  /* LCD paremeters */
  vout_cfg->lcd_display		= LCD_PROG_DISPLAY;
  vout_cfg->lcd_frame_rate	= 0;

  vout_cfg->lcd_data_clk    	= 0;
  vout_cfg->lcd_color_mode  	= 3;
  vout_cfg->lcd_rgb_seq_top 	= 0;
  vout_cfg->lcd_rgb_seq_bot 	= 0;
  vout_cfg->lcd_hscan   	= 0;

  vout_cfg->osd_width   	= 800;
  vout_cfg->osd_height    	= 480;
  vout_cfg->osd_resolution  	= VO_RGB_800_480;

  vout_cfg->clock_hz	    	= 28397174;

  return 0;
}
#endif /*CONFIG_LCD_PANEL_TD043*/

int amboot_bsp_entry(void)
{
#ifndef CONFIG_LCD_PANEL_NONE
	lcd_dev_vout_cfg_t cfg1 = {0};
	int chan1 = 0;
#endif
#ifndef CONFIG_HDMI_MODE_NONE
	lcd_dev_vout_cfg_t cfg2 = {0};
	int chan2 = 1;
#endif
#ifndef CONFIG_CVBS_MODE_NONE
	lcd_dev_vout_cfg_t cfg2 = {0};
	int chan2 = 1;
#endif

#ifdef CONFIG_LCD_PANEL_TD043
	td043_set_vout_ctrl_wvga(&cfg1);
#endif

#ifdef CONFIG_HDMI_MODE_480P
	hdmi_set_vout_ctrl_480p(&cfg2);
#endif

#ifdef CONFIG_HDMI_MODE_576P
	hdmi_set_vout_ctrl_576p(&cfg2);
#endif

#ifdef CONFIG_CVBS_MODE_480I
	cvbs_set_vout_ctrl_480i(&cfg2);
	cvbs_config_480i(&cfg2.dve);
#endif

#ifdef CONFIG_CVBS_MODE_576I
	cvbs_set_vout_ctrl_576i(&cfg2);
	cvbs_config_576i(&cfg2.dve);
#endif

#ifndef CONFIG_LCD_PANEL_NONE
	amb_set_lcd_clock_source(HAL_BASE_VP,
	                         AMB_REFERENCE_CLOCK_SOURCE_CLK_REF, 0);
	amb_set_lcd_clock_frequency(HAL_BASE_VP, cfg1.clock_hz);
#endif
#ifndef CONFIG_HDMI_MODE_NONE
	amb_set_vout_clock_frequency(HAL_BASE_VP, cfg2.clock_hz);
#endif
#ifndef CONFIG_CVBS_MODE_NONE
	amb_set_vout_clock_frequency(HAL_BASE_VP, cfg2.clock_hz);
#endif
	/* DSP vout init */
	dsp_vout_init();
#ifndef CONFIG_LCD_PANEL_NONE
	vout_config(chan1, &cfg1);
	dsp_vout_set(chan1, cfg1.osd_resolution);
#endif

#ifndef CONFIG_HDMI_MODE_NONE
	vout_config(chan2, &cfg2);
	dsp_vout_set(chan2, cfg2.osd_resolution);
#endif

#ifndef CONFIG_CVBS_MODE_NONE
	vout_config(chan2, &cfg2);
	dsp_vout_set(chan2, cfg2.osd_resolution);
#endif

#ifdef CONFIG_LCD_PANEL_TD043
	td043_lcd_config_wvga();
#endif

#ifdef CONFIG_HDMI_MODE_480P
	hdmi_config_480p();
#endif

#ifdef CONFIG_HDMI_MODE_576P
	hdmi_config_576p();
#endif

	return 0;
}
#endif
