/**
 * @file cmd_msg_s2.h
 *
 *
 * History:
 *	2007/09/24 - [Qun Gu] created file.
 *
 * Copyright (C) 2006-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef CMD_MSG_S2_H
#define CMD_MSG_S2_H

#include "amba_sensors.h"
#include "iav_config.h"

/**
 * This API_REVISION string is embedded into the orccode.bin to a predefined location.
 * ARM code also include the same header file. So ARM has the chance to check if both ARM
 * and ucode are using the same API.
 */
#define API_REVISION    "$Rev: 241133 "
#define API_REVISION_U32		(241133)
#define IDSP_REVISION_U32		(241093)

#ifndef DSP_TYPE_DEFINED
typedef char int8_t_dsp;
#ifndef DSP_SMALL_INT
#define int8_t int8_t_dsp
#define DSP_SMALL_INT
#endif

#endif

#ifndef DSP_CMD_SIZE
#define DSP_CMD_SIZE  (32*4) // need to decouple if used by ARM
#endif

#ifndef DSP_MSG_SIZE
#define DSP_MSG_SIZE  (32*4) // need to decouple if used by ARM
#endif

#define	NUM_PIC_TYPES		(3)

// Structure that indicate the vdsp interrupt status
typedef struct vdsp_info_s {
	uint32_t dsp_cmd_rx; // if DSP had read from cmd buf
	uint32_t dsp_msg_tx; // if DSP had written to msg buf
	uint32_t dsp_histogram_tx :1; //if DSP has send at least one histogram to ARM
	uint32_t reserv :31;
	uint32_t padding[5];
} vdsp_info_t;

// Structure that indicate the chip id
typedef enum {
	AMBA_CHIP_ID_S2_UNKNOWN = -1,
	AMBA_CHIP_ID_S2_33 = 0,
	AMBA_CHIP_ID_S2_55 = 1,
	AMBA_CHIP_ID_S2_66 = 2,
	AMBA_CHIP_ID_S2_88 = 3,
	AMBA_CHIP_ID_S2_99 = 4,
	AMBA_CHIP_ID_S2_22 = 5,
	AMBA_CHIP_ID_TOTAL_NUM,
	AMBA_CHIP_ID_FIRST = AMBA_CHIP_ID_S2_33,
	AMBA_CHIP_ID_LAST = AMBA_CHIP_ID_TOTAL_NUM,
} amba_chip_id_t;

////////////////////////////////////////////////////////////////////////////////
// DSP_INIT_DATA, offset in 4B word
typedef struct DSP_INIT_DATAtag {
	uint32_t default_binary_data_addr;
	uint32_t default_binary_data_size;

	/// General purpose cmd/msg channel for DSP-ARM
	uint32_t cmd_data_gen_daddr;
	uint32_t cmd_data_gen_size;
	uint32_t msg_queue_gen_daddr;
	uint32_t msg_queue_gen_size;

	/// Secondary cmd/msg channel for VCAP-ARM
	uint32_t cmd_data_vcap_daddr;
	uint32_t cmd_data_vcap_size;
	uint32_t msg_queue_vcap_daddr;
	uint32_t msg_queue_vcap_size;

	// Third: not used buf still here -qgu
	uint32_t cmd_data_3rd_daddr;
	uint32_t cmd_data_3rd_size;
	uint32_t msg_queue_3rd_daddr;
	uint32_t msg_queue_3rd_size;

	// Fourth: not used buf still here -qgu
	uint32_t cmd_data_4th_daddr;
	uint32_t cmd_data_4th_size;
	uint32_t msg_queue_4th_daddr;
	uint32_t msg_queue_4th_size;

	uint32_t default_config_daddr;
	uint32_t default_config_size;

	uint32_t dsp_buffer_daddr;
	uint32_t dsp_buffer_size;

	uint32_t *vdsp_info_ptr;
	uint32_t *chip_id_ptr;
} DSP_INIT_DATA;

enum {
	DSP_CMD_PORT_GEN = 0,
	DSP_CMD_PORT_VCAP,
	DSP_CMD_PORT_SYNCC,	// 3rd, sync counter
	DSP_CMD_PORT_AUX,
	NUM_DSP_CMD_PORT
};

////////////////////////////////////////////////////////////////////////////////
typedef enum {
	CAT_DSP_HEADER = 0, // specifically for header of cmd/msg comm area
	CAT_DSP_OP_MODE = 1,
	CAT_H264_ENC = 2,
	CAT_H264_DEC = 3,
	CAT_MPEG_ENC = 4,
	CAT_MPEG2_DEC = 5,
	CAT_MPEG4_DEC = 6,
	CAT_VOUT = 7,
	CAT_VCAP = 8,
	CAT_IDSP = 9,
	CAT_POSTP = 10,
	CAT_STILL = 11,
	CAT_UDEC = 14,
	CAT_MEMM = 15,
	CAT_AMB_EXPERIMENTAL = 254,
	CAT_INVALID = 255,
} DSP_CMD_CAT;

/* Command Code */
typedef enum {
	// CAT_DSP_HEADER(0),  this is a special one, must be the first command
	CMD_DSP_HEADER = (CAT_DSP_HEADER << 24) | 0xab,			// 0x000000AB

	// CAT_DSP_OP_MODE(1)
	CMD_DSP_SET_OPERATION_MODE = (CAT_DSP_OP_MODE << 24) | 1, // 0x01000001
	CMD_DSP_ACTIVATE_OPERATION_MODE,						// 0x01000002
	CMD_DSP_SUSPEND_OPERATION_MODE,							// 0x01000003
	CMD_DSP_SET_DEBUG_LEVEL,								// 0x01000004
	CMD_DSP_ARM_MOD_DSP_MEM,								// 0x01000005
	CMD_DSP_SET_DEBUG_THREAD,								// 0x01000006

	// CAT_H264_ENC(2)
	CMD_H264ENC_SETUP = (CAT_H264_ENC << 24) | 1,			// 0x02000001
	CMD_H264ENC_START,										// 0x02000002
	CMD_H264ENC_STOP,										// 0x02000003
	CMD_H264ENC_JPEG_SETUP,									// 0x02000004
	CMD_H264ENC_FADE_SETUP,									// 0x02000005
	CMD_H264ENC_FADE_CMD,									// 0x02000006
	CMD_H264ENC_BITS_FIFO_UPDATE,							// 0x02000007
	CMD_H264ENC_UPDATE_ON_DEMAND_IDR,						// 0x02000008
	CMD_H264ENC_UPDATE_BITRATE_CHANGE,						// 0x02000009
	CMD_H264ENC_UPDATE_GOP_STRUCTURE,						// 0x0200000A
	CMD_H264ENC_MJPEG_CAPTURE,								// 0x0200000B
	CMD_H264ENC_ROTATE,										// 0x0200000C
	CMD_H264ENC_UPDATE_FRMRATE_CHANGE,						// 0x0200000D
	CMD_H264ENC_OSD_INSERT,									// 0x0200000E
	CMD_H264ENC_UPDATE_ENC_PARAMETERS,						// 0x0200000F
	CMD_H264ENC_ENABLE_ENC_STATS,							// 0x02000010
	CMD_H264ENC_BATCH_CMD,									// 0x02000011
	CMD_H264ENC_BATCH_END_CMD,								// 0x02000012

	// CAT_H264_DEC(3)
	CMD_H264DEC_SETUP = (CAT_H264_DEC << 24) | 1,			// 0x03000001
	CMD_H264DEC_DECODE,										// 0x03000002
	CMD_H264DEC_STOP,										// 0x03000003
	CMD_H264DEC_BITSFIFO_UPDATE,							// 0x03000004
	CMD_H264DEC_SPEED,										// 0x03000005
	CMD_H264DEC_TRICKPLAY,									// 0x03000006
	CMD_H264DEC_STAT_ENABLE,								// 0x03000007
	CMD_H264DEC_CAPTURE_VIDEO,								// 0x03000008

	// CAT_MPEG_ENC(4)
	CMD_MPEG2ENC_SETUP = (CAT_MPEG_ENC << 24) | 1,			// 0x04000001
	CMD_MPEG2ENC_START,										// 0x04000002
	CMD_MPEG2ENC_STOP,										// 0x04000003
	CMD_MPEG2ENC_JPEG_SETUP,								// 0x04000004
	CMD_MPEG2ENC_FADE_SETUP,								// 0x04000005
	CMD_MPEG2ENC_FADE_CMD,									// 0x04000006
	CMD_MPEG2ENC_BITS_FIFO_UPDATE,							// 0x04000007

	// CAT_MPEG2_DEC(5)
	CMD_MPEG2DEC_SETUP = (CAT_MPEG2_DEC << 24) | 1,			// 0x05000001
	CMD_MPEG2DEC_DECODE,									// 0x05000002
	CMD_MPEG2DEC_STOP,										// 0x05000003
	CMD_MPEG2DEC_BITS_FIFO_UPDATE,							// 0x05000004
	CMD_MPEG2DEC_PLAYBACK_SPEED,							// 0x05000005
	CMD_MPEG2DEC_TRICKPLAY,									// 0x05000006
	CMD_MPEG2DEC_STAT_ENABLE,								// 0x05000007

	// CAT_MPEG4_DEC(6)
	CMD_MPEG4DEC_SETUP = (CAT_MPEG4_DEC << 24) | 1,			// 0x06000001
	CMD_MPEG4DEC_DECODE,									// 0x06000002
	CMD_MPEG4DEC_STOP,										// 0x06000003
	CMD_MPEG4DEC_VOP,										// 0x06000004
	CMD_MPEG4DEC_SPEED,										// 0x06000005
	CMD_MPEG4DEC_TRICKPLAY,									// 0x06000006

	// CAT_VOUT(7)
	CMD_VOUT_MIXER_SETUP = (CAT_VOUT << 24) | 1,			// 0x07000001
	CMD_VOUT_VIDEO_SETUP,									// 0x07000002
	CMD_VOUT_DEFAULT_IMG_SETUP,								// 0x07000003
	CMD_VOUT_OSD_SETUP,										// 0x07000004
	CMD_VOUT_OSD_BUFFER_SETUP,								// 0x07000005
	CMD_VOUT_OSD_CLUT_SETUP,								// 0x07000006
	CMD_VOUT_DISPLAY_SETUP,									// 0x07000007
	CMD_VOUT_DVE_SETUP,										// 0x07000008
	CMD_VOUT_RESET,											// 0x07000009
	CMD_VOUT_DISPLAY_CSC_SETUP,								// 0x0700000A
	CMD_VOUT_DIGITAL_OUTPUT_MODE_SETUP,						// 0x0700000B
	CMD_VOUT_GAMMA_SETUP,									// 0x0700000C

	// CAT_VCAP(8)
	CMD_VCAP_SETUP = (CAT_VCAP << 24) | 1,					// 0x08000001
	CMD_VCAP_SET_ZOOM,										// 0x08000002
	CMD_VCAP_PREV_SETUP,									// 0x08000003
	CMD_VCAP_MCTF_MV_STAB,									// 0x08000004
	CMD_VCAP_TMR_MODE,										// 0x08000005
	CMD_VCAP_CTRL,											// 0x08000006
	CMD_VCAP_STILL_CAP,										// 0x08000007
	CMD_VCAP_STILL_CAP_IN_REC,								// 0x08000008
	CMD_VCAP_STILL_PROC_MEM,								// 0x08000009
	CMD_VCAP_STILL_CAP_ADV,									// 0x0800000A
	CMD_VCAP_INTERVAL_CAP,								   	// 0x0800000B
	CMD_VCAP_FADE_IN_OUT_SETUP,								// 0x0800000C
	CMD_VCAP_REPEAT_DROP,									// 0x0800000D
	CMD_VCAP_SLOW_SHUTTER,									// 0x0800000E
	CMD_VCAP_AAA_SETUP,										// 0x0800000F
	CMD_VCAP_OSD_BLEND,										// 0x08000010
	CMD_VCAP_LowISO_CFG_SETUP,								// 0x08000011
	CMD_VCAP_BLEND_CONTROL,									// 0x08000012
	CMD_VCAP_MCTF_GMV,										// 0x08000013
	CMD_VCAP_SET_TIMER_INTERVAL,							// 0x08000014
	CMD_VCAP_SET_HDR_PROC_CONTROL,							// 0x08000015
	CMD_STILL_COLLAGE_SETUP,								// 0x08000016
	CMD_STILL_RAW2RAW,										// 0x08000017
	CMD_STILL_RESAMPLE_YUV,									// 0x08000018
	CMD_STILL_JPEG_ENCODE_YUV,								// 0x08000019
	CMD_STILL_SEND_INPUT_DATA,								// 0x0800001A
	CMD_STILL_RAW_FRAME_SUBTRACTION,						// 0x0800001B
	CMD_VCAP_NO_OP,											// 0x0800001C
	CMD_VCAP_UPDATE_CAPTURE_PARAMETERS,						// 0x0800001D
	CMD_VCAP_SET_REGION_WARP_CONTROL,						// 0x0800001E
	CMD_VCAP_SET_PRIVACY_MASK,								// 0x0800001F
	CMD_VCAP_SET_VIDEO_HDR_PROC_CONTROL,					// 0x08000020
	CMD_VCAP_HISO_CONFIG_UPDATE,							// 0x08000021
	CMD_VCAP_ENC_FRM_DRAM_HANDSHAKE,						// 0x08000022	/* FIXME - move to decode */
	CMD_VCAP_UPDATEA_ENC_PARAMETERS,						// 0x08000023

	//obsolete cmds for A7
	CMD_VCAP_FAST_AAA_CAP,									// 0x0800F00A
	CMD_VCAP_TRANSCODE_SETUP,								// 0x0800F00B
	CMD_VCAP_TRANSCODE_CHAN_SW,								// 0x0800F00C
	CMD_VCAP_TRANSCODE_CHAN_STOP,							// 0x0800F00D
	CMD_VCAP_TRANSCODE_PREV_SETUP,							// 0x0800F00E

	// CAT_IDSP(9)

	// -- 0x1001 ~ 0x1FFF   Sensor
	// -- 0x2001 ~ 0x2FFF   Color
	// -- 0x3001 ~ 0x3FFF   Noise
	// -- 0x4001 ~ 0x4FFF   3A Statistics
	// -- 0x5001 ~ 0x5FFF   Miscellaneous / Debug
	// -- 0x6001 ~ 0x6FFF   Debug

	SET_VIN_CAPTURE_WIN = (CAT_IDSP << 24) | 0x1001,// 0x09001001 (0x00002100)
	SENSOR_INPUT_SETUP,								// 0x09001002 (0x00002001)
	AMPLIFIER_LINEARIZATION,						// 0x09001003 (0x00002101)
	PIXEL_SHUFFLE,									// 0x09001004 (0x00002102)
	FIXED_PATTERN_NOISE_CORRECTION,					// 0x09001005 (0x00002106)
	CFA_DOMAIN_LEAKAGE_FILTER,						// 0x09001006 (0x0000200C)
	ANTI_ALIASING,									// 0x09001007 (0x0000211B)
	SET_VIN_CONFIG,									// 0x09001008
	SET_VIN_ROLLOING_SHUTTER_CONFIG,				// 0x09001009
	SET_VIN_CAPTURE_WIN_EXT,						// 0x0900100A
	SET_VIN_PIP_CAPTURE_WIN,						// 0x0900100B
	SET_VIN_PIP_CAPTURE_WIN_EXT,					// 0x0900100C
	SET_VIN_PIP_CONFIG,								// 0x0900100D

	BLACK_LEVEL_GLOBAL_OFFSET = (CAT_IDSP << 24) | 0x2001,// 0x09002001 (0x0000211D)
	BLACK_LEVEL_CORRECTION_CONFIG,					// 0x09002002 (0x00002103)
	BLACK_LEVEL_STATE_TABLES,						// 0x09002003 (0x00002104)
	BLACK_LEVEL_DETECTION_WINDOW,					// 0x09002004 (0x00002105)
	RGB_GAIN_ADJUSTMENT,							// 0x09002005 (0x00002002)
	DIGITAL_GAIN_SATURATION_LEVEL,					// 0x09002006 (0x00002108)
	VIGNETTE_COMPENSATION,							// 0x09002007 (0x00002003)
	LOCAL_EXPOSURE,									// 0x09002008 (0x00002109)
	COLOR_CORRECTION,								// 0x09002009 (0x0000210C)
	RGB_TO_YUV_SETUP,								// 0x0900200A (0x00002007)
	CHROMA_SCALE,									// 0x0900200B (0x0000210E)

	BAD_PIXEL_CORRECT_SETUP = (CAT_IDSP << 24) | 0x3001,// 0x09003001 (0x0000200A)
	CFA_NOISE_FILTER,				// 0x09003002 (0x00002107) new design for A7
	DEMOASIC_FILTER,				// 0x09003003 (0x0000210A)
	CHROMA_NOISE_FILTER,			// 0x09003004 (0x0000210B) ported from A7L
	//RGB_NOISE_FILTER,				// 0x09003004 (0x0000210B) remove from A7
	//RGB_DIRECTIONAL_FILTER,		// 0x09003005 (0x0000211E) remove from A7
	STRONG_GRGB_MISMATCH_FILTER,	// 0x09003005
	CHROMA_MEDIAN_FILTER,			// 0x09003006 (0x0000210D)
	LUMA_SHARPENING,				// 0x09003007 (0x0000210F)
	LUMA_SHARPENING_LINEARIZATION,	// 0x09003008 (0x00002120) remove from A7
	LUMA_SHARPENING_FIR_CONFIG,		// 0x09003009 (0x00002121)
	LUMA_SHARPENING_LNL,			// 0x0900300A (0x00002122)
	LUMA_SHARPENING_TONE,			// 0x0900300B (0x00002123)
	LUMA_SHARPENING_BLEND_CONTROL,	// 0x0900300C (0x00002131)
	LUMA_SHARPENING_LEVEL_CONTROL,	// 0x0900300D (0x00002132)

	AAA_STATISTICS_SETUP = (CAT_IDSP << 24) | 0x4001,// 0x09004001 (0x00002004)
	AAA_PSEUDO_Y_SETUP,								// 0x09004002 (0x00002112)
	AAA_HISTORGRAM_SETUP,							// 0x09004003 (0x00002113)
	AAA_STATISTICS_SETUP1,							// 0x09004004 (0x00002110)
	AAA_STATISTICS_SETUP2,							// 0x09004005 (0x00002111)
	AAA_STATISTICS_SETUP3,							// 0x09004006 (0x00002118)
	AAA_EARLY_WB_GAIN,								// 0x09004007 (0x00002134) A5, A6, A5M, A7
	AAA_FLOATING_TILE_CONFIG,								// 0x09004008

	NOISE_FILTER_SETUP = (CAT_IDSP << 24) | 0x5001,	// 0x09005001 (0x00002009)
	RAW_COMPRESSION,								// 0x09005002 (0x00002114)
	RAW_DECOMPRESSION,								// 0x09005003 (0x00002115)
	ROLLING_SHUTTER_COMPENSATION,					// 0x09005004 (0x00002116) new design for A7
	FPN_CALIBRATION,								// 0x09005005 (0x0000211C)
	HDR_MIXER,										// 0x09005006 (0x0000211F)
	EARLY_WB_GAIN,									// 0x09005007 (0x0000212A) A5, A6, A7
	SET_WARP_CONTROL,								// 0x09005008 (0x00002129) A5M
	CHROMATIC_ABERRATION_WARP_CONTROL,						// 0x09005009
	RESAMPLER_COEFF_ADJUST,									// 0x0900500A

	DUMP_IDSP_CONFIG = (CAT_IDSP << 24) | 0x6001,	// 0x09006001 (0x0000ff07 AMB_DSP_DEBUG_2)
	SEND_IDSP_DEBUG_CMD,							// 0x09006002 (0x0000ff08 AMB_DSP_DEBUG_3)
	UPDATE_IDSP_CONFIG,								// 0x09006003 (0x0000ff09)
	PROCESS_IDSP_BATCH_CMD,							// 0x09006004
	RAW_ENCODE_VIDEO_SETUP_CMD,                                 // 0x09006005

	// CAT_POSTP(10)
	CMD_POSTPROC = (CAT_POSTP << 24) | 1,					// 0x0A000001
	CMD_POSTP_YUV2YUV,										// 0x0A000002
	CMD_DECPP_CONFIG,										// 0x0A000003
	CMD_DECPP_INPIC,										// 0x0A000004
	CMD_DECPP_FLUSH,										// 0x0A000005
	CMD_POSTP_INPIC,										// 0x0A000006
	CMD_POSTP_SPD_ADJUST,									// 0x0A000007
	CMD_POSTPROC_UDEC,										// 0x0A000008
	CMD_POSTP_SET_AUDIO_CLK_OFFSET,							// 0x0A000009
	CMD_POSTP_WAKE_VOUT,									// 0x0A00000A
	CMD_POSTP_RESERVE,										// 0x0A00000B
	CMD_DEINT_INIT,											// 0x0A00000C
	CMD_DEINT_CONF,											// 0x0A00000D
	CMD_POSTP_VIDEO_FADING,									// 0x0A00000E
	CMD_DECPP_CREATE,										// 0x0A00000F
	CMD_POSTP_SET_FIRST_PTS,								// 0x0A000010

	// CAT_STILL_DEC(11)
	CMD_STILL_DEC_SETUP = (CAT_STILL << 24) | 1,			// 0x0B000001
	//CMD_STILL_DEC_START,									// 0x0B000002
	CMD_STILL_DEC_DECODE,									// 0x0B000002
	CMD_STILL_DEC_STOP,										// 0x0B000003
	//CMD_STILL_DEC_UPDWPTR,								// 0x0B000004
	CMD_STILL_MULTIS_SETUP,									// 0x0B000004
	CMD_STILL_MULTIS_DECODE,								// 0x0B000005
	CMD_STILL_FREEZE,										// 0x0B000006
	CMD_STILL_CAPTURE_STILL,								// 0x0B000007

	/////////////////////////////////////////////////
	// UDEC Commands
	/////////////////////////////////////////////////
	// CAT_UDEC(14)
	CMD_UDEC_INIT = (CAT_UDEC << 24) | 1,					// 0x0E000001
	CMD_UDEC_SETUP,											// 0x0E000002
	CMD_UDEC_DECODE,										// 0x0E000003
	CMD_UDEC_STOP,											// 0x0E000004
	CMD_UDEC_EXIT,											// 0x0E000005
	CMD_UDEC_TRICKPLAY,										// 0x0E000006
	CMD_UDEC_PLAYBACK_SPEED,								// 0x0E000007
	CMD_UDEC_CREATE,										// 0x0E000008

	//////////////////////////////////////////////////////////////
	// CAT_MEMM(15)
	// Paired commands
	CMD_MEMM_QUERY_DSP_SPACE_SIZE = (CAT_MEMM << 24) | 1,	// 0x0F000001
	CMD_MEMM_SET_DSP_DRAM_SPACE,							// 0x0F000002
	CMD_MEMM_RESET_DSP_DRAM_SPACE,						// 0x0F000003
	//CMD_MEMM_RESERVED_1 = (CAT_MEMM << 24) | 1,				// 0x0F000001
	//CMD_MEMM_RESERVED_2,									// 0x0F000002
	//CMD_MEMM_RESERVED_3,									// 0x0F000003
	CMD_MEMM_CREATE_FRM_BUF_POOL,							// 0x0F000004
	CMD_MEMM_UPDATE_FRM_BUF_POOL_CONFG,						// 0x0F000005
	CMD_MEMM_REQ_FRM_BUF,									// 0x0F000006
	CMD_MEMM_REQ_RING_BUF,									// 0x0F000007
	CMD_MEMM_CREATE_THUMBNAIL_BUF_POOL,						// 0x0F000008
	CMD_MEMM_GET_FRM_BUF_INFO,								// 0x0F000009
	CMD_MEMM_GET_FREE_FRM_BUF_NUM,							// 0x0F00000A
	CMD_MEMM_GET_FREE_SPACE_SIZE,							// 0x0F00000B
	CMD_MEMM_CONFIG_FRM_BUF_POOL,							// 0x0F00000C
	CMD_MEMM_GET_FRM_BUF_POOL_INFO,							// 0x0F00000D

	// Alone commands
	CMD_MEMM_REL_FRM_BUF = (CAT_MEMM << 24) | (0xF << 20) | 1,	// 0x0FF00001
	CMD_MEMM_REG_FRM_BUF,									// 0x0FF00002

	// for experimental and backward compatible purpose
	AMB_MAIN_RESAMPLER_BANDWIDTH = (CAT_AMB_EXPERIMENTAL << 24) | 1,
	AMB_DSP_DEBUG_2 = DUMP_IDSP_CONFIG,
	AMB_DSP_DEBUG_3 = SEND_IDSP_DEBUG_CMD,
	AMB_UPDATE_IDSP_CONFIG = UPDATE_IDSP_CONFIG,
} DSP_CMD_CODE;

#define GET_DSP_CMD_CAT(x)	 (((x)>>24)&0x7f)

/* Message Code */
typedef enum {
	MSG_DSP_STATUS = (1 << 31) | (CAT_DSP_OP_MODE << 24) | 1,	// 0x81000001

	MSG_H264ENC_STATUS = (1 << 31) | (CAT_H264_ENC << 24) | 1,	// 0x82000001
	MSG_H264BUF_STATUS = (1 << 31) | (CAT_H264_ENC << 24) | 2,	// 0x82000002
	MSG_420T422_STATUS = (1 << 31) | (CAT_H264_ENC << 24) | 3,	// 0x82000003
	MSG_H264DEC_STATUS = (1 << 31) | (CAT_H264_DEC << 24) | 1,	// 0x83000001
	MSG_H264DEC_PICINFO = (1 << 31) | (CAT_H264_DEC << 24) | 2,	// 0x83000002

	MSG_MPEGENC_STATUS = (1 << 31) | (CAT_MPEG_ENC << 24) | 1,	// 0x84000001
	MSG_MP2DEC_STATUS = (1 << 31) | (CAT_MPEG2_DEC << 24) | 1,	// 0x85000001
	MSG_MP2DEC_PICINFO = (1 << 31) | (CAT_MPEG2_DEC << 24) | 2,	// 0x85000002

	MSG_MP4DEC_STATUS = (1 << 31) | (CAT_MPEG4_DEC << 24) | 1,	// 0x86000001

	MSG_VCAP_STATUS = (1 << 31) | (CAT_VCAP << 24) | 1,			// 0x88000001
	MSG_SCAP_STATUS = (1 << 31) | (CAT_VCAP << 24) | 2,			// 0x88000002, StillCapture(Encode) status
	MSG_VCAP_STATUS_EXT = (1 << 31) | (CAT_VCAP << 24) | 3,		// 0x88000003, VCAP status extension

	MSG_STILL_STATUS = (1 << 31) | (CAT_STILL << 24) | 1,		// 0x8B000001

	/////////////////////////////////////////////////
	//UDEC Msgs
	/////////////////////////////////////////////////
	MSG_UDEC_STATUS = (1 << 31) | (CAT_UDEC << 24) | 1,		// 0x8E000001
	MSG_UDEC_OUTPIC,										// 0x8E000002
	MSG_UDEC_FIFO_STATUS,									// 0x8E000003
	MSG_UDEC_DISP_STATUS,									// 0x8E000004
	MSG_UDEC_EOS_STATUS,									// 0x8E000005
	MSG_UDEC_ABORT_STATUS,									// 0x8E000006
	MSG_UDEC_CREATE,										// 0x8E000007
	MSG_DECPP_CREATE,										// 0x8E000008

	/////////////////////////////////////////////////
	//DSP Buffer Management Msgs
	/////////////////////////////////////////////////
	MSG_MEMM_QUERY_DSP_SPACE_SIZE = (1<<31) | (CAT_MEMM<<24) | 1, //0x8F000001
	MSG_MEMM_SET_DSP_DRAM_SPACE,
	MSG_MEMM_RESET_DSP_DRAM_SPACE,
	//MSG_MEMM_RESERVED_1 = (1<<31) | (CAT_MEMM<<24) | 1, //0x8F000001
	//MSG_MEMM_RESERVED_2,
	//MSG_MEMM_RESERVED_3,
	MSG_MEMM_CREATE_FRM_BUF_POOL,
	MSG_MEMM_UPDATE_FRM_BUF_POOL_CONFIG,
	MSG_MEMM_REQ_FRM_BUF,
	MSG_MEMM_REQ_RING_BUF,
	MSG_MEMM_CREATE_THUMBNAIL_BUF_POOL,
	MSG_MEMM_GET_FRM_BUF_INFO,
	MSG_MEMM_GET_FREE_FRM_BUF_NUM,
	MSG_MEMM_GET_FREE_SPACE_SIZE,
	MSG_MEMM_CONFIG_FRM_BUF_POOL,
	MSG_MEMM_GET_FRM_BUF_POOL_INFO,
	/////////////////////////////////////////////////

} DSP_MSG_CODE;

