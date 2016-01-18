/*
 * kernel/private/include/amba_vin.h
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_VIN_H
#define __AMBA_VIN_H

#include <ambas_vin.h>

#define AMBA_VIN_MAX_FPS_TABLE_SIZE		(256)

static inline void amba_vin_source_set_fps_flag(
	struct amba_vin_source_mode_info *pinfo,
	u32 fps)
{
	u32				i;

	for (i = 0; i < pinfo->fps_table_size; i++) {
		if (pinfo->fps_table[i] == AMBA_VIDEO_FPS_AUTO) {
			pinfo->fps_table[i] = fps;
			break;
		}
	}
}

#define AMBA_VIN_ADAP_FLAG_OK_ENABLED		(0x01 << 0)
#define AMBA_VIN_ADAP_FLAG_OK_VSYNC		(0x01 << 1)

#define AMBA_VIN_ADAP_FLAG_ERR_EARLY_HSYNC	(0x01 << 29)
#define AMBA_VIN_ADAP_FLAG_ERR_EARLY_VSYNC	(0x01 << 30)
#define AMBA_VIN_ADAP_FLAG_ERR_FIFO_OVERFLOW	(0x01 << 31)

struct amba_vin_adapter {
	int id;
	u32 dev_type;
	char name[AMBA_VIN_NAME_LENGTH];
};

struct amba_vin_min_HV_sync {
	u16 hs_min;
	u16 vs_min;
};

struct amba_vin_HV_width {
	u16 vwidth;
	u16 hwidth;
	u16 hstart;
	u16 hend;
};

struct amba_vin_H_offset {
	u16 bottom_vsync_offset;
	u16 top_vsync_offset;
};

struct amba_vin_HV {
	u16 pel_clk_a_line;
	u16 line_num_a_field;
};

struct amba_vin_trigger_pin_info {
	u8 enabled;		/**< trigger enable/disable */
	u8 polarity;		/**< set trigger pin polarity at the
				     startline'th line period */
#define AMBA_VIN_TRIG_ACT_LOW	(0)
#define AMBA_VIN_TRIG_ACT_HIGH	(1)
	u16 start_line;		/**< Assert trigger pin at the H-sync
				     of startline'th line after
				     V-sync */
	u16 last_line;		/**< De-assert trigger pin at the
				     H-sync of this line after V-sync */
};

struct amba_vin_blc_info {
	u8 enable;		/**< Enable/disable H/W BLC */
	u16 hstart;		/**< BLC averaging window horizontal
				     start position */
	u16 vstart;		/**< BLC averaging window vertical
				     start position */
	u16 hmax;		/**< BLC averaging window horizontal
				    end position, (hmax-hstart) must
				     be < 64 */
	u16 nlines;		/**< Number of lines to average = (nlines +1) */
	u8 shift_amt;		/**< Number of bits to shift
				     accumulated BLC calibration for
				     each color. */
};

struct amba_vin_clk_info {
	u32 mode;
	u32 so_freq_hz;
	u32 so_pclk_freq_hz;
};

struct amba_vin_irq_fix {
	u32 mode;
	u32 delay;
};

#define AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_ENABLE	(1)
#define AMBA_VIN_ADAP_VIDEO_CAPTURE_WINDOW_DISABLE	(0)

enum amba_vin_adap_cmd {
	AMBA_VIN_ADAP_IDLE = 10000,	//Do nothing
	AMBA_VIN_ADAP_SUSPEND,
	AMBA_VIN_ADAP_RESUME,
	AMBA_VIN_ADAP_NO_ARG_END,

	AMBA_VIN_ADAP_INIT = 10100,
	AMBA_VIN_ADAP_SET_SHADOW_REG_PTR,
	AMBA_VIN_ADAP_GET_INFO,

	AMBA_VIN_ADAP_REGISTER_SOURCE = 10200,
	AMBA_VIN_ADAP_UNREGISTER_SOURCE,
	AMBA_VIN_ADAP_GET_SOURCE_NUM,
	AMBA_VIN_ADAP_FIX_ARCH_VSYNC,

	AMBA_VIN_ADAP_GET_CONFIG = 11000,
	AMBA_VIN_ADAP_GET_CAPTURE_WINDOW,
	AMBA_VIN_ADAP_GET_MIN_HW_SYNC_WIDTH,
	AMBA_VIN_ADAP_GET_VIDEO_CAPTURE_WINDOW,
	AMBA_VIN_ADAP_GET_SW_BLC,
	AMBA_VIN_ADAP_GET_VIN_CLOCK,
	AMBA_VIN_ADAP_GET_VOUT_SYNC_START_LINE,
	AMBA_VIN_ADAP_GET_H_OFFSET,
	AMBA_VIN_ADAP_GET_HV,
	AMBA_VIN_ADAP_GET_TRIGGER0_PIN_INFO,
	AMBA_VIN_ADAP_GET_TRIGGER1_PIN_INFO,
	AMBA_VIN_ADAP_GET_BLC_INFO,
	AMBA_VIN_ADAP_GET_HW_BLC,
	AMBA_VIN_ADAP_GET_VIN_CAP_INFO,
	AMBA_VIN_ADAP_GET_ACTIVE_SOURCE_ID,
	AMBA_VIN_ADAP_GET_HV_WIDTH,

