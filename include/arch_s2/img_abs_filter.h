#ifndef IMG_ABS_FILTER_H
#define IMG_ABS_FILTER_H

#include "basetypes.h"

#define ADJ_MAX_ENTRY_NUM	(16)
#define ADJ_MAX_INNER_TBL_NUM	(4)

typedef enum{
	ADJ_AE_TARGET = 0x0,
	ADJ_WB_RATIO = 0x1,
	ADJ_BLC = 0x2,
	ADJ_ANTIALIASING = 0x3,
	ADJ_GRGB_MISMATCH = 0x4,
	ADJ_DPC = 0x5,
	ADJ_CFANF_LOW = 0x6,
	ADJ_CFANF_HIGH = 0x7,
	ADJ_LE = 0x8,
	ADJ_DEMOSAIC = 0x9,
	ADJ_CC = 0xA,
	ADJ_TONE = 0xB,
	ADJ_RGB2YUV = 0xC,
	ADJ_CHROMA_SCALE = 0xD,
	ADJ_CHROMA_MEDIAN = 0xE,
	ADJ_CHROMANF = 0xF,
	ADJ_CDNR = 0x10,
	ADJ_1STMODE_SEL = 0x11,
	ADJ_ASF = 0x12,
	ADJ_1ST_SHPBOTH = 0x13,
	ADJ_1ST_SHPNOISE = 0x14,
	ADJ_1ST_SHPFIR = 0x15,
	ADJ_1ST_SHPCORING = 0x16,
	ADJ_1ST_SHPCORING_INDEX_SCALE = 0x17,
	ADJ_1ST_SHPCORING_MIN_RESULT = 0x18,
	ADJ_1ST_SHPCORING_SCALE_CORING = 0x19,
	ADJ_FINAL_SHPBOTH = 0x1A,
	ADJ_FINAL_SHPNOISE = 0x1B,
	ADJ_FINAL_SHPFIR = 0x1C,
	ADJ_FINAL_SHPCORING = 0x1D,
	ADJ_FINAL_SHPCORING_INDEX_SCALE = 0x1E,
	ADJ_FINAL_SHPCORING_MIN_RESULT = 0x1F,
	ADJ_FINAL_SHPCORING_SCALE_CORING = 0x20,
	ADJ_VIDEO_MCTF = 0x21,

	ADJ_HISO_ANTIALIASING,		//34
	ADJ_HISO_GRGB_MISMATCH,		//35
	ADJ_HISO_DPC,			//36
	ADJ_HISO_CFANF,			//37
	ADJ_HISO_DEMOSAIC,		//38
	ADJ_HISO_CHROMA_MEDIAN,		//39
	ADJ_HISO_CDNR,			//40

	ADJ_HISO_ASF,			//41
	ADJ_HISO_HIGH_ASF,		//42
	ADJ_HISO_LOW_ASF,		//43
	ADJ_HISO_MED1_ASF,		//44
	ADJ_HISO_MED2_ASF,		//45

	ADJ_HISO_HIGH_SHARPEN_BOTH,	//46
	ADJ_HISO_HIGH_SHARPEN_NOISE,
	ADJ_HISO_HIGH_SHARPEN_FIR,
	ADJ_HISO_HIGH_SHARPEN_CORING,
	ADJ_HISO_HIGH_SHARPEN_INDEX_SCALE,
	ADJ_HISO_HIGH_SHARPEN_MIN_RESULT,
	ADJ_HISO_HIGH_SHARPEN_SCALE_CORING,

	ADJ_HISO_MED_SHARPEN_BOTH,
	ADJ_HISO_MED_SHARPEN_NOISE,
	ADJ_HISO_MED_SHARPEN_FIR,
	ADJ_HISO_MED_SHARPEN_CORING,
	ADJ_HISO_MED_SHARPEN_INDEX_SCALE,
	ADJ_HISO_MED_SHARPEN_MIN_RESULT,
	ADJ_HISO_MED_SHARPEN_SCALE_CORING,

	ADJ_HISO_LISO_SHARPEN_BOTH,
	ADJ_HISO_LISO_SHARPEN_NOISE,
	ADJ_HISO_LISO_SHARPEN_FIR,
	ADJ_HISO_LISO_SHARPEN_CORING,
	ADJ_HISO_LISO_SHARPEN_INDEX_SCALE,
	ADJ_HISO_LISO_SHARPEN_MIN_RESULT,
	ADJ_HISO_LISO_SHARPEN_SCALE_CORING,

	ADJ_HISO_CHROMA_HIGH,
	ADJ_HISO_CHROMA_LOW_VERY_LOW,
	ADJ_HISO_CHROMA_PRE,
	ADJ_HISO_CHROMA_MED,
	ADJ_HISO_CHROMA_LOW,
	ADJ_HISO_CHROMA_VERY_LOW,

	ADJ_HISO_CHROMA_MED_COMBINE,
	ADJ_HISO_CHROMA_LOW_COMBINE,
	ADJ_HISO_CHROMA_VERY_LOW_COMBINE,
	ADJ_HISO_LUMA_NOISE_COMBINE,
	ADJ_HISO_LOW_ASF_COMBINE,
	ADJ_HISO_ISO_COMBINE,
	ADJ_HISO_FREQ_RECOVER,
	ADJ_TEMPORAL_ADJUST,

	ADJ_FILTER_NUM,
}filter_id;

