/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi_edid.h
 *
 * History:
 *    2009/06/05 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#define EDID_SEGMENT_POINTER_ADDR	(0x60 >> 1)
#define EDID_DATA_ACCESS_ADDR		(0xa0 >> 1)
#define EDID_PER_SEGMENT_SIZE		128

#define MAX_CEA_TIMINGS			63
#define MAX_EXTENDED_TIMINGS		5
#define MAX_TOTAL_CEA_TIMINGS		(MAX_CEA_TIMINGS + MAX_EXTENDED_TIMINGS)
#define MAX_NATIVE_TIMINGS		25

#define MAX_SUPPORTING_DDD_STRUTURES	8

typedef enum {HDMI, DVI} amba_hdmi_interface_t;

typedef enum {MONOCHROME, RGB, NONRGB, UNDEFINED} display_type_t;

typedef enum {
	COLORIMETRY_NO_DATA	= 0,
	COLORIMETRY_ITU601	= 1,
	COLORIMETRY_ITU709	= 2,
	COLORIMETRY_EXTENDED	= 3,
} colorimetry_t;

typedef struct {
	u8					*buf;
	u32					len;
} amba_hdmi_raw_edid_t;

typedef struct {
	int					support_ycbcr444;
	int					support_ycbcr422;
} amba_hdmi_color_space_t;

typedef struct {
	char					vendor[4];
	u16					product_code;
	u32 					product_serial_number;
	u8  					manufacture_week;
	u16 					manufacture_year;
} amba_hdmi_product_t;

typedef struct {
	u8	version;
	u8	revision;
} amba_hdmi_edid_version_t;

typedef struct {
	u32					video_input_definition;		/* Analog or Digital */
	u8					max_horizontal_image_size;	/* cm */
	u8					max_vertical_image_size;	/* cm */
	u16					gama;				/* 10^(-2) */
	display_type_t				display_type;
} amba_hdmi_display_feature_t;

typedef struct {
	u16					red_x;			/* 2^(-10) */
	u16					red_y;			/* 2^(-10) */
	u16					green_x;		/* 2^(-10) */
	u16					green_y;		/* 2^(-10) */
	u16					blue_x;			/* 2^(-10) */
	u16					blue_y;			/* 2^(-10) */
	u16					white_x;		/* 2^(-10) */
	u16					white_y;		/* 2^(-10) */
} amba_hdmi_color_characteristics_t;

typedef struct {
	enum amba_video_mode			vmode;
	char					name[32];

	u32					pixel_clock;		/* kHz */

	u16					hsync_offset;		/* pixels */
	u16					hsync_width;		/* pixels */
	u16					h_blanking;		/* pixels */
	u16					h_active;		/* pixels */

	u16					vsync_offset;		/* lines */
	u16					vsync_width;		/* lines */
	u16					v_blanking;		/* lines */
	u16					v_active;		/* lines */

	u8					interlace;		/* 1: Interlace; 0: Progressive */
	u8					hsync_polarity;		/* 1: Positive; 0: Negative */
	u8					vsync_polarity;		/* 1: Positive; 0: Negative */

	u8					pixel_repetition;	/* repetition times */
	u32					aspect_ratio;		/* 16:9, 4:3, ... */
	colorimetry_t				colorimetry;		/* ITU601, ITU709 */
} amba_hdmi_video_timing_t;

typedef struct {
	ddd_structure_t				ddd_structures[MAX_SUPPORTING_DDD_STRUTURES];
	u32					ddd_num;
} amba_hdmi_ddd_support_t;

typedef struct {
	/* List of supported native timings */
	amba_hdmi_video_timing_t		supported_native_timings[MAX_NATIVE_TIMINGS];
	amba_hdmi_ddd_support_t			supported_ddd_structures[MAX_NATIVE_TIMINGS];

	/* NO. of total timings */
	u32					number;
} amba_hdmi_native_timing_t;