	AMBA_VIN_ADAP_SET_CONFIG = 12000,
	AMBA_VIN_ADAP_SET_CAPTURE_WINDOW,
	AMBA_VIN_ADAP_SET_MIN_HW_SYNC_WIDTH,
	AMBA_VIN_ADAP_SET_VIDEO_CAPTURE_WINDOW,
	AMBA_VIN_ADAP_SET_SW_BLC,
	AMBA_VIN_ADAP_SET_VIN_CLOCK,
	AMBA_VIN_ADAP_SET_VOUT_SYNC_START_LINE,
	AMBA_VIN_ADAP_SET_H_OFFSET,
	AMBA_VIN_ADAP_SET_HV,
	AMBA_VIN_ADAP_SET_TRIGGER0_PIN_INFO,
	AMBA_VIN_ADAP_SET_TRIGGER1_PIN_INFO,
	AMBA_VIN_ADAP_SET_BLC_INFO,
	AMBA_VIN_ADAP_SET_HW_BLC,
	AMBA_VIN_ADAP_SET_VIN_CAP_INFO,
	AMBA_VIN_ADAP_SET_ACTIVE_SOURCE_ID,
	AMBA_VIN_ADAP_SET_HV_WIDTH,
	AMBA_VIN_ADAP_SET_MIPI_PHY_ENABLE,
	AMBA_VIN_ADAP_CONFIG_MIPI_RESET,
};

#define AMBA_VIN_SRC_RESET_SW			(1 << 0)
#define AMBA_VIN_SRC_RESET_HW			(1 << 1)
#define AMBA_VIN_SRC_RESET_ALL			(AMBA_VIN_SRC_RESET_HW | AMBA_VIN_SRC_RESET_SW)

#define AMBA_VIN_SRC_POWEROFF			(0)
#define AMBA_VIN_SRC_POWERON			(1)

struct amba_vin_src_still_info {	//AMBA_VIN_SRC_SET_STILL_MODE
	enum amba_vin_capture_mode capture_mode;
	enum amba_vin_trigger_source trigger_source;
	enum amba_video_mode still_mode;
	enum amba_vin_flash_level flash_level;
	enum amba_vin_flash_status flash_status;

	u32 shutter_time;
	s32 gain_db;
	struct amba_vin_black_level_compensation sw_blc;
	u32 fps;
};

struct amba_vin_cap_window {
	u16 start_x;
	u16 start_y;
	u16 end_x;
	u16 end_y;
};

#define AMBA_VIN_SRC_DISABLED		(0)
#define AMBA_VIN_SRC_ENABLED_FOR_VIDEO	(0x01)
#define AMBA_VIN_SRC_ENABLED_FOR_STILL	(0x02)

enum amba_vin_src_cmd {
	AMBA_VIN_SRC_IDLE = 20000,
	AMBA_VIN_SRC_RESET,
	AMBA_VIN_SRC_SET_POWER,
	AMBA_VIN_SRC_SUSPEND,
	AMBA_VIN_SRC_RESUME,

	AMBA_VIN_SRC_GET_INFO = 20100,
	AMBA_VIN_SRC_GET_VIDEO_INFO,
	AMBA_VIN_SRC_GET_CAPABILITY,
	AMBA_VIN_SRC_CHECK_VIDEO_MODE,
	AMBA_VIN_SRC_SELECT_CHANNEL,
	AMBA_VIN_SRC_GET_AGC_INFO,
	AMBA_VIN_SRC_GET_SHUTTER_INFO,
	AMBA_VIN_SRC_GET_VBLANK_TIME,