/* General definitions */
typedef struct DSP_CMDtag {
	uint32_t cmd_code;
	uint32_t palyload[DSP_CMD_SIZE / 4 - 1];
} DSP_CMD;

typedef struct DSP_MSGtag {
	uint32_t msg_code;
	uint32_t palyload[DSP_MSG_SIZE / 4 - 1];
} DSP_MSG;

/**
 * Stream Type definition used in command/message
 */
typedef enum STREAM_TPtag {
	STRM_TP_INVALID = 0,

	/* Decoder Stream Type */
	STRM_TP_HDEC_COMP = 0x01,
	STRM_TP_HDEC_LITE = 0x02,
	STRM_TP_MP2DEC = 0x03,
	STRM_TP_MP1DEC = 0x04,
	STRM_TP_MP4DEC = 0x05,
	STRM_TP_WMVDEC = 0x06,
	STRM_TP_JPGDEC = 0x07,

	/* Video Encoder Stream Type */
	STRM_TP_ENC_FULL_RES = 0x11,
	STRM_TP_ENC_LOW_RES = 0x12,

	STRM_TP_ENC_0 = 0x11,
	STRM_TP_ENC_1 = 0x12,
	STRM_TP_ENC_2 = 0x13,
	STRM_TP_ENC_3 = 0x14,
	STRM_TP_ENC_4 = 0x15,
	STRM_TP_ENC_5 = 0x16,
	STRM_TP_ENC_6 = 0x17,
	STRM_TP_ENC_7 = 0x18,

	STRM_TP_ENC_8 = 0x19,
	STRM_TP_ENC_9 = 0x1A,
	STRM_TP_ENC_10 = 0x1B,
	STRM_TP_ENC_11 = 0x1C,
	STRM_TP_ENC_12 = 0x1D,
	STRM_TP_ENC_13 = 0x1E,
	STRM_TP_ENC_14 = 0x1F,
	STRM_TP_ENC_15 = 0x20,
	STRM_TP_ENC_LAST,
	STRM_TP_ENC_FIRST = STRM_TP_ENC_0,
	STRM_TP_ENC_TOTAL_NUM = STRM_TP_ENC_LAST - STRM_TP_ENC_FIRST,

	/* Still Encoder Stream Type */
	STRM_TP_ENC_STILL_JPEG = 0x21,
	STRM_TP_ENC_STILL_JPEG_THM_1 = 0x22,
	STRM_TP_ENC_STILL_JPEG_THM_2 = 0x23,
	STRM_TP_ENC_STILL_420_TO_JPEG = 0x24,
	STRM_TP_ENC_STILL_422_TO_JPEG = 0x25,
	STRM_TP_ENC_STILL_420_TO_COLLAGE = 0x26,
	STRM_TP_ENC_STILL_MJPEG = 0x27,
	STRM_TP_ENC_STILL_EOS = 0x28,

	STRM_TP_VCAP_CFA_AAA = 0x31,
	STRM_TP_VCAP_YUV_AAA = 0x32,

	STRM_TP_PREV_A = 0x41,
	STRM_TP_PREV_B = 0x42,

	STRM_TP_ALL_STREAMS = 0xFF
} STREAM_TP;

////////////////////////////////////////////////////////////////////////////////
// DSP 'header command', must be the first for each iteration
typedef struct DSP_HEADER_CMDtag {
	uint32_t cmd_code;
	uint32_t cmd_seq_num;
	uint32_t num_cmds;
} DSP_HEADER_CMD;

/************************************************************
 * DSP operation mode command and messages (Category 1)
 */
typedef enum {
	DSP_OP_MODE_IDLE = 0,
	DSP_OP_MODE_CAMERA_RECORD = 1,
	DSP_OP_MODE_CAMERA_PLAYBACK = 2,
	DSP_OP_MODE_COMP_PLAYBACK = 3,  // compliant decoder
	DSP_OP_MODE_MPEG_RECORD = 4,
	DSP_OP_MODE_MPEG2_PLAYBACK = 5,
	DSP_OP_MODE_MPEG4_PLAYBACK = 6,
	DSP_OP_MODE_TRANSCODING = 7,
	DSP_OP_MODE_MPEG4_HW_PLAYBACK = 8,
	DSP_OP_MODE_VC1_PLAYBACK = 9,
	DSP_OP_MODE_RV_PLAYBACK = 10,
	DSP_OP_MODE_STILL_PLAYBACK = 11,
	DSP_OP_MODE_YUV_RECORD = 12,
	DSP_OP_MODE_IP_CAMERA_RECORD = 13,
	DSP_OP_MODE_UDEC	= 14,
	DSP_OP_MODE_DUPLEX_LDELAY = 17,
	DSP_OP_MODE_2_IDLE = 254, // transition state to IDLE
	DSP_OP_MODE_INIT = 255, // before DSP fully invoked, only happen once at the very beginning.
} DSP_OP_MODE;

/// cmd code 0x01000001
typedef struct DSP_SET_OP_MODE_CMDtag {
	uint32_t cmd_code;
	uint8_t dsp_op_mode;
} DSP_SET_OP_MODE_CMD;

#define ENCFLAGS_MCTF_CFG   0x1
typedef struct DSP_ENC_CHAN_CFGtag {
	uint32_t enc_type_full_res :4; // 2: H.264, 4: MPEG2
	uint32_t enc_type_pip :4; // 0: no PIP, 2: H.264, 4: MPEG2
	uint32_t enc_frm_rate :8; // 0: 29.97, 1: 59.94; 30, 60, 24, or 15
	uint32_t enc_flags :8;
	// bit 0 - MCTF configured (1), no MCTF (0)
	// other bits are reserved
	uint32_t enc_type_piv :1; // 0: no PIV, 1: PIV
	uint32_t enc_rsvd :7;
	uint32_t enc_width_full_res :16;
	uint32_t enc_height_full_res :16;
	uint32_t enc_width_pip :16;
	uint32_t enc_height_pip :16;
} DSP_ENC_CHAN_CFG;

typedef struct DSP_DEC_CHAN_CFGtag {
	uint32_t dec_type :8; // 3: H.264, 5: MPEG2
	uint32_t dec_frm_rate :8; // 0: 29.97, 1: 59.94; 30, 60, 24, or 15
	uint32_t dec_rsvd :16;
	uint32_t dec_width :16;
	uint32_t dec_height :16;
} DSP_DEC_CHAN_CFG;

typedef struct DSP_SET_OP_MODE_RECORD_CMDtag {
	uint32_t cmd_code;	   // must be 0x01000001
	uint8_t dsp_op_mode;// must be 1 or 4, we may retire op_mode=4 in the future
	uint8_t mode_flags;
	// bit 0 - YUV422 domain(0), RGB domain(1)
	// bit 1 - VIN mux configured(1), no mux (0)
	// other bits are reserved
	uint8_t num_of_chans;   // number of encode output channels
	                        // valid encoder channel IDs: 0 ~ num_of_chans-1
	                        // for now only VIN mux can generate more than 1 channel
	uint8_t rsvd_2;

	/// encode channels configuration 0-7
	DSP_ENC_CHAN_CFG enc_chan_cfgs[8];
} DSP_SET_OP_MODE_RECORD_CMD;

typedef struct DSP_SET_OP_MODE_DEC_CMDtag {
	uint32_t cmd_code;	   // must be 0x01000001
	uint8_t dsp_op_mode;	// must be 2, 3, 5 or 6
	uint8_t mode_flags;	 // currently not used
	uint8_t num_of_instances;   // number of decode instances
	uint8_t rsvd_1;
} DSP_SET_OP_MODE_DEC_CMD;

typedef struct DSP_SET_OP_MODE_XCODE_CMDtag {
	uint32_t cmd_code;	   // must be 0x01000001
	uint8_t dsp_op_mode;	// must be 7
	uint8_t sub_mode;	   // 0: fixed xcode, 1: dynamic xcode
	uint8_t num_of_chans;   // number of encode output channels
	                        // also the number of decoders in fixed xcode mode
	uint8_t rsvd_1;

	/// xcode channels configuration 0-3
	struct {
		DSP_ENC_CHAN_CFG enc;
		DSP_DEC_CHAN_CFG dec;
	} chan_cfgs[4];

	uint8_t num_vin_mux_chan;   // 0: no VIN input channel
	                            // 1: single VIN channel
	                            // >1 number of muxed VIN channels
} DSP_SET_OP_MODE_XCODE_CMD;

typedef struct DSP_ENC_CFGtag {
	uint16_t max_enc_width;
	uint16_t max_enc_height;
	uint32_t max_GOP_M :8; /* max value of M in the GOP */
	uint32_t vert_search_range_2x :1; /* 2x vertical search range for one ref (only if SMEM is available) */
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t max_num_ref_p : 2 ;
	uint32_t reserved :21;
#else
	uint32_t reserved :23;
#endif
} DSP_ENC_CFG;

typedef enum {
	IPCAM_RECORD_MAX_NUM_ENC = 8,		/* support up to 8 encoders */
	IPCAM_RECORD_MAX_NUM_ENC_EXT = 8,	/* support up to 8 encoders */
	IPCAM_RECORD_MAX_NUM_ENC_ALL = (IPCAM_RECORD_MAX_NUM_ENC + IPCAM_RECORD_MAX_NUM_ENC_EXT),

	IPCAM_RECORD_MAX_NUM_VIN = 4,		/* support up to 4 VINs */
} IPCAM_RECORD_MAX_NUM;

/// cmd code 0x01000001
typedef struct DSP_SET_OP_MODE_IPCAM_RECORD_CMDtag {
	uint32_t cmd_code;			// must be 0x01000001
	uint32_t dsp_op_mode : 8; /* must be DSP_OP_MODE_IP_CAMERA_RECORD */
	uint32_t mode_flags : 8; /* determines the IDSP pipeline to run */
	uint32_t max_num_enc : 4; /* max number of encoders - must be <= IPCAM_RECORD_MAX_NUM_ENC */
	uint32_t preview_A_for_enc : 1; /* 0 - preview A is allocated in 4:2:2 for VOUTA, 1 - preview A is allocated in 4:2:0 + ME1 for encoding */
	uint32_t preview_B_for_enc : 1; /* 0 - preview B is allocated in 4:2:2 for VOUTB, 1 - preview B is allocated in 4:2:0 + ME1 for encoding */
	uint32_t enc_rotation : 1;
	uint32_t hdr_num_exposures_minus_1 : 2; /* number of HDR exposures-1.  Applicable only if mode_flags = 4 ot 5 */
	uint32_t raw_compression_disabled : 1; /* disable raw compression for certain pipelines. It should be 0 by default to minimize DRAM traffic */
	uint32_t num_vin_minus_2 : 2; /* number of VINs-2 - Applicable only if mode_flags = 8 */
	uint32_t max_num_enc_msb : 1;
	uint32_t reserved : 3;
	uint16_t max_main_width;
	uint16_t max_main_height;
	uint16_t max_preview_A_width;
	uint16_t max_preview_A_height;
	uint16_t max_preview_B_width;
	uint16_t max_preview_B_height;
	uint16_t max_preview_C_width;
	uint16_t max_preview_C_height;
	DSP_ENC_CFG enc_cfg[IPCAM_RECORD_MAX_NUM_ENC];
	uint16_t max_warped_main_width;
	uint16_t max_warped_main_height;
	uint16_t max_warped_region_input_width;
	uint16_t max_vin_stats_num_lines_top : 4;
	uint16_t max_vin_stats_num_lines_bot : 4;
	uint16_t max_chroma_filter_radius : 2; /* 0 = 32, 1 = 64, 2 = 128 */
	int16_t extra_dram_buf : 4;	/* The valid range is from -3 to 3 for now. */
	uint16_t h_warp_bypass : 1;
	uint16_t prev_A_extra_line_at_top : 1;
	uint16_t max_warped_region_output_width;
	uint8_t prev_B_extra_line_at_top;
	uint8_t prev_B_extra_line_at_bot;
	uint32_t set_op_mode_ext_daddr;
	int16_t extra_dram_buf_prev_a : 4;
	int16_t extra_dram_buf_prev_b : 4;
	int16_t extra_dram_buf_prev_c : 4;
	int16_t extra_buf_msb_ext : 4;  // Note: the MSB of extra buffer will enable when extra_dram_buf > 0;
	int16_t extra_buf_msb_ext_prev_a : 4;
	int16_t extra_buf_msb_ext_prev_b : 4;
	int16_t extra_buf_msb_ext_prev_c : 4;
	int16_t reserved3 : 4;
} DSP_SET_OP_MODE_IPCAM_RECORD_CMD;

typedef struct DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXTtag {
	/* There will be actually 16 instances MAX, and they are paged in
	 * one instance at a time in ucode
	 */
	DSP_ENC_CFG enc_cfg[IPCAM_RECORD_MAX_NUM_ENC_EXT];
} DSP_SET_OP_MODE_IPCAM_RECORD_CMD_EXT;

/**
 * UDEC Related
 */
// Definitions of mode_flags
// bit0~1
#define UDEC_MODE_PP_NONE       0
#define UDEC_MODE_DECPP_ENABLED 1
#define UDEC_MODE_VOUTPP_ENABLE 2
#define UDEC_MODE_MULTI_WINDOW_ENABLE 3

// bit2
#define UDEC_MODE_DEINT_ENABLED (1<<2)
// bit3
#define UDEC_MODE_OSD_ENABLED   (1<<3)
// bit4
#define UDEC_MODE_WARP_ENABLED  (1<<4)

typedef struct DSP_SET_OP_MODE_UDEC_CMDtag {
	uint32_t cmd_code;       // must be 0x01000001

	uint8_t dsp_op_mode;    // must be 14
	uint8_t mode_flags; // bit0~1: enable PP; 0: no PP; 1- DECPP(to DRAM); 2 - VOUTPP (to VOUT); bit2: enable deinterlacer; bit3: enable OSD, bit4: enable horizontal dewarp;
	uint8_t num_of_instances;   // number of decode instances
	uint8_t pp_chroma_fmt_max;

	uint16_t pp_max_frm_width;
	uint16_t pp_max_frm_height;
	uint16_t pp_max_frm_num;

	uint8_t pp_background_Y; // used in ppmode=3 to designate the background color
	uint8_t pp_background_Cb;

	uint8_t pp_background_Cr;

	uint8_t dual_vout;
} DSP_SET_OP_MODE_UDEC_CMD;

/// cmd code 0x01000002
typedef struct DSP_ACTIVATE_OP_MODE_CMDtag {
	uint32_t cmd_code;
} DSP_ACTIVATE_OP_MODE_CMD;

/// cmd code 0x01000003
typedef struct DSP_SUSPEND_OP_MODE_CMDtag {
	uint32_t cmd_code;
} DSP_SUSPEND_OP_MODE_CMD;

typedef enum {
	DEBUG_COMMON_MODULE = 0,
	DEBUG_VCAP_MODULE = 1,
	DEBUG_VOUT_MODULE = 2,
	DEBUG_VENC_MODULE = 3,
	DEBUG_VDEC_MODULE = 4,
	DEBUG_IDSP_MODULE = 5,
	DEBUG_MEMD_MODULE = 6,
	DEBUG_PERFORM_MODULE = 7,
	DEBUG_MODULE_NUM,
	DEBUG_MODULE_FIRST = 0,
	DEBUG_MODULE_LAST = DEBUG_MODULE_NUM,
	DEBUG_ALL_MODULE = 255,
} DSP_DEBUG_MODULE;

typedef enum {
	DEBUG_SET_MASK = 0,
	DEBUG_ADD_MASK = 1,
} DSP_DEBUG_ACTION_MASK;

/// cmd code 0x01000004
typedef struct DSP_SET_DEBUG_LEVEL_CMDtag {
	uint32_t cmd_code;
	uint8_t module;
	uint8_t add_or_set;
	uint16_t reserved;
	uint32_t debug_mask;
} DSP_SET_DEBUG_LEVEL_CMD;

// cmd code 0x01000005
typedef struct DSP_ARM_MOD_DSP_MEM_CMDtag {
	uint32_t cmd_code;
	uint32_t dram_size;
} DSP_ARM_MOD_DSP_MEM_CMD;

// cmd code 0x01000006
typedef struct DSP_SET_DEBUG_THREAD_CMDtag {
	uint32_t cmd_code;
	uint8_t thread_mask;
	uint8_t reserved1;
	uint16_t reserved2;
} DSP_SET_DEBUG_THREAD_CMD;

// This one must be the first message, and NOT supposed to be written explicity by DSP
typedef struct DSP_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t dsp_mode;
	uint32_t msg_seq_num; //old name is time_code, changed to msg_seq_num for future use
	uint32_t prev_cmd_seq;
	uint32_t prev_cmd_echo;
	uint32_t prev_num_cmds;
	uint32_t num_msgs;
} DSP_STATUS_MSG;

/************************************************************
 * H.264 encoder command and messages (Category 2)
 * MPEG2 encoder command and messages (Category 4)
 */

/// bitstream info, used by ENCODER_SETUP_CMD->info_fifo_base
typedef struct {
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;

	uint32_t frmNo;			  //Frame Number  32 bits
	uint64_t pts_64;			 //PTS counter	   64 bits
	uint32_t start_addr;		 //Start Address	 32 bits

	uint32_t frameTy :3;//frame type, use FRM_TYPE. It is always 0 for Audio bitstream. 0-don't care
	uint32_t levelIDC :3;//level in HierB. ex, for M=4, I0/P4 is L0, B2 is L1, B1/B3 is L2.
	uint32_t refIDC :1;	//the picture is used as reference or not.
	uint32_t picStruct :1;	//0: frame, 1: field
	uint32_t length :24;	//Picture Size  24 bits

	uint32_t top_field_first :1;
	uint32_t repeat_first_field :1;
	uint32_t progressive_sequence :1;
	uint32_t pts_minus_dts_in_pic :5;//(pts-dts) in frame or field unit. (if progressive_sequence=0, in field unit. If progressive_sequence=1, in frame unit.)
	uint32_t jpeg_qlevel :8;
	uint32_t addr_offset_sliceheader :16;	//slice header address offset in

	int32_t cpb_fullness;//cpb fullness with considering real cabac/cavlc bits. use sign int, such that negative value indicate cpb underflow.
	uint32_t reserved2[8];	   //reserved to form 64B
} BIT_STREAM_HDR;   //64B

/// partitional bitstream info, used by ENCODER_SETUP_CMD->bits_partition_base
typedef struct {
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;

	uint32_t frmNo;			  //Frame Number 32 bits
	uint64_t pts_64;			 //PTS counter 64 bits
	uint32_t start_addr;		 //Start Address 32 bits

	uint32_t frameTy :3;//FRM_TYPE. It is always 0 for Audio bitstream. 0-don't care
	uint32_t levelIDC :3;	//level in HierB.
	                        //ex, for M=4, I0/P4 is L0, B2 is L1, B1/B3 is L2.
	uint32_t refIDC :1;	//the picture is used as reference or not.
	uint32_t picStruct :1;	//0: frame, 1: field
	uint32_t length :24;	//Picture Size  24 bits

	uint32_t partition_count :16;
	uint32_t partition_index :16;

	uint32_t reserved;		   // used to make it 32B aligned
} BITS_PARTITION_MSG;   //32B

/// encoder statistic, used by ENCODER_SETUP_CMD->stat_fifo_base
typedef struct {
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;
	uint32_t frmNo;				 // counter
	uint64_t pts_64;				// PTS counter 64 bits
	uint32_t mv_start_addr;	// ref0 16x16 motion vectors start address in DRAM
	uint32_t qp_hist_daddr;
	uint32_t reserved_1[10];		// used to make it 64B aligned */
} ENCODER_STATISTIC;

typedef enum FRM_TYPEtag {
	IDR_I = 1,
	I_PIC_T = 2,
	P_PIC_T = 3,
	B_PIC_T = 4,
	JPG_T = 5,
	JPG_THN_T = 6,
	JPG_THN2_T = 7,
} FRM_TYPE;

/// H264/JPEG/MPEG2 encoder setup (cmd code 0x02000001)
typedef struct ENCODER_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t profile_idc :8;	//profile
	uint32_t level_idc :8;	//level
	uint32_t coding_type :8;	// use CODING_TYPE
	uint32_t scalelist_opt :4;//(h.264 only) 0: disable scaling list. 1: use default scaling list. X>=2: use customized scaling list No.(X-1)
	uint32_t force_intlc_tb_iframe :4;
	uint32_t enc_src :3;// specifies the source image from vcap to encode from
	uint32_t num_mbrows_per_bitspart :8;
	uint32_t chroma_format :1;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t N_div_256 :4;
#else
	uint32_t reserved_0 :4;
#endif
	uint32_t encode_w_sz :16;	//the mage size to be encoded. such as 720x480
	uint32_t encode_h_sz :16;
	uint32_t encode_w_ofs :16;//the offset between input image and real encode image.
	uint32_t encode_h_ofs :16;
	uint32_t aff_mode :8;	 // use PAFF_MODE
	uint32_t M :8;
	uint32_t N :8;
	uint32_t gop_structure :8;
	uint32_t numRef_P :8;	 //0: max. 1: 1fwd. >=2: 2fwd
	uint32_t numRef_B :8;//0/1: max. 2: 1fwd+1bwd. 3: 2fwd+1bwd. >=4: 2fwd+2bwd.
	uint32_t use_cabac :2;	 //0: cavlc, 1: cabac
	uint32_t quality_level :14;

	uint32_t average_bitrate;			   //average bitrate in bps (not kbps!!)
	uint32_t vbr_cntl;
	uint32_t vbr_setting :8;//rate control. 0: disable with constant qp=30, 1: enable for cbr real case, 2: enable for testing cbr with fixed cabac_delay. 3: vbr.  52-103 constant qp with qp=vbr_setting-52. 4: SCBR
	uint32_t allow_I_adv :8;
	uint32_t cpb_buf_idc :8;
	uint32_t en_panic_rc :2;
	uint32_t cpb_cmp_idc :2;	// cpb compliance idc
	uint32_t fast_rc_idc :4;
	uint32_t target_storage_space;
	uint32_t bits_fifo_base;
	uint32_t bits_fifo_limit;
	uint32_t info_fifo_base;//bits_info_xxxx is put in 32 byte align boundary, so we need another info_fifo to tell the exact bitstream start/stop address. refer to struct BIT_STREAM_HDR
	uint32_t info_fifo_limit;
	uint32_t audio_in_freq :8;
	uint32_t encode_frame_rate :16;	// encoder can use this as frame rate that used in RC, change name to "encode_frame_rate" according to DSP doc
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t long_p_interval :6;     // fast seek interval when gop structure is long term ref GOP */
	uint32_t reserved1 :2;
#else
	uint32_t frame_sync :8;	 // useless, can be removed.
#endif
	uint32_t initial_fade_in_gain :16;// when start encode, the fade in duration (from black to real video). Actually fade is the work of PP, so encoder can only forward this message to VCAP. It is put here just because it related to encoder start/stop, and is not used wheren preview only.
	uint32_t final_fade_out_gain :16;// when stop encode, the fade in duration (from real video to black)
	uint32_t idr_interval;
	uint32_t cpb_user_size;	//(bps, just follow the unit of average_bitrate)

	uint32_t stat_fifo_base;	//refer to struct ENCODER_STATISTIC
	uint32_t stat_fifo_limit;

	uint32_t follow_gop :8;
	uint32_t fgop_max_M :8;	//max possible M when (follow_gop || gop_change_on_the_fly)
	uint32_t fgop_max_N :8;	//0: ignore fgop_max_N. >0: If current is P && "distance to previous I" >= fgop_max_N, coded as I
	uint32_t fgop_min_N :8;	//0: ignore fgop_min_N. >0: If current is I && "distance to previous I" < fgop_min_N, coded as P

	uint32_t me_config;		//[0:2]: me2_sr_x, [3:5]: me2_sr_y, [6:31]: reserved

	uint32_t share_smem_option :19;	//share SMEM among all encoders, basically required for multiple encoder when SMEM not enough
	uint32_t share_smem_max_width :13;//max picture width of the encoder that use share SMEM. This is used to compute the share SMEM size.

	uint32_t bits_partition_base;      //-for partial bitstrm info update to ARM
	uint32_t bits_partition_limit;     // to reduce cabac-to-arm letency and for low delay purpose only

	uint32_t custom_encoder_frame_rate;
	uint32_t q_matrix4x4_daddr;
	uint32_t reserved[4];
} ENCODER_SETUP_CMD;

typedef enum CODING_TYPEtag {
	H264_ENC = 1,
	MPEG_ENC = 2,
	MJPEG_ENC = 3,
} CODING_TYPE;

typedef enum PAFF_MODEtag {
	PAFF_ALL_FRM = 1,		// all frame
	PAFF_ALL_FLD = 2,		// all field, for interlaced VIN only
	PAFF_ALL_MBAFF = 3,	// MPEG2 only
} PAFF_MODE;

typedef struct H264ENC_VUI_PARtag {
	uint32_t vui_enable :1;
	uint32_t aspect_ratio_info_present_flag :1;
	uint32_t overscan_info_present_flag :1;
	uint32_t overscan_appropriate_flag :1;
	uint32_t video_signal_type_present_flag :1;
	uint32_t video_full_range_flag :1;
	uint32_t colour_description_present_flag :1;
	uint32_t chroma_loc_info_present_flag :1;
	uint32_t timing_info_present_flag :1;
	uint32_t fixed_frame_rate_flag :1;
	uint32_t nal_hrd_parameters_present_flag :1;
	uint32_t vcl_hrd_parameters_present_flag :1;
	uint32_t low_delay_hrd_flag :1;
	uint32_t pic_struct_present_flag :1;
	uint32_t bitstream_restriction_flag :1;
	uint32_t motion_vectors_over_pic_boundaries_flag :1;
	uint32_t reserved_vui_0 :8;
	// aspect_ratio_info_present_flag
	uint32_t aspect_ratio_idc :8;

	uint32_t SAR_width :16;
	uint32_t SAR_height :16;

	// video_signal_type_present_flag
	uint32_t video_format :8;
	// colour_description_present_flag
	uint32_t colour_primaries :8;
	uint32_t transfer_characteristics :8;
	uint32_t matrix_coefficients :8;

	// chroma_loc_info_present_flag
	uint32_t chroma_sample_loc_type_top_field :8;
	uint32_t chroma_sample_loc_type_bottom_field :8;
	uint32_t reserved_vui_3 :16;

	//nal_hrd_parameters_present_flag/vcl_hrd_parameters_present_flag (used in GenerateHRD_parameters_rbsp())
	uint32_t vbr_cbp_rate;

	// bitstream_restriction_flag
	uint32_t reserved_vui_4 :16;
	uint32_t log2_max_mv_length_horizontal :8;
	uint32_t log2_max_mv_length_vertical :8;

	uint32_t num_reorder_frames :16;
	uint32_t max_dec_frame_buffering :16;
} H264ENC_VUI_PAR;   //28B

/// H264/JPEG/MPEG2 encoder start (cmd code 0x02000002)
typedef struct H264ENC_START_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;

	uint32_t bits_fifo_next;
	uint32_t info_fifo_next;
	uint32_t start_encode_frame_no;
	uint32_t encode_duration;

	uint32_t is_flush :8;
	uint32_t enable_slow_shutter :8;
	uint32_t res_rate_min :8;		// between 0 and 100
	uint32_t reserved_0 :8;

	int32_t alpha :8;		// between -6 and 6
	int32_t beta :8;		// between -6 and 6
	int32_t disable_deblocking_filter :8;		// 0 enable deblocking filtering.
	int32_t reserved_1 :8;

	uint32_t max_upsampling_rate :8;
	uint32_t slow_shutter_upsampling_rate :8;
	uint32_t firstGOPstartB :8;
	uint32_t reserved_2 :8;

	uint32_t I_IDR_sp_rc_handle_mask :8;
	uint32_t IDR_QP_adj_str :8;
	uint32_t IDR_class_adj_limit :8;
	uint32_t reserved_3 :8;

	uint32_t I_QP_adj_str :8;
	uint32_t I_class_adj_limit :8;
	uint32_t frame_cropping_flag :16;

	uint32_t frame_crop_left_offset :16;
	uint32_t frame_crop_right_offset :16;

	uint32_t frame_crop_top_offset :16;
	uint32_t frame_crop_bottom_offset :16;

	uint32_t max_bytes_per_pic_denom :16;
	uint32_t max_bits_per_mb_denom :16;

	uint32_t sony_avc :8;
	uint32_t au_type :8;
	uint32_t reserved_4 :16;

	H264ENC_VUI_PAR h264_vui_par;	//h.264 seq_hdr->VUI parameters
} H264ENC_START_CMD;

