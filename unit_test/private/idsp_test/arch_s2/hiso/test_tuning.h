/********************************************************************
 * test_tuning.h
 *
 * History:
 *	2012/06/23 - [Teng Huang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/

#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"
#include "AmbaDSP_ImgUtility.h"
#include "AmbaDSP_ImgFilter.h"
#include "AmbaDSP_ImgHighIsoFilter.h"

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "AmbaDSP_Img3aStatistics.h"

#ifndef AM_IOCTL
#define AM_IOCTL(_filp, _cmd, _arg)	\
		do { 						\
			if (ioctl(_filp, _cmd, _arg) < 0) {	\
				perror(#_cmd);		\
				return -1;			\
			}						\
		} while (0)
#endif
#define	IMGPROC_PARAM_PATH	"/etc/idsp"
#define INFO_SOCKET_PORT		10006
#define ALL_ITEM_SOCKET_PORT 	10007
#define PREVIEW_SOCKET_PORT 	10008
#define HISTO_SOCKET_PORT 		10009

#define cc_reg_size 		(18752)
#define cc_matrix_size 	(17536)
#define cc_sec_size 		(20608)
#define FIR_COEFF_SIZE  (10)
#define CORING_TABLE_SIZE	(256)
#define MAX_YUV_BUFFER_SIZE		(720*480)		// 1080p
#define ThreeD_size (4096)

//#define MAX_DUMP_BUFFER_SIZE (256*1024)
#define NO_ARG	0
#define HAS_ARG	1

const int n_bin =64;
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define CHECK_RVAL if(rval<0)	printf("error: %s:%d\n",__func__,__LINE__);
#define ROUND_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))



static AMBA_DSP_IMG_LOCAL_EXPOSURE_s local_exposure = { 1, 4, 16, 16, 16, 6,
	{1024,1054,1140,1320,1460,1538,1580,1610,1630,1640,1640,1635,1625,1610,1592,1563,
	1534,1505,1475,1447,1417,1393,1369,1345,1321,1297,1273,1256,1238,1226,1214,1203,
	1192,1180,1168,1157,1149,1142,1135,1127,1121,1113,1106,1098,1091,1084,1080,1077,
	1074,1071,1067,1065,1061,1058,1055,1051,1048,1045,1044,1043,1042,1041,1040,1039,
	1038,1037,1036,1035,1034,1033,1032,1031,1030,1029,1029,1029,1029,1028,1028,1028,
	1028,1027,1027,1027,1026,1026,1026,1026,1025,1025,1025,1025,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,
	1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024,1024}};
static AMBA_DSP_IMG_TONE_CURVE_s tone_curve = {
		{/* red */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* green */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023},
		{/* blue */
                   0,   4,   8,  12,  16,  20,  24,  28,  32,  36,  40,  44,  48,  52,  56,  60,
                  64,  68,  72,  76,  80,  84,  88,  92,  96, 100, 104, 108, 112, 116, 120, 124,
                 128, 132, 136, 140, 144, 148, 152, 156, 160, 164, 168, 173, 177, 181, 185, 189,
                 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233, 237, 241, 245, 249, 253,
                 257, 261, 265, 269, 273, 277, 281, 285, 289, 293, 297, 301, 305, 309, 313, 317,
                 321, 325, 329, 333, 337, 341, 345, 349, 353, 357, 361, 365, 369, 373, 377, 381,
                 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425, 429, 433, 437, 441, 445,
                 449, 453, 457, 461, 465, 469, 473, 477, 481, 485, 489, 493, 497, 501, 505, 509,
                 514, 518, 522, 526, 530, 534, 538, 542, 546, 550, 554, 558, 562, 566, 570, 574,
                 578, 582, 586, 590, 594, 598, 602, 606, 610, 614, 618, 622, 626, 630, 634, 638,
                 642, 646, 650, 654, 658, 662, 666, 670, 674, 678, 682, 686, 690, 694, 698, 702,
                 706, 710, 714, 718, 722, 726, 730, 734, 738, 742, 746, 750, 754, 758, 762, 766,
                 770, 774, 778, 782, 786, 790, 794, 798, 802, 806, 810, 814, 818, 822, 826, 830,
                 834, 838, 842, 846, 850, 855, 859, 863, 867, 871, 875, 879, 883, 887, 891, 895,
                 899, 903, 907, 911, 915, 919, 923, 927, 931, 935, 939, 943, 947, 951, 955, 959,
                 963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}
	};

enum ITEM_ID
{
	TUNING_DONE=0,

	BlackLevelCorrection=65,//'A'
	ColorCorrection,//66
	ToneCurve,//67
	RGBtoYUVMatrix,//68
	WhiteBalanceGains,//69
	DGainSaturaionLevel,//70
	LocalExposure,//71
	ChromaScale,//72

	FPNCorrection,//73
	BadPixelCorrection,//74
	CFALeakageFilter,//75
	AntiAliasingFilter,//76
	CFANoiseFilter,//77
	ChromaMedianFiler,//78
	SharpeningA_ASF,//79
	MCTFControl,//80
	SharpeningBControl,//81
	ColorDependentNoiseReduction,//82
	ChromaNoiseFilter,//83
	GMVSETTING,//84

	ConfigAAAControl,//85
	TileConfiguration,//86
	AFStatisticSetupEx,//87
	ExposureControl,//88

	CHECK_IP =97,//'a'
	GET_RAW ,//98
	DSP_DUMP ,//99
	BW_MODE ,//100
	BP_CALI ,//101
	LENS_CALI,//102
	AWB_CALI,//103
	DEMOSAIC,//104
	MisMatchGr_Gb,//105
	MCTFMBTemporal,//106
	MCTFLEVEL,//107


	ITUNER_HISO_ANTI_ALIASING_STRENGTH =110,
	ITUNER_HISO_CFA_LEAKAGE_FILTER,
	ITUNER_HISO_DYNAMIC_BAD_PIXEL_CORRECTION,
	ITUNER_HISO_CFA_NOISE_FILTER,
	ITUNER_HISO_GB_GR_MISMATCH,
	ITUNER_HISO_DEMOSAIC_FILTER,
	ITUNER_HISO_CHROMA_MEDIAN_FILTER,
	ITUNER_HISO_CDNR,
	ITUNER_HISO_DEFER_COLOR_CORRECTION,

