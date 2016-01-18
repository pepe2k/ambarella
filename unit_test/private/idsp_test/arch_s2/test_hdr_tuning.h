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

#define NO_ARG	0
#define HAS_ARG	1

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#define CHECK_RVAL if(rval<0)	\
	printf("error: %s:%d\n",__func__,__LINE__);

static coring_table_t coring = {{
          18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 12, 8, 6, 6, 4, 4,
	    4, 4, 6, 6, 8, 12, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
          18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
	   18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18}};
#if 0
static tone_curve_t tone_curve_hdr = {
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
	963, 967, 971, 975, 979, 983, 987, 991, 995, 999,1003,1007,1011,1015,1019,1023}};
#endif
statistics_config_t   TileConfig= {
	1,
	1,

	32,
	32,
	0,
	0,
	128,
	128,
	128,
	128,
	0,
	16383,

	12,
	8,
	0,
	8,
	340,
	512,

	16,
	16,
	0,
	0,
	512,
	819,
	512,
	819,

	0,
	16383,
};

enum ITEM_ID
{
	BlackLevelCorrection='A',
	ColorCorrection='B',
	ToneCurve='C',
	RGBtoYUVMatrix='D',
	WhiteBalanceGains='E',
	DGainSaturaionLevel='F',
	LocalExposure='G',
	ChromaScale='H',

	FPNCorrection='I',
	BadPixelCorrection='J',
	CFALeakageFilter='K',
	AntiAliasingFilter='L',
	CFANoiseFilter='M',
	ChromaMedianFiler='N',
	SharpeningA_ASF='O',
	MCTFControl='P',
	SharpeningBControl='Q',
	ColorDependentNoiseReduction='R',
	ChromaNoiseFilter='S',
	GMVSETTING='T',

	ConfigAAAControl='U',
	TileConfiguration='V',
	HDR_LOCALEXPOSURE='W',
	HDR_MIXER='X',
	AFStatisticSetupEx='Y',
	ExposureControl='Z',

	CHECK_IP ='a',
	GET_RAW ='b',
	DSP_DUMP ='c',
	BW_MODE ='d',
	HDR_SHUTTER ='e',
	HDR_BLC='f',
	HDR_CFA ='g',
	HDR_ToneCurve='h',
	HDR_Alpha ='i',
	HDR_CT ='j',
	HDR_WB ='k',
	HDR_AGC ='l',
	HDR_CONTRAST='m',
};

enum REQ_ID
{
	LOAD='0',
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
	u32 agc_index;
	u32 shutter_index;
	u16 dgain;

}EC_INFO;
/*
typedef struct
{
	u16	tile_num_col;
	u16	tile_num_row;
	u16	tile_col_start;
	u16	tile_row_start;
	u16	tile_width;
	u16	tile_height;
	u16	tile_active_width;
	u16	tile_active_height;
	u16	pix_min_value;
	u16	pix_max_value;

}TILE_CONFIG_INFO;
*/
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
//	TILE_CONFIG_INFO tile_config_info;
	statistics_config_t stat_config_info;
	CFA_TILE_INFO   ae_tile_info;
	CFA_TILE_INFO	awb_tile_info;
	CFA_TILE_INFO	af_tile_info;

}TILE_PKG;

typedef struct
{
	fir_t fir;
	coring_table_t coring;
	luma_high_freq_nr_t luma_hfnr_strength;
	u16 linearization_strength;
	max_change_t max_change;
	u8 retain_level;
	sharpen_level_t sharpen_min;
	sharpen_level_t sharpen_overall;

}SHARPEN_PKG;

typedef struct
{
	u8 		select_mode;
	SHARPEN_PKG sa_info;
	asf_info_t	asf_info;

}SA_ASF_PKG;