/// H264/JPEG/MPEG2 encoder stop (cmd code 0x02000003)
typedef struct ENCODER_STOP_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;
	uint32_t stop_method;	   // use ENC_STOP_METHOD
} ENCODER_STOP_CMD;

typedef enum ENC_STOP_METHODtag {
	H264_STOP_IMMEDIATELY = 0,
	H264_STOP_ON_NEXT_IP = 1,
	H264_STOP_ON_NEXT_I = 2,
	H264_STOP_ON_NEXT_IDR = 3,
	EMERG_STOP = 0xff,
} ENC_STOP_METHOD;

typedef enum ENC_PAUSE_METHODtag {
	H264_PAUSE_IMMEDIATELY = 0,
	H264_PAUSE_ON_NEXT_IP = 1,
	H264_PAUSE_ON_NEXT_I = 2,
	H264_PAUSE_ON_NEXT_IDR = 3,
	H264_PAUSE_UNKNOWN = 4,
} ENC_PAUSE_METHOD;

/// JPEG encoding setup (cmd code 0x02000004)
typedef struct JPEG_ENCODE_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t encode_w_sz :16; /* encode w.  Applicable only if is_mjpeg=1 */
	uint32_t chroma_format :32; // 0: 422, 1: 420, 2: 400 (mono)
	uint32_t bits_fifo_base;
	uint32_t bits_fifo_limit;
	uint32_t info_fifo_base;
	uint32_t info_fifo_limit;
	uint32_t quant_matrix_addr;
	uint32_t custom_encoder_frame_rate; /* change name from "frame_rate" */
	uint32_t is_mjpeg :1;
	uint32_t enc_src :3; /* specifies the source image from vcap to encode from.  Applicable only if is_mjpeg=1 */
	uint32_t reserved1 :28;
	uint32_t target_bpp :16;	//target bit per pixel
	uint32_t encode_h_sz :16; /* encode h.  Applicable only if is_mjpeg=1 */
	uint32_t jpeg_qlevel :8;	/* change name from "qlevel" */
	uint32_t tolerance :8;
	uint32_t max_enc_loop :8;
	uint32_t rate_curve_points :8;
	uint32_t rate_curve_addr;
	uint32_t encode_w_ofs : 16;	// the offset between input image and real encode image.
	uint32_t encode_h_ofs : 16;
	uint32_t frame_rate_division_factor : 8;
	uint32_t frame_rate_multiplication_factor : 8;
	uint32_t reserved : 16;
} JPEG_ENCODE_SETUP_CMD;

/// Vid fade in out setup (cmd code 0x02000005)
typedef struct FADE_IN_OUT_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;
	uint32_t fade_in_duration :16;
	uint32_t fade_out_duration :16;
} FADE_IN_OUT_SETUP_CMD;

/// Vid Fade In Out (cmd code 0x02000006)
typedef struct FADE_IN_OUT_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;
	uint32_t cmd :8;	 // use FADE_IO_CMD_TP
	uint32_t reserved_0 :24;
} FADE_IN_OUT_CMD;

typedef enum FADE_IO_CMD_TPtag {
	FADE_IN_START = 0,
	FADE_IN_STOP = 1,
	FADE_OUT_START = 2,
	FADE_OUT_STOP = 3,
	ENABLE_ENC_FADE_IN = 4,	// use before encode start
	ENABLE_ENC_FADE_OUT = 5,	// use before encode stop
	DISABLE_ENC_FADE_IN = 6,
	DISABLE_ENC_FADE_OUT = 7,
} FADE_IO_CMD_TP;

/// Bits FIFO update (cmd code 0x02000007)
typedef struct BITS_FIFO_UPDATE_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;
	uint32_t bits_fifo_end :32;
} BITS_FIFO_UPDATE_CMD;

/// H264/JPEG/MPEG2 encoder update: on demand IDR (cmd code 0x02000008)
typedef struct ENCODER_UPDATE_ON_DEMAND_IDR_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;
	uint32_t on_demand_IDR :8;//0: not used. 1: change next I/P to IDR. 2: change the first I/P that has PTS >= PTS_to_change_to_IDR to be IDR
	uint32_t reserved1 :24;
	uint32_t reserved2;	//to keep the following 64b pts start from 64b boundary
	uint64_t PTS_to_change_to_IDR;//PTS to change to IDR, used for on_demand_IDR=2
} ENCODER_UPDATE_ON_DEMAND_IDR_CMD;

/// H264/JPEG/MPEG2 encoder update: bitrate change (cmd code 0x02000009)
typedef struct ENCODER_UPDATE_BITRATE_CHANGE_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;
	uint32_t average_bitrate;//0: not change bitrate. >0: change average bitrate to this value (bps, not kbps)
	uint32_t reserved1;	//to keep the following 64b pts start from 64b boundary
	uint64_t PTS_to_change_bitrate;	 //PTS to change bitrate
} ENCODER_UPDATE_BITRATE_CHANGE_CMD;

/// H264/JPEG/MPEG2 encoder update: gop structure (cmd code 0x0200000A)
// NOTE: The command only take effect when ENCODER_SETUP_CMD->gog_chage_on_the_fly>0
typedef struct ENCODER_UPDATE_GOP_STRUCTURE_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;

	uint32_t change_gop_option :8;//0: not change. 1: change gop. 2: change gop and force first I/P after change gop to be IDR (only for follow_gop=0)	//follow gop parameter: used when follow_gop>0
	uint32_t follow_gop :8;
	uint32_t fgop_max_N :8;
	uint32_t fgop_min_N :8;
	//fix gop parameter: used when follow_gop=0
	uint32_t M :8;
	uint32_t N :8;
	uint32_t gop_structure :8;
	uint32_t idr_interval :8;

	uint64_t PTS_to_change_gop;	//PTS to change gop structure, gop will change at the first I/P boundary after this PTS
} ENCODER_UPDATE_GOP_STRUCTURE_CMD;

///MJPEG capture (cmd code 0x0200000B)
typedef struct MJPEG_CAPTURE_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved :16;
	uint32_t bits_fifo_next;
	uint32_t info_fifo_next;
	uint32_t start_encode_frame_no;
	uint32_t encode_duration;
	uint8_t framerate_control_M;
	uint8_t framerate_control_N;
	uint16_t reserve;
} MJPEG_CAPTURE_CMD;

/// H264/JPEG/MPEG2 encoder rotate: bitrate change (cmd code 0x0200000C)
typedef struct ENCODER_ROTATE_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :16;
	uint32_t hflip :1;
	uint32_t vflip :1;
	uint32_t rotate :1;
	uint32_t vert_black_bar_en :1;
	uint32_t vert_black_bar_MB_width :8;
	uint32_t reserved1 :20;
} ENCODER_ROTATE_CMD;

/// H264/JPEG/MPEG2 encoder update: framerate change (cmd code 0x0200000D)
typedef struct ENCODER_UPDATE_FRMRATE_CHANGE_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved0 :8;
	uint32_t multiplication_factor :8;
	uint32_t division_factor :8;
	uint32_t reserved1 :24;
} ENCODER_UPDATE_FRMRATE_CHANGE_CMD;

typedef enum {
	OSD_TYPE_MAP_CLUT_8BIT = 1,
	OSD_TYPE_DIRECT_16BIT = 2,
} osd_type_t;

typedef enum {
	OSD_MODE_CLUT = 0,
	OSD_MODE_AYUV = 7,
	OSD_MODE_ARGB = 12,
} osd_mode_t;

/// H264/JPEG/MPEG2 encoder: OSD insertion (cmd code 0x0200000E)
typedef struct ENCODER_OSD_INSERT_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t vout_id :1;
	uint32_t osd_enable :2;
	uint32_t osd_mode :5;
	uint32_t osd_num_regions :2;
	uint32_t reserved :2;
	uint32_t osd_enable_ex :1; // Flag to signal extension (for backward compatibility)
	uint32_t osd_num_regions_ex :2 ; // Extension for more overlay areas.
	uint32_t osd_insert_always : 1;	/* 0 - OSD is inserted only if the frame is encoded, 1 - OSD is inserted always */
	uint32_t osd_csc_param_dram_address;
	uint32_t osd_clut_dram_address[3];
	uint32_t osd_buf_dram_address[3];
	uint16_t osd_buf_pitch[3];
	uint16_t osd_win_offset_x[3];
	uint16_t osd_win_offset_y[3];
	uint16_t osd_win_w[3];
	uint16_t osd_win_h[3];
	uint16_t reserved1;
	// OSD command extension to support up to 4 overlays
	uint32_t osd_clut_dram_address_ex[3];
	uint32_t osd_buf_dram_address_ex[3];
	uint16_t osd_buf_pitch_ex[3];
	uint16_t osd_win_offset_x_ex[3];
	uint16_t osd_win_offset_y_ex[3];
	uint16_t osd_win_w_ex[3];
	uint16_t osd_win_h_ex[3];
	uint16_t reserved2;
} ENCODER_OSD_INSERT_CMD;

typedef struct ENCODER_UPDATE_ROI_PARAMStag {
	uint32_t roi_daddr;
	int8_t roi_delta[NUM_PIC_TYPES][4];
} ENCODER_UPDATE_ROI_PARAMS;

/// H264/JPEG/MPEG2 encoder update: multiple parameters (cmd code 0x0200000F)
typedef struct ENCODER_UPDATE_ENC_PARAMETERS_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t multiplication_factor :8;
	uint32_t division_factor :8;
	uint32_t enable_flags;
	uint32_t on_demand_IDR :1; //0: not used. 1: change next I/P to IDR.
	uint32_t N :8; /* 0: not used. > 0: change existing N of GOP to this value */
	uint32_t idr_interval :8; /* 0: not used. > 0: change existing IDR interval of GOP to this value */
	uint32_t is_monochrome :1;
	uint32_t frame_drop :9;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t N_div_256 :4;
	uint32_t reserved :1;
#else
	uint32_t reserved :5;
#endif
	uint32_t average_bitrate; //0: not change bitrate. >0: change average bitrate to this value (bps, not kbps)
	uint32_t encode_w_ofs :16; //the x offset between input image and real encode image.
	uint32_t encode_h_ofs :16; //the y offset between input image and real encode image.
	int8_t aqp; /* range is 0~4, default is 2 */
	int8_t max_qp_i; /* range is 0~51, default is 51 */
	int8_t min_qp_i; /* range is 0~51, default is 14 */
	int8_t max_qp_p; /* range is 0~51, default is 51 */
	int8_t min_qp_p; /* range is 0~51, default is 17 */
	int8_t max_qp_b; /* range is 0~51, default is 51 */
	int8_t min_qp_b; /* range is 0~51, default is 21 */
	int8_t i_qp_reduce; /* range is 1~10, default is 6 */
	int8_t p_qp_reduce; /* range is 1~5, default is 3 */
	uint8_t skip_flags;
	ENCODER_UPDATE_ROI_PARAMS roi;
	uint16_t P_IntraBiasAdd;
	uint16_t B_IntraBiasAdd;
	uint16_t nonSkipCandidate_bias;
	uint16_t skipCandidate_threshold;
	uint32_t custom_encoder_frame_rate;
	uint32_t quant_matrix_addr;	/* for MJPEG */
	int8_t max_qp_c;
	int8_t min_qp_c;
	int8_t b_qp_reduce;
	uint8_t mctf_privacy_mask_Y ; /* for SGOP only */
	uint8_t mctf_privacy_mask_U ; /* for SGOP only */
	uint8_t mctf_privacy_mask_V ; /* for SGOP only */
	uint16_t mctf_privacy_mask_dpitch ; /* for SGOP only */
	uint32_t mctf_privacy_mask_daddr ; /* for SGOP only */
	uint16_t panic_num;
	uint16_t panic_den;
	uint16_t cpb_underflow_num;
	uint16_t cpb_underflow_den;
	uint8_t zmv_threshold;	/* mv threshold for static case */
	uint8_t jpeg_qlevel;		/* change name from "qlevel" */
	int8_t max_qp_d;			/* Max / Min QP for D-Pics in SVCT 4 levels. */
	int8_t min_qp_d;
	int8_t c_qp_reduce;		/* This field specifies how much better to make D QP relative to C QP. */
	int8_t user_aqp;			/* 0: Basic QP user class. 2: Adv QP offset. */
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint8_t user1_intra_bias;
	uint8_t user1_direct_bias;
	uint8_t user2_intra_bias;
	uint8_t user2_direct_bias;
	uint8_t user3_intra_bias;
	uint8_t user3_direct_bias;
	uint8_t qp_adjust_after_md;
	int8_t reserved1[3];
#else
	int8_t log_q_num_per_gop_minus_1;
	int8_t max_qp_q;
	int8_t min_qp_q;
	int8_t q_qp_reduce;
	int8_t modeBias_I4Add;
	int8_t modeBias_I16Add;
	int8_t modeBias_Inter8Add;
	int8_t modeBias_Inter16Add;
	int8_t modeBias_DirectAdd;
	int8_t reserved1;
	uint32_t roi_daddr_p_pic;
	uint32_t roi_daddr_b_pic;
#endif
} ENCODER_UPDATE_ENC_PARAMETERS_CMD;

/// H264/JPEG/MPEG2 encoder statistics enabled: (cmd code 0x02000010)
typedef struct ENCODER_ENABLE_ENC_STATS_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t mvdump_division_factor :8;
	uint32_t reserved :8;
	uint32_t enable_flags;
	uint32_t mvdump_fifo_base;
	uint32_t mvdump_fifo_limit;
	uint32_t mvdump_fifo_unit_sz; /* mvdump fifo size must be an integer multiple of unit size */
	uint32_t mvdump_dpitch; /* DRAM pitch of each unit */
	uint32_t qp_hist_fifo_base;
	uint32_t qp_hist_fifo_limit;
	uint32_t qp_hist_fifo_unit_sz;
} ENCODER_ENABLE_ENC_STATS_CMD;

/// H264/JPEG encoder batch command: (cmd code 0x02000011)
typedef struct ENCODER_BATCH_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;	/* Must be STRM_TP_ENC_0 */
	uint32_t cmd_buf_num :8;	/* number of DSP cmds packed in cmd_buf_addr */
	uint32_t reserved :8;
	uint32_t cmd_buf_addr;
} ENCODER_BATCH_CMD;

/// H264/JPEG/MPEG2 encoder status
typedef struct ENCODER_STATUS_MSGtag {
	//header (28B)
	uint32_t msg_code;
	uint32_t total_bits_info_ctr_h264;
	uint32_t total_bits_info_ctr_mjpeg;
	uint32_t total_bits_info_ctr_jpeg;
	uint32_t total_bits_info_ctr_tjpeg;
	uint32_t stream_report_number :8;
	uint32_t bits_fifo_offset_reset :1;
	uint32_t reserved0 :23;
	uint32_t reserved1;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t encode_state :8;
	uint32_t reserved2 :8;
	uint32_t reserved3;		  //uint64_t r0_pts_base;
	uint32_t start_encode_pts;
	uint32_t fade_io_gain;
	uint32_t encode_yuv_y_daddr;
	uint32_t encode_yuv_uv_daddr;
	uint32_t encode_yuv_dpitch;
} ENCODER_STATUS_MSG;   //56B

// ENCODER STATUS (ENC_STATUS) in dsp msg
typedef enum {
	ENC_IDLE_STATE = 0x00,
	ENC_BUSY_STATE = 0x01,
	ENC_INIT_STATE = 0xFF,
} encode_state_t;

typedef struct ENCODER_BUFFER_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t encode_state :8;
	uint32_t reserved0 :8;

	uint32_t encode_me1_daddr;
	uint32_t encode_me1_dpitch;
	uint32_t encode_me1_pts;
	uint32_t encode_me1_repeat :3;
	uint32_t encode_me1_isfirst :1;
	uint32_t reserved1 :28;
} ENCODER_BUFFER_STATUS_MSG;

typedef struct STILL_420T422_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t pic_422_cnt;
	uint32_t yuv_422_luma_addr;
	uint32_t yuv_422_chroma_addr;
	uint32_t yuv_422_pitch;
	uint32_t me1_buf_addr;
	uint32_t me1_buf_pitch;
	uint32_t me1_buf_wdith;
	uint32_t me1_buf_height;
} STILL_420T422_STATUS_MSG;

/************************************************************
 * H.264 decoder command and messages (Category 3)
 */
// (cmd code 0x03000001)
typedef struct H264DEC_SETUP_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;
	uint8_t decode_type;
	uint8_t enable_pic_info;
	uint8_t use_tiled_dram;

	uint32_t bits_fifo_base;
	uint32_t bits_fifo_limit;
	uint32_t rbuf_smem_size;
	uint32_t fbuf_dram_size;
	uint32_t pjpeg_buf_size;
	uint8_t cabac_2_recon_delay;
	uint8_t force_fld_tiled;
	uint8_t ec_mode;
	uint8_t stitch_allowed;
	uint16_t frm_buf_width;
	uint16_t frm_buf_height;
} H264DEC_SETUP_CMD;


// (cmd code 0x03000002)
typedef struct H264DEC_DECODE_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t reserved[3];
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
	uint32_t num_pics;
	uint32_t num_frame_decode;
	uint32_t first_frame_display;
} H264DEC_DECODE_CMD;

// (cmd code 0x03000003)
typedef struct H264DEC_STOP_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t seamless_flag;
	uint8_t reserved[2];
	uint8_t stop_flag;
} H264DEC_STOP_CMD;

// (cmd code 0x03000004)
typedef struct H264DEC_BITS_FIFO_UPDATE_CMDtag {
	uint32_t cmd_code;
	uint32_t decoder_id;
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
	uint32_t num_pics;
} H264DEC_BITS_FIFO_UPDATE_CMD;

typedef enum {
	SPEED_SCAN_IPB_ALL = 0,
	SPEED_SCAN_REF_ONLY = 1,
	SPEED_SCAN_I_ONLY = 2,
} SPEED_SCAN_MODE;

typedef enum {
	DIR_FWD = 0,
	DIR_BWD = 1,
} SPEED_DIRECTION;

// (cmd code 0x03000005)
typedef struct H264DEC_PLAYBACK_SPEED_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t reserved[3];
	uint16_t speed;
	uint8_t scan_mode;  // see SPEED_SCAN_MODE
	uint8_t direction;  // see SPEED_DIRECTION
} H264DEC_PLAYBACK_SPEED_CMD;

typedef enum {
	TRICK_PAUSE = 0,
	TRICK_RESUME = 1,
	TRICK_STEP = 2,
} TRICKPLAY_MODE;

// (cmd code 0x03000006)
typedef struct H264DEC_TRICKPLAY_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t mode;  // TRICK_PAUSE = 0, TRICK_RESUME, TRICK_STEP
	uint8_t reserved;
} H264DEC_TRICKPLAY_CMD;

// (cmd code 0x03000007)
typedef struct H264DEC_STAT_ENABLE_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;
	uint8_t enable_pic_info_msg;
	uint8_t enable_coded_pic_dadder_in_bs;
	uint8_t enable_sps_daddr_in_bs;

	uint8_t enable_pic_stat_daddr;
	uint8_t enable_mb_info;
	uint8_t reserved_0;
	uint8_t reserved_1;
} H264DEC_STAT_ENABLE_CMD;

/**
 * H264 decode status msg (0x83000001)
 */
typedef struct H264DEC_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t timecode;
	uint32_t latest_clock_counter;
	uint32_t decode_id;

	uint32_t decode_state;
	uint32_t reserved;
	uint32_t latest_pts_low;
	uint32_t latest_pts_high;

	uint32_t h264_bits_fifo_next;

	uint32_t error_status;
	uint32_t dis_pic_y_addr;
	uint32_t dis_pic_uv_addr;

	uint32_t decoded_pic_number;

	uint16_t frm_pitch;
	uint16_t frm_width;
	uint16_t frm_height;
	uint8_t chroma_format_idc;
	uint8_t reserved2;

	/* for capture video information */
	uint32_t jpeg_capture_cnt;
	uint32_t main_coded_size;
	uint32_t thumb_coded_size;
	uint32_t screen_coded_size;
	uint32_t yuv_capture_cnt;
	uint32_t cap_luma_daddr;
	uint32_t cap_chma_daddr;
} H264DEC_STATUS_MSG;

/**
 * H264 decode pic_info (0x83000002)
 */
typedef struct H264DEC_PICINFO_STRUCTtag {
	uint8_t pic_type;
	uint8_t pic_struct;

	uint32_t sps_daddr_in_bs;
	uint32_t pic_stat_daddr;
	uint32_t mb_info_daddr;
	uint32_t coded_pic_daddr_in_bs;

	uint16_t sps_size;
	uint16_t pic_stat_size;
	uint32_t mb_info_size;
	uint32_t coded_pic_size;

	uint32_t decoded_pic_y_addr;
	uint32_t decoded_pic_uv_addr;

	uint16_t decoded_pic_pitch;
	uint16_t decoded_pic_width;
	uint16_t decoded_pic_height;
} H264DEC_PICINFO_STRUCT;

typedef struct H264DEC_PICINFO_MSGtag {
	uint32_t msg_code;

	uint32_t pts_low;
	uint32_t pts_high;

	union {
		H264DEC_PICINFO_STRUCT frm;
		H264DEC_PICINFO_STRUCT top;
	} FRM_TOP;
	H264DEC_PICINFO_STRUCT bot;
} H264DEC_PICINFO_MSG;

/************************************************************
 * MPEG2 decoder command and messages (Category 5) N/A on A7
 */
// (cmd code 0x05000001)
// (cmd code 0x05000002)
// (cmd code 0x05000003)
// (cmd code 0x05000004)
// (cmd code 0x05000005)
// (cmd code 0x05000006)
// (cmd code 0x05000007)
/**
 * MPEG2 decode status msg (0x85000001)
 */

/**
 * MPEG2 decode picinfo msg (0x85000002)
 */

/************************************************************
 * MPEG4 decoder command and messages (Category 6) N/A on A7
 */
// (cmd code 0x06000001)
// (cmd code 0x06000002)
// (cmd code 0x06000003)
// (cmd code 0x06000004)
// (cmd code 0x06000005)
// (cmd code 0x06000006)
/**
 * MPEG4 decode status msg (0x86000001)
 */

/************************************************************
 * VOUT command and messages (Category 7)
 */

typedef enum {
	VOUT_ID_A = 0,
	VOUT_ID_B = 1,
} VOUT_ID;

typedef enum {
	VOUT_SRC_DEFAULT_IMG = 0,
	VOUT_SRC_BACKGROUND = 1,
	VOUT_SRC_H264_DEC = 3,
	VOUT_SRC_UDEC = 4,
	VOUT_SRC_MPEG2_DEC = 5,
	VOUT_SRC_MPEG4_DEC = 6,
	VOUT_SRC_MIXER_A = 7,
	VOUT_SRC_VCAP = 8,
	VOUT_SRC_STILL = 9,
} VOUT_SRC;

typedef enum {
	OSD_SRC_MAPPED_IN = 0,
	OSD_SRC_DIRECT_IN_16 = 1,
	OSD_SRC_DIRECT_IN_32 = 2,
} OSD_SRC;

typedef enum {
	CSC_DIGITAL = 0,
	CSC_ANALOG = 1,
	CSC_HDMI = 2,
} CSC_TYPE;

// (cmd code 0x07000001)
typedef struct VOUT_MIXER_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint8_t interlaced;
	uint8_t frm_rate;
	uint16_t act_win_width;
	uint16_t act_win_height;
	uint8_t back_ground_v;
	uint8_t back_ground_u;
	uint8_t back_ground_y;
	uint8_t mixer_444;
	uint8_t highlight_v;
	uint8_t highlight_u;
	uint8_t highlight_y;
	uint8_t highlight_thresh;
	uint8_t reverse_en;
	uint8_t csc_en;
	uint8_t reserved[2];
	uint32_t csc_parms[9];
} VOUT_MIXER_SETUP_CMD;

// (cmd code 0x07000002)
typedef struct VOUT_VIDEO_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint8_t en;
	uint8_t src;
	uint8_t flip;
	uint8_t rotate;
	uint16_t reserved;
	uint16_t win_offset_x;
	uint16_t win_offset_y;
	uint16_t win_width;
	uint16_t win_height;
	uint32_t default_img_y_addr;
	uint32_t default_img_uv_addr;
	uint16_t default_img_pitch;
	uint8_t default_img_repeat_field;
	uint8_t  num_stat_lines_at_top;
	uint32_t default_img_info_dram_addr;
} VOUT_VIDEO_SETUP_CMD;

// (cmd code 0x07000003)
typedef struct VOUT_DEFAULT_IMG_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t default_img_y_addr;
	uint32_t default_img_uv_addr;
	uint16_t default_img_pitch;
	uint8_t default_img_repeat_field;
	uint8_t reserved2;
} VOUT_DEFAULT_IMG_SETUP_CMD;

// (cmd code 0x07000004)
typedef struct VOUT_OSD_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint8_t en;
	uint8_t src;
	uint8_t flip;
	uint8_t rescaler_en;
	uint8_t premultiplied;
	uint8_t global_blend;
	uint16_t win_offset_x;
	uint16_t win_offset_y;
	uint16_t win_width;
	uint16_t win_height;
	uint16_t rescaler_input_width;
	uint16_t rescaler_input_height;
	uint32_t osd_buf_dram_addr;
	uint16_t osd_buf_pitch;
	uint8_t osd_buf_repeat_field;
	uint8_t osd_direct_mode;
	uint16_t osd_transparent_color;
	uint8_t osd_transparent_color_en;
	uint8_t osd_swap_bytes;
	uint32_t osd_buf_info_dram_addr;
} VOUT_OSD_SETUP_CMD;

// (cmd code 0x07000005)
typedef struct VOUT_OSD_BUF_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t osd_buf_dram_addr;
	uint16_t osd_buf_pitch;
	uint8_t osd_buf_repeat_field;
	uint8_t reserved2;
} VOUT_OSD_BUF_SETUP_CMD;

// (cmd code 0x07000006)
typedef struct VOUT_OSD_CLUT_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t clut_dram_addr;
} VOUT_OSD_CLUT_SETUP_CMD;

// (cmd code 0x07000007)
typedef struct VOUT_DISPLAY_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t disp_config_dram_addr;
} VOUT_DISPLAY_SETUP_CMD;

// (cmd code 0x07000008)
typedef struct VOUT_DVE_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t dve_config_dram_addr;
} VOUT_DVE_SETUP_CMD;

// (cmd code 0x07000009)
typedef struct VOUT_RESET_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint8_t reset_mixer;
	uint8_t reset_disp;
} VOUT_RESET_CMD;

// (cmd code 0x0700000A)
typedef struct VOUT_DISPLAY_CSC_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t csc_type; // 0: digital; 1: analog; 2: hdmi
	uint32_t csc_parms[9];
} VOUT_DISPLAY_CSC_SETUP_CMD;

// (cmd code 0x0700000B)
typedef struct VOUT_DIGITAL_OUTPUT_MODE_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint16_t reserved;
	uint32_t output_mode;
} VOUT_DIGITAL_OUTPUT_MODE_SETUP_CMD;

// (cmd code 0x0700000C)
typedef struct VOUT_GAMMA_SETUP_CMDtag {
	uint32_t cmd_code;
	uint16_t vout_id;
	uint8_t enable;
	uint8_t setup_gamma_table;
	uint32_t gamma_dram_addr;
} VOUT_GAMMA_SETUP_CMD;

/************************************************************
 * Video Capture & Preprocessing (VCAP) (Category 8)
 */
#ifdef ARM_GCC
enum {
	DRAM_PREVIEW = 0,
	SMEM_PREVIEW,
	VOUT_TH_PREVIEW,
	VOUT_TH_SMEM_PREIVEW
}VOUT_PREVIEW_SELECT;
#else
enum VOUT_PREVIEW_SELECT {
	DRAM_PREVIEW = 0,
	SMEM_PREVIEW,
	VOUT_TH_PREVIEW,
	VOUT_TH_SMEM_PREIVEW
};
#endif

enum HDR_READ_PROTOCOL {
	HDR_MULTI_EXPO_IN_SEPARATE_LINE = 0,
	HDR_MULTI_EXPO_IN_SINGLE_LINE = 1,
};

