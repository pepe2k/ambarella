/*
 * iav_config.h
 *
 * History:
 *	2012/02/20 - [Jian Tang] create this file for S2 configurations
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_CONFIG_H__
#define __IAV_CONFIG_H__

// Just test some features, need special ucode.
#ifdef CONFIG_IAV_FOR_DEBUG_DSP
#define DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
#endif

#ifndef KB
#define KB		(1024)
#endif

#ifndef MB
#define MB		(1024 * 1024)
#endif


/************************************************
 *
 * Following definitions can be modified.
 * They are software limitations based on different
 * NAND size and memory configurations.
 *
 ***********************************************/

/*
 * Debug option
 */
#define CONFIG_AMBARELLA_IAV_DEBUG


/*
 * Bit stream info descriptor total number, each item is 64 bytes
 */
#define	NUM_BS_DESC				(512)


/*
 * Motion vector memory
 */
#define	DEFAULT_MV_MALLOC_SIZE		(1 * MB)


/*
 * Encode statistics info description total number, each item is 64 bytes
 */
#define	NUM_STATIS_DESC			(64)
#define	NUM_SD_PER_STREAM		(NUM_STATIS_DESC / IAV_MAX_ENCODE_STREAMS_NUM)


/*
 * QP histogram memory, each item is 48 bytes
 */
#define	DEFAULT_QP_HIST_SIZE			(PAGE_ALIGN(NUM_STATIS_DESC * sizeof(iav_qp_hist_ex_t)))


/*
 * Default toggled buffer number to update the internal buffer per frame
 */
#define	DEFAULT_TOGGLED_BUFFER_NUM		(4)


/*
 * Parameters for motion vector dump
 */
typedef enum {
	MAX_STATIS_POLLING_TIMES = 1000,
	MAX_STATIS_POLLING_PERIOD_MS = 10,

	MAX_MVDUMP_DIV_FACTOR = 100,
} IAV_ENCODE_STATISTICS_PARAMS;


/*
 * Parameters for polling readout protocol
 */
typedef enum {
	MAX_BSB_POLLING_TIMES = 1000,
	MAX_BSB_POLLING_PERIOD_MS = 2,
} IAV_ENCODE_BSB_PARAMS;



/************************************************
 *
 * Following definitions CANNOT be modified !!
 * They are HW limitations !!
 *
 ***********************************************/

/*
 * Basic configuration
 */
#define PIXEL_IN_MB						(16)
#define MAX_B_FRAME_NUM				(3)
#define IAV_MAX_ENCODE_BITRATE		(50 * MB)
#define AUDIO_CLK_KHZ					(12288)
#define MIN_CMD_READ_DELAY_IN_MS		(4)
#define MAX_EIS_DELAY_COUNT			(2)
#define MIN_CMD_READ_DELAY_IN_CLK	(AUDIO_CLK_KHZ * MIN_CMD_READ_DELAY_IN_MS)
#define MAX_VIN_FPS_FOR_FULL_FEATURE		(110)
#define MAX_VIN_FPS_FOR_DUAL_1080P	(90)
#define MAX_VIN_FPS_FOR_TRIPLE_1080P	(60)
#define MAX_FRAME_DROP_COUNT			(300)

#define TIMEOUT_JIFFIES		msecs_to_jiffies(2000)

/*
 * Encode configuration
 */
typedef enum {
	IAV_DEFAULT_ENCODE_STREAMS_NUM = 4,
	IAV_MAX_HDR_STREAMS_NUM = 5,
	IAV_MAX_DEWARP_STREAMS_NUM = 6,
	IAV_MEDIUM_ENCODE_STREAMS_NUM = 8,
	IAV_MAX_ENCODE_STREAMS_NUM = 8,

	STREAM_ID_MASK = ((1 << IAV_MAX_ENCODE_STREAMS_NUM) - 1),
} IAV_ENCODE_STREAM_PARAMS;

#define IAV_MAX_PREVIEW_NUM			(1)
#define PREVIEW_ID_MASK	((1 << IAV_MAX_PREVIEW_NUM) - 1)


#define IAV_MAX_ENC_DRAM_BUF_NUM	(10)
#define IAV_MIN_ENC_DRAM_FRAME_NUM		(2)


/*
 * Encode timing configuration
 */
typedef enum {
	/* Wait before enocding for buffer size change */
	IAV_WAIT_VSYNC_BEFORE_ENCODE_MIN = 3,
	IAV_WAIT_VSYNC_BEFORE_ENCODE = 4,
	IAV_WAIT_VSYNC_BEFORE_ENCODE_MAX = 5,

	/* Wait to sync up the offset and warp param */
	IAV_WAIT_VSYNC_SYNC_OFFSET_WARP = 4,
} IAV_ENCODE_TIMING;