typedef struct argu2 {int argu[2];} argu2;
typedef struct argu4 {int argu[4];} argu4;
typedef struct argu8 {int argu[8];} argu8;
typedef struct argu16 {int argu[16];} argu16;
typedef struct argu32 {int argu[32];} argu32;
typedef struct argu64 {int argu[64];} argu64;

//for u16 PerDirFirIsoStrengths[9]
typedef struct table_u16x16 {u16 element[16];} table_u16x16;
//for s16  Coefs[9][25]
typedef struct table_s16x256 {s16 element[256];} table_s16x256;
//for u8   Coring[256];
typedef struct table_u8x256 {u8 element[256];} table_u8x256;
// for rgb2yuv[12]
typedef struct table_s16x16 {s16 element[16];} table_s16x16;
// for u16  CS[256];
typedef struct table_u16x128 {u16 element[128];} table_u16x128;
// for  u16  ToneCurve[256]; LE: u16  GainCurveTable[256];
typedef struct table_u16x256 {u16 element[256];} table_u16x256;
//for cc 3d
typedef struct table_u32x4096 {u32 element[4096];} table_u32x4096;


typedef struct tbl_des_s{
	int auto_adj_tbl;
	int active_element_num;
	int tbl_entry_start_idx;
	int active_tbl_entry_idx;
}tbl_des_t;

typedef struct filter_container_header_s{
	int filter_container_id;
	int auto_adj_argu;	//-1 not in use, 0 no interpo, 1 interpo, 2 only for init
	int active_argu_num;
	int argu_entry_start_idx;
	int active_argu_entry_idx;
	int inner_tbl_num;
	tbl_des_t tbl_info[ADJ_MAX_INNER_TBL_NUM];
}filter_container_header_t;