// CMD_VCAP_SETUP (0x08000001)
typedef struct VCAP_SETUP_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t vid_skip :16;
	uint32_t input_format :8;	 // use VCAP_INPUT_FORMAT
	uint32_t sensor_id :8;
	uint32_t keep_states :8;
	uint32_t interlaced_output :8;
	uint32_t vidcap_w :16;
	uint32_t vidcap_h :16;
	uint32_t main_w :16;
	uint32_t main_h :16;
	uint32_t preview_w_A :16;
	uint32_t preview_h_A :16;
	uint32_t input_center_x :32;
	uint32_t input_center_y :32;
	uint32_t zoom_factor_x :32;
	uint32_t zoom_factor_y :32;
	uint32_t CmdReadDly :32;
	uint32_t sensor_readout_mode :32;
	uint32_t noise_filter_strength :2;
	uint32_t mctf_chan :2;
	uint32_t sharpen_b_chan :2;
	uint32_t cc_en :1;
	uint32_t cmpr_en :1;
	uint32_t cmpr_dither :1;
	uint32_t mode :2;
	uint32_t image_stabilize_strength :5;
	uint32_t bit_resolution :8;
	uint32_t bayer_pattern :8;
	uint32_t preview_format_A :4;
	uint32_t preview_format_B :3;
	uint32_t no_pipelineflush :1;
	uint32_t preview_frame_rate_A :8;
	uint32_t preview_w_B :16;
	uint32_t preview_h_B :16;
	uint32_t preview_frame_rate_B :8;
	uint32_t preview_A_src :4;
	uint32_t preview_B_src :3;
	uint32_t preview_B_stop :1;
	uint32_t nbr_prevs_x :16;
	uint32_t nbr_prevs_y :16;
	uint32_t vin_frame_rate :16;
	uint32_t idsp_out_frame_rate :16;
	uint32_t pip_w :16;
	uint32_t pip_h :16;
	uint32_t vcap_cmd_msg_dec_rate :16;
	uint32_t pip_frame_dec_rate :16;
	uint32_t vin_frame_rate_frac :16;
	uint32_t idsp_out_frame_rate_frac :16;
	uint32_t pts_delta;
	uint32_t video_delay_mode;
	uint32_t eis_update_addr;
	uint32_t bbar_sz_mbs :8;
	uint32_t preview_A_pipeline_pos :1;
	uint32_t preview_B_pipeline_pos :1;
	uint32_t pip_pipeline_pos :1;
	uint32_t vwarp_blk_h :8;

	/* 0 - preview A for VOUTA, preview B for VOUTB, 1 - preview A for VOUTB, preview B for VOUTA */
	uint32_t vout_swap :1;

	/* 0 - different exposures in different lines, 1 - different exposures in single line */
	uint32_t hdr_data_read_protocol :1;
	uint32_t eis_delay_count :2;

	/* 0 - MCTF based PM, 1 - HDR based PM */
	uint32_t hdr_based_pm :1;
	/* 0 - preview copy disabled, 1/2/3 - copy from preview A/B/C */
	uint32_t preview_cp_id :2;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t enc_dummy_latency :3;
	uint32_t reserved0 :3;
#else
	uint32_t yuv_input_fps_enhanced :1;
	uint32_t reserved0 :5;
#endif
	uint16_t preview_src_w_A;
	uint16_t preview_src_h_A;
	uint16_t preview_src_x_offset_A;
	uint16_t preview_src_y_offset_A;
	uint32_t reserved1;
	uint32_t hiLowISO_proc_cfg_ptr;
} VCAP_SETUP_CMD;

typedef VCAP_SETUP_CMD VCAP_PP_SETUP_CMD;	  // define an alias
typedef enum VCAP_INPUT_FORMATtag { // used by VCAP_SETUP_CMD->input_format
	RGB_RAW = 0,
	YUV_422_INTLC = 1,
	YUV_422_PROG = 2,
	DRAM_INTLC = 3,
	DRAM_PROG = 4,
} VCAP_INPUT_FORMAT;

/// VCAP_SET_ZOOM_CMD (cmd code 0x08000002)
typedef struct VCAP_SET_ZOOM_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved;
	uint32_t zoom_x;
	uint32_t zoom_y;
	uint32_t x_center_offset;
	uint32_t y_center_offset;
} VCAP_SET_ZOOM_CMD;

/// VCAP_PREV_SETUP_CMD (cmd code 0x08000003)
typedef struct VCAP_PREV_SETUP_CMDtag {
	uint32_t cmd_code;
	uint8_t preview_id;
	uint8_t preview_format;
	uint16_t preview_w;
	uint16_t preview_h;
	uint8_t preview_frame_rate;
	uint8_t preview_src : 3;
	uint8_t preview_stop : 1;
	uint8_t disable_extra_2x : 1;
	uint8_t reserved : 3;
	uint16_t preview_src_w;
	uint16_t preview_src_h;
	uint16_t preview_src_x_offset;
	uint16_t preview_src_y_offset;
} VCAP_PREV_SETUP_CMD;

typedef struct VCAP_MCTF_MV_STAB_UPDATE_CMDtag {
	uint32_t mctf_config_update :1;	// load# 5-7
	uint32_t mctf_hist_update :1;	// load# 8-11
	uint32_t mctf_curves_update :1;	// load# 12-15
	uint32_t mcts_config_update :1;	// load# 16
	uint32_t shpb_config_update :1;	// load# 17
	uint32_t shpc_config_update :1;	// load# 18
	uint32_t shpb_fir1_update :1;	// load# 19
	uint32_t shpb_fir2_update :1;	// load# 20
	uint32_t shpc_fir1_update :1;	// load# 21
	uint32_t shpc_fir2_update :1;	// load# 22
	uint32_t shpb_alphas_update :1;	// load# 23
	uint32_t shpc_alphas_update :1;	// load# 24
	uint32_t tone_hist_update :1;	// load# 25-26
	uint32_t shpb_coring1_update :1;	// load# 27
	uint32_t shpc_coring1_update :1;	// load# 28
	uint32_t shpb_coring2_update :1;	// load# 29
	uint32_t shpb_linear_update :1;	// load# 30
	uint32_t shpc_linear_update :1;	// load# 31
	uint32_t shpb_inv_linear_update :1;	// load# 32
	uint32_t shpc_inv_linear_update :1;	// load# 33
	uint32_t mctf_3d_level_update :1;	// load# 34,35
	uint32_t shpb_3d_level_update :1;	// load# 36,37
	uint32_t cc_config_update :1;	// load# 48
	uint32_t cc_input_table_update :1;	// load# 49
	uint32_t cc_output_table_update :1;	// load# 50
	uint32_t cc_3d_table_update :1;	// load# 51-58
	uint32_t cc_matrix_update :1;	// load# 59
	uint32_t cc_blend_input_update :1;	// load# 60
	uint32_t cmpr_all_update :1;	// load# 62-63
	uint32_t mctf_mcts_all_update :1;
	uint32_t cc_all_update :1;
	uint32_t reserved :1;
} VCAP_MCTF_MV_STAB_CMD_UPDATE;

/// VCAP_MCTF_MV_STAB_CMD (cmd code 0x08000004)
typedef struct VCAP_MCTF_MV_STAB_CMDtag {
	uint32_t cmd_code;
	uint32_t noise_filter_strength :2;
	uint32_t mctf_chan :2;
	uint32_t sharpen_b_chan :2;
	uint32_t cc_en :1;
	uint32_t cmpr_en :1;
	uint32_t cmpr_dither :1;
	uint32_t mode :2;
	uint32_t image_stabilize_strength :5;
	uint32_t bitrate_y :6;
	uint32_t bitrate_uv :6;
	uint32_t use_zmv_as_predictor :1;
	uint32_t reserved :3;
	uint32_t mctf_cfg_dbase;
	uint32_t cc_cfg_dbase;
	uint32_t cmpr_cfg_dbase;

	// config update flags
	union {
		VCAP_MCTF_MV_STAB_CMD_UPDATE cmds;
		uint32_t word;
	} loadcfg_type;

	uint32_t mb_temporal_reserved : 7 ;
	uint32_t mb_temporal_frames_combine_num : 2 ; /* 1 to 3 */
	uint32_t mb_temporal_motion_detection_delay : 4 ; /* 1 to 10 */
	uint32_t mb_temporal_shift : 3 ; /* 0 to 6 */
	uint32_t mb_temporal_min2 : 4 ; /* 0 to 15 */
	uint32_t mb_temporal_max2 : 4 ; /* 0 to 15 */;
	uint32_t mb_temporal_max1 : 8 ; /* 0 to 255 */
	uint32_t mb_temporal_min1 : 8 ; /* 0 to 255 */
	uint32_t mb_temporal_frames_combine_thresh : 8 ; /* 0 to 255 */
	uint32_t mb_temporal_frames_combine_min_above_thresh : 8 ; /* 0 to 255 */
	uint32_t mb_temporal_frames_combine_val_below_thresh : 8 ; /* 0 to 255 */
	uint16_t mb_temporal_sub; /* 0 to 16383 */
	uint16_t mb_temporal_mul; /* 0 to 65535 */
} VCAP_MCTF_MV_STAB_CMD;

/// VCAP_TMR_MODE_CMD (cmd code 0x08000005)
typedef struct VCAP_TMR_MODE_CMDtag {
	uint32_t cmd_code;
	uint8_t timer_scaler;
	uint8_t display_opt;

	// 0 terminate with frame wait, 1 reset idsp, and terminate right away
	uint8_t video_term_opt;
	uint8_t reserved;
} VCAP_TMR_MODE_CMD;

/// VCAP_CTRL_CMD (cmd code 0x08000006)
typedef struct VCAP_CTRL_CMDtag {
	uint32_t cmd_code;
	uint32_t ctrl_code;
	uint32_t par_1;
	uint32_t par_2;
	uint32_t par_3;
	uint32_t par_4;
	uint32_t par_5;
	uint32_t par_6;
	uint32_t par_7;
	uint32_t par_8;
} VCAP_CTRL_CMD;

/// VCAP_STILL_CAP_CMD (cmd code 0x08000007)
typedef struct VCAP_STILL_CAP_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint8_t still_process_mode;	// 1==hiso, 2==liso, 3==faststill, 4==multi-frame hiso
	uint8_t num_yuv_buffers;
	uint8_t output_select;
	uint8_t input_format;
	uint8_t vsync_skip;
	uint8_t num_raw_buffers;
	uint32_t number_frames_to_capture;
	uint16_t vidcap_w;
	uint16_t vidcap_h;
	uint16_t main_w;
	uint16_t main_h;
	uint16_t encode_w;
	uint16_t encode_h;
	uint16_t preview_w;
	uint16_t preview_h;
	uint16_t thumbnail_w_active;
	uint16_t thumbnail_h_active;
	uint16_t thumbnail_w_dram;
	uint16_t thumbnail_h_dram;
	uint32_t jpeg_bit_fifo_start;
	uint32_t jpeg_bit_info_fifo_start;
	uint32_t sensor_readout_mode;
	uint16_t sensor_id;
	uint8_t sensor_resolution;
	uint8_t sensor_pattern;
	uint16_t preview_w_B;
	uint16_t preview_h_B;
	uint32_t raw_cap_cntl;
	uint16_t raw2yuv_proc_mode;
	uint16_t jpeg_encode_cntl;
	uint32_t raw_capture_resource_ptr;
	uint32_t hiLowISO_proc_cfg_ptr;

	uint32_t preview_control :8;
	uint32_t jpeg_enc_mode :8;
	uint32_t hdr_processing :1;
	uint32_t disable_quickview :1;
	uint32_t reserved_1 :14;

	uint32_t screen_thumbnail_active_w :16;
	uint32_t screen_thumbnail_active_h :16;
	uint32_t screen_thumbnail_dram_w :16;
	uint32_t screen_thumbnail_dram_h :16;
	uint8_t num_yuv422_buffers;
	uint8_t reserve2;
	uint16_t reserve3;
} VCAP_STILL_CAP_CMD;

/**
 * Still capture ADV (cmd code 0x0800000A)
 */
typedef struct VCAP_STILL_ADV_CMDtag {
	uint32_t cmd_code;

	uint32_t adv_yuv2jpeg :1;
	uint32_t adv_raw2yuv :1;
	uint32_t adv_raw_cap :1;
	uint32_t terminate_raw_cap :1;
	uint32_t discard_raw :1;		// discard oldest raw image in pipeline
	uint32_t discard_yuv :1;// discard oldest set of yuv images (main+screen+thumb) in pipeline.
	                        // only valid when such full frame images are available in pipeline
	uint32_t rawcap_preview :1;	// valid only when adv_raw_cap is set. invalid when adv_raw2yuv is set. generate preview using the newest captured raw image
	uint32_t adv_cfa_3a :1;
	uint32_t adv_hdr_out :1;
	uint32_t adv_hdr_yuv :1;
	uint32_t disable_quickview :1;
	uint32_t raw_is_compressed :1;	// App supplied raw buffer is compressed.
	uint32_t reserve :20;

	uint8_t vsync_skip;
	uint8_t reserve2;
	uint16_t raw_buf_pitch;

	// App has option to adv_raw2yuv using an App-supplied raw buffer
	// raw_buf_addr==0 means use dsp internal raw buffer already in pipeline
	// raw_buf_addr!=0 means use App supplied raw buffer.
	// when raw_buf_addr==0, raw_buf_pitch, raw_is_compressed, raw_img_w, raw_img_h are don't care

	uint32_t raw_buf_addr;
	uint16_t raw_img_width;
	uint16_t raw_img_height;

	uint32_t yuv_buf_addr;
	uint32_t me1_buf_addr;

	uint16_t yuv_buf_pitch;
	uint16_t yuv_buf_width;
	uint16_t yuv_buf_height;

	uint16_t me1_buf_pitch;
	uint16_t me1_buf_width;
	uint16_t me1_buf_height;
} VCAP_STILL_ADV_CMD;

/// VCAP_STILL_HDR_PROC_CNTRL_CMD (cmd code 0x08000015)
typedef struct VCAP_STILL_HDR_PROC_CNTRL_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;

	uint8_t is_frm_ofs_daddr;
	uint8_t norm_frm_proc_mode;

	uint16_t d_pitch;
	uint16_t d_width;
	uint16_t d_height;
	uint8_t hdr_group_sz;
	uint8_t reserved_1;

	uint32_t hdr_cntrl_param_daddr;
	uint32_t hdr_in_1st_daddr;
	uint32_t hdr_out_daddr;
	uint16_t hdr_out_slice_width;
	uint16_t reserved_2;

	uint32_t norm_frm_ofs;
	uint32_t frm_ofs[10];
} VCAP_STILL_HDR_PROC_CNTRL_CMD;

typedef enum VCAP_STILL_OUTPUT_SELECT_MASKStag {
	VCAP_STILL_OUTPUT_JPEG = 0x01,
	VCAP_STILL_OUTPUT_RAW = 0x02,
	VCAP_STILL_OUTPUT_JPEG_THM = 0x4,
	VCAP_STILL_OUTPUT_MAIN_YUV = 0x8,
	VCAP_STILL_OUTPUT_PREV_A_YUV = 0x10,
	VCAP_STILL_OUTPUT_PREV_B_YUV = 0x20,
	VCAP_STILL_OUTPUT_COMP_RAW = 0x40,
} VCAP_STILL_OUTPUT_MASKS;

typedef enum VCAP_STILL_INPUT_FORMATtag {
	VCAP_STILL_INPUT_RGB = 0,
	VCAP_STILL_INPUT_YUV422,
	VCAP_STILL_INPUT_YUV420,
} VCAP_STILL_INPUT_FORMAT;

typedef enum VCAP_STILL_OUTPUT_FORMATtag {
	VCAP_STILL_OUTPUT_RGB = 0,
	VCAP_STILL_OUTPUT_YUV422,
	VCAP_STILL_OUTPUT_YUV420,
} VCAP_STILL_OUTPUT_FORMAT;

/// VCAP_STILL_CAP_IN_REC_CMD (cmd code 0x08000008)
typedef struct VCAP_STILL_CAP_IN_REC_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved0;
	uint16_t main_jpg_w;
	uint16_t main_jpg_h;
	uint16_t encode_jpg_w;
	uint16_t encode_jpg_h;
	int16_t encode_x_ctr_offset;
	int16_t encode_y_ctr_offset;
	uint16_t thumbnail_w_active;
	uint16_t thumbnail_h_active;
	uint16_t thumbnail_w_dram;
	uint16_t thumbnail_h_dram;
	uint16_t blank_period_duration; // absolute duration, time unit: 1/60000 second.
	uint16_t is_use_compaction;	  // if compaction is needed
	uint32_t screen_thumbnail_active_w :16;
	uint32_t screen_thumbnail_active_h :16;
	uint32_t screen_thumbnail_dram_w :16;
	uint32_t screen_thumbnail_dram_h :16;
	uint16_t thumb_blank_period_duration;
	uint16_t screen_nail_blank_period_duration;
	uint8_t sn_src;
	uint8_t tn_src;

#define PIV_SCR_NAIL_SRC_PREV_A	 (0)
#define PIV_SCR_NAIL_SRC_PIV_MAIN   (1)

	uint16_t reserved1;
} VCAP_STILL_CAP_IN_REC_CMD;

/// VCAP_STILL_PROC_MEM_CMD (cmd code 0x08000009)
typedef struct VCAP_STILL_PROC_MEM_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint8_t still_process_mode;	// 1==hiso, 2==liso, 3==faststill, 4==multi-frame hiso
	uint8_t hdr_processing :1;
	uint8_t reserved :7;
	uint8_t output_select;
	uint8_t input_format;
	uint8_t bayer_pattern;
	uint8_t resolution;
	uint32_t input_address;
	uint32_t input_chroma_address;
	uint16_t input_pitch;
	uint16_t input_chroma_pitch;
	uint16_t input_w;
	uint16_t input_h;
	uint16_t main_w;
	uint16_t main_h;
	uint16_t encode_w;
	uint16_t encode_h;
	int16_t encode_x_ctr_offset;
	int16_t encode_y_ctr_offset;
	uint16_t thumbnail_w_active;
	uint16_t thumbnail_h_active;
	uint16_t thumbnail_w_dram;
	uint16_t thumbnail_h_dram;
	uint16_t preview_w_A;
	uint16_t preview_h_A;
	uint16_t preview_w_B;
	uint16_t preview_h_B;
	uint32_t hiLowISO_proc_cfg_ptr;
	uint32_t preview_control :8;
	uint32_t raw2yuv_proc_mode :16;
	uint32_t raw_cap_cntl :8;
	uint32_t screen_thumbnail_active_w :16;
	uint32_t screen_thumbnail_active_h :16;
	uint32_t screen_thumbnail_dram_w :16;
	uint32_t screen_thumbnail_dram_h :16;
	uint32_t jpeg_enc_mode :8;
	uint32_t num_yuv_buffers :8;
	uint32_t num_yuv422_buffers :8;
	uint32_t reserved_1 :8;
	uint32_t number_frames_to_capture;
} VCAP_STILL_PROC_MEM_CMD;

/// VCAP_INTERVAL_CAP_CMD (cmd code 0x0800000B)
typedef struct VCAP_INTERVAL_CAP_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t mode;
	uint32_t interval_cap_act;
	uint32_t frames_to_cap;
} VCAP_INTERVAL_CAP_CMD;

/// VCAP_VID_FADE_IN_OUT_CMD (cmd code 0x0800000C)
typedef struct VCAP_VID_FADE_IN_OUT_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved;
	uint16_t fade_in_duration;
	uint16_t fade_out_duration;
} VCAP_VID_FADE_IN_OUT_CMD;

/// VCAP_SET_REPEAT_DROP_FRM_CMD (cmd code 0x0800000D)
typedef struct VCAP_SET_REPEAT_DROP_FRM_CMD_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved;
	uint32_t mode_sel;
	uint32_t frame_cnt;
} VCAP_SET_REPEAT_DROP_FRM_CMD;

/// VCAP_SET_SLOW_SHUTTER_RATE_CMD (cmd code 0x0800000E)
typedef struct VCAP_SET_SLOW_SHUTTER_RATE_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved;
	uint32_t slow_shutter_upsampling_rate;
} VCAP_SET_SLOW_SHUTTER_RATE_CMD;

/// VCAP_SET_STATISTICS_CMD (cmd code 0x0800000F)
typedef struct VCAP_SET_STATISTICS_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
} VCAP_SET_STATISTICS_CMD;

/// VCAP_BLDENDING_CMD (cmd code 0x08000010)
typedef struct VCAP_BLENDING_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;

	uint32_t enable :8;
	uint32_t area_id :8;
	uint32_t reserved_1 :16;

	uint32_t osd_addr_y;
	uint32_t osd_addr_uv;
	uint32_t alpha_addr_y;
	uint32_t alpha_addr_uv;

	uint32_t osd_width :16;
	uint32_t osd_pitch :16;

	uint32_t osd_height :16;
	uint32_t osd_start_x :16;

	uint32_t osd_start_y :16;
	uint32_t reserved_2 :16;
} VCAP_BLENDING_SETUP;

typedef struct VCAP_LOW_ISO_CFG_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;

	uint32_t step_a_cfg_addr;
	uint32_t num_step_a_cmd;

	uint32_t step_b_cfg_addr;
	uint32_t num_step_b_cmd;

	uint32_t step_c_cfg_addr;
	uint32_t num_step_c_cmd;

	uint32_t step_d_cfg_addr;
	uint32_t num_step_d_cmd;

	uint32_t step_e_cfg_addr;
	uint32_t num_step_e_cmd;
} VCAP_LOW_ISO_CFG_CMD;

// VCAP_BLDENDING_CMD (cmd code 0x08000012)
typedef struct VCAP_OSD_BLEND_CNTL_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;

	uint32_t blending_ctnl;
	uint32_t osd_area_mask;
} VCAP_OSD_BLEND_CNTL_CMD;

/// VCAP_GMV_CMD (cmd code 0x08000013)
typedef struct VCAP_MCTF_GMV_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;

	uint32_t enable :1;
	uint32_t reserved_1 :31;

	uint32_t gmv;
} VCAP_MCTF_GMV_CMD;

typedef struct VCAP_SET_TIMER_INTERVAL_CMDtag {
	uint32_t cmd_code;
	uint32_t timer_val;
} VCAP_SET_TIMER_INTVL;

#define	STILL_COLLAGE_MAX_NUM_TILES	(16)

/// STILL_COLLAGE_SETUP_CMD (cmd code 0x08000016)
typedef struct VCAP_STILL_COLLAGE_SETUP_CMDtag {
	uint32_t cmd_code;

	uint8_t num_tiles_horz;
	uint8_t num_tiles_vert;
	uint16_t incremental_quickview :1;
	uint16_t reserved :15;

	uint16_t mat_border_left;
	uint16_t mat_border_right;
	uint16_t mat_border_top;
	uint16_t mat_border_bottom;

	uint16_t tile_border_left;
	uint16_t tile_border_right;
	uint16_t tile_border_top;
	uint16_t tile_border_bottom;

	uint8_t mat_border_y;
	uint8_t mat_border_u;
	uint8_t mat_border_v;
	uint8_t reserved2;

	uint8_t tile_border_y;
	uint8_t tile_border_u;
	uint8_t tile_border_v;
	uint8_t reserved3;

	uint8_t pic_order[STILL_COLLAGE_MAX_NUM_TILES];
} STILL_COLLAGE_SETUP_CMD;

/// VCAP_STILL_COLLAGE_SETUP_CMD (cmd code 0x08000017)
typedef struct STILL_RAW2RAW_CMDtag {
	uint32_t cmd_code;

	uint32_t src_addr;
	uint32_t dst_addr;

	uint16_t src_width;
	uint16_t src_pitch;
	uint16_t src_height;

	uint16_t dst_pitch;

	uint8_t src_compr;	// compression mode of src image. 0=no uncompressed
	uint8_t dst_compr;	// compression mode of dst image,
	uint8_t AAA_enb;
	uint8_t reserved;
} STILL_RAW2RAW_CMD;

/// STILL_RESAMPLE_YUV_CMD (cmd code 0x08000018)
typedef struct STILL_RESAMPLE_YUV_CMDtag {
	uint32_t cmd_code;

	uint32_t src_y_daddr;
	uint32_t src_uv_daddr;

	uint32_t dst_y_daddr;
	uint32_t dst_uv_daddr;

	uint16_t src_y_width;
	uint16_t src_y_height;
	uint16_t src_y_pitch;
	uint16_t src_uv_pitch;

	uint16_t dst_y_width;
	uint16_t dst_y_height;
	uint16_t dst_y_pitch;
	uint16_t dst_uv_pitch;

	uint8_t src_uv_format;		// 0=4:2:0, 1=4:2:2, others illegal
	uint8_t dst_uv_format;		// 0=4:2:0, 1=4:2:2, others illegal
	uint16_t reserved;

	uint32_t idsp_cfg_addr;
} STILL_RESAMPLE_YUV_CMD;

//
// STILL_JPEG_ENCODE_YUV_CMD (cmd code 0x08000019)
//
typedef struct STILL_JPEG_ENCODE_YUV_CMDtag {
	uint32_t cmd_code;

	uint32_t quant_table_addr;		// 0 means use default quant table
	uint32_t y_daddr;
	uint32_t uv_daddr;

	uint16_t y_width;
	uint16_t y_pitch;
	uint16_t y_height;

	uint16_t uv_width;
	uint16_t uv_pitch;
	uint16_t uv_height;
} STILL_JPEG_ENCODE_YUV_CMD;

/// VCAP_STILL_RAW_FRAME_SUBTRACTION_CMD (cmd code 0x0800001B)
typedef struct STILL_RAW_FRAME_SUBTRACTION_CMDtag {
	uint32_t cmd_code;

	uint8_t channel_id;
	uint8_t stream_type;
	uint16_t reserved;

	uint32_t src_addr1;
	uint32_t src_addr2;
	uint32_t dst_addr;

	uint32_t pitch;
	uint32_t width;
	uint32_t height;

	uint32_t positive_offset;
} STILL_RAW_FRAME_SUBTRACTION_CMD;

/// CMD_STILL_SEND_INPUT_DATA (cmd code 0x0800001A)
typedef struct VCAP_SEND_INPUT_DATA_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint8_t stop_control;
	uint8_t input_type;
	uint32_t num_fb;
	uint32_t fb_id[4];
	uint32_t blend_alpha[4];
	uint32_t blend_fb_id[4];
	uint32_t blend_out_fb_id[4];
} VCAP_SEND_INPUT_DATA_CMD;

/// CMD_VCAP_UPDATE_CAPTURE_PARAMETERS (cmd code 0x0800001D)
typedef struct VCAP_UPDATE_CAPTURE_PARAMETERS_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id : 8;
	uint32_t preview_id : 2;
	uint32_t frame_rate_division_factor : 8;
	uint32_t freeze_en : 1;
	uint32_t reserved : 13;
	uint32_t enable_flags;
	uint32_t custom_vin_frame_rate;
} VCAP_UPDATE_CAPTURE_PARAMETERS_CMD;

#define VCAP_MAX_NUM_WARP_REGION	(8)

/// VCAP_SET_REGION_WARP_CONTROL (cmd code 0x0800001E)
typedef struct VCAP_SET_REGION_WARP_CONTROL_CMDtag {
	uint32_t cmd_code;
	uint32_t input_win_offset_x :16;
	uint32_t input_win_offset_y :16;
	uint32_t input_win_w :16;
	uint32_t input_win_h :16;
	uint32_t output_win_offset_x :16;
	uint32_t output_win_offset_y :16;
	uint32_t output_win_w :16;
	uint32_t output_win_h :16;
	uint32_t rotate_flip_input :3;
	uint32_t h_warp_enabled :1;
	uint32_t h_warp_grid_w :6; /* <= 32 */
	uint32_t h_warp_grid_h :6; /* <= 48 */
	uint32_t h_warp_bypass :1;
	uint32_t warp_copy_enabled :1;
	uint32_t reserved :14;
	uint32_t h_warp_h_grid_spacing_exponent :3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	uint32_t h_warp_v_grid_spacing_exponent :3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	uint32_t v_warp_enabled :1;
	uint32_t v_warp_grid_w :6; /* <= 32 */
	uint32_t v_warp_grid_h :6; /* <= 48 */
	uint32_t v_warp_h_grid_spacing_exponent :3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	uint32_t v_warp_v_grid_spacing_exponent :3; // 2^(4+grid_spacing) => 0:16 1:32 2:64 3:128 4:256 5:512
	uint32_t region_id :3; /* 0..VCAP_MAX_NUM_WARP_REGION-1 */
	uint32_t num_regions :4; /* 1..VCAP_MAX_NUM_WARP_REGION */
	uint32_t h_warp_grid_addr;
	uint32_t v_warp_grid_addr;
	uint32_t prev_a_input_offset_x : 16;
	uint32_t prev_a_input_offset_y : 16;
	uint32_t prev_a_input_w : 16;
	uint32_t prev_a_input_h : 16;
	uint32_t prev_a_output_offset_x :16;
	uint32_t prev_a_output_offset_y :16;
	uint32_t prev_a_output_w :16;
	uint32_t prev_a_output_h :16;
	uint32_t prev_c_input_offset_x : 16;
	uint32_t prev_c_input_offset_y : 16;
	uint32_t prev_c_input_w : 16;
	uint32_t prev_c_input_h : 16;
	uint32_t prev_c_output_offset_x :16;
	uint32_t prev_c_output_offset_y :16;
	uint32_t prev_c_output_w :16;
	uint32_t prev_c_output_h :16;
} VCAP_SET_REGION_WARP_CONTROL_CMD;