/*
 * Memory configuration
 */
typedef enum {
	IAV_DRAM_SIZE_1Gb = 0x01,
	IAV_DRAM_SIZE_2Gb = 0x02,
	IAV_DRAM_SIZE_4Gb = 0x04,
	IAV_DRAM_SIZE_8Gb = 0x08,
	IAV_DRAM_SIZE_16Gb = 0x10,
	IAV_DRAM_SIZE_32Gb = 0x20,
} IAV_DRAM_CAPACITY;


/*
 * Memory allocation
 */
typedef enum {
	USER_MEM_SIZE = (10 * MB),
	OVERLAY_MEM_SIZE = (4 * MB),
	BITSTREAM_MEM_MAX_SIZE = (100 * MB),
	BITSTREAM_MEM_SIZE = (10 * MB),
	PRIVACYMASK_MEM_SIZE_MIN = (4 * MB),

#ifdef CONFIG_IMGPROC_MEM_LARGE
	IMGPROC_MEM_SIZE = (64 * MB),
#else
	IMGPROC_MEM_SIZE = (5 * MB),
#endif
	IMG_KERNEL_OFFSET = (4 * MB),
	QP_MATRIX_FRAME_TYPE_NUM = 3,
	STREAM_QP_MATRIX_MEM_SIZE = (128 * 1024) * QP_MATRIX_FRAME_TYPE_NUM,
	STREAM_QP_MATRIX_MEM_TOTAL_SIZE = (STREAM_QP_MATRIX_MEM_SIZE *
		IAV_MAX_ENCODE_STREAMS_NUM),
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	DEFAULT_TOGGLED_ENC_CMD_NUM = 8,		//max dummy latency frames(4) * 2.
	VCAP_ENC_CMD_SIZE = 128,			//for ENCODER_UPDATE_ENC_PARAMETERS_CMD
	VCAP_ENC_CMD_MEM_TOTAL_SIZE = (VCAP_ENC_CMD_SIZE *
		DEFAULT_TOGGLED_ENC_CMD_NUM * IAV_MAX_ENCODE_STREAMS_NUM),
#endif
} IAV_DRAM_ALLOCATION;


typedef enum {
	IAV_EXTRA_BUF_FRAME_DEFAULT = 0,
	IAV_EXTRA_BUF_FRAME_MIN = -4,
	IAV_EXTRA_BUF_FRAME_MAX = 20,
	IAV_EXTRA_BUF_FRAME_TOTAL = 20,
} IAV_EXTRA_DRAM_BUF;


/*
 * Encode performance load
 */
typedef enum {
	// 293460 = (1920/16) * (1088/16) * 31 + (720/16) * (480/16) * 30
	// 409140 = (2048/16) * (1536/16) * 30 + (720/16) * (480/16) * 30
	// 491520 = (2048/16) * (2048/16) * 30
	// 538260 = (1920/16) * (1088/16) * 61 + (720/16) * (480/16) * 30
	// 570600 = (1920/16) * (1088/16) * 60 + (720/16) * (480/16) * 60
	// 633420 = (2592/16) * (1952/16) * 30 + (720/16) * (480/16) * 30
	// 852950 = (4096/16) * (2048/16) * 25 + (720/16) * (480/16) * 25
	// 897600 = (1920/16) * (1088/16) * 110
	// 979200 = (1920/16) * (1088/16) * 120
	// 983040 = (4096/16) * (2048/16) * 30
	LOAD_1080P31_480P30 = (1920 * 1088 / 256 * 31 + 720 * 480 / 256 * 30),
	LOAD_3MP30_480P30 = (2048 * 1536 / 256 * 30 + 720 * 480 /256 * 30),
	LOAD_4MP30 = (2048 * 2048 / 256 * 30),
	LOAD_1080P61_480P30 = (1920 * 1088 / 256 * 61 + 720 * 480 / 256 * 30),
	LOAD_1080P60_480P60 = (1920 * 1088 / 256 * 60 + 720 * 480 / 256 * 60),
	LOAD_5MP30_480P30 = (2592 * 1952 / 256 * 30 + 720 * 480 / 256 * 30),
	LOAD_4KP25_480P25 = (4096 * 2048 / 256 * 25 + 720 * 480 / 256 * 25),
	LOAD_4KP25_480P30 = (4096 * 2048 / 256 * 25 + 720 * 480 / 256 * 30),
	LOAD_1080P110 = (1920 * 1088 / 256 * 110),
	LOAD_1080P120 = (1920 * 1088 / 256 * 120),
	LOAD_4Kx2KP30 = (4096 * 2048 / 256 * 30),

	SYSTEM_LOAD_S2_22 = LOAD_1080P31_480P30,
	SYSTEM_LOAD_S2_33 = LOAD_3MP30_480P30,
	SYSTEM_LOAD_S2_55 = LOAD_1080P60_480P60,
	SYSTEM_LOAD_S2_66 = LOAD_5MP30_480P30,
	SYSTEM_LOAD_S2_88 = LOAD_1080P120,
	SYSTEM_LOAD_S2_99 = LOAD_1080P120,
	SYSTEM_LOAD_NUM = 6,
} IAV_SYSTEM_LOAD;