	ITUNER_HISO_ASF,
	ITUNER_HISO_HIGH_ASF,
	ITUNER_HISO_LOW_ASF,
	ITUNER_HISO_MED1_ASF,
	ITUNER_HISO_MED2_ASF,
	ITUNER_HISO_LI2ND_ASF,
	ITUNER_HISO_CHROMA_ASF,

	ITUNER_HISO_SHARPEN_HI_HIGH,
	ITUNER_HISO_SHARPEN_HI_MED,
	ITUNER_HISO_SHARPEN_HI_LI,
	ITUNER_HISO_SHARPEN_HI_LI2ND,

	ITUNER_HISO_CHROMA_FILTER_HIGH,
	ITUNER_HISO_CHROMA_FILTER_LOW_VERY_LOW,
	ITUNER_HISO_CHROMA_FILTER_PRE,
	ITUNER_HISO_CHROMA_FILTER_MED,
	ITUNER_HISO_CHROMA_FILTER_LOW,
	ITUNER_HISO_CHROMA_FILTER_VERY_LOW,
	ITUNER_HISO_CHROMA_FILTER_MED_COMBINE,
	ITUNER_HISO_CHROMA_FILTER_LOW_COMBINE,
	ITUNER_HISO_CHROMA_FILTER_VERY_LOW_COMBINE,
	ITUNER_HISO_LUMA_NOISE_COMBINE,
	ITUNER_HISO_LOW_ASF_COMBINE,
	ITUNER_HIGH_ISO_COMBINE,
	ITUNER_HISO_FREQ_RECOVER,
	ITUNER_HISO_LOW2_LUMA_BLEND,
	ITEM_NUMBER,

};

enum REQ_ID
{
	LOAD=48,//'0',
	APPLY,
	OTHER,

};

enum TAB_ID
{
	OnlineTuning ='0',
	HDRTuning,

};

typedef struct
{
	u8 req_id;
	u8 tab_id;
	u8 item_id;

}TUNING_ID;



typedef struct
{
	u32 gain_tbl_idx;
	u32 shutter_row;
	u32 iris_idx;
	u16 dgain;

}EC_INFO;
typedef struct
{
	 AMBA_DSP_IMG_BLACK_CORRECTION_s blc;
	 u8 defblc_enable;
}BLC_INFO;
typedef struct
{
	u16 cfa_tile_num_col;
	u16 cfa_tile_num_row;
	u16 cfa_tile_col_start;
	u16 cfa_tile_row_start;
	u16 cfa_tile_width;
	u16 cfa_tile_height;
	u16 cfa_tile_active_width;
	u16 cfa_tile_active_height;
	u16 cfa_tile_cfa_y_shift;
	u16 cfa_tile_rgb_y_shift;
	u16 cfa_tile_min_max_shift;

}CFA_TILE_INFO;
typedef struct
{
	AMBA_DSP_IMG_AAA_STAT_INFO_s stat_config_info;

}TILE_PKG;

typedef struct
{
    UINT8 YToneOffset;
    UINT8 YToneShift;
    UINT8 YToneBits;
    UINT8 UToneOffset;
    UINT8 UToneShift;
    UINT8 UToneBits;
    UINT8 VToneOffset;
    UINT8 VToneShift;
    UINT8 VToneBits;
    UINT8 pTable[ThreeD_size];
} TUNING_TABLE_INDEXING_s;

typedef struct
{
    UINT8  Enable;
    UINT8  Mode;
    UINT16 EdgeThresh;
    UINT8  WideEdgeDetect;
    TUNING_TABLE_INDEXING_s    ThreeD;
    AMBA_DSP_IMG_MAX_CHANGE_s   MaxChange;
}TUNING_SHARPEN_BOTH_s;

typedef struct
{
	UINT8 load_3d_flag;
	AMBA_DSP_IMG_FIR_s fir;
	AMBA_DSP_IMG_CORING_s coring;
	AMBA_DSP_IMG_LEVEL_s	ScaleCoring;
	AMBA_DSP_IMG_LEVEL_s	MinCoringResult;
	AMBA_DSP_IMG_LEVEL_s	CoringIndexScale;
	TUNING_SHARPEN_BOTH_s both;
	AMBA_DSP_IMG_SHARPEN_NOISE_s noise;
}SHARPEN_PKG;


typedef struct
{

    UINT8                AlphaMinUp;
    UINT8                AlphaMaxUp;
    UINT8                T0Up;
    UINT8                T1Up;
    UINT8                AlphaMinDown;
    UINT8                AlphaMaxDown;
    UINT8                T0Down;
    UINT8                T1Down;
    TUNING_TABLE_INDEXING_s   ThreeD;
}TUNING_ADPT;

typedef struct
{
    UINT8 			    load_3d_flag;
    UINT8                   Enable;
    AMBA_DSP_IMG_FIR_s      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    TUNING_ADPT      		Adapt;
    AMBA_DSP_IMG_LEVEL_s    LevelStrAdjust;
    AMBA_DSP_IMG_LEVEL_s    T0T1Div;
    UINT8                   MaxChangeUp;
    UINT8                   MaxChangeDown;
    UINT8                   Reserved;   /* to keep 32 alignment */  //not sure
    UINT16                  Reserved1;  /* to keep 32 alignment */
}TUNING_ASF ;

typedef struct
{
    UINT8 YToneOffset;
    UINT8 YToneShift;
    UINT8 YToneBits;
    UINT8 UToneOffset;
    UINT8 UToneShift;
    UINT8 UToneBits;
    UINT8 VToneOffset;
    UINT8 VToneShift;
    UINT8 VToneBits;
    UINT8 pTable[ThreeD_size];

}TUNING_IMG_TABLE_INDEXING_s;

typedef struct
{
    UINT8                   Enable;
    AMBA_DSP_IMG_FIR_s      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    UINT8                   AlphaMin;
    UINT8                   AlphaMax;
    UINT8                   T0;
    UINT8                   T1;
    TUNING_IMG_TABLE_INDEXING_s   ThreeD;
    AMBA_DSP_IMG_LEVEL_s    LevelStrAdjust;
    AMBA_DSP_IMG_LEVEL_s    T0T1Div;
    UINT8                   MaxChange;
    UINT8                   Reserved;   /* to keep 32 alignment */  //not sure
    UINT16                  Reserved1;  /* to keep 32 alignment */


}TUNING_IMG_CHROMA_ASF_INFO_s;