/// VCAP_SET_PRIVACY_MASK (cmd code 0x0800001F)
typedef struct VCAP_SET_PRIVACY_MASK_CMDtag {
	uint32_t cmd_code;
	uint32_t enabled_flags_dram_addr;
	uint32_t enabled_flags_dram_pitch;
	uint32_t Y :8;
	uint32_t U :8;
	uint32_t V :8;
	uint32_t reserved :8;
} VCAP_SET_PRIVACY_MASK_CMD;

#define	MAX_EXPOSURE_NUM		(4)
// CMD_VCAP_SET_VIDEO_HDR_PROC_CONTROL (cmd code 0x08000020)
typedef struct VCAP_SET_VIDEO_HDR_PROC_CONTROL_CMDtag {
	uint32_t cmd_code;
	uint32_t exp_0_updated :1;
	uint32_t exp_1_updated :1;
	uint32_t exp_2_updated :1;
	uint32_t exp_3_updated :1;
	uint32_t exp_0_start_offset_updated :1;
	uint32_t exp_1_start_offset_updated :1;
	uint32_t exp_2_start_offset_updated :1;
	uint32_t exp_3_start_offset_updated :1;
	uint32_t yuv_ctrl_updated :1;
	uint32_t low_pass_filter_radius : 2 ;
	uint32_t contrast_enhance_gain : 12;	/* CSC style CE gain (2.10) format */
	uint32_t reserved1 : 9;

	/* Up to 4 exposures */
	uint32_t hdr_ctrl_param_daddr[MAX_EXPOSURE_NUM];
	uint32_t color_correction_cmd_alpha_map_daddr[MAX_EXPOSURE_NUM];

	/* Start offset in the line interleaved raw buffer for each exposure.
	 * The start offset of each exposure depends on the shutter speed.
	 * This field shall be set to 0 for frame interleaved HDR sensor.
	 */
	uint32_t raw_daddr_start_offset[MAX_EXPOSURE_NUM];
	uint32_t yuv_ctrl_param_daddr;

	/* Up to 4 exposures, ignored when hdr_data_read_protocol = 0 */
	uint32_t raw_daddr_start_offset_x[MAX_EXPOSURE_NUM];
} VCAP_SET_VIDEO_HDR_PROC_CONTROL_CMD;

// CMD_VCAP_HISO_CONFIG_UPDATE (cmd code 0x08000021)
typedef struct VCAP_HISO_CONFIG_UPDATEtag {
	uint32_t hiso_config_common_update : 1 ;
	uint32_t hiso_config_color_update : 1 ;
	uint32_t hiso_config_mctf_update : 1 ;
	uint32_t hiso_config_step1_update : 1 ;
	uint32_t hiso_config_step2_update : 1 ;
	uint32_t hiso_config_step3_update : 1 ;
	uint32_t hiso_config_step4_update : 1 ;
	uint32_t hiso_config_step5_update : 1 ;
	uint32_t hiso_config_step6_update : 1 ;
	uint32_t hiso_config_step11_update : 1 ;
	uint32_t hiso_config_step12_update : 1 ;
	uint32_t hiso_config_step13_update : 1 ;
	uint32_t hiso_config_step14_update : 1 ; // LI2
	uint32_t hiso_config_step15_update : 1 ; // vwarp update
	uint32_t hiso_config_aaa_updata : 1 ;	// AAA setup update
	uint32_t reserved : 17 ;
} VCAP_HISO_CONFIG_UPDATE;

typedef struct VCAP_HISO_CONFIG_UPDATE_CMDtag {
	uint32_t cmd_code;
	uint32_t hiso_param_daddr;

	// config update flags
	union {
		VCAP_HISO_CONFIG_UPDATE cmds;
		uint32_t word;
	} loadcfg_type;
} VCAP_HISO_CONFIG_UPDATE_CMD ;

/* FIXME - Move to decode category */
// CMD_VCAP_ENC_FRM_DRAM_HANDSHAKE (cmd code 0x08000022)
typedef struct VCAP_ENC_FRM_DRAM_HANDSHAKE_CMDtag {
	uint32_t cmd_code;
	uint16_t cap_buf_id;		/* This is the frm_buf_id returned in MEMM_REQ_FRM_BUF_MSG for YUV */
	uint16_t me1_buf_id;		/* This is the frm_buf_id returned in MEMM_REQ_FRM_BUF_MSG for ME1 */
	uint32_t pts;				/* This is the PTS pass ucode the regularity of the frame inserts by ARM */
	uint32_t enc_src : 3;		/* This is the source for VCAP to encode from. */
	uint32_t reserved : 29;
} VCAP_ENC_FRM_DRAM_HANDSHAKE_CMD;

typedef struct CMD_VCAP_UPDATE_ENC_PARAMETERStag
{
	uint32_t cmd_code;
	uint32_t target_pts;
	uint32_t cmd_daddr;
	uint16_t cmd_daddr_enable_flag;
	uint16_t reserved;
} CMD_VCAP_UPDATE_ENC_PARAMETERS_CMD;

typedef struct VCAP_NO_OP_CMDtag {
	uint32_t cmd_code;
	uint32_t channel_id :8;		// 0x0
	uint32_t stream_type :8;		// 0x0
	uint32_t reserved :16;
} VCAP_NO_OP_CMD;

//CMD_VCAP_MCTF_ME_PROC
typedef struct VCAP_MCTF_ME_PROC_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;
	uint8_t chroma_format;  // 0 = 420, 1 = 422
	uint8_t use_capture_order_or_dram;
	uint32_t tar_luma_addr_or_cap_order;
	uint32_t tar_chroma_addr;
	uint32_t tar_me1_addr;
	uint32_t ref_luma_addr_or_cap_order;
	uint32_t ref_chroma_addr;
	uint32_t ref_me1_addr;
	uint32_t frame_width;
	uint32_t frame_pitch;
	uint32_t frame_height;
	uint32_t me1_width;
	uint32_t me1_pitch;
	uint32_t me1_height;
	uint32_t mctf_cfg_addr;
	uint16_t vertical_mv_max;
	uint16_t me_level_bypass;
	uint32_t ext_ME_init_value_addr;
	uint16_t mv_buf_width;
	uint16_t mv_buf_pitch;
	uint16_t mv_buf_height;
	uint32_t mctf_out_addr;
	uint16_t me_level_w_ext_init_pos;	// 0 means use internal defaults
	int16_t init_x_sr_pos;
	int16_t init_y_sr_pos;
	uint8_t me1_lambda;
	uint8_t me2_3_4_lambda;
	uint16_t horz_sr_range;			  // 0 means use internal default
	uint16_t vert_sr_range;			  // 0 means use internal default
} VCAP_MCTF_ME_PROC_CMD;

// CMD_VCAP_SET_MEM
typedef struct VCAP_SET_EXTERNAL_MEM_CMDtag {
	uint32_t cmd_code;
	uint8_t channel_id;
	uint8_t stream_type;

	uint16_t rawbuf_alloc_type :2;// 0=address of each buffer speficified individually. 1=buffers are contingous
	uint16_t yuvbuf_alloc_type :2;// 0=address of each buffer speficified individually. 1=buffers are contingous
	uint16_t me1buf_alloc_type :2;// 0=address of each buffer speficified individually. 1=buffers are contingous
	uint16_t reserved :10;

	uint16_t num_raw_buffers;
	uint16_t raw_buf_pitch;
	uint16_t raw_buf_width;
	uint16_t raw_buf_height;
	uint16_t num_yuv_buffers;
	uint16_t yuv_buf_pitch;
	uint16_t yuv_buf_width;
	uint16_t yuv_buf_height;
	uint16_t num_me1_buffers;
	uint16_t me1_buf_pitch;
	uint16_t me1_buf_width;
	uint16_t me1_buf_height;
	uint32_t addr_array[128 / 4 - 8];
} VCAP_SET_EXTERNAL_MEM_CMD;

// following commands have been obsolete in A7.
/// VCAP_FAST_AAA_CAP_CMD (cmd code 0x0800000A)
typedef struct fast_aaa_capture_s {
	uint32_t cmd_code;
	uint16_t input_image_width;
	uint16_t input_image_height;
	uint32_t start_record_id;
} fast_aaa_capture_t;

/// VCAP_TRANSCODE_SETUP_CMD (cmd code 0x0800000B)
typedef struct VCAP_TRANSCODE_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;

	// The next 8-bit bit map is used to indicate whether DSP
	// should use the corresponding parameters or not in the command
	// 0: not used, 1: used
	uint32_t bm_noise_filter_strength :1;
	uint32_t bm_look_ahead_buffer_depth :1;
	uint32_t bm_flow_ctrl_option :1;
	uint32_t bm_underflow_action_option :1;
	uint32_t reserved_0 :4;

	uint32_t reserved_1 :8;

	uint32_t noise_filter_strength :8;
	uint32_t look_ahead_buffer_depth :8;
	uint32_t flow_ctrl_option :4;
	uint32_t underflow_action_option :4;
	uint32_t reserved_2 :8;
} VCAP_TRANSCODE_SETUP_CMD;
typedef enum VCAP_UNDERFLOW_ACTtag { // for underflow_action_option
	UNFL_WAIT_DATA = 0,
	UNFL_REPEAT_FRM,
	UNFL_REPEAT_FLD,
	UNFL_INS_BLACK_PIC,
} VCAP_UNDERFLOW_ACT;

/// VCAP_TRANSCODE_CHAN_SW_CMD (cmd code 0x0800000C)
typedef struct VCAP_TRANSCODE_CHAN_SW_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;

	uint32_t new_input_channel :8;
	uint32_t new_input_stream_type :8;
	uint32_t switch_mode :8;
	uint32_t reserved_1 :8;

	uint32_t switch_params_0 :32;
	uint32_t switch_params_1 :32;
	uint32_t switch_params_2 :32;
	uint32_t switch_params_3 :32;
} VCAP_TRANSCODE_CHAN_SW_CMD;
typedef enum VCAP_XCODE_SW_MODEtag { // for switch_mode
	XCODE_SW_IMMEDIATELY = 0,
	XCODE_SW_VIN,
	XCODE_SW_DEC_TS,
} VCAP_XCODE_SW_MODE;

/// VCAP_TRANSCODE_CHAN_STOP_CMD (cmd code 0x0800000D)
typedef struct VCAP_TRANSCODE_CHAN_STOP_CMDtag {
	uint32_t cmd_code;

	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t reserved_0 :16;
} VCAP_TRANSCODE_CHAN_STOP_CMD;

/// VCAP_TRANSCODE_PREV_SETUP_CMD (cmd code 0x0800000E)
typedef struct VCAP_TRANSCODE_PREV_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t is_set_prev_a :1;	 // 0: no, 1: yes
	uint32_t is_set_prev_b :1;	 // 0: no, 1: yes
	uint32_t is_prev_a_interlaced :1;	 // 0: progressive, 1: interlaced
	uint32_t is_prev_b_interlaced :1;	 // 0: progressive, 1: interlaced
	uint32_t is_prev_a_vout_th_mode :1;	// 0: DRAM preview, 1: vout thread preview
	uint32_t is_prev_b_vout_th_mode :1;	// 0: DRAM preview, 1: vout thread preview
	uint32_t reserved_0 :2;

	uint32_t prev_a_src :4;
	uint32_t prev_b_src :4;

	uint32_t reserved_1 :16;

	uint32_t prev_a_chan_id :8;
	uint32_t prev_a_strm_tp :8;

	uint32_t prev_b_chan_id :8;
	uint32_t prev_b_strm_tp :8;

	uint32_t preview_w_A :16;
	uint32_t preview_h_A :16;
	uint32_t preview_w_B :16;
	uint32_t preview_h_B :16;
} VCAP_TRANSCODE_PREV_SETUP_CMD;

typedef enum VCAP_XCODE_PREV_SRCtag { // for prev_a_src & prev_b_src
	XCODE_PREV_SRC_DECODE = 0, ///< we can display decoded picture in VCAP
	XCODE_PREV_SRC_VCAP,
	XCODE_PREV_SRC_VCAP_PIP,
	XCODE_PREV_SRC_RECON,
} VCAP_XCODE_PREV_SRC;

/**
 * VCAP status msg (0x88000001)
 */

typedef struct VCAP_PREV_REPORTtag {
	uint32_t prev_b_src :3;
	uint32_t prev_b_stop :1;
	uint32_t prev_b_fps :8;
	uint32_t prev_b_interlace :1;
	uint32_t reserved :19;
	uint16_t prev_b_h;
	uint16_t prev_b_w;
} VCAP_PREV_REPORT;

typedef struct {
	uint32_t msg_code;

	uint32_t reserved :8;
	uint32_t mode :8;
	uint32_t state :8;
	uint32_t report_type :8;

	uint32_t raw_buf_dram_addr;

	uint32_t raw_buf_pitch :16;
	uint32_t raw_buf_width :16;

	uint32_t raw_buf_height :16;
	uint32_t hdr_cap_cnt :8;
	uint32_t num_vert_slices :8;

	uint32_t cfa_3a_stat_dram_addr;
	uint32_t yuv_3a_stat_dram_addr;

	uint32_t cfa_3a_stat_sz :16;
	uint32_t yuv_3a_stat_sz :16;

	uint32_t main_luma_addr;
	uint32_t main_chroma_addr;
	uint32_t main_dram_pitch;

	uint32_t prev_A_luma_addr;
	uint32_t prev_A_chroma_addr;

	uint32_t prev_B_luma_addr;
	uint32_t prev_B_chroma_addr;

	uint32_t prev_A_dram_pitch :16;
	uint32_t prev_B_dram_pitch :16;

	uint32_t thumbnail_luma_addr;
	uint32_t thumbnail_chroma_addr;

	uint32_t screennail_luma_addr;
	uint32_t screennail_chroma_addr;

	uint32_t thumbnail_dram_pitch :16;
	uint32_t screennail_dram_pitch :16;

	uint32_t collage_luma_addr;
	uint32_t collage_chroma_addr;
	uint32_t collage_dram_pitch :16;
	uint32_t collage_dram_height :16;

	uint16_t raw_cap_cnt;
	uint16_t yuv_pic_cnt;


	// ucode supports two types of auxiliary still commands
	// that are additional to the core low iso, high iso processing.
	// Examples are STILL_RAW2RAW, STILL_RESAMPLE_YUV, STILL_JPEG_ENCODE_YUV
	// there are two types of auxiliary commands. one type involves the
	// use of idsp/mctf pipeline. the other involces use of jpeg encoder
	// the following counters counts the number of each type of commands
	// processed.
	uint16_t num_isp_cmds_processed;
	uint16_t num_enc_cmds_processed;

	uint32_t cfa_3a_stat_dram_addr_right;
	uint32_t yuv_3a_stat_dram_addr_right;

	uint32_t mv_mvHist_addr;
	uint32_t mv_buf_addr;
	uint32_t mv_buf_sz;
} SCAP_STATUS_MSG;

typedef struct VCAP_STRM_REPORTtag {
	uint32_t channel_id :8;
	uint32_t stream_type :8;
	uint32_t stream_state :8; ///< use VCAP_STRM_STATE defined below
	uint32_t report_type :8;
	uint32_t raw_capture_dram_addr;
	uint32_t raw_cap_buf_pitch :16;
	uint32_t raw_cap_buf_width :16;
	uint32_t cfa_3a_stat_dram_addr;
	uint32_t yuv_3a_stat_dram_addr;
	uint32_t cfa_3a_stat_sz :16;
	uint32_t yuv_3a_stat_sz :16;
	uint32_t main_pict_luma_addr;
	uint32_t main_pict_chroma_addr;
	uint32_t main_pict_dram_pitch;
	uint32_t prev_A_luma_addr;
	uint32_t prev_A_chroma_addr;
	uint32_t prev_B_luma_addr;
	uint32_t prev_B_chroma_addr;
	uint32_t prev_A_dram_pitch :16;
	uint32_t prev_B_dram_pitch :16;
	uint32_t PIP_luma_addr;
	uint32_t PIP_chroma_addr;
	uint32_t piv_luma_addr;
	uint32_t piv_chroma_addr;
	uint32_t piv_dram_pitch :16;
	uint32_t pip_dram_pitch :16;
	uint32_t thumbnail_luma_addr;
	uint32_t thumbnail_chroma_addr;
	uint32_t screennail_luma_addr;
	uint32_t screennail_chroma_addr;
	uint32_t thumbnail_dram_pitch :16;
	uint32_t screennail_dram_pitch :16;
	uint32_t sw_pts;	// mctf out PTS, mostly it is used as YUV buffer PTS
	uint32_t raw_cap_buf_height :16;
	uint32_t hdr_cap_cnt :8;
	uint32_t reserved :8;
	uint32_t raw_cap_cnt;
	uint32_t yuv_pic_cnt;
} VCAP_STRM_REPORT;

typedef struct VCAP_STRM_REPORT_EXTtag {
	uint32_t main_pict_me1_addr;
	uint32_t prev_A_me1_addr;
	uint32_t prev_B_me1_addr;
	uint32_t prev_C_me1_addr;
	uint32_t main_pict_me1_pitch :16;
	uint32_t prev_A_me1_pitch :16;
	uint32_t prev_B_me1_pitch :16;
	uint32_t prev_C_me1_pitch :16;
	uint32_t sec2_out_sw_pts;   // sec2 out pts, mostly it is used as 3A pts and unwarped preview pts
	uint32_t vcap_audio_clk_counter;	/* used to sync audio clk counter with wall time clk */
	uint32_t hw_pts;
	uint32_t sec2_out_hw_pts;
	uint32_t raw_hw_pts;
	/* for special case where they want early access to the prev A addr */
	uint32_t prev_A_luma_addr_early;
	uint32_t prev_A_chroma_addr_early;

	/* prev_cp_pitch depends on which preview buffer it copies from
	 * (prev_A_dram_pitch / prev_B_dram_pitch / pip_dram_pitch)*/
	uint32_t prev_cp_luma_addr;
	uint32_t prev_cp_chroma_addr;
#ifdef DSP_DEBUG_FOR_ENC_DUMMY_LATENCY_LONG_GOP_REF
	uint32_t vcap_enc_cmd_updata_fail : 1;
	uint32_t reserved : 31;
#endif
} VCAP_STRM_REPORT_EXT;

typedef enum VCAP_STRM_STATEtag {
	VCAP_STRM_STA_IDLE = 0,
	VCAP_STRM_STA_PDATA = 1,
	VCAP_STRM_STA_PBUF = 2,
	VCAP_STRM_STA_BUSY = 3,
	VCAP_STRM_STA_FLUSH = 4,
} VCAP_STRM_STATE;

typedef struct VCAP_STATUS_MSGtag {
	uint32_t msg_code;

	// uint32_t reserved_0;

	uint8_t vcap_setup_mode;	///< use VCAP_SETUP_MODE defined below
	uint8_t num_of_reports;	 ///< number of stream reports in this message
	uint16_t idsp_err_code       :1;
	uint16_t reserved_1          :15;

	// uint32_t reserved_2;
	// uint32_t reserved_3;

	VCAP_STRM_REPORT strm_reports;   // 24 bytes per report
	VCAP_PREV_REPORT prev_report;
} VCAP_STATUS_MSG;

typedef struct VCAP_STATUS_EXT_MSGtag {
	uint32_t msg_code;
	VCAP_STRM_REPORT_EXT strm_report_ext;
} VCAP_STATUS_EXT_MSG;

/// VCAP "SETUP" modes
typedef enum VCAP_SETUP_MODEtag {
	VCAP_RESET_MODE = 0,	///< actually we won't use this mode in code

	// VCAP_IDLE_MODE	  = 10,  ///< idle mode and timer mode are treated as the same
	VCAP_TIMER_MODE = 10,  ///< timer mode is the idle mode


	VCAP_VIDEO_MODE = 20,  ///< video (recording) mode
	VCAP_RJPEG_MODE = 30,  ///< raw JPEG mode
	VCAP_FAST3A_MODE = 40,  ///< fast AAA mode
	VCAP_CALIB_MODE = 50,  ///< calibration mode
	VCAP_TRANSCODE_MODE = 60,  ///< transcode mode
} VCAP_SETUP_MODE;

/************************************************************
 * IDSP commands (Category 9)
 */

/**
 * Ser Vin Capture windows (ARCH_VER >= 3) (0x09001001)
 */
typedef struct vin_common_s {
	uint16_t s_ctrl_reg;
	uint16_t s_inp_cfg_reg;
	uint16_t reserved;
	uint16_t s_v_width_reg;
	uint16_t s_h_width_reg;
	uint16_t s_h_offset_top_reg;
	uint16_t s_h_offset_bot_reg;
	uint16_t s_v_reg;
	uint16_t s_h_reg;
	uint16_t s_min_v_reg;
	uint16_t s_min_h_reg;
	uint16_t s_trigger_0_start_reg;
	uint16_t s_trigger_0_end_reg;
	uint16_t s_trigger_1_start_reg;
	uint16_t s_trigger_1_end_reg;
	uint16_t s_vout_start_0_reg;
	uint16_t s_vout_start_1_reg;
	uint16_t s_cap_start_v_reg;
	uint16_t s_cap_start_h_reg;
	uint16_t s_cap_end_v_reg;
	uint16_t s_cap_end_h_reg;
	uint16_t s_blank_leng_h_reg;
	uint32_t vsync_timeout;
	uint32_t hsync_timeout;
} vin_common_t;

typedef struct vin_cap_win_s {
	uint32_t cmd_code;
	uint16_t s_ctrl_reg;
	uint16_t s_inp_cfg_reg;
	uint16_t reserved;
	uint16_t s_v_width_reg;
	uint16_t s_h_width_reg;
	uint16_t s_h_offset_top_reg;
	uint16_t s_h_offset_bot_reg;
	uint16_t s_v_reg;
	uint16_t s_h_reg;
	uint16_t s_min_v_reg;
	uint16_t s_min_h_reg;
	uint16_t s_trigger_0_start_reg;
	uint16_t s_trigger_0_end_reg;
	uint16_t s_trigger_1_start_reg;
	uint16_t s_trigger_1_end_reg;
	uint16_t s_vout_start_0_reg;
	uint16_t s_vout_start_1_reg;
	uint16_t s_cap_start_v_reg;
	uint16_t s_cap_start_h_reg;
	uint16_t s_cap_end_v_reg;
	uint16_t s_cap_end_h_reg;
	uint16_t s_blank_leng_h_reg;
	uint32_t vsync_timeout;
	uint32_t hsync_timeout;

	uint16_t mipi_cfg1_reg;
	uint16_t mipi_cfg2_reg;
	uint16_t mipi_bdphyctl_reg;
	uint16_t mipi_sdphyctl_reg;
	uint16_t reserved_0[2];

	//for SLVDS control
	uint16_t slvs_control;
	uint16_t slvs_frame_line_w;
	uint16_t slvs_act_frame_line_w;
	uint16_t slvs_vsync_max;
	uint16_t slvs_lane_mux_select[4];
	uint16_t slvs_status;
	uint16_t slvs_line_sync_timeout;
	uint16_t slvs_debug;

	u32 extra_vin_setup_addr[4];
	u32 rawH;
	u32 prog_opt :4;
	u32 num_extra_vin :4;
	u32 reserved_1 :24;

	u8 is_turbo_ena;
} vin_cap_win_t;
typedef vin_cap_win_t VIN_SETUP_CMD;

/**
 * Sensor Input setup (0x09001002)
 */
typedef struct sensor_input_setup_s {
	uint32_t cmd_code;
	uint8_t sensor_id;
	uint8_t field_format;
	uint8_t sensor_resolution;
	uint8_t sensor_pattern;
	uint8_t first_line_field_0;
	uint8_t first_line_field_1;
	uint8_t first_line_field_2;
	uint8_t first_line_field_3;
	uint8_t first_line_field_4;
	uint8_t first_line_field_5;
	uint8_t first_line_field_6;
	uint8_t first_line_field_7;
	uint32_t sensor_readout_mode;
} sensor_input_setup_t;

/**
 * Amplifier linearization (0x09001003)
 */
typedef struct amplifier_linear_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t look_up_table_addr;
	uint32_t exponent_table_addr;
	uint32_t black_level_offset_table_addr;
} amplifier_linear_t;

/**
 * Pixel Shuffle (0x09001004)
 */
typedef struct pixel_shuffle_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t reorder_mode;
	uint32_t input_width;
	uint16_t start_index[8];
	uint16_t pitch[8];
} pixel_shuffle_t;

/**
 * Fixed pattern noise correction (0x09001005)
 */
typedef struct fixed_pattern_noise_correct_s {
	uint32_t cmd_code;
	uint32_t fpn_pixel_mode;
	uint32_t row_gain_enable;
	uint32_t column_gain_enable;
	uint32_t num_of_rows;
	uint16_t num_of_cols;
	uint16_t fpn_pitch;
	uint32_t fpn_pixels_addr;
	uint32_t fpn_pixels_buf_size;
	uint32_t intercept_shift;
	uint32_t intercepts_and_slopes_addr;
	uint32_t row_gain_addr;
	uint32_t column_gain_addr;
} fixed_pattern_noise_correct_t;

/**
 * CFA Domain Leakage Filter setup (0x09001006)
 */
typedef struct cfa_domain_leakage_filter_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint8_t alpha_rr;
	uint8_t alpha_rb;
	uint8_t alpha_br;
	uint8_t alpha_bb;
	uint16_t saturation_level;
} cfa_domain_leakage_filter_t;

/**
 * Anti-Aliasing Filter setup (0x09001007)
 */
typedef struct anti_aliasing_filter_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t threshold;
	uint32_t shift;
} anti_aliasing_filter_t;

/**
 * Set VIN Configuration (0x09001008)
 */
typedef struct set_vin_config_s {
	uint32_t cmd_code;
	uint16_t vin_width;
	uint16_t vin_height;
	uint32_t vin_config_dram_addr;
	uint16_t config_data_size;
	uint8_t sensor_resolution;
	uint8_t sensor_bayer_pattern;
	uint32_t vin_set_dly;
	uint8_t vin_enhance_mode;   // valid only on A7S
	uint8_t vin_panasonic_mode; // valid only on A7S
	uint8_t reserved[2];
	uint16_t vin_cap_start_x;	// exclude sensor def cap start x
	uint16_t vin_cap_start_y;	// exclude sensor def cap start y
} set_vin_config_t;

/**
 * Set VIN Rolling Shutter Configuration (0x09001009)
 */
typedef struct set_vin_rolling_shutter_config_s {
	uint32_t cmd_code;
	uint32_t rolling_shutter_config_dram_addr; // 0x28
	uint16_t vertical_enable :1;				// 0x2A
	uint16_t horizontal_enable :1;
	uint16_t arm_interrupt_enable :1;
	uint16_t register_config_mask :1;
	uint16_t use_software_sample :1;
	uint16_t reserved :11;
	uint16_t interrupt_delay;				  // 0x2B
	uint16_t horizontal_software_gyro_sample;
	uint16_t horizontal_offset;
	uint16_t horizontal_scale;
	uint16_t vertical_software_gyro_sample;
	uint16_t vertical_offset;
	uint16_t vertical_scale;				   // 0x31
} set_vin_rolling_shutter_config_t;

/**
 * Set VIN Capture Window Extension (0x0900100A)
 */