/*
 * Encode resource limitation
 */
typedef enum {
	MIN_WIDTH_WITH_ROTATE = 240,
	MIN_WIDTH_WITHOUT_ROTATE = 320,
	MIN_REGION_WIDTH_IN_WARP = 352,
	MIN_HEIGHT_IN_STREAM = 192,

	MAX_WIDTH_IN_BUFFER = (7 << 10),
	MAX_HEIGHT_IN_BUFFER = (4 << 10),

	MAX_WIDTH_IN_OVERLAY = 1920,

	MAX_WIDTH_IN_2X_SEARCH_RANGE = 2592,
	MAX_HEIGHT_IN_2X_SEARCH_RANGE = 1944,

	MAX_WIDTH_IN_WARP_REGION = 2560,

	MAX_WIDTH_IN_MULTI_VIN = (16 << 10),
	MAX_HEIGHT_IN_MULTI_VIN = (8 << 10),

	/* Mode 0: Full frame rate mode for CFA */
	MAX_WIDTH_IN_FULL_FPS = 2716,
	MAX_HEIGHT_IN_FULL_FPS = MAX_HEIGHT_IN_BUFFER,
	MIN_WIDTH_IN_FULL_FPS = MIN_WIDTH_WITH_ROTATE,
	MIN_HEIGHT_IN_FULL_FPS = MIN_HEIGHT_IN_STREAM,

	/* Mode 1: Multi-region warping mode for CFA input */
	MAX_PRE_WIDTH_IN_WARP = 2640,
	MAX_PRE_HEIGHT_IN_WARP = 2640,
	MAX_WIDTH_IN_WARP = MAX_WIDTH_IN_BUFFER,
	MAX_HEIGHT_IN_WARP = MAX_HEIGHT_IN_BUFFER,
	MIN_WIDTH_IN_WARP = MIN_WIDTH_WITHOUT_ROTATE,
	MIN_HEIGHT_IN_WARP = MIN_HEIGHT_IN_STREAM,

	/* Mode 2: High Mega pixel mode for CFA input */
	MAX_WIDTH_IN_HIGH_MP = (5 << 10),
	MAX_HEIGHT_IN_HIGH_MP = (8 << 10),
	MAX_WIDTH_FOR_1080P_MAIN_IN_HIGH_MP = 3840,
	MIN_WIDTH_IN_HIGH_MP = MIN_WIDTH_WITHOUT_ROTATE,
	MIN_HEIGHT_IN_HIGH_MP = MIN_HEIGHT_IN_STREAM,

	/* Mode 3: Calibration mode for CFA input */
	MAX_WIDTH_IN_CALIB = MAX_WIDTH_IN_HIGH_MP,
	MAX_HEIGHT_IN_CALIB = MAX_HEIGHT_IN_HIGH_MP,
	MIN_WIDTH_IN_CALIB = MIN_WIDTH_WITHOUT_ROTATE,
	MIN_HEIGHT_IN_CALIB = MIN_HEIGHT_IN_STREAM,

	/* Mode 4: Frame interleaved HDR sensor */
	MAX_WIDTH_IN_HDR_FI = MAX_WIDTH_IN_FULL_FPS,
	MAX_HEIGHT_IN_HDR_FI = MAX_HEIGHT_IN_FULL_FPS,
	MIN_WIDTH_IN_HDR_FI = MIN_WIDTH_IN_FULL_FPS,
	MIN_HEIGHT_IN_HDR_FI = MIN_HEIGHT_IN_FULL_FPS,

	/* Mode 5: Line interleaved HDR sensor */
	MAX_WIDTH_IN_HDR_LI = MAX_WIDTH_IN_FULL_FPS,
	MAX_HEIGHT_IN_HDR_LI = MAX_HEIGHT_IN_FULL_FPS,
	MIN_WIDTH_IN_HDR_LI = MIN_WIDTH_IN_FULL_FPS,
	MIN_HEIGHT_IN_HDR_LI = MIN_HEIGHT_IN_FULL_FPS,

	/* Mode 6: High Mega Pixel & full performance mode for CFA input */
	MAX_WIDTH_IN_HIGH_MP_FP = MAX_WIDTH_IN_HIGH_MP,
	MAX_HEIGHT_IN_HIGH_MP_FP = MAX_HEIGHT_IN_HIGH_MP,
	MIN_WIDTH_IN_HIGH_MP_FP = MIN_WIDTH_IN_HIGH_MP,
	MIN_HEIGHT_IN_HIGH_MP_FP = MIN_HEIGHT_IN_HIGH_MP,

	/* Mode 7: Full frame rate & full performance mode for CFA input */
	MAX_WIDTH_IN_FULL_FPS_FP = MAX_WIDTH_IN_FULL_FPS,
	MAX_HEIGHT_IN_FULL_FPS_FP = MAX_HEIGHT_IN_FULL_FPS,
	MIN_WIDTH_IN_FULL_FPS_FP = MIN_WIDTH_IN_FULL_FPS,
	MIN_HEIGHT_IN_FULL_FPS_FP = MIN_HEIGHT_IN_FULL_FPS,

	/* Mode 8: Multiple CFA VIN */
	MAX_WIDTH_IN_MULTI_CFA_VIN = MAX_WIDTH_IN_MULTI_VIN,
	MAX_HEIGHT_IN_MULTI_CFA_VIN = MAX_HEIGHT_IN_MULTI_VIN,
	MIN_WIDTH_IN_MULTI_CFA_VIN = MIN_WIDTH_WITHOUT_ROTATE,
	MIN_HEIGHT_IN_MULTI_CFA_VIN = MIN_HEIGHT_IN_FULL_FPS,

	/* Mode 9: HISO */
	MAX_WIDTH_IN_HISO = 1920,
	MAX_HEIGHT_IN_HISO = 1080,
	MIN_WIDTH_IN_HISO = MIN_WIDTH_WITH_ROTATE,
	MIN_HEIGHT_IN_HISO = MIN_HEIGHT_IN_STREAM,

	/* Mode 10: High Mega Pixel & Multi-region warping mode */
	MAX_PRE_WIDTH_IN_HIGH_MP_WARP = 4096,
	MAX_PRE_HEIGHT_IN_HIGH_MP_WARP = 3072,
	MAX_WIDTH_IN_HIGH_MP_WARP = MAX_WIDTH_IN_BUFFER,
	MAX_HEIGHT_IN_HIGH_MP_WARP = MAX_HEIGHT_IN_BUFFER,
	MIN_WIDTH_IN_HIGH_MP_WARP =MIN_WIDTH_WITHOUT_ROTATE,
	MIN_HEIGHT_IN_HIGH_MP_WARP = MIN_HEIGHT_IN_STREAM,
} RESOLUTION_RESOURCE_LIMIT;