typedef struct  {
    UINT8 T0;
    UINT8 T1;
    UINT8 AlphaMax;
    UINT8 AlphaMin;
    UINT8 MaxChangeNotT0T1LevelBased;
    UINT8 MaxChange;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevel;
    UINT8 SignalPreserve;
    TUNING_IMG_TABLE_INDEXING_s    ThreeD;
} TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s;

typedef struct {
    UINT8 T0Cb;
    UINT8 T0Cr;
    UINT8 T1Cb;
    UINT8 T1Cr;
    UINT8 AlphaMaxCb;
    UINT8 AlphaMaxCr;
    UINT8 AlphaMinCb;
    UINT8 AlphaMinCr;
    UINT8 MaxChangeNotT0T1LevelBasedCb;
    UINT8 MaxChangeNotT0T1LevelBasedCr;
    UINT8 MaxChangeCb;
    UINT8 MaxChangeCr;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevelCb;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevelCr;
    UINT8 SignalPreserveCb;
    UINT8 SignalPreserveCr;
   TUNING_IMG_TABLE_INDEXING_s    ThreeD;
}TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s;

typedef struct  {
    UINT8 T0Cb;
    UINT8 T0Cr;
    UINT8 T0Y;
    UINT8 T1Cb;
    UINT8 T1Cr;
    UINT8 T1Y;
    UINT8 AlphaMaxCb;
    UINT8 AlphaMaxCr;
    UINT8 AlphaMaxY;
    UINT8 AlphaMinCb;
    UINT8 AlphaMinCr;
    UINT8 AlphaMinY;
    UINT8 MaxChangeNotT0T1LevelBasedCb;
    UINT8 MaxChangeNotT0T1LevelBasedCr;
    UINT8 MaxChangeNotT0T1LevelBasedY;
    UINT8 MaxChangeCb;
    UINT8 MaxChangeCr;
    UINT8 MaxChangeY;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevelCb;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevelCr;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevelY;
    UINT8 SignalPreserveCb;
    UINT8 SignalPreserveCr;
    UINT8 SignalPreserveY;
    TUNING_IMG_TABLE_INDEXING_s    ThreeD;
}TUNING_IMG_HISO_COMBINE_s ;

typedef struct
{
	UINT8 		select_mode;
	SHARPEN_PKG sa_info;
	TUNING_ASF	asf_info;

}SA_ASF_PKG;

typedef struct
{
	u32 ae_lin_y[96];
	u32 awb_r[1024];
	u32 awb_g[1024];
	u32 awb_b[1024];
	u16 af_fv2[256];

}AAA_INFO_PKG;
typedef struct {
      UINT8 SharpenBothThreeDTable[8192];
      UINT8 FinalSharpenBothThreeDTable[8192];
      UINT8 AsfInfoThreeDTable[8192];
      UINT8 HisoAsfThreeDTable[8192];
      UINT8 HisoHighAsfThreeDTable[8192];
      UINT8 HisoLowAsfThreeDTable[8192];
      UINT8 HisoMed1AsfThreeDTable[8192];
      UINT8 HisoMed2AsfThreeDTable[8192];
      UINT8 HisoLi2ndAsfThreeDTable[8192];
      UINT8 HisoChromaAsfThreeDTable[8192];
      UINT8 HisoHighSharpenBothThreeDTable[8192];
      UINT8 HisoMedSharpenBothThreeDTable[8192];
      UINT8 HisoLiso1SharpenBothThreeDTable[8192];
      UINT8 HisoLiso2SharpenBothThreeDTable[8192];
      UINT8 HisoChromaFilterMedCombineThreeDTable[8192];
      UINT8 HisoChromaFilterLowCombineThreeDTable[8192];
      UINT8 HisoChromaFilterVeryLowCombineThreeDTable[8192];
      UINT8 HisoLumaNoiseCombineThreeDTable[8192];
      UINT8 HisoLowASFCombineThreeDTable[8192];
      UINT8 HighIsoCombineThreeDTable[8192];
} TUNING_TABLE_s;
void feed_shp_dsp_both(SHARPEN_PKG* sharpen_pkg,AMBA_DSP_IMG_SHARPEN_BOTH_s* both)
{
	both->Enable =sharpen_pkg->both.Enable;
	both->Mode =sharpen_pkg->both.Mode;
	both->EdgeThresh=sharpen_pkg->both.EdgeThresh;
	both->WideEdgeDetect =sharpen_pkg->both.WideEdgeDetect;
	both->ThreeD.YToneShift =sharpen_pkg->both.ThreeD.YToneShift;
	both->ThreeD.UToneShift =sharpen_pkg->both.ThreeD.UToneShift;
	both->ThreeD.VToneShift =sharpen_pkg->both.ThreeD.VToneShift;
	both->ThreeD.YToneBits =sharpen_pkg->both.ThreeD.YToneBits;
	both->ThreeD.UToneBits =sharpen_pkg->both.ThreeD.UToneBits;
	both->ThreeD.VToneBits =sharpen_pkg->both.ThreeD.VToneBits;
	both->ThreeD.YToneOffset=sharpen_pkg->both.ThreeD.YToneOffset;
	both->ThreeD.UToneOffset =sharpen_pkg->both.ThreeD.UToneOffset;
	both->ThreeD.VToneOffset=sharpen_pkg->both.ThreeD.VToneOffset;

	if(sharpen_pkg->both.ThreeD.pTable!=NULL)
		memcpy(both->ThreeD.pTable ,sharpen_pkg->both.ThreeD.pTable,ThreeD_size);
	memcpy(&both->MaxChange,&sharpen_pkg->both.MaxChange,sizeof(AMBA_DSP_IMG_MAX_CHANGE_s));
}
void feed_tuning_both(TUNING_SHARPEN_BOTH_s* tuning_both,AMBA_DSP_IMG_SHARPEN_BOTH_s* both)
{
	tuning_both->Enable =both->Enable;
	tuning_both->Mode =both->Mode;
	tuning_both->EdgeThresh=both->EdgeThresh;
	tuning_both->WideEdgeDetect =both->WideEdgeDetect;
	tuning_both->ThreeD.YToneShift =both->ThreeD.YToneShift;
	tuning_both->ThreeD.UToneShift =both->ThreeD.UToneShift;
	tuning_both->ThreeD.VToneShift =both->ThreeD.VToneShift;
	tuning_both->ThreeD.YToneBits =both->ThreeD.YToneBits;
	tuning_both->ThreeD.UToneBits =both->ThreeD.UToneBits;
	tuning_both->ThreeD.VToneBits =both->ThreeD.VToneBits;
	tuning_both->ThreeD.YToneOffset=both->ThreeD.YToneOffset;
	tuning_both->ThreeD.UToneOffset =both->ThreeD.UToneOffset;
	tuning_both->ThreeD.VToneOffset=both->ThreeD.VToneOffset;
	if(both->ThreeD.pTable!=NULL)
		memcpy(tuning_both->ThreeD.pTable ,both->ThreeD.pTable,ThreeD_size);
	memcpy(&tuning_both->MaxChange,&both->MaxChange,sizeof(AMBA_DSP_IMG_MAX_CHANGE_s));
}