typedef struct vin_cap_win_ext_s {
	uint32_t cmd_code;
	uint8_t enhance_mode;
	uint8_t syncmap_mode;

	uint16_t s_slvs_ctrl_0;
	uint16_t s_slvs_line_width;
	uint16_t s_slvs_active_width;
	uint16_t s_slvs_vsync_max;

	uint16_t s_slvs_lane_mux_sel_0;
	uint16_t s_slvs_lane_mux_sel_1;
	uint16_t s_slvs_lane_mux_sel_2;
	uint16_t s_slvs_lane_mux_sel_3;

	uint16_t s_slvs_line_sync_timeout;

	uint32_t vin_interrupt_start;

	uint16_t s_active_height_min;
	uint16_t s_active_width_min;

	uint32_t controller_trigger_start;

	uint16_t s_slvs_ctrl_1;

	uint16_t s_slvs_sav_vzero_map;
	uint16_t s_slvs_sav_vone_map;
	uint16_t s_slvs_eav_vzero_map;
	uint16_t s_slvs_eav_vone_map;

	uint16_t s_slvs_vsync_horizontal_start;
	uint16_t s_slvs_vsync_horizontal_end;

	uint16_t reserved;
} vin_cap_win_ext_t;

/**
 * Black Level Global Offset (0x09002001)
 */
typedef struct black_level_global_offset_s {
	uint32_t cmd_code;
	uint32_t global_offset_ee;
	uint32_t global_offset_eo;
	uint32_t global_offset_oe;
	uint32_t global_offset_oo;
	uint16_t black_level_offset_red;
	uint16_t black_level_offset_green;
	uint16_t black_level_offset_blue;
	uint16_t gain_depedent_offset_red;
	uint16_t gain_depedent_offset_green;
	uint16_t gain_depedent_offset_blue;
	uint16_t def_blk_red;
	uint16_t def_blk_green;
	uint16_t def_blk_blue;
	uint16_t bayer_pattern : 2;
	uint16_t reserved : 14;
} black_level_global_offset_t;

/**
 * Black level correction (0x09002002)
 */
typedef struct black_level_correcttion_s {
	uint32_t cmd_code;
	uint32_t column_enable;
	uint32_t row_enable;
	uint32_t black_level_mode;
	uint32_t bad_pixel_mode_column;
	uint32_t bad_pixel_mode_row;
	uint32_t cold_pixel_thresh;
	uint32_t hot_pixel_thresh;
	uint32_t center_offset;
	uint32_t column_replace;
	uint32_t column_invalid_replace;
	uint32_t column_invalid_thresh;
	uint32_t row_invalid_thresh;
	uint32_t column_average_k_frames;
	uint32_t row_average_k_frames;
	uint32_t column_black_gain;
	uint32_t row_black_gain;
	uint32_t column_bad_pixel_subtract;
	uint32_t global_offset_ee;
	uint32_t global_offset_eo;
	uint32_t global_offset_oe;
	uint32_t global_offset_oo;
} black_level_correcttion_t;

/**
 * Black level state tables (0x09002003)
 */
typedef struct black_level_state_table_s {
	uint32_t cmd_code;
	uint32_t num_columns;
	uint32_t column_frame_acc_addr;
	uint32_t column_average_acc_addr;
	uint32_t num_rows;
	uint32_t row_fixed_offset_addr;
	uint32_t row_average_acc_addr;
} black_level_state_table_t;

/**
 * Black level detection window (0x09002004)
 */
typedef struct black_level_detect_win_s {
	uint32_t cmd_code;
	uint8_t top_black_present;
	uint8_t bottom_black_present;
	uint8_t left_black_present;
	uint8_t right_black_present;
	uint32_t top_black_start;
	uint32_t top_black_end;
	uint32_t bottom_black_start;
	uint32_t bottom_black_end;
	uint32_t left_black_start;
	uint32_t left_black_end;
	uint32_t right_black_start;
	uint32_t right_black_end;
} black_level_detect_win_t;

/**
 * RGB gain adjustment (0x09002005)
 */
typedef struct rgb_gain_adjust_s {
	uint32_t cmd_code;
	uint32_t r_gain;
	uint32_t g_even_gain;
	uint32_t g_odd_gain;
	uint32_t b_gain;
} rgb_gain_adjust_t;

/**
 * Digital gain saturation level (0x09002006)
 */
typedef struct digital_gain_level_s {
	uint32_t cmd_code;
	uint32_t level_red;
	uint32_t level_green_even;
	uint32_t level_green_odd;
	uint32_t level_blue;
} digital_gain_level_t;

/**
 * Vignette compensation (0x09002007)
 */
typedef struct vignette_compensation_s {
	uint32_t cmd_code;
	uint8_t enable;
	uint8_t gain_shift;
	uint16_t reserved;
	uint32_t tile_gain_addr;
	uint32_t tile_gain_addr_green_even;
	uint32_t tile_gain_addr_green_odd;
	uint32_t tile_gain_addr_blue;

	uint8_t comp_shift0;
	uint8_t comp_shift1;
	uint8_t comp_shift2;
	uint8_t comp_shift3;
} vignette_compensation_t;

/**
 * Local exposure (0x09002008)
 */
typedef struct local_exposure_s {
	uint32_t cmd_code;
	uint8_t group_index;
	uint8_t enable;
	uint16_t radius;
	uint8_t luma_weight_red;
	uint8_t luma_weight_green;
	uint8_t luma_weight_blue;
	uint8_t luma_weight_sum_shift;
	uint32_t gain_curve_table_addr;
	uint16_t luma_offset;
	uint16_t reserved;
} local_exposure_t;

/**
 * Color correction (0x09002009)
 */
typedef struct color_correction_s {
	uint32_t cmd_code;
	uint32_t group_index;
	uint8_t enable;
	uint8_t no_interpolation;
	uint8_t yuv422_foramt;
	uint8_t uv_center;
	uint32_t multi_red;
	uint32_t multi_green;
	uint32_t multi_blue;
	uint32_t in_lookup_table_addr;
	uint32_t matrix_addr;
	uint32_t output_lookup_bypass;
	uint32_t out_lookup_table_addr;
} color_correction_t;

/**
 * RGB to YUV setup (0x0900200A)
 */
typedef struct rgb_to_yuv_stup_s {
	uint32_t cmd_code;
	uint32_t group_index;
	uint16_t matrix_values[9];
	int16_t y_offset;
	int16_t u_offset;
	int16_t v_offset;
} rgb_to_yuv_setup_t;

/**
 * Chroma scale (0x0900200B)
 */
typedef struct chroma_scale_s {
	uint32_t cmd_code;
	uint16_t group_index;
	uint16_t enable;
	uint32_t make_legal;
	int16_t u_weight_0;
	int16_t u_weight_1;
	int16_t u_weight_2;
	int16_t v_weight_0;
	int16_t v_weight_1;
	int16_t v_weight_2;
	uint32_t gain_curver_addr;
} chroma_scale_t;

/**
 * Bad pixel correct setup (0x09003001)
 */
typedef struct bad_pixel_correct_setup {
	uint32_t cmd_code;
	uint8_t dynamic_bad_pixel_detection_mode;
	uint8_t dynamic_bad_pixel_correction_method;
	uint16_t correction_mode;
	uint32_t hot_pixel_thresh_addr;
	uint32_t dark_pixel_thresh_addr;
	uint16_t hot_shift0_4;
	uint16_t hot_shift5;
	uint16_t dark_shift0_4;
	uint16_t dark_shift5;
} bad_pixel_correct_setup_t;

/**
 * CFA noise filter (0x09003002)
 */
typedef struct cfa_noise_filter_s {
	uint32_t cmd_code;
	uint8_t enable :2;
	uint8_t mode :2;
	uint8_t shift_coarse_ring1 :2;
	uint8_t shift_coarse_ring2 :2;
	uint8_t shift_fine_ring1 :2;
	uint8_t shift_fine_ring2 :2;
	uint8_t shift_center_red :4;
	uint8_t shift_center_green :4;
	uint8_t shift_center_blue :4;
	uint8_t target_coarse_red;
	uint8_t target_coarse_green;
	uint8_t target_coarse_blue;
	uint8_t target_fine_red;
	uint8_t target_fine_green;
	uint8_t target_fine_blue;
	uint8_t cutoff_red;
	uint8_t cutoff_green;
	uint8_t cutoff_blue;
	uint16_t thresh_coarse_red;
	uint16_t thresh_coarse_green;
	uint16_t thresh_coarse_blue;
	uint16_t thresh_fine_red;
	uint16_t thresh_fine_green;
	uint16_t thresh_fine_blue;
} cfa_noise_filter_t;

/**
 * Demoasic Filter (0x09003003)
 */
typedef struct demoasic_filter_s {
	uint32_t cmd_code;
	uint8_t group_index;
	uint8_t enable;
	uint8_t clamp_directional_candidates;
	uint8_t activity_thresh;
	uint16_t activity_difference_thresh;
	uint16_t grad_clip_thresh;
	uint16_t grad_noise_thresh;
	uint16_t grad_noise_difference_thresh;
	uint16_t zipper_noise_difference_add_thresh;
	uint8_t zipper_noise_difference_mult_thresh;
	uint8_t max_const_hue_factor;
} demoasic_filter_t;

/**
 * Chroma noise filter (0x09003004)
 */
typedef struct chroma_noise_filter_s {
	u32 cmd_code;
	u8 enable;
	u8 radius;
	u16 mode;
	u16 thresh_u;
	u16 thresh_v;
	u16 shift_center_u;
	u16 shift_center_v;
	u16 shift_ring1;
	u16 shift_ring2;
	u16 target_u;
	u16 target_v;
} chroma_noise_filter_t;

/**
 * Strong GrGb mismatch filter (0x09003005)
 */
typedef struct strong_grgb_mismatch_filter_s {
	u32 cmd_code;
	u16 enable_correction;
	u16 grgb_mismatch_wide_thresh;
	u16 grgb_mismatch_reject_thresh;
	u16 reserved;
} strong_grgb_mismatch_filter_t;

/**
 * Chroma media filter (0x09003006)
 */
typedef struct chroma_median_filter_info_s {
	uint32_t cmd_code;
	uint16_t group_index;
	uint16_t enable;
	uint32_t k0123_table_addr;
	uint16_t u_sat_t0;
	uint16_t u_sat_t1;
	uint16_t v_sat_t0;
	uint16_t v_sat_t1;
	uint16_t u_act_t0;
	uint16_t u_act_t1;
	uint16_t v_act_t0;
	uint16_t v_act_t1;
} chroma_median_filter_info_t;

/**
 * Luma sharpening (0x09003007)
 */
typedef struct luma_sharpening_s {
	uint32_t cmd_code;
	uint8_t group_index :4;
	uint8_t enable :1;
	uint8_t use_generated_low_pass :3;

	uint8_t input_B_enable :1;
	uint8_t input_C_enable :1;
	uint8_t FIRs_input_from_B_minus_C :1;
	uint8_t coring1_input_from_B_minus_C :1;
	uint8_t abs :1;
	uint8_t yuv :2;
	uint8_t reserved :1;

	uint8_t clip_low;
	uint8_t clip_high;

	uint8_t max_change_down;
	uint8_t max_change_up;
	uint8_t max_change_down_center;
	uint8_t max_change_up_center;

	// alpha control
	uint8_t grad_thresh_0;
	uint8_t grad_thresh_1;
	uint8_t smooth_shift;
	uint8_t edge_shift;
	uint32_t alpha_table_addr;

	// edge control
	uint8_t wide_weight :4;
	uint8_t narrow_weight :4;
	uint8_t edge_threshold_multiplier;
	uint16_t edge_thresh;
} luma_sharpening_t;

/**
 * Luma Sharpening FIR Config (0x09003009)
 */
typedef struct luma_sharpening_FIR_config_s {
	uint32_t cmd_code;
	uint8_t group_index;
	uint8_t enable_FIR1;
	uint8_t enable_FIR2 :4;
	uint8_t add_in_non_alpha1 :4;
	uint8_t add_in_alpha1 :4;
	uint8_t add_in_alpha2 :4;
	uint16_t fir1_clip_low;
	uint16_t fir1_clip_high;
	uint16_t fir2_clip_low;
	uint16_t fir2_clip_high;
	uint32_t coeff_FIR1_addr;
	uint32_t coeff_FIR2_addr;
	uint32_t coring_table_addr;
} luma_sharpening_FIR_config_t;

/**
 * Luma Sharpening Low Noise Luma (0x0900300A)
 */
typedef struct luma_sharpening_LNL_s {
	uint32_t cmd_code;
	uint8_t group_index;
	uint8_t enable :1;
	uint8_t output_normal_luma_size_select :1;
	uint8_t output_low_noise_luma_size_select :1;
	uint8_t reserved :5;
	uint8_t input_weight_red;
	uint8_t input_weight_green;
	uint8_t input_weight_blue;
	uint8_t input_shift_red;
	uint8_t input_shift_green;
	uint8_t input_shift_blue;
	uint16_t input_clip_red;
	uint16_t input_clip_green;
	uint16_t input_clip_blue;
	uint16_t input_offset_red;
	uint16_t input_offset_green;
	uint16_t input_offset_blue;
	uint8_t output_normal_luma_weight_a;
	uint8_t output_normal_luma_weight_b;
	uint8_t output_normal_luma_weight_c;
	uint8_t output_low_noise_luma_weight_a;
	uint8_t output_low_noise_luma_weight_b;
	uint8_t output_low_noise_luma_weight_c;
	uint8_t output_combination_min;
	uint8_t output_combination_max;
	uint32_t tone_curve_addr;
} luma_sharpening_LNL_t;

/**
 * Luma Sharpening Tone Control (0x0900300B)
 */
typedef struct luma_sharpening_tone_control_s {
	uint32_t cmd_code;
	uint8_t group_index :4;
	uint8_t shift_y :4;
	uint8_t shift_u :4;
	uint8_t shift_v :4;
	uint8_t offset_y;
	uint8_t offset_u;
	uint8_t offset_v;
	uint8_t bits_u :4;
	uint8_t bits_v :4;
	uint16_t reserved;
	uint32_t tone_based_3d_level_table_addr;
} luma_sharpening_tone_control_t;

/**
 * Luma Sharpening Blend Config (0x0900300C)
 */
typedef struct luma_sharpening_blend_control_s {
	uint32_t cmd_code;
	uint32_t group_index;
	uint16_t enable;
	uint8_t edge_threshold_multiplier;
	uint8_t iso_threshold_multiplier;
	uint16_t edge_threshold0;
	uint16_t edge_threshold1;
	uint16_t dir_threshold0;
	uint16_t dir_threshold1;
	uint16_t iso_threshold0;
	uint16_t iso_threshold1;
} luma_sharpening_blend_control_t;

/**
 * Luma Sharpening Level Control (0x0900300D)
 */
typedef struct luma_sharpening_level_control_s {
	uint32_t cmd_code;
	uint32_t group_index;
	uint32_t select;
	uint8_t low;
	uint8_t low_0;
	uint8_t low_delta;
	uint8_t low_val;
	uint8_t high;
	uint8_t high_0;
	uint8_t high_delta;
	uint8_t high_val;
	uint8_t base_val;
	uint8_t area;
	uint16_t level_control_clip_low;
	uint16_t level_control_clip_low2;
	uint16_t level_control_clip_high;
	uint16_t level_control_clip_high2;
} luma_sharpening_level_control_t;

/**
 * AAA statistics setup (0x09004001)
 */
typedef struct aaa_statistics_setup_s {
	uint32_t cmd_code;
	uint32_t on :8;
	uint32_t auto_shift :8;
	uint32_t reserved :16;
	uint32_t data_fifo_base;
	uint32_t data_fifo_limit;
	uint32_t data_fifo2_base;
	uint32_t data_fifo2_limit;
	uint16_t awb_tile_num_col;
	uint16_t awb_tile_num_row;
	uint16_t awb_tile_col_start;
	uint16_t awb_tile_row_start;
	uint16_t awb_tile_width;
	uint16_t awb_tile_height;
	uint16_t awb_tile_active_width;
	uint16_t awb_tile_active_height;
	uint16_t awb_pix_min_value;
	uint16_t awb_pix_max_value;
	uint16_t ae_tile_num_col;
	uint16_t ae_tile_num_row;
	uint16_t ae_tile_col_start;
	uint16_t ae_tile_row_start;
	uint16_t ae_tile_width;
	uint16_t ae_tile_height;
	uint16_t ae_pix_min_value;
	uint16_t ae_pix_max_value;
	uint16_t af_tile_num_col;
	uint16_t af_tile_num_row;
	uint16_t af_tile_col_start;
	uint16_t af_tile_row_start;
	uint16_t af_tile_width;
	uint16_t af_tile_height;
	uint16_t af_tile_active_width;
	uint16_t af_tile_active_height;
} aaa_statistics_setup_t;

/**
 * AAA pseudo Y setup (0x09004002)
 */
typedef struct aaa_pseudo_y_s {
	uint32_t cmd_code;
	uint32_t mode;
	uint32_t sum_shift;
	uint8_t pixel_weight[4];
	uint8_t tone_curve[32];
} aaa_pseudo_y_t;

#define AAA_FILTER_SELECT_BOTH 0
#define AAA_FILTER_SELECT_CFA  1
#define AAA_FILTER_SELECT_YUV  2

/**
 * AAA histogram setup (0x09004003)
 */
typedef struct aaa_histogram_s {
	uint32_t cmd_code;
	uint16_t mode;
	uint16_t histogram_select;
	uint16_t ae_file_mask[8];
} aaa_histogram_t;

/**
 * AAA statistics setup 1 (0x09004004)
 */

typedef struct aaa_statistics_setup1_s {
	uint32_t cmd_code;
	uint8_t af_horizontal_filter1_mode :4;
	uint8_t af_horizontal_filter1_select :4;
	uint8_t af_horizontal_filter1_stage1_enb;
	uint8_t af_horizontal_filter1_stage2_enb;
	uint8_t af_horizontal_filter1_stage3_enb;
	uint16_t af_horizontal_filter1_gain[7];
	uint16_t af_horizontal_filter1_shift[4];
	uint16_t af_horizontal_filter1_bias_off;
	uint16_t af_horizontal_filter1_thresh;
	uint16_t af_vertical_filter1_thresh;
	uint16_t af_tile_fv1_horizontal_shift;
	uint16_t af_tile_fv1_vertical_shift;
	uint16_t af_tile_fv1_horizontal_weight;
	uint16_t af_tile_fv1_vertical_weight;
} aaa_statistics_setup1_t;

/**
 * AAA statistics setup 2 (0x09004005)
 */
typedef struct aaa_statistics_setup2_s {
	uint32_t cmd_code;
	uint8_t af_horizontal_filter2_mode :4;
	uint8_t af_horizontal_filter2_select :4;
	uint8_t af_horizontal_filter2_stage1_enb;
	uint8_t af_horizontal_filter2_stage2_enb;
	uint8_t af_horizontal_filter2_stage3_enb;
	uint16_t af_horizontal_filter2_gain[7];
	uint16_t af_horizontal_filter2_shift[4];
	uint16_t af_horizontal_filter2_bias_off;
	uint16_t af_horizontal_filter2_thresh;
	uint16_t af_vertical_filter2_thresh;
	uint16_t af_tile_fv2_horizontal_shift;
	uint16_t af_tile_fv2_vertical_shift;
	uint16_t af_tile_fv2_horizontal_weight;
	uint16_t af_tile_fv2_vertical_weight;
} aaa_statistics_setup2_t;

/**
 * AAA statistics setup 3 (0x09004006)
 */
typedef struct aaa_statistics_setup3_s {
	uint32_t cmd_code;
	uint16_t awb_tile_rgb_shift;
	uint16_t awb_tile_y_shift;
	uint16_t awb_tile_min_max_shift;
	uint16_t ae_tile_y_shift;
	uint16_t ae_tile_linear_y_shift;
	uint16_t af_tile_cfa_y_shift;
	uint16_t af_tile_y_shift;
} aaa_statistics_setup3_t;

/*
 * AAA early WB gain (0x09004007)
 */
typedef struct aaa_early_wb_gain_s {
	uint32_t cmd_code;
	uint32_t red_multiplier;
	uint32_t green_multiplier_even;
	uint32_t green_multiplier_odd;
	uint32_t blue_multiplier;
	uint8_t enable_ae_wb_gain;
	uint8_t enable_af_wb_gain;
	uint8_t enable_histogram_wb_gain;
	uint8_t reserved;
	uint32_t red_wb_multiplier;
	uint32_t green_wb_multiplier_even;
	uint32_t green_wb_multiplier_odd;
	uint32_t blue_wb_multiplier;
} aaa_early_wb_gain_t;

/*
 * AAA floating tile configuration (0x09004008)
 */
typedef struct aaa_floating_tile_config_s {
	uint32_t cmd_code;
	uint16_t frame_sync_id;
	uint16_t number_of_tiles;
	uint32_t floating_tile_config_addr;
} aaa_floating_tile_config_t;

/*
 * AAA tile configuration (internal use only)
 */
typedef struct aaa_tile_config_s {
	// 1 u32
	uint16_t awb_tile_col_start;
	uint16_t awb_tile_row_start;
	// 2 u32
	uint16_t awb_tile_width;
	uint16_t awb_tile_height;
	// 3 u32
	uint16_t awb_tile_active_width;
	uint16_t awb_tile_active_height;
	// 4 u32
	uint16_t awb_rgb_shift;
	uint16_t awb_y_shift;
	// 5 u32
	uint16_t awb_min_max_shift;
	uint16_t ae_tile_col_start;
	// 6 u32
	uint16_t ae_tile_row_start;
	uint16_t ae_tile_width;
	// 7 u32
	uint16_t ae_tile_height;
	uint16_t ae_y_shift;
	// 8 u32
	uint16_t ae_linear_y_shift;
	uint16_t af_tile_col_start;
	// 9 u32
	uint16_t af_tile_row_start;
	uint16_t af_tile_width;
	// 10 u32
	uint16_t af_tile_height;
	uint16_t af_tile_active_width;
	// 11 u32
	uint16_t af_tile_active_height;
	uint16_t af_y_shift;
	// 12 u32
	uint16_t af_cfa_y_shift;
	uint8_t awb_tile_num_col;
	uint8_t awb_tile_num_row;
	// 13 u32
	uint8_t ae_tile_num_col;
	uint8_t ae_tile_num_row;
	uint8_t af_tile_num_col;
	uint8_t af_tile_num_row;
	// 14 u32
	uint8_t total_slices_x;
	uint8_t total_slices_y;
	uint8_t slice_index_x;
	uint8_t slice_index_y;
	// 15 u32
	uint16_t slice_width;
	uint16_t slice_height;
	// 16 u32
	uint16_t slice_start_x;
	uint16_t slice_start_y;
	// 17 u32
	uint8_t total_exposures;
	uint8_t exposure_index;
	uint8_t total_channel_num;
	uint8_t channel_idx;
} aaa_tile_config_t;

/**
 * Noise filter setup (0x09005001)
 */
typedef struct noise_filter_setup_s {
	uint32_t cmd_code;
	uint32_t strength;
} noise_filter_setup_t;

/**
 * Raw compression (0x09005002)
 */
typedef struct raw_compression_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t lossy_mode;
	uint32_t vic_mode;
} raw_compression_t;

/**
 * Raw decompression (0x09005003)
 */
typedef struct raw_decompression_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t lossy_mode;
	uint32_t vic_mode;
	uint32_t image_width;
	uint32_t image_height;
} raw_decompression_t;

/**
 * Rolling shutter compensation (0x09005004)
 */
typedef struct rolling_shutter_compensation_s {
	uint32_t cmd_code;
	uint32_t enable;
	uint32_t skew_init_phase_horizontal;
	uint32_t skew_phase_incre_horizontal;
	uint32_t skew_phase_incre_vertical;
} rolling_shutter_compensation_t;

/**
 * FPN Calibration (0x09005005)
 */
typedef struct fpn_calibration_s {
	uint32_t cmd_code;
	uint32_t dram_addr;
	uint32_t width;
	uint32_t height;
	uint32_t num_of_frames;
} fpn_calibration_t;

/**
 * HDR Mixer (0x09005006)
 */
typedef struct hdr_mixer_s {
	uint32_t cmd_code;
	uint32_t mixer_mode;
	uint8_t radius;
	uint8_t luma_weight_red;
	uint8_t luma_weight_green;
	uint8_t luma_weight_blue;
	uint16_t threshold;
	uint8_t thresh_delta;
	uint8_t long_exposure_shift;
	uint16_t luma_offset;
} hdr_mixer_t;

/*
 * early WB gain (0x09005007)
 */
typedef struct early_wb_gain_s {
	uint32_t cmd_code;
	uint32_t cfa_early_red_multiplier;
	uint32_t cfa_early_green_multiplier_even;
	uint32_t cfa_early_green_multiplier_odd;
	uint32_t cfa_early_blue_multiplier;
	uint32_t aaa_early_red_multiplier;
	uint32_t aaa_early_green_multiplier_even;
	uint32_t aaa_early_green_multiplier_odd;
	uint32_t aaa_early_blue_multiplier;
} early_wb_gain_t;

/*
 * Set Warp Control (0x09005008)
 */
typedef struct set_warp_control_s {
#define	WARP_CONTROL_DISABLE	0
#define	WARP_CONTROL_ENABLE	1
	uint32_t cmd_code;
	uint32_t warp_control;
	uint32_t warp_horizontal_table_address;
	uint32_t warp_vertical_table_address;
	uint32_t actual_left_top_x;
	uint32_t actual_left_top_y;
	uint32_t actual_right_bot_x;
	uint32_t actual_right_bot_y;
	uint32_t zoom_x;
	uint32_t zoom_y;
	uint32_t x_center_offset;
	uint32_t y_center_offset;

	uint8_t grid_array_width;
	uint8_t grid_array_height;
	uint8_t horz_grid_spacing_exponent;
	uint8_t vert_grid_spacing_exponent;

	uint8_t vert_warp_enable;
	uint8_t vert_warp_grid_array_width;
	uint8_t vert_warp_grid_array_height;
	uint8_t vert_warp_horz_grid_spacing_exponent;

	uint8_t vert_warp_vert_grid_spacing_exponent;
	uint8_t force_v4tap_disable;
	uint16_t reserved_2;
	int hor_skew_phase_inc;

	/*
	 * This one is used for ARM to calcuate the
	 * dummy window for Ucode, these fields should be
	 * zero for turbo command in case of EIS. could be
	 * non-zero valid value only when this warp command is send
	 * in non-turbo command way.
	 */

	uint16_t dummy_window_x_left;
	uint16_t dummy_window_y_top;
	uint16_t dummy_window_width;
	uint16_t dummy_window_height;

	/*
	 * This field is used for ARM to calculate the
	 * cfa prescaler zoom factor which will affect
	 * the warping table value. this should also be zero
	 * during the turbo command sending.Only valid on the
	 * non-turbo command time.
	 */
	uint16_t cfa_output_width;
	uint16_t cfa_output_height;
	uint32_t extra_sec2_vert_out_vid_mode;
} set_warp_control_t;

/*
 * Set Chromatic Aberration Warp Control (0x09005009)
 */
typedef struct set_chroma_aberration_warp_control_s {
	uint32_t cmd_code;
	uint16_t horz_warp_enable;
	uint16_t vert_warp_enable;

	uint8_t horz_pass_grid_array_width;
	uint8_t horz_pass_grid_array_height;
	uint8_t horz_pass_horz_grid_spacing_exponent;
	uint8_t horz_pass_vert_grid_spacing_exponent;
	uint8_t vert_pass_grid_array_width;
	uint8_t vert_pass_grid_array_height;
	uint8_t vert_pass_horz_grid_spacing_exponent;
	uint8_t vert_pass_vert_grid_spacing_exponent;

	uint16_t red_scale_factor;
	uint16_t blue_scale_factor;
	uint32_t warp_horizontal_table_address;
	uint32_t warp_vertical_table_address;
} set_chromatic_aberration_warp_control_t;

/**
 * Resampler Coefficient Adjust (0x0900500A)
 */
 typedef enum {
	RESAMP_COEFF_M2 = (1 << 0),
	RESAMP_COEFF_M4 = (1 << 1),
	RESAMP_COEFF_M8 = (1 << 2),
	RESAMP_COEFF_ADJUST = ((1 << 0) | (1 << 1) |(1 << 2)),
	RESAMP_COEFF_RECTWIN = (1 << 3),
	RESAMP_COEFF_LP_STRONG = (1 << 4),
	RESAMP_COEFF_LP_MEDIUM = (1 << 5),
	RESAMP_COEFF_KAISER25 = (1 << 6),
} RESAMP_COEFF_FLAG;

typedef enum {
	RESAMP_SELECT_CFA = (1 << 0),
	RESAMP_SELECT_MAIN = (1 << 1),
	RESAMP_SELECT_PA = (1 << 2),
	RESAMP_SELECT_PB = (1 << 3),
	RESAMP_SELECT_PC = (1 << 4),
	RESAMP_SELECT_CFA_V = (1 << 5),
	RESAMP_SELECT_MAIN_V = (1 << 6),
	RESAMP_SELECT_PA_V = (1 << 7),
	RESAMP_SELECT_PB_V = (1 << 8),
	RESAMP_SELECT_PC_V = (1 << 9),
} RESAMP_SELECT_FLAG;