/*
 * IDSP Capture resource limitation
 */
typedef enum {
	MAX_CAPTURE_PPS_IDSP = 264*1000*1000,			// 264Mpps
	MAX_CAPTURE_PPS_12MP30 = 4016 * 3016 * 30,		// 12MP30
	MAX_CAPTURE_PPS_8P9MP30 = 4096 * 2160 * 30,	// 8.9MP30
	MAX_CAPTURE_PPS_8P9MP60 = 4096 * 2160 * 60,	// 8.9MP60

	MAX_CAP_PPS_IN_FULL_FPS = MAX_CAPTURE_PPS_8P9MP60,
	MAX_CAP_PPS_IN_WARP = MAX_CAPTURE_PPS_8P9MP60,
	MAX_CAP_PPS_IN_HIGH_MP = MAX_CAPTURE_PPS_8P9MP60,
	MAX_CAP_PPS_IN_CALIB = MAX_CAPTURE_PPS_IDSP,
	MAX_CAP_PPS_IN_HDR_FI = MAX_CAPTURE_PPS_8P9MP60,
	MAX_CAP_PPS_IN_HDR_LI = MAX_CAPTURE_PPS_8P9MP60,
	MAX_CAP_PPS_IN_HIGH_MP_FP = MAX_CAP_PPS_IN_HIGH_MP,
	MAX_CAP_PPS_IN_FULL_FPS_FP = MAX_CAP_PPS_IN_FULL_FPS,
	MAX_CAP_PPS_IN_MULTI_CFA_VIN = MAX_CAP_PPS_IN_FULL_FPS,
	MAX_CAP_PPS_IN_HISO = MAX_CAP_PPS_IN_FULL_FPS,
	MAX_CAP_PPS_IN_HIGH_MP_WARP = MAX_CAP_PPS_IN_FULL_FPS,
} CAPTURE_RESOURCE_LIMIT;