void feed_dsp_chroma_asf(TUNING_IMG_CHROMA_ASF_INFO_s* t_chroma_asf,
	AMBA_DSP_IMG_CHROMA_ASF_INFO_s* dsp_chroma_asf)
{
	dsp_chroma_asf->Enable =t_chroma_asf->Enable;
	memcpy(&dsp_chroma_asf->Fir,&t_chroma_asf->Fir,sizeof(AMBA_DSP_IMG_FIR_s));
	dsp_chroma_asf->DirectionalDecideT0 =t_chroma_asf->DirectionalDecideT0;
	dsp_chroma_asf->DirectionalDecideT1 =t_chroma_asf->DirectionalDecideT1;
	dsp_chroma_asf->AlphaMin =t_chroma_asf->AlphaMin;
	dsp_chroma_asf->AlphaMax =t_chroma_asf->AlphaMax;
	dsp_chroma_asf->T0 =t_chroma_asf->T0;
	dsp_chroma_asf->T1 =t_chroma_asf->T1;

	dsp_chroma_asf->ThreeD.YToneShift=t_chroma_asf->ThreeD.YToneShift;
	dsp_chroma_asf->ThreeD.UToneShift =t_chroma_asf->ThreeD.UToneShift;
	dsp_chroma_asf->ThreeD.VToneShift =t_chroma_asf->ThreeD.VToneShift;
	dsp_chroma_asf->ThreeD.YToneBits =t_chroma_asf->ThreeD.YToneBits;
	dsp_chroma_asf->ThreeD.UToneBits =t_chroma_asf->ThreeD.UToneBits;
	dsp_chroma_asf->ThreeD.VToneBits =t_chroma_asf->ThreeD.VToneBits;
	dsp_chroma_asf->ThreeD.YToneOffset=t_chroma_asf->ThreeD.YToneOffset;
	dsp_chroma_asf->ThreeD.UToneOffset =t_chroma_asf->ThreeD.UToneOffset;
	dsp_chroma_asf->ThreeD.VToneOffset=t_chroma_asf->ThreeD.VToneOffset;

	if(t_chroma_asf->ThreeD.pTable!=NULL)
		memcpy(dsp_chroma_asf->ThreeD.pTable ,t_chroma_asf->ThreeD.pTable,ThreeD_size);
	memcpy(&dsp_chroma_asf->LevelStrAdjust,&t_chroma_asf->LevelStrAdjust,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&dsp_chroma_asf->T0T1Div,&t_chroma_asf->T0T1Div,sizeof(AMBA_DSP_IMG_LEVEL_s));
	dsp_chroma_asf->MaxChange =t_chroma_asf->MaxChange;
}
void feed_tuning_chroma_asf(TUNING_IMG_CHROMA_ASF_INFO_s* t_chroma_asf,
	AMBA_DSP_IMG_CHROMA_ASF_INFO_s* dsp_chroma_asf)
{
	t_chroma_asf->Enable =dsp_chroma_asf->Enable;
	memcpy(&t_chroma_asf->Fir,&dsp_chroma_asf->Fir,sizeof(AMBA_DSP_IMG_FIR_s));
	t_chroma_asf->DirectionalDecideT0 =dsp_chroma_asf->DirectionalDecideT0;
	t_chroma_asf->DirectionalDecideT1 =dsp_chroma_asf->DirectionalDecideT1;
	t_chroma_asf->AlphaMin =dsp_chroma_asf->AlphaMin;
	t_chroma_asf->AlphaMax =dsp_chroma_asf->AlphaMax;
	t_chroma_asf->T0 =dsp_chroma_asf->T0;
	t_chroma_asf->T1 =dsp_chroma_asf->T1;

	t_chroma_asf->ThreeD.YToneShift=dsp_chroma_asf->ThreeD.YToneShift;
	t_chroma_asf->ThreeD.UToneShift =dsp_chroma_asf->ThreeD.UToneShift;
	t_chroma_asf->ThreeD.VToneShift =dsp_chroma_asf->ThreeD.VToneShift;
	t_chroma_asf->ThreeD.YToneBits =dsp_chroma_asf->ThreeD.YToneBits;
	t_chroma_asf->ThreeD.UToneBits =dsp_chroma_asf->ThreeD.UToneBits;
	t_chroma_asf->ThreeD.VToneBits =dsp_chroma_asf->ThreeD.VToneBits;
	t_chroma_asf->ThreeD.YToneOffset=dsp_chroma_asf->ThreeD.YToneOffset;
	t_chroma_asf->ThreeD.UToneOffset =dsp_chroma_asf->ThreeD.UToneOffset;
	t_chroma_asf->ThreeD.VToneOffset=dsp_chroma_asf->ThreeD.VToneOffset;
	if(dsp_chroma_asf->ThreeD.pTable!=NULL)
		memcpy(t_chroma_asf->ThreeD.pTable ,dsp_chroma_asf->ThreeD.pTable,ThreeD_size);
	memcpy(&t_chroma_asf->LevelStrAdjust,&dsp_chroma_asf->LevelStrAdjust,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&t_chroma_asf->T0T1Div,&dsp_chroma_asf->T0T1Div,sizeof(AMBA_DSP_IMG_LEVEL_s));
	t_chroma_asf->MaxChange =dsp_chroma_asf->MaxChange;

}
void feed_dsp_asf(TUNING_ASF* t_asf,AMBA_DSP_IMG_ASF_INFO_s* dsp_asf)
{
	dsp_asf->Enable =t_asf->Enable;
	dsp_asf->DirectionalDecideT0 =t_asf->DirectionalDecideT0;
	dsp_asf->DirectionalDecideT1 =t_asf->DirectionalDecideT1;
	dsp_asf->MaxChangeUp =t_asf->MaxChangeUp;
	dsp_asf->MaxChangeDown =t_asf->MaxChangeDown;

	memcpy(&dsp_asf->Fir,&t_asf->Fir,sizeof(AMBA_DSP_IMG_FIR_s));
	memcpy(&dsp_asf->LevelStrAdjust,&t_asf->LevelStrAdjust,sizeof(AMBA_DSP_IMG_FIR_s));
	memcpy(&dsp_asf->T0T1Div,&t_asf->T0T1Div,sizeof(AMBA_DSP_IMG_FIR_s));

	dsp_asf->Adapt.AlphaMinUp=t_asf->Adapt.AlphaMinUp;
	dsp_asf->Adapt.AlphaMaxUp=t_asf->Adapt.AlphaMaxUp;
	dsp_asf->Adapt.T0Up=t_asf->Adapt.T0Up;
	dsp_asf->Adapt.T1Up=t_asf->Adapt.T1Up;
	dsp_asf->Adapt.AlphaMinDown=t_asf->Adapt.AlphaMinDown;
	dsp_asf->Adapt.AlphaMaxDown=t_asf->Adapt.AlphaMaxDown;
	dsp_asf->Adapt.T0Down=t_asf->Adapt.T0Down;
	dsp_asf->Adapt.T1Down=t_asf->Adapt.T1Down;
	dsp_asf->Adapt.ThreeD.YToneShift=t_asf->Adapt.ThreeD.YToneShift;
	dsp_asf->Adapt.ThreeD.UToneShift =t_asf->Adapt.ThreeD.UToneShift;
	dsp_asf->Adapt.ThreeD.VToneShift =t_asf->Adapt.ThreeD.VToneShift;
	dsp_asf->Adapt.ThreeD.YToneBits =t_asf->Adapt.ThreeD.YToneBits;
	dsp_asf->Adapt.ThreeD.UToneBits =t_asf->Adapt.ThreeD.UToneBits;
	dsp_asf->Adapt.ThreeD.VToneBits =t_asf->Adapt.ThreeD.VToneBits;
	dsp_asf->Adapt.ThreeD.YToneOffset=t_asf->Adapt.ThreeD.YToneOffset;
	dsp_asf->Adapt.ThreeD.UToneOffset =t_asf->Adapt.ThreeD.UToneOffset;
	dsp_asf->Adapt.ThreeD.VToneOffset=t_asf->Adapt.ThreeD.VToneOffset;
	if(t_asf->Adapt.ThreeD.pTable!=NULL)
		memcpy(dsp_asf->Adapt.ThreeD.pTable ,t_asf->Adapt.ThreeD.pTable,ThreeD_size);
}
void feed_tuning_asf(TUNING_ASF* t_asf,AMBA_DSP_IMG_ASF_INFO_s* dsp_asf)
{
	t_asf->Enable =dsp_asf->Enable;
	t_asf->DirectionalDecideT0 =dsp_asf->DirectionalDecideT0;
	t_asf->DirectionalDecideT1 =dsp_asf->DirectionalDecideT1;
	t_asf->MaxChangeUp =dsp_asf->MaxChangeUp;
	t_asf->MaxChangeDown =dsp_asf->MaxChangeDown;
	memcpy(&t_asf->Fir,&dsp_asf->Fir,sizeof(AMBA_DSP_IMG_FIR_s));
	memcpy(&t_asf->LevelStrAdjust,&dsp_asf->LevelStrAdjust,sizeof(AMBA_DSP_IMG_FIR_s));
	memcpy(&t_asf->T0T1Div,&dsp_asf->T0T1Div,sizeof(AMBA_DSP_IMG_FIR_s));

	t_asf->Adapt.AlphaMinUp=dsp_asf->Adapt.AlphaMinUp;
	t_asf->Adapt.AlphaMaxUp=dsp_asf->Adapt.AlphaMaxUp;
	t_asf->Adapt.T0Up=dsp_asf->Adapt.T0Up;
	t_asf->Adapt.T1Up=dsp_asf->Adapt.T1Up;
	t_asf->Adapt.AlphaMinDown=dsp_asf->Adapt.AlphaMinDown;
	t_asf->Adapt.AlphaMaxDown=dsp_asf->Adapt.AlphaMaxDown;
	t_asf->Adapt.T0Down=dsp_asf->Adapt.T0Down;
	t_asf->Adapt.T1Down=dsp_asf->Adapt.T1Down;
	t_asf->Adapt.ThreeD.YToneShift=dsp_asf->Adapt.ThreeD.YToneShift;
	t_asf->Adapt.ThreeD.UToneShift =dsp_asf->Adapt.ThreeD.UToneShift;
	t_asf->Adapt.ThreeD.VToneShift =dsp_asf->Adapt.ThreeD.VToneShift;
	t_asf->Adapt.ThreeD.YToneBits =dsp_asf->Adapt.ThreeD.YToneBits;
	t_asf->Adapt.ThreeD.UToneBits =dsp_asf->Adapt.ThreeD.UToneBits;
	t_asf->Adapt.ThreeD.VToneBits =dsp_asf->Adapt.ThreeD.VToneBits;
	t_asf->Adapt.ThreeD.YToneOffset=dsp_asf->Adapt.ThreeD.YToneOffset;
	t_asf->Adapt.ThreeD.UToneOffset =dsp_asf->Adapt.ThreeD.UToneOffset;
	t_asf->Adapt.ThreeD.VToneOffset=dsp_asf->Adapt.ThreeD.VToneOffset;
	if(dsp_asf->Adapt.ThreeD.pTable!=NULL)
		memcpy(t_asf->Adapt.ThreeD.pTable ,dsp_asf->Adapt.ThreeD.pTable,ThreeD_size);
}
void feed_dsp_high_combine(TUNING_IMG_HISO_COMBINE_s* high_combine,
	AMBA_DSP_IMG_HISO_COMBINE_s* dsp_high_combine)
{
	dsp_high_combine->T0Cb =high_combine->T0Cb;
	dsp_high_combine->T0Cr =high_combine->T0Cr;
	dsp_high_combine->T0Y =high_combine->T0Y;
	dsp_high_combine->T1Cb =high_combine->T1Cb;
	dsp_high_combine->T1Cr =high_combine->T1Cr;
	dsp_high_combine->T1Y =high_combine->T1Y;

	dsp_high_combine->AlphaMaxCb =high_combine->AlphaMaxCb;
	dsp_high_combine->AlphaMaxCr =high_combine->AlphaMaxCr;
	dsp_high_combine->AlphaMaxY =high_combine->AlphaMaxY;
	dsp_high_combine->AlphaMinCb =high_combine->AlphaMinCb;
	dsp_high_combine->AlphaMinCr =high_combine->AlphaMinCr;
	dsp_high_combine->AlphaMinY =high_combine->AlphaMinY;
	dsp_high_combine->MaxChangeNotT0T1LevelBasedCb =high_combine->MaxChangeNotT0T1LevelBasedCb;
	dsp_high_combine->MaxChangeNotT0T1LevelBasedCr =high_combine->MaxChangeNotT0T1LevelBasedCr;
	dsp_high_combine->MaxChangeNotT0T1LevelBasedY =high_combine->MaxChangeNotT0T1LevelBasedY;
	dsp_high_combine->MaxChangeCb =high_combine->MaxChangeCb;
	dsp_high_combine->MaxChangeCr =high_combine->MaxChangeCr;
	dsp_high_combine->MaxChangeY =high_combine->MaxChangeY;
	dsp_high_combine->SignalPreserveCb =high_combine->SignalPreserveCb;
	dsp_high_combine->SignalPreserveCr =high_combine->SignalPreserveCr;
	dsp_high_combine->SignalPreserveY =high_combine->SignalPreserveY;
	memcpy(&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelCb,
		&high_combine->EitherMaxChangeOrT0T1AddLevelCb,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelCr,
		&high_combine->EitherMaxChangeOrT0T1AddLevelCr,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelY,
		&high_combine->EitherMaxChangeOrT0T1AddLevelY,sizeof(AMBA_DSP_IMG_LEVEL_s));
	dsp_high_combine->ThreeD.YToneShift=high_combine->ThreeD.YToneShift;
	dsp_high_combine->ThreeD.UToneShift =high_combine->ThreeD.UToneShift;
	dsp_high_combine->ThreeD.VToneShift =high_combine->ThreeD.VToneShift;
	dsp_high_combine->ThreeD.YToneBits =high_combine->ThreeD.YToneBits;
	dsp_high_combine->ThreeD.UToneBits =high_combine->ThreeD.UToneBits;
	dsp_high_combine->ThreeD.VToneBits =high_combine->ThreeD.VToneBits;
	dsp_high_combine->ThreeD.YToneOffset=high_combine->ThreeD.YToneOffset;
	dsp_high_combine->ThreeD.UToneOffset =high_combine->ThreeD.UToneOffset;
	dsp_high_combine->ThreeD.VToneOffset=high_combine->ThreeD.VToneOffset;
	if(high_combine->ThreeD.pTable!=NULL)
		memcpy(dsp_high_combine->ThreeD.pTable ,high_combine->ThreeD.pTable,ThreeD_size);

}
void feed_tuning_high_combine(TUNING_IMG_HISO_COMBINE_s* high_combine,
	AMBA_DSP_IMG_HISO_COMBINE_s* dsp_high_combine)
{
	high_combine->T0Cb =dsp_high_combine->T0Cb;
	high_combine->T0Cr =dsp_high_combine->T0Cr;
	high_combine->T0Y =dsp_high_combine->T0Y;
	high_combine->T1Cb =dsp_high_combine->T1Cb;
	high_combine->T1Cr =dsp_high_combine->T1Cr;
	high_combine->T1Y =dsp_high_combine->T1Y;

	high_combine->AlphaMaxCb =dsp_high_combine->AlphaMaxCb;
	high_combine->AlphaMaxCr =dsp_high_combine->AlphaMaxCr;
	high_combine->AlphaMaxY =dsp_high_combine->AlphaMaxY;
	high_combine->AlphaMinCb =dsp_high_combine->AlphaMinCb;
	high_combine->AlphaMinCr =dsp_high_combine->AlphaMinCr;
	high_combine->AlphaMinY =dsp_high_combine->AlphaMinY;
	high_combine->MaxChangeNotT0T1LevelBasedCb =dsp_high_combine->MaxChangeNotT0T1LevelBasedCb;
	high_combine->MaxChangeNotT0T1LevelBasedCr =dsp_high_combine->MaxChangeNotT0T1LevelBasedCr;
	high_combine->MaxChangeNotT0T1LevelBasedY =dsp_high_combine->MaxChangeNotT0T1LevelBasedY;
	high_combine->MaxChangeCb =dsp_high_combine->MaxChangeCb;
	high_combine->MaxChangeCr =dsp_high_combine->MaxChangeCr;
	high_combine->MaxChangeY =dsp_high_combine->MaxChangeY;
	high_combine->SignalPreserveCb =dsp_high_combine->SignalPreserveCb;
	high_combine->SignalPreserveCr =dsp_high_combine->SignalPreserveCr;
	high_combine->SignalPreserveY =dsp_high_combine->SignalPreserveY;
	memcpy(&high_combine->EitherMaxChangeOrT0T1AddLevelCb,
		&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelCb,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&high_combine->EitherMaxChangeOrT0T1AddLevelCr,
		&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelCr,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&high_combine->EitherMaxChangeOrT0T1AddLevelY,
		&dsp_high_combine->EitherMaxChangeOrT0T1AddLevelY,sizeof(AMBA_DSP_IMG_LEVEL_s));
	high_combine->ThreeD.YToneShift=dsp_high_combine->ThreeD.YToneShift;
	high_combine->ThreeD.UToneShift =dsp_high_combine->ThreeD.UToneShift;
	high_combine->ThreeD.VToneShift =dsp_high_combine->ThreeD.VToneShift;
	high_combine->ThreeD.YToneBits =dsp_high_combine->ThreeD.YToneBits;
	high_combine->ThreeD.UToneBits =dsp_high_combine->ThreeD.UToneBits;
	high_combine->ThreeD.VToneBits =dsp_high_combine->ThreeD.VToneBits;
	high_combine->ThreeD.YToneOffset=dsp_high_combine->ThreeD.YToneOffset;
	high_combine->ThreeD.UToneOffset =dsp_high_combine->ThreeD.UToneOffset;
	high_combine->ThreeD.VToneOffset=dsp_high_combine->ThreeD.VToneOffset;
	if(dsp_high_combine->ThreeD.pTable!=NULL)
		memcpy(high_combine->ThreeD.pTable ,dsp_high_combine->ThreeD.pTable,ThreeD_size);

}
void feed_dsp_chroma_filter_combine(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s* t_chroma_filter_combine,
	AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s* dsp_chroma_filter_combine)
{
	dsp_chroma_filter_combine->T0Cb =t_chroma_filter_combine->T0Cb;
	dsp_chroma_filter_combine->T0Cr =t_chroma_filter_combine->T0Cr;
	dsp_chroma_filter_combine->T1Cb =t_chroma_filter_combine->T1Cb;
	dsp_chroma_filter_combine->T1Cr =t_chroma_filter_combine->T1Cr;
	dsp_chroma_filter_combine->AlphaMaxCb =t_chroma_filter_combine->AlphaMaxCb;
	dsp_chroma_filter_combine->AlphaMaxCr =t_chroma_filter_combine->AlphaMaxCr;
	dsp_chroma_filter_combine->AlphaMinCb =t_chroma_filter_combine->AlphaMinCb;
	dsp_chroma_filter_combine->AlphaMinCr =t_chroma_filter_combine->AlphaMinCr;
	dsp_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCb =t_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCb;
	dsp_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCr =t_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCr;
	dsp_chroma_filter_combine->MaxChangeCb =t_chroma_filter_combine->MaxChangeCb;
	dsp_chroma_filter_combine->MaxChangeCr =t_chroma_filter_combine->MaxChangeCr;
	memcpy(&dsp_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCb,
		&t_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCb,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&dsp_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCr,
		&t_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCr,sizeof(AMBA_DSP_IMG_LEVEL_s));
	dsp_chroma_filter_combine->SignalPreserveCb =t_chroma_filter_combine->SignalPreserveCb;
	dsp_chroma_filter_combine->SignalPreserveCr =t_chroma_filter_combine->SignalPreserveCr;
	dsp_chroma_filter_combine->ThreeD.YToneShift=t_chroma_filter_combine->ThreeD.YToneShift;
	dsp_chroma_filter_combine->ThreeD.UToneShift =t_chroma_filter_combine->ThreeD.UToneShift;
	dsp_chroma_filter_combine->ThreeD.VToneShift =t_chroma_filter_combine->ThreeD.VToneShift;
	dsp_chroma_filter_combine->ThreeD.YToneBits =t_chroma_filter_combine->ThreeD.YToneBits;
	dsp_chroma_filter_combine->ThreeD.UToneBits =t_chroma_filter_combine->ThreeD.UToneBits;
	dsp_chroma_filter_combine->ThreeD.VToneBits =t_chroma_filter_combine->ThreeD.VToneBits;
	dsp_chroma_filter_combine->ThreeD.YToneOffset=t_chroma_filter_combine->ThreeD.YToneOffset;
	dsp_chroma_filter_combine->ThreeD.UToneOffset =t_chroma_filter_combine->ThreeD.UToneOffset;
	dsp_chroma_filter_combine->ThreeD.VToneOffset=t_chroma_filter_combine->ThreeD.VToneOffset;
	if(t_chroma_filter_combine->ThreeD.pTable!=NULL)
		memcpy(dsp_chroma_filter_combine->ThreeD.pTable ,t_chroma_filter_combine->ThreeD.pTable,ThreeD_size);

}
void feed_tuning_chroma_filter_combine(TUNING_IMG_HISO_CHROMA_FILTER_COMBINE_s* t_chroma_filter_combine,
	AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s* dsp_chroma_filter_combine)
{
	t_chroma_filter_combine->T0Cb =dsp_chroma_filter_combine->T0Cb;
	t_chroma_filter_combine->T0Cr =dsp_chroma_filter_combine->T0Cr;
	t_chroma_filter_combine->T1Cb =dsp_chroma_filter_combine->T1Cb;
	t_chroma_filter_combine->T1Cr =dsp_chroma_filter_combine->T1Cr;
	t_chroma_filter_combine->AlphaMaxCb =dsp_chroma_filter_combine->AlphaMaxCb;
	t_chroma_filter_combine->AlphaMaxCr =dsp_chroma_filter_combine->AlphaMaxCr;
	t_chroma_filter_combine->AlphaMinCb =dsp_chroma_filter_combine->AlphaMinCb;
	t_chroma_filter_combine->AlphaMinCr =dsp_chroma_filter_combine->AlphaMinCr;
	t_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCb =dsp_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCb;
	t_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCr =dsp_chroma_filter_combine->MaxChangeNotT0T1LevelBasedCr;
	t_chroma_filter_combine->MaxChangeCb =dsp_chroma_filter_combine->MaxChangeCb;
	t_chroma_filter_combine->MaxChangeCr =dsp_chroma_filter_combine->MaxChangeCr;
	memcpy(&t_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCb,
		&dsp_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCb,sizeof(AMBA_DSP_IMG_LEVEL_s));
	memcpy(&t_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCr,
		&dsp_chroma_filter_combine->EitherMaxChangeOrT0T1AddLevelCr,sizeof(AMBA_DSP_IMG_LEVEL_s));
	t_chroma_filter_combine->SignalPreserveCb =dsp_chroma_filter_combine->SignalPreserveCb;
	t_chroma_filter_combine->SignalPreserveCr =dsp_chroma_filter_combine->SignalPreserveCr;
	t_chroma_filter_combine->ThreeD.YToneShift=dsp_chroma_filter_combine->ThreeD.YToneShift;
	t_chroma_filter_combine->ThreeD.UToneShift =dsp_chroma_filter_combine->ThreeD.UToneShift;
	t_chroma_filter_combine->ThreeD.VToneShift =dsp_chroma_filter_combine->ThreeD.VToneShift;
	t_chroma_filter_combine->ThreeD.YToneBits =dsp_chroma_filter_combine->ThreeD.YToneBits;
	t_chroma_filter_combine->ThreeD.UToneBits =dsp_chroma_filter_combine->ThreeD.UToneBits;
	t_chroma_filter_combine->ThreeD.VToneBits =dsp_chroma_filter_combine->ThreeD.VToneBits;
	t_chroma_filter_combine->ThreeD.YToneOffset=dsp_chroma_filter_combine->ThreeD.YToneOffset;
	t_chroma_filter_combine->ThreeD.UToneOffset =dsp_chroma_filter_combine->ThreeD.UToneOffset;
	t_chroma_filter_combine->ThreeD.VToneOffset=dsp_chroma_filter_combine->ThreeD.VToneOffset;
	if(dsp_chroma_filter_combine->ThreeD.pTable!=NULL)
		memcpy(t_chroma_filter_combine->ThreeD.pTable ,dsp_chroma_filter_combine->ThreeD.pTable,ThreeD_size);

}
void feed_dsp_luma_noise_combine(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s* t_luma_noise_combine,
	AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s* dsp_luma_noise_combine)
{

	dsp_luma_noise_combine->T0 =t_luma_noise_combine->T0;
	dsp_luma_noise_combine->T1 =t_luma_noise_combine->T1;
	dsp_luma_noise_combine->AlphaMax=t_luma_noise_combine->AlphaMax;
	dsp_luma_noise_combine->AlphaMin=t_luma_noise_combine->AlphaMin;
	dsp_luma_noise_combine->MaxChangeNotT0T1LevelBased=t_luma_noise_combine->MaxChangeNotT0T1LevelBased;
	dsp_luma_noise_combine->MaxChange =t_luma_noise_combine->MaxChange;
	dsp_luma_noise_combine->SignalPreserve =t_luma_noise_combine->SignalPreserve;

	memcpy(&dsp_luma_noise_combine->EitherMaxChangeOrT0T1AddLevel,
		&t_luma_noise_combine->EitherMaxChangeOrT0T1AddLevel,sizeof(AMBA_DSP_IMG_LEVEL_s));
	dsp_luma_noise_combine->ThreeD.YToneShift=t_luma_noise_combine->ThreeD.YToneShift;
	dsp_luma_noise_combine->ThreeD.UToneShift =t_luma_noise_combine->ThreeD.UToneShift;
	dsp_luma_noise_combine->ThreeD.VToneShift =t_luma_noise_combine->ThreeD.VToneShift;
	dsp_luma_noise_combine->ThreeD.YToneBits =t_luma_noise_combine->ThreeD.YToneBits;
	dsp_luma_noise_combine->ThreeD.UToneBits =t_luma_noise_combine->ThreeD.UToneBits;
	dsp_luma_noise_combine->ThreeD.VToneBits =t_luma_noise_combine->ThreeD.VToneBits;
	dsp_luma_noise_combine->ThreeD.YToneOffset=t_luma_noise_combine->ThreeD.YToneOffset;
	dsp_luma_noise_combine->ThreeD.UToneOffset =t_luma_noise_combine->ThreeD.UToneOffset;
	dsp_luma_noise_combine->ThreeD.VToneOffset=t_luma_noise_combine->ThreeD.VToneOffset;
	if(t_luma_noise_combine->ThreeD.pTable!=NULL)
		memcpy(dsp_luma_noise_combine->ThreeD.pTable ,t_luma_noise_combine->ThreeD.pTable,ThreeD_size);
}
void feed_tuning_luma_noise_combine(TUNING_IMG_HISO_LUMA_FILTER_COMBINE_s* t_luma_noise_combine,
	AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s* dsp_luma_noise_combine)
{

	t_luma_noise_combine->T0 =dsp_luma_noise_combine->T0;
	t_luma_noise_combine->T1 =dsp_luma_noise_combine->T1;
	t_luma_noise_combine->AlphaMax=dsp_luma_noise_combine->AlphaMax;
	t_luma_noise_combine->AlphaMin=dsp_luma_noise_combine->AlphaMin;
	t_luma_noise_combine->MaxChangeNotT0T1LevelBased=dsp_luma_noise_combine->MaxChangeNotT0T1LevelBased;
	t_luma_noise_combine->MaxChange =dsp_luma_noise_combine->MaxChange;
	t_luma_noise_combine->SignalPreserve =dsp_luma_noise_combine->SignalPreserve;

	memcpy(&t_luma_noise_combine->EitherMaxChangeOrT0T1AddLevel,
		&dsp_luma_noise_combine->EitherMaxChangeOrT0T1AddLevel,sizeof(AMBA_DSP_IMG_LEVEL_s));
	t_luma_noise_combine->ThreeD.YToneShift=dsp_luma_noise_combine->ThreeD.YToneShift;
	t_luma_noise_combine->ThreeD.UToneShift =dsp_luma_noise_combine->ThreeD.UToneShift;
	t_luma_noise_combine->ThreeD.VToneShift =dsp_luma_noise_combine->ThreeD.VToneShift;
	t_luma_noise_combine->ThreeD.YToneBits =dsp_luma_noise_combine->ThreeD.YToneBits;
	t_luma_noise_combine->ThreeD.UToneBits =dsp_luma_noise_combine->ThreeD.UToneBits;
	t_luma_noise_combine->ThreeD.VToneBits =dsp_luma_noise_combine->ThreeD.VToneBits;
	t_luma_noise_combine->ThreeD.YToneOffset=dsp_luma_noise_combine->ThreeD.YToneOffset;
	t_luma_noise_combine->ThreeD.UToneOffset =dsp_luma_noise_combine->ThreeD.UToneOffset;
	t_luma_noise_combine->ThreeD.VToneOffset=dsp_luma_noise_combine->ThreeD.VToneOffset;
	if(dsp_luma_noise_combine->ThreeD.pTable!=NULL)
		memcpy(t_luma_noise_combine->ThreeD.pTable ,dsp_luma_noise_combine->ThreeD.pTable,ThreeD_size);
}