typedef enum {
	RESAMP_MODE_VIDEO = 0x0,
	RESAMP_MODE_STILL = 0x1,		// only one frame
} RESAMP_MODE_FLAG;

typedef struct resampler_coefficient_adjust_s {
	uint32_t cmd_code;
	uint32_t control_flag;
	uint16_t resampler_select;
	uint16_t mode;			 // 0:video (always), 1: still (one-frame)
} resampler_coefficient_adjust_t;

/*
 * Dump IDSP config (0x09006001)
 */
typedef struct amb_dsp_debug_2_s {
	uint32_t cmd_code;
	uint32_t dram_addr;
	uint32_t dram_size;
	uint32_t mode;
} amb_dsp_debug_2_t;

typedef amb_dsp_debug_2_t dump_idsp_config_t;

/*
 * Send IDSP debug command (0x09006002)
 */
typedef struct amb_dsp_debug_3_s {
	uint32_t cmd_code;
	uint32_t mode;
	uint32_t param1;
	uint32_t param2;
	uint32_t param3;
	uint32_t param4;
	uint32_t param5;
	uint32_t param6;
	uint32_t param7;
	uint32_t param8;
} amb_dsp_debug_3_t;

typedef amb_dsp_debug_2_t send_idsp_debug_cmd_t;

/*
 * Update IDSP config (0x09006003)
 */
typedef struct amb_update_idsp_config_s {
	uint32_t cmd_code;
	uint16_t section_id;
	uint8_t mode;
	uint8_t table_sel;
	uint32_t dram_addr;
	uint32_t data_size;
} amb_update_idsp_config_t;

typedef amb_update_idsp_config_t update_idsp_config_t;

/*
 *  Propcess IDSP Command Batch (0x09006004)
 */
typedef struct process_idsp_batch_cmd_s {
	uint32_t cmd_code;
	uint32_t group_index;
	uint32_t idsp_cmd_buf_addr;
	uint32_t cmd_buf_size;
} process_idsp_batch_cmd_t;

/*
 *  Raw encode video setup command (0x09006005)
 */
typedef struct raw_encode_video_setup_cmd_t_s {
	uint32_t cmd_code;
	uint32_t sensor_raw_start_daddr; /* frm 0 start address */
	uint32_t daddr_offset ; /* offset from sensor_raw_start_daddr to the start address of the next frame in DRAM */
	uint16_t raw_width ; /* image width in pixels */
	uint16_t raw_height ; /* image height in pixels */
	uint32_t dpitch : 16 ; /* buffer pitch in bytes */
	uint32_t raw_compressed : 1 ; /* whether raw compression is done */
	uint32_t num_frames : 8 ; /* number of frames stored in DRAM starting from sensor_raw_start_daddr */
	uint32_t reserved : 7 ;
} raw_encode_video_setup_cmd_t;

/************************************************************
 * Postprocess commands (Category 10)
 */
// cmd code 0x0A000001
typedef struct CMD_POSTPROCtag {
	uint32_t cmd_code;

	uint8_t decode_cat_id;
	uint8_t decode_id;
	uint16_t reserved_0;

	uint8_t voutA_enable;
	uint8_t voutB_enable;
	uint8_t pip_enable;
	uint8_t reserved_1;

	uint32_t reserved_1_1;
	uint32_t reserved_1_2;

	uint16_t input_center_x;
	uint16_t input_center_y;

	// voutA window
	uint16_t voutA_target_win_offset_x;
	uint16_t voutA_target_win_offset_y;
	uint16_t voutA_target_win_width;
	uint16_t voutA_target_win_height;

	uint32_t voutA_zoom_factor_x;
	uint32_t voutA_zoom_factor_y;

	uint16_t voutB_target_win_offset_x;
	uint16_t voutB_target_win_offset_y;
	uint16_t voutB_target_win_width;
	uint16_t voutB_target_win_height;

	uint32_t voutB_zoom_factor_x;
	uint32_t voutB_zoom_factor_y;

	// PIP display
	uint16_t pip_target_offset_x;
	uint16_t pip_target_offset_y;
	uint16_t pip_target_x_size;
	uint16_t pip_target_y_size;

	uint32_t pip_zoom_factor_x;
	uint32_t pip_zoom_factor_y;

	// rotate and flip
	uint8_t vout0_flip;
	uint8_t vout0_rotate;
	uint16_t vout0_win_offset_x;
	uint16_t vout0_win_offset_y;
	uint16_t vout0_win_width;
	uint16_t vout0_win_height;

	uint8_t vout1_flip;
	uint8_t vout1_rotate;
	uint16_t vout1_win_offset_x;
	uint16_t vout1_win_offset_y;
	uint16_t vout1_win_width;
	uint16_t vout1_win_height;
} POSTPROC_CMD;

// cmd code 0x0A000002
typedef struct POSTP_YUV2YUV_CMDtag {
	uint32_t cmd_code;

	uint8_t decode_cat_id;
	uint8_t decode_id;
	uint16_t reserved;

	uint32_t fir_pts_low;
	uint32_t fir_pts_high;

	uint16_t matrix_values[9];
	int16_t y_offset;
	int16_t u_offset;
	int16_t v_offset;
} POSTP_YUV2YUV_CMD;

// cmd code 0x0A000003
typedef struct DECPP_CONFIG_CMDtag {
	uint32_t cmd_code;

	uint8_t dec_id;
	uint8_t dec_type;
	uint8_t interlaced_out;
	uint8_t packed_out; // 0: planar yuv; 1: packed yuyv
	uint8_t out_chroma_format; // 0: 420; 1: 422
	uint8_t use_deint :4; // 0: no deinterlacing; 1: use de-interlacing filter;
	uint8_t half_deint_frame_rate :4; // 0: full frame rate; 1: half frame rate (for 60i->30p);

	uint16_t reserved0;

	uint16_t input_center_x;
	uint16_t input_center_y;

	uint16_t out_win_offset_x;
	uint16_t out_win_offset_y;
	uint16_t out_win_width;
	uint16_t out_win_height;

	uint32_t out_zoom_factor_x;
	uint32_t out_zoom_factor_y;

	// horizontal de-warpping
	uint32_t horz_warp_enable;
	uint32_t warp_horizontal_table_address;

	uint8_t grid_array_width;
	uint8_t grid_array_height;
	uint8_t horz_grid_spacing_exponent;
	uint8_t vert_grid_spacing_exponent;
} DECPP_CONFIG_CMD;

// 0x0A000004
typedef struct DECPP_INPIC_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;
	uint8_t decoder_type;
	uint16_t pic_buf_id;
} DECPP_INPIC_CMD;

// 0x0A00000F
typedef struct DECPP_CREATE_CMDtag {
	uint32_t cmd_code; // we should create postp instance for ppmode=1 in video editing mode only

	uint8_t mode_flag; // bit0~1:reserved. bit2: enable de-interlace. bit3: enable OSD deinterlace
	uint8_t num_of_instances;   // number of decode instances
	uint8_t pp_chroma_fmt_max;
	uint8_t reserved;

	uint16_t pp_max_frm_width;
	uint16_t pp_max_frm_height;
	uint16_t pp_max_frm_num;
} DECPP_CREATE_CMD;

// 0x0A000006
typedef struct POSTP_INPIC_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t progressive;
	uint16_t frm_buf_id;
	uint32_t pts_low;
	uint32_t pts_high;
} POSTP_INPIC_CMD;

// 0x0A000007
typedef struct POSTP_SPD_ADJUST_CMDtag {
	uint32_t cmd_code;
} POSTP_SPD_ADJUST_CMD;

/************************************************************
 * Postprocess commands for UDEC ppmode=2 (Category 10)
 * cmd code 0x0A000008
 ************************************************************/
typedef struct CMD_POSTPROC_UDECtag {
	uint32_t cmd_code;

	uint8_t decode_cat_id;
	uint8_t decode_id;
	uint8_t decode_type;
	uint8_t reserved_0;

	uint8_t voutA_enable;
	uint8_t voutB_enable;
	uint8_t pipA_enable;
	uint8_t pipB_enable;

	uint32_t fir_pts_low;
	uint32_t fir_pts_high;

	uint16_t input_center_x;
	uint16_t input_center_y;

	// voutA window
	uint16_t voutA_target_win_offset_x;
	uint16_t voutA_target_win_offset_y;
	uint16_t voutA_target_win_width;
	uint16_t voutA_target_win_height;

	uint32_t voutA_zoom_factor_x;
	uint32_t voutA_zoom_factor_y;

	uint16_t voutB_target_win_offset_x;
	uint16_t voutB_target_win_offset_y;
	uint16_t voutB_target_win_width;
	uint16_t voutB_target_win_height;

	uint32_t voutB_zoom_factor_x;
	uint32_t voutB_zoom_factor_y;

	// PIPA display
	uint16_t pipA_target_offset_x;
	uint16_t pipA_target_offset_y;
	uint16_t pipA_target_x_size;
	uint16_t pipA_target_y_size;

	uint32_t pipA_zoom_factor_x;
	uint32_t pipA_zoom_factor_y;

	// PIPB display
	uint16_t pipB_target_offset_x;
	uint16_t pipB_target_offset_y;
	uint16_t pipB_target_x_size;
	uint16_t pipB_target_y_size;

	uint32_t pipB_zoom_factor_x;
	uint32_t pipB_zoom_factor_y;

	// rotate and flip
	uint8_t vout0_flip;
	uint8_t vout0_rotate;
	uint16_t vout0_win_offset_x;
	uint16_t vout0_win_offset_y;
	uint16_t vout0_win_width;
	uint16_t vout0_win_height;

	uint8_t vout1_flip;
	uint8_t vout1_rotate;
	uint16_t vout1_win_offset_x;
	uint16_t vout1_win_offset_y;
	uint16_t vout1_win_width;
	uint16_t vout1_win_height;

	// horizontal de-warpping
	uint32_t horz_warp_enable;
	uint32_t warp_horizontal_table_address;

	uint8_t grid_array_width;
	uint8_t grid_array_height;
	uint8_t horz_grid_spacing_exponent;
	uint8_t vert_grid_spacing_exponent;
} POSTPROC_UDEC_CMD;

/************************************************************
 * command to set audio clock offset for AV sync (Category 10)
 * cmd code 0x0A000009
 ************************************************************/
typedef struct POSTP_SET_AUDIO_CLK_OFFSET_CMDTag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t reserve[3];
	int32_t audio_clk_offset;
} POSTP_SET_AUDIO_CLK_OFFSET_CMD;

/************************************************************
 * command to change vout status from DORMANT to RUN in post processing.
 * cmd code 0x0A00000A
 ************************************************************/
typedef struct POSTP_WAKE_VOUT_CMDTag {
	uint32_t cmd_code;
	uint8_t decoder_id;
} POSTP_WAKE_VOUT_CMD;

/************************************************************
 * command to initialize deinterlacer.
 * cmd code 0x0A00000C
 ************************************************************/
typedef struct DEINT_INIT_CMDtag {
	uint32_t cmd_code;
	uint8_t init_tff;
	uint8_t deint_lu_en;
	uint8_t deint_ch_en;
	uint8_t osd_en;
} DEINT_INIT_CMD;

/************************************************************
 * command to setup deinterlacer.
 * cmd code 0x0A00000D
 ************************************************************/
typedef struct DEINT_CONF_CMDtag {
	uint32_t cmd_code;

	uint32_t deint_mode :8;
	uint32_t osd0_enabled :1;
	uint32_t osd1_enabled :1;
	uint32_t resereved :22;

	uint8_t deint_spatial_shift;
	uint8_t deint_lowpass_shift;

	uint8_t deint_lowpass_center_weight;
	uint8_t deint_lowpass_hor_weight;
	uint8_t deint_lowpass_ver_weight;

	uint8_t deint_gradient_bias;
	uint8_t deint_predict_bias;
	uint8_t deint_candidate_bias;

	int16_t deint_spatial_score_bias;
	int16_t deint_temporal_score_bias;

	int16_t osd_active_height;
	int16_t osd_active_width;
	int16_t osd_video_row_start;
	int16_t osd_video_row_end;
	int16_t osd_video_col_start;
	int16_t osd_video_col_end;

	int16_t osd0_row_start;
	int16_t osd0_row_end;
	int16_t osd0_col_start;
	int16_t osd0_col_end;

	int16_t osd1_row_start;
	int16_t osd1_row_end;
	int16_t osd1_col_start;
	int16_t osd1_col_end;
} DEINT_CONF_CMD;

// cmd code 0x0A00000E
typedef struct fade_in_fade_out_s {
	uint16_t matrix_values_start[9];
	uint16_t matrix_values_end[9];
	int16_t y_offset_start;
	int16_t u_offset_start;
	int16_t v_offset_start;
	int16_t y_offset_end;
	int16_t u_offset_end;
	int16_t v_offset_end;
	uint32_t start_pts;
	uint32_t duration;
} fade_in_fade_out_t;

typedef struct POSTP_VIDEO_FADING_CMDtag {
	uint32_t cmd_code;
	fade_in_fade_out_t effect[2];

} POSTP_VIDEO_FADING_CMD;

/************************************************************
 * STILL command and message (Category B)
 */
/*
 typedef enum{
 STILL_RAW	   = 0,
 STILL_JPG
 }STILL_TYPE;
 */
// STILL decode setup command for initilization ,cmd code 0x0B000001
typedef struct STILLDEC_SETUP_CMDtag {
	uint32_t cmd_code;
	//uint32_t decoder_id;
	uint32_t bits_fifo_base;
	uint32_t bits_fifo_limit;
	uint32_t cross_fade_alpha_start;
	uint32_t cross_fade_alpha_step;
	uint32_t cross_fade_total_frames;
	uint8_t background_y;
	uint8_t background_cb;
	uint8_t background_cr;
	//uint8_t still_type;
	//uint32_t fbuf_dram_size;
	uint8_t bits_circular;

	uint16_t max_decode_width;
	uint16_t max_decode_height;

	uint32_t hcb_dram_size;
	uint16_t vout_frame_buf_len;
	uint16_t reserved;
} STILLDEC_SETUP_CMD;

// STILL decode start command for start vout and codingORC, cmd code 0x0B000002
typedef struct STILLDEC_START_CMDtag {
	uint32_t cmd_code;
	//uint32_t decoder_id;
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
} STILLDEC_START_CMD;

// STILL decode command ,cmd code 0x0B000003
typedef struct STILLDEC_DECODE_CMDtag {
	uint32_t cmd_code;
	//uint32_t decoder_id;
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
	uint8_t tv_rotation;
	uint8_t ycbcrposition;
	uint16_t reserved;
	uint32_t frame_duration;
	uint32_t frame_w_decode;
	uint8_t has_decoded;
	uint8_t lcd_rotation;
	uint16_t reserved1;
} STILLDEC_DECODE_CMD;

// STILL decode stop command ,cmd code 0x0B000004
typedef struct STILLDEC_STOP_CMDtag {
	uint32_t cmd_code;
	//uint32_t decoder_id;
	uint32_t seamless_flag :8;
	uint32_t reserved :24;
} STILLDEC_STOP_CMD;

// STILL decode upwptr command ,cmd code 0x0B000005
typedef struct STILLDEC_UPWPTR_CMDtag {
	uint32_t cmd_code;
	//uint32_t decoder_id;
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
} STILLDEC_UPWPTR_CMD;

typedef struct STILL_MULTIS_SETUP_CMDtag {
	uint32_t cmd_code;

	uint32_t if_save_thumbnail :8;
	uint32_t if_save_large_thumbnail :8;
	uint32_t total_thumbnail :16;

	uint32_t saving_thumbnail_width :16;
	uint32_t saving_thumbnail_height :16;

	uint32_t total_large_thumbnail :16;
	uint32_t saving_large_thumbnail_width :16;

	uint32_t saving_large_thumbnail_height :16;
	uint32_t if_capture_large_size_thumb :8;
	uint32_t reserved :8;

	uint32_t large_thumbnail_pic_width :16;
	uint32_t large_thumbnail_pic_height :16;

	uint32_t visual_effect_type :8;
	uint32_t reserved2 :24;

	//added for app to create the thumb related buffer
	uint32_t small_thumb_daddr;
	uint32_t large_thumb_daddr;
	uint32_t capture_thumb_daddr;
} STILL_MULTIS_SETUP_CMD;
//below is old type, removed first
//typedef struct SCENE_STRUCTUREtag{
//	int16_t offset_x;
//	int16_t offset_y;
//
//	uint16_t width;
//	uint16_t height;
//
//	uint32_t source_base;
//
//	uint32_t source_size : 24;
//	uint32_t source_type : 4;
//	uint32_t rotation : 4;
//
//	uint32_t thumbnail_id : 8;
//	uint32_t decode_only : 4;
//	uint32_t reserved : 20;
//
//	int16_t sec_offset_x;
//	int16_t sec_offset_y;
//
//	uint16_t sec_width;
//	uint16_t sec_height;
//
//	//int16_t cap_offset_x;
//	//int16_t cap_offset_y;
//	//uint16_t cap_width;
//	//uint16_t cap_height;
//}SCENE_STRUCTURE;
//
//typedef struct STILL_MULTIS_DECODE_CMDtag{
//	uint32_t cmd_code;
//
//	uint32_t total_scenes : 8;
//	uint32_t start_scene_num : 8;
//	uint32_t scene_num : 8;
//	uint32_t end : 4;
//	uint32_t fast_mode : 4;
//
//	SCENE_STRUCTURE scene[4];
//
//}STILL_MULTIS_DECODE_CMD;

typedef struct SCENE_STRUCTUREtag {
	int16_t input_offset_x;	//for input cropping
	int16_t input_offset_y;

	uint16_t input_width;
	uint16_t input_height;

	int16_t main_offset_x;	//for TV
	int16_t main_offset_y;

	uint16_t main_width;
	uint16_t main_height;

	int16_t sec_offset_x;	//for LCD
	int16_t sec_offset_y;

	uint16_t sec_width;
	uint16_t sec_height;

	uint32_t source_base;

	uint32_t source_size :24;
	uint32_t source_type :4;
	uint32_t tv_rotation :4;

	uint32_t lcd_rotation :4;
	uint32_t thumbnail_id :11;
	uint32_t decode_only :1;
	uint32_t luma_gain :8;	//for luma scaling
	uint32_t thumb_type :2;	//indicate diff thumb size,0:small thumb size;1:large
	uint32_t thumb_crop_en :1;
	uint32_t reserved :5;
} SCENE_STRUCTURE;

typedef struct STILL_MULTIS_DECODE_CMDtag {
	uint32_t cmd_code;

	uint32_t total_scenes :8;
	uint32_t start_scene_num :8;
	uint32_t scene_num :8;
	uint32_t end :1;
	uint32_t update_lcd_only :1;
	uint32_t buffer_source_only :1;
	uint32_t fast_mode :1;
	uint32_t use_single_prev_filter :1; // indicate if using only 1 preview filter
	uint32_t reserved :3;

	SCENE_STRUCTURE scene[3];
} STILL_MULTIS_DECODE_CMD;

typedef struct STILLDEC_FREEZE_CMDtag {
	uint32_t cmd_code;


	uint32_t freeze_state :8;
	uint32_t reserved :24;
} STILLDEC_FREEZE_CMD;

//Capture still command,0x0b000007
typedef struct STILLDEC_CAP_STILL_CMDtag {
	uint32_t cmd_code;

	uint32_t coded_pic_base;
	uint32_t coded_pic_limit;
	uint32_t thumbnail_pic_base;
	uint32_t thumbnail_pic_limit;

	uint16_t thumbnail_width;
	uint16_t thumbnail_height;

	uint16_t thumbnail_letterbox_strip_width;
	uint16_t thumbnail_letterbox_strip_height;

	uint8_t thumbnail_letterbox_strip_y;
	uint8_t thumbnail_letterbox_strip_cb;
	uint8_t thumbnail_letterbox_strip_cr;
	uint8_t capture_multi_scene;

	uint32_t quant_matrix_addr;
	uint32_t screennail_pic_base;
	uint32_t screennail_pic_limit;

	uint16_t screennail_width;
	uint16_t screennail_height;

	uint16_t screennail_letterbox_strip_width;
	uint16_t screennail_letterbox_strip_height;

	uint8_t screennail_letterbox_strip_y;
	uint8_t screennail_letterbox_strip_cb;
	uint8_t screennail_letterbox_strip_cr;
	uint8_t rotate_direction;

	uint16_t input_offset_x;
	uint16_t input_offset_y;
	uint16_t input_width;
	uint16_t input_height;
	uint16_t target_pic_width;
	uint16_t target_pic_height;

	uint32_t quant_matrix_addr_thumbnail;
	uint32_t quant_matrix_addr_screennail;

	uint32_t input_luma_addr;
	uint32_t input_chroma_addr;
	uint16_t input_orig_pic_width;
	uint16_t input_orig_pic_height;
	uint16_t input_pitch;
	uint8_t input_chroma_fmt;
	uint8_t yuv_capture_on;
} STILLDEC_CAP_STILL_CMD;

// cmd code - 0x0B000008
typedef struct STILLDEC_RESCALE_ONLY_CMDtag {
	uint32_t cmd_code;

	uint32_t in_luma_daddr;
	uint32_t in_chma_daddr;

	uint32_t in_width :16;
	uint32_t in_height :16;

	uint32_t in_pitch :16;
	uint32_t in_chmafmt :2;
	uint32_t reserved_0 :14;

	uint32_t out_luma_daddr;
	uint32_t out_chma_daddr;

	uint32_t out_width :16;
	uint32_t out_height :16;

	uint32_t out_pitch :16;
	uint32_t out_chmafmt :2;
	uint32_t reserved_1 :14;
} STILLDEC_RESCALE_ONLY_CMD;

// cmd code = 0x0B000009
typedef struct STILLDEC_ADV_PIC_ADJtag {
	uint32_t cmd_code;

	uint32_t enable :8; //0:default;1:depend on user; when use has set the enable bit, no way to back default, user need to control them to default by themselves
	uint32_t reserved :24;
} STILLDEC_ADV_PIC_ADJ;

typedef struct BLEND_PARAMtag {
	uint32_t src1_luma;
	uint32_t src1_chma;
	uint32_t src2_luma;
	uint32_t src2_chma;
	uint32_t dst_luma;
	uint32_t dst_chma;
	uint32_t width :16;/* relay blend area */
	uint32_t height :16;
	uint32_t src1_pitch :16;
	uint32_t src2_pitch :16;
	uint32_t alpha :8;
	uint32_t chma_type :8;
	uint32_t dst_pitch :16;
} BLEND_PARAM;

// cmd code = 0x0B00000a
typedef struct STILLDEC_BLEND_CMDtag {
	uint32_t cmd_code;

	uint32_t total :8;
	uint32_t reserve1 :24;

	BLEND_PARAM blend_param[3];
} STILLDEC_BLEND_CMD;

//Capture video command,0x03000008, may put at video mode
typedef struct STILLDEC_CAP_VIDEO_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;

	/* cap_step:0:yuv->jpeg(direct mode);1:yuv->pause;2:pause->jpeg;3:pause->release
	 * if the first cap_step is 1, the next command must be cap_step 2 or 3,otherwise assert
	 */
	uint8_t cap_step;
	uint8_t reserved[2];

	uint32_t coded_pic_base;
	uint32_t coded_pic_limit;
	uint32_t thumbnail_pic_base;
	uint32_t thumbnail_pic_limit;

	uint16_t thumbnail_width;
	uint16_t thumbnail_height;

	uint16_t thumbnail_letterbox_strip_width;
	uint16_t thumbnail_letterbox_strip_height;

	uint8_t thumbnail_letterbox_strip_y;
	uint8_t thumbnail_letterbox_strip_cb;
	uint8_t thumbnail_letterbox_strip_cr;
	uint8_t rotate_direction;

	uint32_t quant_matrix_addr;

	uint16_t target_pic_width;
	uint16_t target_pic_height;

	uint32_t pic_structure :8;
	uint32_t res1 :24;

	uint32_t screennail_pic_base;
	uint32_t screennail_pic_limit;

	uint16_t screennail_width;
	uint16_t screennail_height;

	uint16_t screennail_letterbox_strip_width;
	uint16_t screennail_letterbox_strip_height;

	uint8_t screennail_letterbox_strip_y;
	uint8_t screennail_letterbox_strip_cb;
	uint8_t screennail_letterbox_strip_cr;
	uint8_t res2;

	uint32_t quant_matrix_addr_thumbnail;
	uint32_t quant_matrix_addr_screennail;
} STILLDEC_CAP_VIDEO_CMD;

// STILL status message, cmd code 0x8B000001
typedef struct STILL_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t timecode;
	uint32_t latest_clock_counter;
	uint32_t latest_pts;
	uint32_t jpeg_frame_count;
	//uint32_t decode_id;//no multiple still decoder at stilldec mode
	uint32_t yuv422_y_addr;
	uint32_t yuv422_uv_addr;
	uint32_t jpeg_bits_fifo_next; //for encode jpeg
	uint32_t decode_state;
	uint32_t error_status;
	uint32_t total_error_count;
	uint32_t decoded_pic_number;
	uint32_t jpeg_thumbnail_size; //for encode
	uint16_t jpeg_rescale_buf_pitch;
	uint16_t jpeg_rescale_buf_width;
	uint16_t jpeg_rescale_buf_height;
	uint16_t reserved1;
	uint32_t jpeg_rescale_buf_address_y;
	uint32_t jpeg_rescale_buf_address_uv;
	uint16_t second_rescale_buf_pitch;
	uint16_t second_rescale_buf_width;
	uint16_t second_rescale_buf_height;
	uint16_t blend_done_num;
	uint32_t second_rescale_buf_address_y;
	uint32_t second_rescale_buf_address_uv;
	uint32_t jpeg_y_addr;
	uint32_t jpeg_uv_addr;
	uint16_t jpeg_pitch;
	uint16_t jpeg_width;
	uint16_t jpeg_height;
	uint16_t rescale_cnt; //update when rescale only command done
	uint32_t jpeg_capture_count; //for encode
	uint32_t jpeg_screennail_size; //for encode
	uint32_t jpeg_thumbnail_buf_dbase;
	uint32_t jpeg_thumbnail_buf_pitch;
	uint32_t jpeg_large_thumbnail_buf_dbase;
	uint32_t jpeg_large_thumbnail_buf_pitch;
	uint16_t jpeg_cabac_message_queue_fullness;
	uint16_t jpeg_rescale_message_queue_fullness;
} STILL_STATUS_MSG;

typedef enum {
	SMALL_SIZE,
	LARGE_SIZE,
	MAIN_SIZE
} THUMB_TYPE;

/* chmafmt, the same with h264 decode standard */
typedef enum {
	RESCALE_YUV_420 = 1,
	RESCALE_YUV_422 = 2,
} RESCALE_CHROMA_TYPE;

/////////////////////////////////////////////////////////
// DSP Buffer Management CMDs and MSGs
/////////////////////////////////////////////////////////

/**
 * CMD_MEMM_QUERY_DSP_SPACE_SIZE (0x0F000001)
 */
typedef struct MEMM_SpaceQueryCfgUDecModetag {
	uint8_t  tiled_mode;
	uint8_t  frm_chroma_fmt_max;
	uint16_t dec_types;     // supported decoder types;
	uint32_t max_fifo_size; // The maximum size of mv fifo + bits fifo
	uint16_t max_frm_num;
	uint16_t max_frm_width;
	uint16_t max_frm_height;
} MEMM_SpaceQueryCfgUDecMode_t;

typedef struct MEMM_QUERY_DSP_SPACE_SIZE_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint8_t op_mode;
	uint8_t reserved[3];
	union
	{
//		 MEMM_SpaceQueryCfgCameraEncMode_t  camera_enc;
//		 MEMM_SpaceQueryCfgCameraDecMode_t  camera_dec;
//		 MEMM_SpaceQueryCfgH264DecMode_t	h264_dec;
//		 MEMM_SpaceQueryCfgMp2DecMode_t	 mp2_dec;
//		 MEMM_SpaceQueryCfgMp4DecMode_t	 mp4_dec;
//		 MEMM_SpaceQueryCfgMp4HWDecMode_t   mp4_hw_dec;
//		 MEMM_SpaceQueryCfgVc1DecMode_t	 vc1_dec;
//		 MEMM_SpaceQueryCfgRVDecMode_t	  rv_dec;
//		 MEMM_SpaceQueryCfgStillImgPbMode_t stillImg_playback;
		MEMM_SpaceQueryCfgUDecMode_t	   u_dec;
	} config;
} MEMM_QUERY_DSP_SPACE_SIZE_CMD;