char bin_fn_hdr[64][64]= {
	"default_MCTF.bin",
	"S2_cmpr_config.bin",
	"lowiso_MCTF.bin",
	"high_iso/01_mctf_level_based.bin",
	"high_iso/02_mctf_vlf_adapt.bin",
	"high_iso/03_mctf_l4c_edge.bin",
	"high_iso/04_mctf_mix_124x.bin",
	"high_iso/05_mctf_mix_main.bin",
	"high_iso/06_mctf_c4.bin",
	"high_iso/07_mctf_c8.bin",
	"high_iso/08_mctf_cA.bin",
	"high_iso/09_mctf_vvlf_adapt.bin",
	"high_iso/10_sec_cc_l4c_edge.bin",
	"high_iso/11_cc_reg_normal.bin",
	"high_iso/12_cc_3d_normal.bin",
	"high_iso/13_cc_out_normal.bin",
	"high_iso/14_thresh_dark_normal.bin",
	"high_iso/15_thresh_hot_normal.bin",
	"high_iso/16_fir1_normal.bin",
	"high_iso/17_fir2_normal.bin",
	"high_iso/18_coring_normal.bin",
	"high_iso/19_lnl_tone_normal.bin",
	"high_iso/20_chroma_scale_gain_normal.bin",
	"high_iso/21_local_exposure_gain_normal.bin",
	"high_iso/22_cmf_table_normal.bin",
	"high_iso/23_alpha_luma.bin",
	"high_iso/24_alpha_chroma.bin",
	"high_iso/25_cc_reg.bin",
	"high_iso/26_cc_3d.bin",
	"high_iso/27_fir1_spat_dep.bin",
	"high_iso/28_fir2_spat_dep.bin",
	"high_iso/29_coring_spat_dep.bin",
	"high_iso/30_fir1_vlf.bin",
	"high_iso/31_fir2_vlf.bin",
	"high_iso/32_coring_vlf.bin",
	"high_iso/33_fir1_nrl.bin",
	"high_iso/34_fir2_nrl.bin",
	"high_iso/35_coring_nrl.bin",
	"high_iso/36_fir1_124x.bin",
	"high_iso/37_coring_coring4x.bin",
	"high_iso/38_coring_coring2x.bin",
	"high_iso/39_coring_coring1x.bin",
	"high_iso/40_fir1_high_noise.bin",
	"high_iso/41_coring_high_noise.bin",
	"high_iso/42_alpha_unity.bin",
	"high_iso/43_zero.bin",
	"high_iso/46_mctf_1.bin",
	"high_iso/47_mctf_2.bin",
	"high_iso/48_mctf_3.bin",
	"high_iso/49_mctf_4.bin",
	"high_iso/50_mctf_5.bin",
	"high_iso/51_mctf_6.bin",
	"high_iso/52_mctf_7.bin",
	"high_iso/53_mctf_8.bin"
};
typedef struct
{
	u8 auto_mode;
	hdr_mixer_config_t mixer_config[MAX_HDR_EXPOSURE_NUM];
}HDR_MIXER_INFO;

typedef struct
{
	int r;
	int g;
	int b;

}blc_3channel;

typedef struct
{
	u8 auto_mode;
	blc_level_t blc_offset[MAX_HDR_EXPOSURE_NUM];

}HDR_BLC_INFO;

typedef struct
{
	u8  auto_mode;
	cfa_noise_filter_info_t cfa_noise_filter_setup[MAX_HDR_EXPOSURE_NUM];

}HDR_CFA_INFO;

typedef struct
{
	u8 auto_mode;
	u8 index_or_row;
	int shutter_index[MAX_HDR_EXPOSURE_NUM];
	int shutter_row[MAX_HDR_EXPOSURE_NUM];
	u16 ae_luma_target[MAX_HDR_EXPOSURE_NUM];

}HDR_SHT_INFO;

typedef struct
{
	u8 auto_mode;
	le_curve_config_t le_config[MAX_HDR_EXPOSURE_NUM];
	local_exposure_t le_info[MAX_HDR_EXPOSURE_NUM];

}HDR_LE_INFO;

typedef struct
{
	u8 auto_mode;
	float alpha;
	u16 speed;
	tone_curve_t tone;

}HDR_TONE_INFO;

typedef struct
{
     histogram_stat_t dsp_histo_data[MAX_HDR_EXPOSURE_NUM];

}HDR_HISTO_PKG;

typedef struct
{
	u8 auto_mode;
	u8 select[MAX_HDR_EXPOSURE_NUM];
	out_lut_level_t outlut_level[MAX_HDR_EXPOSURE_NUM];
	out_lut_table_t outlut_table[MAX_HDR_EXPOSURE_NUM];

}HDR_ALPHA_PKG;

typedef struct
{
	u8 cc_matrix[cc_matrix_size];

}cc_info;

typedef struct
{
	u8 cc_load_flag;
	u8 tone_load_flag;
	tone_curve_t   tone_info[MAX_HDR_EXPOSURE_NUM];
	cc_info cc[MAX_HDR_EXPOSURE_NUM];

}HDR_CT_PKG;

typedef struct
{
	wb_gain_t wb[MAX_HDR_EXPOSURE_NUM];

}HDR_WB_PKG;

typedef struct
{
	int agc[MAX_HDR_EXPOSURE_NUM];
	u32 dgain[MAX_HDR_EXPOSURE_NUM];

}HDR_AGC_INFO;

typedef struct
{
	  u8 enable;
         s16 fir_coeff[10];
         coring_table_t coring;
         u8 radius;

}HDR_CONTRAST_PKG;