	AMBA_VIN_SRC_GET_VIDEO_MODE = 21000,
	AMBA_VIN_SRC_GET_STILL_MODE,
	AMBA_VIN_SRC_GET_FPN,
	AMBA_VIN_SRC_GET_BLC,
	AMBA_VIN_SRC_GET_CAP_WINDOW,
	AMBA_VIN_SRC_GET_FRAME_RATE,	//Ref AMBA_VIDEO_FPS
	AMBA_VIN_SRC_GET_BLANK,
	AMBA_VIN_SRC_GET_PIXEL_SKIP_BIN,
	AMBA_VIN_SRC_GET_SHUTTER_TIME,
	AMBA_VIN_SRC_GET_GAIN_DB,
	AMBA_VIN_SRC_GET_CAPTURE_MODE,
	AMBA_VIN_SRC_GET_TRIGGER_MODE,
	AMBA_VIN_SRC_GET_LOW_LIGHT_MODE,
	AMBA_VIN_SRC_GET_SLOWSHUTTER_MODE,
	AMBA_VIN_SRC_GET_MIRROR_MODE,
	AMBA_VIN_SRC_GET_ANTI_FLICKER,
	AMBA_VIN_SRC_GET_DGAIN_RATIO,
	AMBA_VIN_SRC_GET_EIS_INFO,
	AMBA_VIN_SRC_GET_OPERATION_MODE,
	AMBA_VIN_SRC_GET_WDR_AGAIN,
	AMBA_VIN_SRC_GET_WDR_DGAIN_GROUP,
	AMBA_VIN_SRC_GET_WDR_SHUTTER_GROUP,
	AMBA_VIN_SRC_GET_SENSOR_TEMPERATURE,
	AMBA_VIN_SRC_GET_AAA_INFO,

	AMBA_VIN_SRC_SET_VIDEO_MODE = 22000,
	AMBA_VIN_SRC_SET_STILL_MODE,
	AMBA_VIN_SRC_SET_FPN,
	AMBA_VIN_SRC_SET_BLC,
	AMBA_VIN_SRC_SET_CAP_WINDOW,
	AMBA_VIN_SRC_SET_FRAME_RATE,	//Ref AMBA_VIDEO_FPS
	AMBA_VIN_SRC_SET_BLANK,
	AMBA_VIN_SRC_SET_PIXEL_SKIP_BIN,
	AMBA_VIN_SRC_SET_SHUTTER_TIME,
	AMBA_VIN_SRC_SET_GAIN_DB,
	AMBA_VIN_SRC_GET_SHUTTER_TIME_WIDTH,
	AMBA_VIN_SRC_SET_CAPTURE_MODE,
	AMBA_VIN_SRC_SET_TRIGGER_MODE,
	AMBA_VIN_SRC_SET_LOW_LIGHT_MODE,
	AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE,
	AMBA_VIN_SRC_SET_MIRROR_MODE,
	AMBA_VIN_SRC_SET_ANTI_FLICKER,
	AMBA_VIN_SRC_SET_OPERATION_MODE,
	AMBA_VIN_SRC_SET_DGAIN_RATIO,
	AMBA_VIN_SRC_SET_WDR_AGAIN,
	AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX,
	AMBA_VIN_SRC_SET_WDR_DGAIN_GROUP,
	AMBA_VIN_SRC_SET_WDR_DGAIN_INDEX_GROUP,
	AMBA_VIN_SRC_SET_WDR_AGAIN_INDEX_GROUP,
	AMBA_VIN_SRC_SET_WDR_SHUTTER_GROUP,
	AMBA_VIN_SRC_WDR_SHUTTER2ROW,
	AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW,
	AMBA_VIN_SRC_SET_WDR_SHUTTER_ROW_GROUP,
	AMBA_VIN_SRC_SET_GAIN_INDEX,
	AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW_SYNC,
	AMBA_VIN_SRC_SET_GAIN_INDEX_SYNC,
	AMBA_VIN_SRC_SET_SHUTTER_AND_GAIN_SYNC,
	AMBA_VIN_SRC_GET_WDR_WIN_OFFSET,

	AMBA_VIN_SRC_TEST_DUMP_REG = 29000,
	AMBA_VIN_SRC_TEST_GET_DEV_ID,
	AMBA_VIN_SRC_TEST_GET_REG_DATA,
	AMBA_VIN_SRC_TEST_SET_REG_DATA,

	AMBA_VIN_SRC_TEST_DUMP_SBRG_REG = 30000,
	AMBA_VIN_SRC_TEST_GET_SBRG_REG_DATA,
	AMBA_VIN_SRC_TEST_SET_SBRG_REG_DATA,
};

#define AMBA_VIN_INPUT_FORMAT_RGB_RAW		(0)
#define AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC	(1)
#define AMBA_VIN_INPUT_FORMAT_YUV_422_PROG	(2)

#include "amba_arch_vin.h"

struct iav_context_s;
/* ========================================================================== */
int amba_vin_adapter_cmd(int adapid, enum amba_vin_adap_cmd cmd, void *args);
int amba_vin_source_cmd(int srcid, int chid,
	enum amba_vin_src_cmd cmd, void *args);
int amba_vin_pm(u32 pmval);

/* VIN FPS STAT */
int amba_vin_vsync_calc_fps(struct amba_fps_report_s *fps_report);
int amba_vin_vsync_calc_fps_reset(void);
void amba_vin_vsync_calc_fps_wait(void);

#endif