/**
 * MSG_MEMM_QUERY_DSP_SPACE_SIZE (0x8F000001)
 */
typedef struct MEMM_QUERY_DSP_SPACE_SIZE_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t required_space_size;
} MEMM_QUERY_DSP_SPACE_SIZE_MSG;

/**
 * CMD_MEMM_SET_DSP_DRAM_SPACE (0x0F000002)
 */
typedef struct MEMM_SET_DSP_DRAM_SPACE_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint32_t dram_space_addr;
	uint32_t dram_space_size;
	uint32_t page_tb_addr;
	uint32_t dram_space_id : 8;
	uint32_t reserved : 24;
} MEMM_SET_DSP_DRAM_SPACE_CMD;

/**
 * MSG_MEMM_SET_DSP_DRAM_SPACE (0x8F000002)
 */
typedef struct MEMM_SET_DSP_DRAM_SPACE_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t error_code;
	uint32_t dram_space_id : 8;
	uint32_t reserved : 24;
} MEMM_SET_DSP_DRAM_SPACE_MSG;

/**
 * CMD_MEMM_RESET_DSP_DRAM_SPACE (0x0F000003)
 */
typedef struct MEMM_RESET_DSP_DRAM_SPACE_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint32_t dram_space_id : 8;
	uint32_t reserved : 24;
} MEMM_RESET_DSP_DRAM_SPACE_CMD;

/**
 * MSG_MEMM_RESET_DSP_DRAM_SPACE (0x8F000003)
 */
typedef struct MEMM_RESET_DSP_DRAM_SPACE_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t error_code;
	uint32_t dram_space_id : 8;
	uint32_t reserved : 24;
} MEMM_RESET_DSP_DRAM_SPACE_MSG;

/**
 * CMD_MEMM_CREATE_FRM_BUF_POOL (0x0F000004)
 */
typedef struct MEMM_CREATE_FRM_BUF_POOL_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t max_num_of_frm_bufs;	/* suggested number is 5 */
	uint16_t buf_width;			/* YUV_width, 640 */
	uint16_t buf_height;			/* YUV_height, 480 */
	uint16_t luma_img_width;		/* 640 */
	uint16_t luma_img_height;	/* 480 */
	uint8_t luma_img_row_offset;	/* 0 */
	uint8_t luma_img_col_offset;	/* 0 */
	uint8_t chroma_img_row_offset;	/* 0 */
	uint8_t chroma_img_col_offset;	/* 0 */
	uint8_t chroma_format;		/* 1 for YUV420, 0 for YUV_MONO (ME1) */
	uint8_t tile_mode;			/* 0 */
} MEMM_CREATE_FRM_BUF_POOL_CMD;

/**
 * MSG_MEMM_CREATE_FRM_BUF_POOL (0x8F000004)
 */
typedef struct MEMM_CREATE_FRM_BUF_POOL_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t error_code;		/* 0 for success, 1 for fail */
	uint16_t frm_buf_pool_id;	/* id needs ARM remember and relay to ucode later */
	uint16_t num_of_frm_bufs;	/* number of frm buffers that was created. */
} MEMM_CREATE_FRM_BUF_POOL_MSG;

/**
 * CMD_MEMM_CONFIG_FRM_BUF_POOL (0x0F00000C)
 */
typedef struct MEMM_CONFIG_FRM_BUF_POOL_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;

	uint16_t frm_buf_pool_id;
	uint8_t chroma_format;
	uint8_t tile_mode;

	uint16_t buf_width;
	uint16_t buf_height;

	uint16_t luma_img_width;
	uint16_t luma_img_height;

	uint8_t luma_img_row_offset;
	uint8_t luma_img_col_offset;
	uint8_t chroma_img_row_offset;
	uint8_t chroma_img_col_offset;
} MEMM_CONFIG_FRM_BUF_POOL_CMD;

typedef enum {
	FRM_BUF_POOL_TYPE_RAW = 0,
	FRM_BUF_POOL_TYPE_MAIN_CAPTURE,
	FRM_BUF_POOL_TYPE_PREVIEW_A,
	FRM_BUF_POOL_TYPE_PREVIEW_B,
	FRM_BUF_POOL_TYPE_PREVIEW_C,
	FRM_BUF_POOL_TYPE_MAIN_CAPTURE_ME1,
	FRM_BUF_POOL_TYPE_PREVIEW_A_ME1,
	FRM_BUF_POOL_TYPE_PREVIEW_B_ME1,
	FRM_BUF_POOL_TYPE_PREVIEW_C_ME1,
	FRM_BUF_POOL_TYPE_CFA_AAA,
	FRM_BUF_POOL_TYPE_RGB_AAA,
	FRM_BUF_POOL_TYPE_EFM, /* Same as FRM_BUF_POOL_TYPE_EXT defined in DSP, we rename it to FRM_BUF_POOL_TYPE_EFM at ARM side*/
	FRM_BUF_POOL_TYPE_WARPED_MAIN_CAPTURE, /* post-main, for dewarp pipeline only */
	FRM_BUF_POOL_TYPE_NUM,
} MEMM_FRM_BUF_POOL_TYPE;

/**
 * MSG_MEMM_CONFIG_FRM_BUF_POOL (0x8F00000C)
 */
typedef struct MEMM_CONFIG_FRM_BUF_POOL_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t error_code;
	uint16_t frm_buf_pool_id;
	uint16_t num_of_frm_bufs;
} MEMM_CONFIG_FRM_BUF_POOL_MSG;

/**
 * CMD_MEMM_UPDATE_FRM_BUF_POOL (0x0F000005)
 */
typedef struct MEMM_UPDATE_FRM_BUF_POOL_CONFG_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t frm_buf_pool_id;
	uint16_t buf_width;

	uint16_t buf_height;
	uint16_t luma_img_width;

	uint16_t luma_img_height;
	uint8_t luma_img_row_offset;
	uint8_t luma_img_col_offset;

	uint8_t chroma_img_row_offset;
	uint8_t chroma_img_col_offset;
	uint8_t update_img_info_only;
	uint8_t chroma_format;

	uint8_t tile_mode;
} MEMM_UPDATE_FRM_BUF_POOL_CONFG_CMD;

/**
 * MSG_MEMM_UPDATE_FRM_BUF_POOL_CONFIG (0x8F000005)
 */
typedef struct MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t error_code;
	uint32_t frm_buf_pool_id :16;
	uint32_t reserved :16;
} MEMM_UPDATE_FRM_BUF_POOL_CONFG_MSG;

/**
 * CMD_MEMM_REQ_FRM_BUF (0x0F000006)
 */
typedef struct MEMM_REQ_FRM_BUF_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t frm_buf_pool_id;	/* This is the fram_buf_pool_id in MEMM_CREATE_FRM_BUF_POOL_MSG that was sent back to ARM in the CMD_MEMM_CREATE_FRM_BUF_POOL */
	uint8_t pic_struct;	/* This should be 3 to singal frame */
	uint8_t reserved;
} MEMM_REQ_FRM_BUF_CMD;

/**
 * MSG_MEMM_REQ_FRM_BUF (0x8F000006)
 */
typedef struct MEMM_REQ_FRM_BUF_MSGtag {
	uint32_t msg_code;	/* MSG_MEMM_REQ_FRM_BUF */
	uint32_t callback_id;
	uint32_t luma_buf_base_addr;		/* return the luma dram address to put luma in */
	uint32_t luma_img_base_addr;
	uint32_t chroma_buf_base_addr;	/* returns the chroma dram address to put chroma in, return 0xDEADBEEF for ME1 buffer */
	uint32_t chroma_img_base_addr;
	uint32_t error_code;	/* returns 0 if frame buffer request is valid, otherwise returns 1 */
	uint16_t frm_buf_id;	/* this is the frm_buf_id need ARM remember to pass into the API for CMD_VCAP_ENC_FRM_DRAM_HANDSHAKE */
	uint16_t buf_pitch;	/* this is the dram pitch of the buffer */
	uint16_t frm_buf_pool_id;
	uint16_t avail_frm_num;
} MEMM_REQ_FRM_BUF_MSG;

/**
 * CMD_MEMM_REQ_RING_BUF (0x0F000007)
 */
typedef struct MEMM_REQ_RING_BUF_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint32_t buf_size;
} MEMM_REQ_RING_BUF_CMD;

/**
 * MSG_MEMM_REQ_RING_BUF (0x8F000007)
 */
typedef struct MEMM_REQ_RING_BUF_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t dram_base_addr;
	uint32_t buf_size;
	uint32_t error_code;
	uint32_t ring_buf_id :8;
	uint32_t reserved :24;
} MEMM_REQ_RING_BUF_MSG;

/**
 * CMD_MEMM_CREATE_THUMBNAIL_BUF_POOL (0x0F000008)
 */
typedef struct MEMM_CREATE_THUMBNAIL_BUF_POOL_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t num_of_bufs;
	uint16_t buf_width;

	uint16_t buf_height;
	uint8_t  reserved;
	uint8_t  chroma_format;
} MEMM_CREATE_THUMBNAIL_BUF_POOL_CMD;

/**
 * CMD_MEMM_GET_FRM_BUF_POOL_INFO (0x0F00000D)
 */
typedef struct MEMM_GET_FRM_BUF_POOL_INFO_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint32_t frm_buf_pool_type : 8; /* see MEMM_FRM_BUF_POOL_TYPE */
	uint32_t fbp_id : 8; /* when frm_buf_pool_type = FRM_BUF_POOL_TYPE_EFM */
	uint32_t reserved : 16;
} MEMM_GET_FRM_BUF_POOL_INFO_CMD;

/**
 * MSG_MEMM_GET_FRM_BUF_ADDR_INFO (0x8F00000D)
 */
typedef struct MEMM_GET_FRM_BUF_ADDR_INFO_MSGtag {
    uint32_t msg_code;
    uint32_t callback_id;
    uint32_t start_base_daddr ;
    uint32_t size ; /* in bytes */
} MEMM_GET_FRM_BUF_ADDR_INFO_MSG;

/**
 * MSG_MEMM_GET_FRM_BUF_POOL_INFO
 */
typedef struct MEMM_GET_FRM_BUF_POOL_INFO_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;

	uint16_t total_frm_bufs;    // The total number of frame buffers in the given frm buf pool.
	uint16_t next_frm_buf_id;   // the frm buffer id should be used as the start_frm_buf_id for the next round of frm buffer pool query.

	uint16_t buf_width;
	uint16_t buf_height;

	uint16_t luma_img_width;
	uint16_t luma_img_height;

	uint8_t  luma_img_row_offset;
	uint8_t  luma_img_col_offset;
	uint8_t  chroma_img_row_offset;
	uint8_t  chroma_img_col_offset;

	uint16_t buf_pitch;
	uint8_t  chroma_format;
	uint8_t  tile_mode;

	uint32_t chroma_offset;     // The image base difference between luma and chroma.

	uint16_t frm_buf_id[16];
	uint32_t luma_img_base_addr[16];
} MEMM_GET_FRM_BUF_POOL_INFO_MSG;

/**
 * MSG_MEMM_CREATE_THUMBNAIL_BUF_POOL (0x8F000008)
 */
typedef struct MEMM_CREATE_THUMBNAIL_BUF_POOL_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t base_addr;
	uint32_t error_code;
	uint16_t num_of_bufs;
	uint8_t thum_buf_pool_id;
	uint8_t reserved;
} MEMM_CREATE_THUMBNAIL_BUF_POOL_MSG;

/**
 * CMD_MEMM_GET_FRM_BUF_INFO (0x0F000009)
 */
typedef struct MEMM_GET_FRM_BUF_INFO_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t frm_buf_id;
	uint16_t reserved;
} MEMM_GET_FRM_BUF_INFO_CMD;

/**
 * MSG_MEMM_GET_FRM_BUF_INFO (0x8F000009)
 */
typedef struct MEMM_GET_FRM_BUF_INFO_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t luma_buf_base_addr;
	uint32_t luma_img_base_addr;
	uint32_t chroma_buf_base_addr;
	uint32_t chroma_img_base_addr;
	uint16_t frm_buf_id;
	uint16_t buf_width;

	uint16_t buf_height;
	uint16_t luma_img_width;

	uint16_t luma_img_height;
	uint16_t chroma_img_width;

	uint16_t chroma_img_height;
	uint16_t buf_pitch;

	uint8_t chroma_format;
	uint8_t tile_mode;
	uint16_t reserved;
} MEMM_GET_FRM_BUF_INFO_MSG;

/**
 * CMD_MEMM_GET_FREE_FRM_BUF_NUM (0x0F00000A)
 */
typedef struct MEMM_GET_FREE_FRM_BUF_NUM_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
	uint16_t frm_buf_pool_id;
	uint16_t reserved;
} MEMM_GET_FREE_FRM_BUF_NUM_CMD;

/**
 * MSG_MEMM_GET_FREE_FRM_BUF_NUM (0x8F00000A)
 */
typedef struct MEMM_GET_FREE_FRM_BUF_NUM_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint16_t frm_buf_pool_id;
	uint16_t num_of_free_bufs;
} MEMM_GET_FREE_FRM_BUF_NUM_MSG;

/**
 * CMD_MEMM_GET_FREE_SPACE_SIZE (0x0F00000B)
 */
typedef struct MEMM_GET_FREE_SPACE_SIZE_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
} MEMM_GET_FREE_SPACE_SIZE_CMD;

/**
 * MSG_MEMM_GET_FREE_SPACE_SIZE (0x8F0000B)
 */
typedef struct MEMM_GET_FREE_SPACE_SIZE_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t free_dram_size;
} MEMM_GET_FREE_SPACE_SIZE_MSG;

/**
 * CMD_MEMM_REL_FRM_BUF (0x0FF00001)
 */
typedef struct MEMM_REL_FRM_BUF_CMDtag {
	uint32_t cmd_code;
	uint16_t frm_buf_id;
	uint16_t reserved;
} MEMM_REL_FRM_BUF_CMD;

/**
 * CMD_MEMM_REG_FRM_BUF (0x0FF00002)
 */
typedef struct MEMM_REG_FRM_BUF_CMDtag {
	uint32_t cmd_code;
	uint16_t frm_buf_id;
	uint16_t reserved;
} MEMM_REG_FRM_BUF_CMD;

/************************************************************/

typedef struct EIS_COEFF_INFOtag {
	uint32_t actual_left_top_x;
	uint32_t actual_left_top_y;
	uint32_t actual_right_bot_x;
	uint32_t actual_right_bot_y;
	int32_t hor_skew_phase_inc;
	uint32_t zoom_y;
	uint16_t dummy_window_x_left;
	uint16_t dummy_window_y_top;
	uint16_t dummy_window_width;
	uint16_t dummy_window_height;
	uint32_t arm_sys_tim;
	uint32_t reserved[7];
} EIS_COEFF_INFO;

typedef struct FLG_PIV_INFOtag {
	u32 flg_PIV_status;
	u32 reserved_2[7];
} FLG_PIV_INFO;

typedef struct EIS_UPDATE_INFOtag {
	uint32_t latest_eis_coeff_addr;
	uint32_t reserved[7];
	uint32_t sec2_hori_out_luma_addr_1;
	uint32_t sec2_hori_out_luma_addr_2;
	uint32_t sec2_hori_out_chroma_addr_1;
	uint32_t sec2_hori_out_chroma_addr_2;
	uint32_t sec2_vert_out_luma_addr_1;
	uint32_t sec2_vert_out_luma_addr_2;
	uint32_t flg_PIV_status;
	uint32_t arm_syst_tim;
} EIS_UPDATE_INFO;

/////////////////////////////////////////////////////////
// Universal Decoder  CMDs and MSGs
/////////////////////////////////////////////////////////

// UDEC State Definitions
typedef enum {
	UDEC_STAT_INVALID = 0,
	UDEC_STAT_INIT = 1,
	UDEC_STAT_IDLE = 2,
	UDEC_STAT_READY = 3,
	UDEC_STAT_RUN = 4,  // running
	UDEC_STAT_RUN_2_READY = 5,  // running
	UDEC_STAT_RUN_2_IDLE = 6,
	UDEC_STAT_IDLE_2_READY = 7,
	UDEC_STAT_READY_2_RUN = 8,
	UDEC_STAT_ERROR = 9,
} UDEC_STATE;

// UDEC(Universal DECoder) init command, cmd code =0x0e000001
typedef struct UDEC_INIT_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;
	uint8_t dsp_dram_sp_id;
	uint8_t tiled_mode;
	uint8_t frm_chroma_fmt_max;

	uint16_t dec_types;
	uint16_t max_frm_num;

	uint16_t max_frm_width;
	uint16_t max_frm_height;
	uint32_t max_fifo_size; // The maximum size of mv fifo + bits fifo

	uint32_t no_fmo :8;
	uint32_t reserved :24;
} UDEC_INIT_CMD;

// UDEC_SETUP cmd code = 0x0e000002
/**
 * Base setup command for all decoder types
 */
typedef struct UDEC_SETUP_BASE_CMDtag {
	uint32_t cmd_code;

	uint8_t decoder_id;
	uint8_t decoder_type;
	uint8_t enable_pp; // should not set for both MPEG4 SW and RV decoders
	uint8_t enable_deint :1;
	uint8_t enable_err_handle :1;
	uint8_t validation_only :1; // 1: validation mode decoding; 0: otherwise
	uint8_t force_decode :1; // 1: force decoder to start decoding from any type of frames without dropping frames; 0: otherwise
	uint8_t reserved :4;
	uint16_t concealment_mode;
	uint16_t concealment_ref_frm_buf_id;
	uint32_t bits_fifo_size;
	uint32_t ref_cache_size;
} UDEC_SETUP_BASE_CMD;

/**
 * UDEC_SETUP command for VC1
 */
typedef struct UDEC_SETUP_VC1_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
} UDEC_SETUP_VC1_CMD;

/**
 * UDEC_SETUP command for H264
 */
typedef struct UDEC_SETUP_H264_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
	uint32_t pjpeg_buf_size;
} UDEC_SETUP_H264_CMD;

/**
 * UDEC_SETUP command for MPEG2 and MPEG4
 */
typedef struct UDEC_SETUP_MPEG24_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
	uint32_t deblocking_flag;
	uint32_t pquant_mode; // 1: uniform mode; 2:lookup table mode;
	uint8_t pquant_table[32]; // if pquant_mode is uniform mode, only pquant_table[0] is valid.
	uint32_t is_avi_flag;
} UDEC_SETUP_MPEG24_CMD;

/**
 * UDEC_SETUP command for Software MPEG4
 */
typedef struct UDEC_SETUP_MPEG4S_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
	uint32_t mv_fifo_size;
	uint32_t deblocking_flag;
	uint32_t pquant_mode; // 1: uniform mode; 2: lookup table mode;
	uint8_t pquant_table[32]; // if pquant_mode is uniform mode, only pquant_table[0] is valid.
} UDEC_SETUP_MPEG4S_CMD;

/**
 * UDEC_SETUP command for RV
 */
typedef struct UDEC_SETUP_RV40_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
	uint32_t mv_fifo_size;
} UDEC_SETUP_RV40_CMD;

/**
 * UDEC_SETUP command for SW
 */
typedef struct UDEC_SETUP_SW_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
} UDEC_SETUP_SW_CMD;

/**
 * UDEC_SETUP command for Still Decoder
 */

/* planar yuv dram address */
typedef struct planar_yuv_daddr_s {
	uint32_t y_addr;
	uint32_t uv_addr;
} planar_yuv_daddr_t;

/* packed yuyv dram address */
typedef struct packed_yuyv_daddr_s {
	uint32_t yuyv_addr;
	uint32_t reserved;
} packed_yuyv_daddr_t;

typedef union still_output_daddr_union_s {
	planar_yuv_daddr_t output_yuv;
	packed_yuyv_daddr_t output_yuyv;
} still_output_daddr_union_t;

typedef struct UDEC_SETUP_SDEC_CMDtag {
	UDEC_SETUP_BASE_CMD base_cmd;
	uint16_t still_bits_circular;
	uint16_t reserved1;
	uint16_t still_max_decode_width;
	uint16_t still_max_decode_height;
	still_output_daddr_union_t still_output_daddr; // output dram address
	uint16_t still_output_pitch; // output dram pitch
} UDEC_SETUP_SDEC_CMD;

// definition of decoder_types
typedef enum {
	UDEC_TYPE_NULL = 0,
	UDEC_TYPE_H264 = 1,
	UDEC_TYPE_MP12 = 2,
	UDEC_TYPE_MP4H = 3,
	UDEC_TYPE_MP4S = 4,
	UDEC_TYPE_VC1 = 5,
	UDEC_TYPE_RV40 = 6,
	UDEC_TYPE_STILL = 7,
	UDEC_TYPE_SW = 8,
} UDEC_DEC_TYPES;

// cmd code = 0x0e000003
typedef struct UDEC_DECODE_BASE_CMDtag {
	// invariant of decoder_type
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t decoder_type;
	uint16_t num_of_pics;
} UDEC_DECODE_BASE_CMD;

// cmd code = 0x0e000003, decoder_id = 1, 2, 3, 5, 7
typedef struct UDEC_DEC_BFIFO_CMDtag {
	UDEC_DECODE_BASE_CMD base_cmd;
	//
	uint32_t bits_fifo_start;
	uint32_t bits_fifo_end;
} UDEC_DEC_FIFO_CMD;

// cmd code = 0x0e000003, decoder_id = UDEC_TYPE_MP4S
typedef struct UDEC_DEC_MP4S_CMDtag {
	UDEC_DECODE_BASE_CMD base_cmd;
	//
	uint32_t vop_coef_start_addr;
	uint32_t vop_coef_end_addr;
	uint32_t vop_mv_start_addr;
	uint32_t vop_mv_end_addr;
} UDEC_DEC_MP4S_CMD;

// cmd code = 0x0e000003, decoder_id = UDEC_TYPE_RV40
typedef enum {
	RV40_I_PIC = 1,
	RV40_P_PIC = 2,
	RV40_B_PIC = 3,
} RV40_PIC_TYPE;

typedef struct UDEC_DEC_RV40_CMDtag {
	UDEC_DECODE_BASE_CMD base_cmd;
	uint32_t residual_fifo_start;
	uint32_t residual_fifo_end;
	uint32_t mv_fifo_start;
	uint32_t mv_fifo_end;

	uint16_t fwd_ref_buf_id;
	uint16_t bwd_ref_buf_id;

	uint16_t target_buf_id;
	uint16_t pic_width;

	uint16_t pic_height;
	uint8_t pic_coding_type; // 1: I; 2: P; 3: B
	//uint8_t tiled_mode;       //0: raster-scan mode, 5: interleaved tiled mode
	uint8_t reserved;
} UDEC_DEC_RV40_CMD;

typedef struct UDEC_DEC_SW_CMDtag {
	UDEC_DECODE_BASE_CMD base_cmd;
} UDEC_DEC_SW_CMD;

// cmd code = 0x0e000004
typedef struct UDEC_STOP_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t stop_flag;
	uint16_t reserved;
} UDEC_STOP_CMD;

// cmd code = 0x0e000005
typedef UDEC_STOP_CMD UDEC_EXIT_CMD;

// cmd code = 0x0E000006
typedef struct UDEC_TRICKPLAY_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t mode;  //0: PAUSE; 1: RESUME; 2: STEP
	uint16_t reserved;
} UDEC_TRICKPLAY_CMD;

// CMD_UDEC_PLAYBACK_SPEED 0x0E000007
typedef struct UDEC_PLAYBACK_SPEED_CMDtag {
	uint32_t cmd_code;
	uint8_t decoder_id;
	uint8_t direction;  // 0: DIR_FWD; 1: DIR_BWD
	uint16_t speed; // 0: play as fast as possible; otherwise: 8bits.8bits fractional supported speed control
} UDEC_PLAYBACK_SPEED_CMD_t;

// CMD_UDEC_CREATE 0x0E000008
typedef struct UDEC_CREATE_CMDtag {
	uint32_t cmd_code;
	uint32_t callback_id;
} UDEC_CREATE_CMD;

// MSG_UDEC_CREATE 0x8E000009
typedef struct UDEC_CREATE_MSGtag {
	uint32_t msg_code;
	uint32_t callback_id;
	uint32_t decoder_id :8;
	uint32_t reserved :24;
} UDEC_CREATE_MSG;

/**
 * UDEC decode status msg (0x8e000001)
 */
typedef struct UDEC_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t msg_seq_no;

	uint8_t decoder_id;
	uint8_t decoder_type;
	uint8_t decoder_state;
	uint8_t dram_sp_id;

	uint8_t frame_fbp_id;
	uint8_t vout_state;
	uint8_t bs_fifo_cpb_id;
	uint8_t mv_fifo_cpb_id;

	uint32_t bs_fifo_base;
	uint32_t mv_fifo_base;
	uint32_t total_decoded_pic_num;
	uint32_t cur_decoded_pic_num;
	uint32_t error_code;
	uint32_t last_pic_pts_low;
	uint32_t last_pic_pts_high;
} UDEC_STATUS_MSG;

// Definition of dec_pic_info when decoder_type == UDEC_TYPE_H264
typedef struct H264_DEC_PICINFO_WORDtag {
	uint32_t valid :1;
	uint32_t reserved :4;
	uint32_t error_level :4;
	uint32_t pic_type :3;
	uint32_t pic_struct :2;
	uint32_t broken_link :1;
	uint32_t repeat_first_field :1;

	uint32_t top_field_first :1;
	uint32_t entropy_mode :1;
	uint32_t is_mbaff :1;
	uint32_t reserved0 :13;
} H264_DEC_PICINFO_WORD;

/**
 * UDEC OUTPIC msg (0x8e000002)
 */
typedef struct UDEC_OUTPIC_MSGtag {
	uint32_t msg_code;

	uint8_t decoder_id;
	uint8_t decoder_type;
	uint8_t tiled_fmt;
	uint8_t chroma_fmt;

	uint16_t frm_buf_id;
	uint16_t buf_pitch;

	uint16_t pic_width;
	uint16_t pic_height;

	uint32_t luma_buf_base_addr;
	uint32_t luma_img_base_addr;
	uint32_t chroma_buf_base_addr;
	uint32_t chroma_img_base_addr;

	uint32_t pic_info_word_top_or_frm; // dec0der_type dependent
	uint32_t pic_info_word_bot;
	uint32_t reserved; // end_of_seq;

	uint32_t frm_num; // frame number in display order

	uint32_t pts_low;
	uint32_t pts_high;
	uint8_t frm_buf_pool_id; //output frame buffer pool id
} UDEC_OUTPIC_MSG;

/**
 * UDEC Fifo status msg (0x8E000003)
 */

typedef struct UDEC_FIFO_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t decoder_id :8;
	uint32_t reserved :24;
	uint16_t bits_fifo_id;
	uint16_t mv_fifo_id;
	uint32_t bits_fifo_error_code;
	uint32_t mv_fifo_error_code;
	uint32_t bits_fifo_cur_pos;
	uint32_t mv_fifo_cur_pos;
} UDEC_FIFO_STATUS_MSG;

/**
 * UDEC display status msg (0x8E000004)
 */

typedef struct UDEC_DISP_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t decoder_id :8;
	uint32_t reserved :24;
	uint32_t latest_clock_counter;
	uint32_t latest_pts_low;
	uint32_t latest_pts_high;
	uint32_t dis_pic_y_addr;
	uint32_t dis_pic_uv_addr;
} UDEC_DISP_STATUS_MSG;

/**
 * UDEC EOS status msg (0x8E000005)
 */
typedef struct UDEC_EOS_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t decoder_id :8;
	uint32_t reserved :24;
	uint32_t last_pts_low;
	uint32_t last_pts_high;
} UDEC_EOS_STATUS_MSG;

/**
 * MSG_UDEC_ABORT_STATUS msg (0x8E000006)
 */
typedef struct UDEC_ABORT_STATUS_MSGtag {
	uint32_t msg_code;
	uint32_t decoder_id :8;
	uint32_t reserved :24;
	uint32_t code_timeout_pc;
	uint32_t code_timeout_reason;
	uint32_t mdxf_timeout_pc;
	uint32_t mdxf_timeout_reason;
} UDEC_ABORT_STATUS_MSG;

#if defined(DSP_SMALL_INT)
#undef int8_t
#endif
/************************************************************/

////////////////////////////////////////////////////////////////////////////////
#endif // CMD_MSG_S2_H

