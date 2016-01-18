/*
 * kernel/private/include/amba_dsp.h
 *
 * History:
 *    2008/08/03 - [Anthony Ginger] Create
 *
 * Copyright (C) 2008-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_DSP_H
#define __AMBA_DSP_H

/* ==========================================================================*/
#define dsp_print(arg)		\
	printk("%s: "#arg" = 0x%X\n", __func__, (u32)dsp_cmd.arg);

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif
/* ==========================================================================*/
#define AMBA_DSP_VFR(vfr)			(vfr)
#define AMBA_DSP_VFR_INTERLACE(vfr)		(vfr|0x80)
#define AMBA_DSP_VFR_29_97_INTERLACE		AMBA_DSP_VFR(0)
#define AMBA_DSP_VFR_29_97_PROGRESSIVE		AMBA_DSP_VFR(1)
#define AMBA_DSP_VFR_59_94_INTERLACE		AMBA_DSP_VFR(2)
#define AMBA_DSP_VFR_59_94_PROGRESSIVE		AMBA_DSP_VFR(3)
#define AMBA_DSP_VFR_23_976_PROGRESSIVE		AMBA_DSP_VFR(4)
#define AMBA_DSP_VFR_12_5_PROGRESSIVE		AMBA_DSP_VFR(5)
#define AMBA_DSP_VFR_6_25_PROGRESSIVE		AMBA_DSP_VFR(6)
#define AMBA_DSP_VFR_3_125_PROGRESSIVE		AMBA_DSP_VFR(7)
#define AMBA_DSP_VFR_7_5_PROGRESSIVE		AMBA_DSP_VFR(8)
#define AMBA_DSP_VFR_3_75_PROGRESSIVE		AMBA_DSP_VFR(9)
#define AMBA_DSP_VFR_10_PROGRESSIVE		AMBA_DSP_VFR(10)
#define AMBA_DSP_VFR_15_PROGRESSIVE		AMBA_DSP_VFR(15)
#define AMBA_DSP_VFR_24_PROGRESSIVE		AMBA_DSP_VFR(24)
#define AMBA_DSP_VFR_25_PROGRESSIVE		AMBA_DSP_VFR(25)
#define AMBA_DSP_VFR_30_PROGRESSIVE		AMBA_DSP_VFR(30)
#define AMBA_DSP_VFR_50_PROGRESSIVE		AMBA_DSP_VFR(50)
#define AMBA_DSP_VFR_60_PROGRESSIVE		AMBA_DSP_VFR(60)
#define AMBA_DSP_VFR_120_PROGRESSIVE		AMBA_DSP_VFR(120)
#define AMBA_DSP_VFR_25_INTERLACE		AMBA_DSP_VFR_INTERLACE(25)
#define AMBA_DSP_VFR_50_INTERLACE		AMBA_DSP_VFR_INTERLACE(50)

#define AMBA_DSP_VFR_23_976_INTERLACED		AMBA_DSP_VFR_INTERLACE(4)
#define AMBA_DSP_VFR_12_5_INTERLACED		AMBA_DSP_VFR_INTERLACE(5)
#define AMBA_DSP_VFR_6_25_INTERLACED		AMBA_DSP_VFR_INTERLACE(6)
#define AMBA_DSP_VFR_3_125_INTERLACED		AMBA_DSP_VFR_INTERLACE(7)
#define AMBA_DSP_VFR_7_5_INTERLACED		AMBA_DSP_VFR_INTERLACE(8)
#define AMBA_DSP_VFR_3_75_INTERLACED		AMBA_DSP_VFR_INTERLACE(9)

#define MIN_HDR_EXPO_NUM	(1)
#define MAX_HDR_EXPO_NUM	(5)

#if defined(CONFIG_ARCH_A5S) || defined(CONFIG_ARCH_A7L) || defined(CONFIG_ARCH_I1)
static inline u32 amba_iav_fps_format_to_vfr(u32 fps, u32 format)
{
	u32 vfr;

	switch (format) {
	case AMBA_VIDEO_FORMAT_INTERLACE:
		switch (fps) {
		case AMBA_VIDEO_FPS_59_94:
			vfr = AMBA_DSP_VFR_59_94_INTERLACE;
			break;
		case AMBA_VIDEO_FPS_29_97:
			vfr = AMBA_DSP_VFR_29_97_INTERLACE;
			break;
		case AMBA_VIDEO_FPS_23_976:
			vfr = AMBA_DSP_VFR_23_976_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_12_5:
			vfr = AMBA_DSP_VFR_12_5_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_6_25:
			vfr = AMBA_DSP_VFR_6_25_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_3_125:
			vfr = AMBA_DSP_VFR_3_125_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_7_5:
			vfr = AMBA_DSP_VFR_7_5_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_3_75:
			vfr = AMBA_DSP_VFR_3_75_INTERLACED;
			break;
		default :
			vfr = AMBA_DSP_VFR_INTERLACE(DIV_ROUND(512000000, fps));
			break;
		}
		break;

	case AMBA_VIDEO_FORMAT_PROGRESSIVE:
		switch (fps) {
		case AMBA_VIDEO_FPS_59_94:
			vfr = AMBA_DSP_VFR_59_94_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_29_97:
			vfr = AMBA_DSP_VFR_29_97_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_23_976:
			vfr = AMBA_DSP_VFR_23_976_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_12_5:
			vfr = AMBA_DSP_VFR_12_5_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_6_25:
			vfr = AMBA_DSP_VFR_6_25_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_3_125:
			vfr = AMBA_DSP_VFR_3_125_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_7_5:
			vfr = AMBA_DSP_VFR_7_5_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_3_75:
			vfr = AMBA_DSP_VFR_3_75_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_25:
			vfr = AMBA_DSP_VFR_25_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_30:
			vfr = AMBA_DSP_VFR_30_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_24:
			vfr = AMBA_DSP_VFR_24_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_15:
			vfr = AMBA_DSP_VFR_15_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_50:
			vfr = AMBA_DSP_VFR_50_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_60:
			vfr = AMBA_DSP_VFR_60_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_120:
			vfr = AMBA_DSP_VFR_120_PROGRESSIVE;
			break;
		default :
			vfr = AMBA_DSP_VFR(DIV_ROUND(512000000, fps));
			break;
		}
		break;

	default :
		vfr = AMBA_DSP_VFR_29_97_PROGRESSIVE;
		break;
	}

	return vfr;
}

#endif

#ifdef CONFIG_ARCH_S2
static inline u32 amba_iav_fps_format_to_vfr(u32 fps, u32 format, u32 multi_frames)
{
	u32 vfr;

	switch (format) {
	case AMBA_VIDEO_FORMAT_INTERLACE:
		switch (fps) {
		case AMBA_VIDEO_FPS_59_94:
			vfr = AMBA_DSP_VFR_59_94_INTERLACE;
			break;
		case AMBA_VIDEO_FPS_29_97:
			vfr = AMBA_DSP_VFR_29_97_INTERLACE;
			break;
		case AMBA_VIDEO_FPS_23_976:
			vfr = AMBA_DSP_VFR_23_976_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_12_5:
			vfr = AMBA_DSP_VFR_12_5_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_6_25:
			vfr = AMBA_DSP_VFR_6_25_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_3_125:
			vfr = AMBA_DSP_VFR_3_125_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_7_5:
			vfr = AMBA_DSP_VFR_7_5_INTERLACED;
			break;
		case AMBA_VIDEO_FPS_3_75:
			vfr = AMBA_DSP_VFR_3_75_INTERLACED;
			break;
		default:
			vfr = AMBA_DSP_VFR_INTERLACE(DIV_ROUND(512000000, fps));
			break;
		}
		break;

	case AMBA_VIDEO_FORMAT_PROGRESSIVE:
		switch (fps) {
		case AMBA_VIDEO_FPS_59_94:
			vfr = (multi_frames == 2) ? AMBA_DSP_VFR_29_97_PROGRESSIVE :
				AMBA_DSP_VFR_59_94_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_29_97:
			vfr = (multi_frames == 2) ? AMBA_DSP_VFR_15_PROGRESSIVE :
				AMBA_DSP_VFR_29_97_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_23_976:
			vfr = AMBA_DSP_VFR_23_976_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_12_5:
			vfr = AMBA_DSP_VFR_12_5_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_6_25:
			vfr = AMBA_DSP_VFR_6_25_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_3_125:
			vfr = AMBA_DSP_VFR_3_125_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_7_5:
			vfr = AMBA_DSP_VFR_7_5_PROGRESSIVE;
			break;
		case AMBA_VIDEO_FPS_3_75:
			vfr = AMBA_DSP_VFR_3_75_PROGRESSIVE;
			break;
		default:
			vfr = AMBA_DSP_VFR(DIV_ROUND(512000000, fps));
			if (multi_frames > MIN_HDR_EXPO_NUM &&
					multi_frames < MAX_HDR_EXPO_NUM) {
				vfr /= multi_frames;
			}
			/* Fixme: Convert "interlaced VFR" (2fps) to progressive. */
			if (vfr == AMBA_DSP_VFR_59_94_INTERLACE) {
				vfr = AMBA_DSP_VFR_59_94_PROGRESSIVE;
			} else if (vfr == AMBA_DSP_VFR_29_97_INTERLACE) {
				vfr = AMBA_DSP_VFR_29_97_PROGRESSIVE;
			}

			break;
		}
		break;

	default:
		vfr = AMBA_DSP_VFR_29_97_PROGRESSIVE;
		break;
	}

	return vfr;
}
#endif

/* ==========================================================================*/
#define AMBA_DSP_VIDEO_FORMAT(format)		(format)
#define AMBA_DSP_VIDEO_FORMAT_PROGRESSIVE	AMBA_DSP_VIDEO_FORMAT(0)
#define AMBA_DSP_VIDEO_FORMAT_INTERLACE		AMBA_DSP_VIDEO_FORMAT(1)
#define AMBA_DSP_VIDEO_FORMAT_DEF_PROGRESSIVE	AMBA_DSP_VIDEO_FORMAT(2)
#define AMBA_DSP_VIDEO_FORMAT_DEF_INTERLACE	AMBA_DSP_VIDEO_FORMAT(3)
#define AMBA_DSP_VIDEO_FORMAT_TOP_PROGRESSIVE	AMBA_DSP_VIDEO_FORMAT(4)
#define AMBA_DSP_VIDEO_FORMAT_BOT_PROGRESSIVE	AMBA_DSP_VIDEO_FORMAT(5)
#define AMBA_DSP_VIDEO_FORMAT_NO_VIDEO		AMBA_DSP_VIDEO_FORMAT(6)

static inline u8 amba_iav_format_to_format(u32 format)
{
	u8 new_format = AMBA_DSP_VIDEO_FORMAT_NO_VIDEO;

	if (format == AMBA_VIDEO_FORMAT_INTERLACE) {
		new_format = AMBA_DSP_VIDEO_FORMAT_INTERLACE;
	} else
	if (format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		new_format = AMBA_DSP_VIDEO_FORMAT_PROGRESSIVE;
	}

	return new_format;
}

/* ==========================================================================*/
#define AMBA_DSP_VIDEO_FPS(format)		(format)
#define AMBA_DSP_VIDEO_FPS_29_97		AMBA_DSP_VIDEO_FPS(0)
#define AMBA_DSP_VIDEO_FPS_59_94		AMBA_DSP_VIDEO_FPS(1)
#define AMBA_DSP_VIDEO_FPS_23_976		AMBA_DSP_VIDEO_FPS(2)
#define AMBA_DSP_VIDEO_FPS_12_5			AMBA_DSP_VIDEO_FPS(3)
#define AMBA_DSP_VIDEO_FPS_6_25			AMBA_DSP_VIDEO_FPS(4)
#define AMBA_DSP_VIDEO_FPS_3_125		AMBA_DSP_VIDEO_FPS(5)
#define AMBA_DSP_VIDEO_FPS_7_5			AMBA_DSP_VIDEO_FPS(6)
#define AMBA_DSP_VIDEO_FPS_3_75			AMBA_DSP_VIDEO_FPS(7)
#define AMBA_DSP_VIDEO_FPS_15			AMBA_DSP_VIDEO_FPS(15)
#define AMBA_DSP_VIDEO_FPS_24			AMBA_DSP_VIDEO_FPS(24)
#define AMBA_DSP_VIDEO_FPS_25			AMBA_DSP_VIDEO_FPS(25)
#define AMBA_DSP_VIDEO_FPS_30			AMBA_DSP_VIDEO_FPS(30)
#define AMBA_DSP_VIDEO_FPS_50			AMBA_DSP_VIDEO_FPS(50)
#define AMBA_DSP_VIDEO_FPS_60			AMBA_DSP_VIDEO_FPS(60)
#define AMBA_DSP_VIDEO_FPS_120			AMBA_DSP_VIDEO_FPS(120)

static inline u8 amba_iav_fps_to_fps(u32 fps)
{
	u8 new_fps;
	switch (fps) {
	case AMBA_VIDEO_FPS_29_97:
		new_fps = AMBA_DSP_VIDEO_FPS_29_97;
		break;
	case AMBA_VIDEO_FPS_59_94:
		new_fps = AMBA_DSP_VIDEO_FPS_59_94;
		break;
	case AMBA_VIDEO_FPS_25:
		new_fps = AMBA_DSP_VIDEO_FPS_25;
		break;
	case AMBA_VIDEO_FPS_30:
		new_fps = AMBA_DSP_VIDEO_FPS_30;
		break;
	case AMBA_VIDEO_FPS_50:
		new_fps = AMBA_DSP_VIDEO_FPS_50;
		break;
	case AMBA_VIDEO_FPS_60:
		new_fps = AMBA_DSP_VIDEO_FPS_60;
		break;
	case AMBA_VIDEO_FPS_15:
		new_fps = AMBA_DSP_VIDEO_FPS_15;
		break;
	case AMBA_VIDEO_FPS_7_5:
		new_fps = AMBA_DSP_VIDEO_FPS_7_5;
		break;
	case AMBA_VIDEO_FPS_3_75:
		new_fps = AMBA_DSP_VIDEO_FPS_3_75;
		break;
	case AMBA_VIDEO_FPS_12_5:
		new_fps = AMBA_DSP_VIDEO_FPS_12_5;
		break;
	case AMBA_VIDEO_FPS_6_25:
		new_fps = AMBA_DSP_VIDEO_FPS_6_25;
		break;
	case AMBA_VIDEO_FPS_3_125:
		new_fps = AMBA_DSP_VIDEO_FPS_3_125;
		break;
	case AMBA_VIDEO_FPS_23_976:
		new_fps = AMBA_DSP_VIDEO_FPS_23_976;
		break;
	case AMBA_VIDEO_FPS_24:
		new_fps = AMBA_DSP_VIDEO_FPS_24;
		break;
	case AMBA_VIDEO_FPS_120:
		new_fps = AMBA_DSP_VIDEO_FPS_120;
		break;

	default:
	//	printk(KERN_DEBUG "FPS not caught in amba_iav_fps_to_fps, use default 29.97\n");
		new_fps = AMBA_DSP_VIDEO_FPS_29_97;
		break;
	}

	return new_fps;
}

/* ==========================================================================*/

#endif	//__AMBA_DSP_H