typedef struct {
	/* List of supported CEA/EIA-861B timings */
	u32					supported_cea_timings[MAX_TOTAL_CEA_TIMINGS];
	amba_hdmi_ddd_support_t			supported_ddd_structures[MAX_TOTAL_CEA_TIMINGS];

	/* NO. of total timings */
	u32					number;
} amba_hdmi_cea_timing_t;

const static char * AMBA_3D_STRUCTURE_NAMES[] = {
	"Frame packing",
	"Field alternative",
	"Line alternative",
	"Side-by-Side(Full)",
	"L + depth",
	"L + depth + graphics + grahpics-depth",
	"Top-and-Bottom",
	"Reserved for future use",
	"Side-by-Side(Half)",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Reserved for future use",
	"Not in use",
};

const static amba_hdmi_video_timing_t CEA_Timings[MAX_TOTAL_CEA_TIMINGS] = {
	/* Format 0 */
	{AMBA_VIDEO_MODE_MAX,                         "",         0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 1 */
	{AMBA_VIDEO_MODE_VGA,               "640x480p60",     25175,   16,   96,  160,  640,   10,    2,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_4_3,	COLORIMETRY_NO_DATA},
	/* Format 2 */
	{AMBA_VIDEO_MODE_D1_NTSC,           "720x480p60",     27000,   16,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_4_3,	COLORIMETRY_ITU601 },
	/* Format 3 */
	{AMBA_VIDEO_MODE_D1_NTSC,           "720x480p60",     27000,   16,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU601 },
	/* Format 4 */
	{AMBA_VIDEO_MODE_720P,             "1280x720p60",     74176,  110,   40,  370, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 5 */
	{AMBA_VIDEO_MODE_1080I,           "1920x1080i60",     74176,   88,   44,  280, 1920,    2,    5,   22,  540,    1,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 6 */
	{AMBA_VIDEO_MODE_480I,              "720x480i60",     27000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_4_3,	COLORIMETRY_ITU601 },
	/* Format 7 */
	{AMBA_VIDEO_MODE_480I,              "720x480i60",     27000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU601 },
	/* Format 8 */
	{AMBA_VIDEO_MODE_MAX,               "720x240p60",     27000,   38,  124,  276, 1440,    4,    3,   22,  240,    0,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 9 */
	{AMBA_VIDEO_MODE_MAX,               "720x240p60",     27000,   38,  124,  276, 1440,    4,    3,   22,  240,    0,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 10 */
	{AMBA_VIDEO_MODE_MAX,              "2880x480i60",     54000,   76,  248,  552, 2880,    4,    3,   22,  240,    1,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 11 */
	{AMBA_VIDEO_MODE_MAX,              "2880x480i60",     54000,   76,  248,  552, 2880,    4,    3,   22,  240,    1,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 12 */
	{AMBA_VIDEO_MODE_MAX,              "2880x240p60",     54000,   76,  248,  552, 2880,    4,    3,   22,  240,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 13 */
	{AMBA_VIDEO_MODE_MAX,              "2880x240p60",     54000,   76,  248,  552, 2880,    4,    3,   22,  240,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 14 */
	{AMBA_VIDEO_MODE_MAX,              "1440x480p60",     54000,   32,  124,  276, 1440,    9,    6,   45,  480,    0,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 15 */
	{AMBA_VIDEO_MODE_MAX,              "1440x480p60",     54000,   32,  124,  276, 1440,    9,    6,   45,  480,    0,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 16 */
	{AMBA_VIDEO_MODE_1080P,           "1920x1080p60",    148352,   88,   44,  280, 1920,    4,    5,   45, 1080,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 17 */
	{AMBA_VIDEO_MODE_D1_PAL,            "720x576p50",     27000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_4_3,	COLORIMETRY_ITU601 },
	/* Format 18 */
	{AMBA_VIDEO_MODE_D1_PAL,            "720x576p50",     27000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU601 },
	/* Format 19 */
	{AMBA_VIDEO_MODE_720P_PAL,         "1280x720p50",     74250,  440,   40,  700, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 20 */
	{AMBA_VIDEO_MODE_1080I_PAL,       "1920x1080i50",     74250,  528,   44,  720, 1920,    2,    5,   22,  540,    1,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 21 */
	{AMBA_VIDEO_MODE_576I,              "720x576i50",     27000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_4_3,	COLORIMETRY_ITU601 },
	/* Format 22 */
	{AMBA_VIDEO_MODE_576I,              "720x576i50",     27000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0, 	2,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU601 },
	/* Format 23 */
	{AMBA_VIDEO_MODE_MAX,               "720x288p50",     27000,   24,  126,  288, 1440,    3,    3,   25,  288,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 24 */
	{AMBA_VIDEO_MODE_MAX,               "720x288p50",     27000,   24,  126,  288, 1440,    3,    3,   25,  288,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 25 */
	{AMBA_VIDEO_MODE_MAX,              "2880x576i50",     54000,   48,  252,  576, 2880,    2,    3,   24,  288,    1,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 26 */
	{AMBA_VIDEO_MODE_MAX,              "2880x576i50",     54000,   48,  252,  576, 2880,    2,    3,   24,  288,    1,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 27 */
	{AMBA_VIDEO_MODE_MAX,              "2880x288p50",     54000,   48,  252,  576, 2880,    3,    3,   25,  288,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 28 */
	{AMBA_VIDEO_MODE_MAX,              "2880x288p50",     54000,   48,  252,  576, 2880,    3,    3,   25,  288,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 29 */
	{AMBA_VIDEO_MODE_MAX,              "1440x576p50",     54000,   24,  128,  288, 1440,    5,    5,   49,  576,    0,    0,   1,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 30 */
	{AMBA_VIDEO_MODE_MAX,              "1440x576p50",     54000,   24,  128,  288, 1440,    5,    5,   49,  576,    0,    0,   1,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 31 */
	{AMBA_VIDEO_MODE_1080P_PAL,       "1920x1080p50",    148500,  528,   44,  720, 1920,    4,    5,   45, 1080,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 32 */
	{AMBA_VIDEO_MODE_1080P24,         "1920x1080p24",     74250,  638,   44,  830, 1920,    4,    5,   45, 1080,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 33 */
	{AMBA_VIDEO_MODE_1080P25,         "1920x1080p25",     74250,  528,   44,  720, 1920,    4,    5,   45, 1080,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 34 */
	{AMBA_VIDEO_MODE_1080P30,         "1920x1080p30",     74176,   88,   44,  280, 1920,    4,    5,   45, 1080,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 35 */
	{AMBA_VIDEO_MODE_MAX,              "2880x480p60",    108000,   96,  248,  552, 2880,    9,    6,   45,  480,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 36 */
	{AMBA_VIDEO_MODE_MAX,              "2880x480p60",    108000,   96,  248,  552, 2880,    9,    6,   45,  480,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 37 */
	{AMBA_VIDEO_MODE_MAX,              "2880x576p50",    108000,   48,  256,  576, 2880,    5,    5,   49,  576,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 38 */
	{AMBA_VIDEO_MODE_MAX,              "2880x576p50",    108000,   48,  256,  576, 2880,    5,    5,   49,  576,    0,    0,   0,	4,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 39 */
	{AMBA_VIDEO_MODE_1080I_PAL,       "1920x1080i50",     72000,   32,  168,  384, 1920,   23,    5,   85,  540,    1,    1,   0,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 40 */
	{AMBA_VIDEO_MODE_MAX,            "1920x1080i100",    148500,  528,   44,  720, 1920,    2,    5,   22,  540,    1,    1,   1,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 41 */
	{AMBA_VIDEO_MODE_MAX,             "1280x720p100",    148500,  440,   40,  700, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 42 */
	{AMBA_VIDEO_MODE_MAX,              "720x576p100",     54000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 43 */
	{AMBA_VIDEO_MODE_MAX,              "720x576p100",     54000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 44 */
	{AMBA_VIDEO_MODE_MAX,              "720x576i100",     54000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 45 */
	{AMBA_VIDEO_MODE_MAX,              "720x576i100",     54000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 46 */
	{AMBA_VIDEO_MODE_MAX,            "1920x1080i120",    148352,   88,   44,  280, 1920,    2,    5,   22,  540,    1,    1,   1,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 47 */
	{AMBA_VIDEO_MODE_MAX,             "1280x720p120",    148352,  110,   40,  370, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 48 */
	{AMBA_VIDEO_MODE_MAX,              "720x480p120",     54000,   16,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 49 */
	{AMBA_VIDEO_MODE_MAX,              "720x480p120",     54000,   16,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 50 */
	{AMBA_VIDEO_MODE_MAX,              "720x480i120",     54000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 51 */
	{AMBA_VIDEO_MODE_MAX,              "720x480i120",     54000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 52 */
	{AMBA_VIDEO_MODE_MAX,              "720x576p200",    108000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 53 */
	{AMBA_VIDEO_MODE_MAX,              "720x576p200",    108000,   12,   64,  144,  720,    5,    5,   49,  576,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 54 */
	{AMBA_VIDEO_MODE_MAX,              "720x576i200",    108000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 55 */
	{AMBA_VIDEO_MODE_MAX,              "720x576i200",    108000,   24,  126,  288, 1440,    2,    3,   24,  288,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 56 */
	{AMBA_VIDEO_MODE_MAX,              "720x480p240",    108000,   12,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 57 */
	{AMBA_VIDEO_MODE_MAX,              "720x480p240",    108000,   12,   62,  138,  720,    9,    6,   45,  480,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 58 */
	{AMBA_VIDEO_MODE_MAX,              "720x480i240",    108000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 59 */
	{AMBA_VIDEO_MODE_MAX,              "720x480i240",    108000,   38,  124,  276, 1440,    4,    3,   22,  240,    1,    0,   0,	2,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Format 60 */
	{AMBA_VIDEO_MODE_720P24,           "1280x720p24",     59400, 1760,   40, 2020, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 61 */
	{AMBA_VIDEO_MODE_720P25,           "1280x720p25",     74250, 2420,   40, 2680, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },
	/* Format 62 */
	{AMBA_VIDEO_MODE_720P30,           "1280x720p30",     74176, 1760,   40, 2020, 1280,    5,    5,   30,  720,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_ITU709 },


	/* Extended Format 0 */
	{AMBA_VIDEO_MODE_MAX,                         "",         0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,   0,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
	/* Extended Format 1 */
	{AMBA_VIDEO_MODE_2160P30,         "3840x2160p30",    297000,  176,   88,  560, 3840,   8,   10,   90, 2160,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_NO_DATA},
	/* Extended Format 2 */
	{AMBA_VIDEO_MODE_2160P25,         "3840x2160p25",    297000,  1056,  88, 1440, 3840,   8,   10,   90, 2160,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_NO_DATA},
	/* Extended Format 3 */
	{AMBA_VIDEO_MODE_2160P24,         "3840x2160p24",    297000,  1276,  88, 1660, 3840,   8,   10,   90, 2160,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_16_9,	COLORIMETRY_NO_DATA},
	/* Extended Format 4 */
	{AMBA_VIDEO_MODE_2160P24_SE,      "4096x2160p24",    297000,  1020,  88, 1404, 4096,   8,   10,   90, 2160,    0,    1,   1,	1,	AMBA_VIDEO_RATIO_AUTO,	COLORIMETRY_NO_DATA},
};

typedef struct {
	u16					physical_address;
	u16					logical_address;
} amba_hdmi_cec_t;

typedef struct {
	amba_hdmi_interface_t			interface;
	amba_hdmi_color_space_t			color_space;
	amba_hdmi_product_t			product;
	amba_hdmi_edid_version_t		edid_version;
	amba_hdmi_display_feature_t		display_feature;
	amba_hdmi_color_characteristics_t	color_characteristics;
	amba_hdmi_native_timing_t		native_timings;
	amba_hdmi_cea_timing_t			cea_timings;
	amba_hdmi_ddd_support_t			ddd_structure;
	amba_hdmi_cec_t				cec;
} amba_hdmi_edid_t;