/*
 * Capture source buffer configuration
 */
#define IAV_MAX_SOURCE_BUFFER_NUM	(IAV_ENCODE_SOURCE_TOTAL_NUM)
#define IAV_EXTRA_BUF_FOR_VCA_CP		(2)

typedef enum {
	MAX_ZOOM_IN_FACTOR_FOR_1ST = 8,
	MAX_ZOOM_OUT_FACTOR_FOR_1ST = 8,

	MAX_ZOOM_IN_FACTOR_FOR_2ND = 1,
	MAX_ZOOM_OUT_FACTOR_FOR_2ND = 16,

	MAX_ZOOM_IN_FACTOR_FOR_3RD = 8,
	MAX_ZOOM_OUT_FACTOR_FOR_3RD = 8,

	MAX_ZOOM_IN_FACTOR_FOR_4TH = 8,
	MAX_ZOOM_OUT_FACTOR_FOR_4TH = 16,

	MAX_ZOOM_IN_FACTOR_FOR_1ST_DRAM = 1,
	MAX_ZOOM_OUT_FACTOR_FOR_1ST_DRAM = 1,
} SOURCE_BUFFER_ZOOM_FACTOR_LIMIT;

typedef enum {
	MAX_WIDTH_FOR_1ST = MAX_WIDTH_IN_BUFFER,
	MAX_HEIGHT_FOR_1ST = MAX_HEIGHT_IN_BUFFER,

	MAX_WIDTH_FOR_2ND = 720,
	MAX_HEIGHT_FOR_2ND = 720,

	MAX_WIDTH_FOR_3RD = 2716,
	MAX_HEIGHT_FOR_3RD = 2716,

	MAX_WIDTH_FOR_4TH = 2048,
	MAX_HEIGHT_FOR_4TH = 2048,

	MAX_WIDTH_FOR_1ST_DRAM = 4096,
	MAX_HEIGHT_FOR_1ST_DRAM = 2160,
} SOURCE_BUFFER_RESOLUTION_LIMIT;


/*
 * RAW statistics lines capture configuration
 */
typedef enum {
	MAX_RAW_STATS_LINES_TOP = 15,
	MAX_RAW_STATS_LINES_BOT = 15,
} IAV_RAW_STATS_LINES_LIMIT;


/*
 * HDR configuration
 */
typedef enum {
	// Multi exposures supported in DSP
	MIN_HDR_EXPOSURES = 1,
	MAX_HDR_EXPOSURES = 4,
} IAV_HDR_PARAMS_LIMIT;


/*
 * Multiple CFA VIN configuration
 */
typedef enum {
	MIN_CFA_VIN_NUM = 2,
	MAX_CFA_VIN_NUM = 4,
} IAV_MULTI_VIN_PARAMS_LIMIT;


/*
 * Buffer capture configuration (YUV / ME1 / RAW)
 */
typedef enum {
	IAV_BUFCAP_MIN = 9,
	IAV_BUFCAP_MAX = IAV_BUFCAP_MIN + IAV_EXTRA_BUF_FRAME_MAX,
} IAV_BUFCAP_LIMIT;


/*
 * Overlay memory configuration
 */
typedef enum {
	// OSD width CANNOT be larger than 1920, it cannot do stitching
	OVERLAY_AREA_MAX_WIDTH = MAX_WIDTH_IN_OVERLAY,
} IAV_OVERLAY_PARAMS_LIMIT;


/*
 * Privacy mask memory configuration
 */
typedef enum {
	PRIVACY_MASK_WIDTH_MAX = MAX_WIDTH_IN_HIGH_MP,
	PRIVACY_MASK_HEIGHT_MAX = MAX_HEIGHT_IN_HIGH_MP,
} IAV_PRIVACY_MASK_PARAMS_LIMIT;


/*
 * De-Warp configuration
 */
typedef enum {
	MIN_WARP_REGIONS_NUM = 0,
	MEDIUM_WARP_REGIONS_NUM = 6,
	MAX_WARP_REGIONS_NUM = 8,
} IAV_WARP_PARAMS_LIMIT;

#endif	// __IAV_CONFIG_H__