//AMBA_DSP_IMG_ASF_INFO_s
typedef struct filter_container_p64_t4_s{
	filter_container_header_t header;
	argu64 param[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl2[ADJ_MAX_ENTRY_NUM];
	table_s16x256 tbl3[ADJ_MAX_ENTRY_NUM];
}filter_container_p64_t4_t;

//AMBA_DSP_IMG_SHARPEN_NOISE_s
typedef struct filter_container_p16_t4_s{
	filter_container_header_t header;
	argu16 param[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x16 tbl2[ADJ_MAX_ENTRY_NUM];
	table_s16x256 tbl3[ADJ_MAX_ENTRY_NUM];
}filter_container_p16_t4_t;

//AMBA_DSP_IMG_LOCAL_EXPOSURE_s, AMBA_DSP_IMG_CHROMA_SCALE_s
typedef struct filter_container_p8_t1_s{
	filter_container_header_t header;
	argu8 param[ADJ_MAX_ENTRY_NUM];
	table_u16x128 tbl0[ADJ_MAX_ENTRY_NUM];
}filter_container_p8_t1_t;

//AMBA_DSP_IMG_CORING_s
typedef struct filter_container_p0_t1_s{
	filter_container_header_t header;
	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u8x256 tbl0[ADJ_MAX_ENTRY_NUM];
}filter_container_p0_t1_t;

//Tone curve
typedef struct filter_container_p0_t3_s{
	filter_container_header_t header;
//	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u16x256 tbl0[ADJ_MAX_ENTRY_NUM];
	table_u16x256 tbl1[ADJ_MAX_ENTRY_NUM];
	table_u16x256 tbl2[ADJ_MAX_ENTRY_NUM];
}filter_container_p0_t3_t;

//rgb2yuv
typedef struct filter_container_p4_t3_s{
	filter_container_header_t header;
	argu4 param[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl0[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl1[ADJ_MAX_ENTRY_NUM];
	table_s16x16 tbl2[ADJ_MAX_ENTRY_NUM];
}filter_container_p4_t3_t;

typedef struct filter_container_p2_s{
	filter_container_header_t header;
	argu2 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p2_t;

typedef struct filter_container_p4_s{
	filter_container_header_t header;
	argu4 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p4_t;

typedef struct filter_container_p8_s{
	filter_container_header_t header;
	argu8 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p8_t;

typedef struct filter_container_p16_s{
	filter_container_header_t header;
	argu16 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p16_t;

typedef struct filter_container_p32_s{
	filter_container_header_t header;
	argu32 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p32_t;

typedef struct filter_container_p64_s{
	filter_container_header_t header;
	argu64 param[ADJ_MAX_ENTRY_NUM];
}filter_container_p64_t;


typedef struct filter_header_s{
	int filter_id;
	int filter_update_period;
	int filter_update_cnt;
	int filter_update_flag;
	int inner_tbl_num;
	int force_commit_flag;
}filter_header_t;

typedef struct filter_p64_t4_s{
	filter_header_t header;
	argu64 param;
	table_u16x16 tbl0;
	table_u16x16 tbl1;
	table_u16x16 tbl2;
	table_s16x256 tbl3;
}filter_p64_t4_t;

typedef struct filter_p16_t4_s{
	filter_header_t header;
	argu16 param;
	table_u16x16 tbl0;
	table_u16x16 tbl1;
	table_u16x16 tbl2;
	table_s16x256 tbl3;
}filter_p16_t4_t;

typedef struct filter_p8_t1_s{
	filter_header_t header;
	argu8 param;
	table_u16x128 tbl0;
}filter_p8_t1_t;

typedef struct filter_p0_t1_s{
	filter_header_t header;
	table_u8x256 tbl0;
}filter_p0_t1_t;

typedef struct filter_p0_t3_s{
	filter_header_t header;
//	argu2 param;	//add this field only for make itself different size than filter_container_p64_t
	table_u16x256 tbl0;
	table_u16x256 tbl1;
	table_u16x256 tbl2;
}filter_p0_t3_t;

typedef struct filter_p4_t3_s{
	filter_header_t header;
	argu4 param;
	table_s16x16 tbl0;
	table_s16x16 tbl1;
	table_s16x16 tbl2;
}filter_p4_t3_t;

typedef struct filter_p2_s{
	filter_header_t header;
	argu2 param;
}filter_p2_t;

typedef struct filter_p4_s{
	filter_header_t header;
	argu4 param;
}filter_p4_t;

typedef struct filter_p8_s{
	filter_header_t header;
	argu8 param;
}filter_p8_t;

typedef struct filter_p16_s{
	filter_header_t header;
	argu16 param;
}filter_p16_t;

typedef struct filter_p32_s{
	filter_header_t header;
	argu32 param;
}filter_p32_t;

typedef struct filter_p64_s{
	filter_header_t header;
	argu64 param;
}filter_p64_t;
#endif
