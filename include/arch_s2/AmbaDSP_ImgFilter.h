/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaDSP_ImgFilter.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Ambarella DSP Image Kernel Normal Filter APIs
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/12/2012  Steve Chen  Created
 *
 *  $LastChangedDate: 2013-11-20 15:28:50 +0800 (週三, 20 十一月 2013) $
 *  $LastChangedRevision: 7498 $
 *  $LastChangedBy: ychuanga $
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/DSP/inc/AmbaDSP_ImgFilter.h $
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_DSP_IMG_FILTER_H_
#define _AMBA_DSP_IMG_FILTER_H_

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"

#define AMBA_DSP_IMG_NUM_EXPOSURE_CURVE         256
#define AMBA_DSP_IMG_NUM_TONE_CURVE             256
#define AMBA_DSP_IMG_NUM_CHROMA_GAIN_CURVE      128
#define AMBA_DSP_IMG_NUM_CORING_TABLE_INDEX     256
#define AMBA_DSP_IMG_NUM_MAX_FIR_COEFF          10

#define AMBA_DSP_IMG_CC_3D_SIZE                 (17536)
#define AMBA_DSP_IMG_SEC_CC_SIZE                (20608)
#define AMBA_DSP_IMG_CC_REG_SIZE                (18752)

#define AMBA_DSP_IMG_AWB_UNIT_SHIFT             12

#define AMBA_DSP_IMG_MCTF_CFG_SIZE              27600
#define AMBA_DSP_IMG_CC_CFG_SIZE                20608
#define AMBA_DSP_IMG_CMPR_CFG_SIZE              544

typedef enum _AMBA_DSP_IMG_BAYER_PATTERN_e_ {
    AMBA_DSP_IMG_BAYER_PATTERN_RG = 0,
    AMBA_DSP_IMG_BAYER_PATTERN_BG,
    AMBA_DSP_IMG_BAYER_PATTERN_GR,
    AMBA_DSP_IMG_BAYER_PATTERN_GB
} AMBA_DSP_IMG_BAYER_PATTERN_e;

typedef struct _AMBA_DSP_IMG_SENSOR_INFO_s_ {
    UINT8   SensorID;
    UINT8   NumFieldsPerFormat; /* maxumum 8 fields per frame */
    UINT8   SensorResolution;   /* Number of bits for data representation */
    UINT8   SensorPattern;      /* Bayer patterns RG, BG, GR, GB */
    UINT8   FirstLineField[8];
    UINT32  SensorReadOutMode;
} AMBA_DSP_IMG_SENSOR_INFO_s;

typedef struct _AMBA_DSP_IMG_BLACK_CORRECTION_s_ {
    INT16   BlackR;
    INT16   BlackGr;
    INT16   BlackGb;
    INT16   BlackB;
} AMBA_DSP_IMG_BLACK_CORRECTION_s;

#define    AMBA_DSP_IMG_VIG_VER_1_0     0x20130218

typedef enum _AMBA_DSP_IMG_VIGNETTE_VIGSTRENGTHEFFECT_MODE_e_ {
    AMBA_DSP_IMG_VIGNETTE_DefaultMode = 0,
    AMBA_DSP_IMG_VIGNETTE_KeepRatioMode = 1,
} AMBA_DSP_IMG_VIGNETTE_VIGSTRENGTHEFFECT_MODE_e;

typedef struct _AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s_ {
    UINT32  Version;
    int     TableWidth;
    int     TableHeight;
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  CalibVinSensorGeo;
    UINT32  Reserved;                   // Reserved for extention.
    UINT32  Reserved1;                  // Reserved for extention.
    UINT16  *pVignetteRedGain;          // Vignette table array addr.
    UINT16  *pVignetteGreenEvenGain;    // Vignette table array addr.
    UINT16  *pVignetteGreenOddGain;     // Vignette table array addr.
    UINT16  *pVignetteBlueGain;         // Vignette table array addr.
} AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s;

#define AMBA_IMG_DSP_VIGNETTE_CONTROL_VERT_FLIP    0x1  /* vertical flip. */
typedef struct _AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s_ {
    UINT8   Enb;
    UINT8   GainShift;
    UINT8   VigStrengthEffectMode;
    UINT8   Control;
    UINT32  ChromaRatio;
    UINT32  VigStrength;
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  CurrentVinSensorGeo;
    AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s  CalibVignetteInfo;
} AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s;

typedef struct _AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s_ {
    UINT8   Enable;
    UINT16  GainShift;
    UINT16  *pRedGain;             // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pGreenEvenGain;       // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pGreenOddGain;        // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pBlueGain;            // Pointer to one of tables in gain_path of A7l structure
} AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s;

typedef struct _AMBA_DSP_IMG_CFA_LEACKAGE_FILTER_s_ {
    UINT32  Enb;
    INT8    AlphaRR;
    INT8    AlphaRB;
    INT8    AlphaBR;
    INT8    AlphaBB;
    UINT16  SaturationLevel;
} AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s;

typedef struct _AMBA_DSP_IMG_DBP_CORRECTION_s_ {
    UINT8   Enb;    /* 0: disable                       */
                    /* 1: hot 1st order, dark 2nd order */
                    /* 2: hot 2nd order, dark 1st order */
                    /* 3: hot 2nd order, dark 2nd order */
                    /* 4: hot 1st order, dark 1st order */
    UINT8   HotPixelStrength;
    UINT8   DarkPixelStrength;
    UINT8   CorrectionMethod;   /* 0: video, 1:still    */
} AMBA_DSP_IMG_DBP_CORRECTION_s;

#define    AMBA_DSP_IMG_SBP_VER_1_0     0x20130218
typedef struct _AMBA_DSP_IMG_CALIB_SBP_INFO_s_ {
    UINT32  Version;            /* 0x20130218 */
    UINT8   *SbpBuffer;
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;/* Vin sensor geometry when calibrating. */
    UINT32  Reserved;           /* Reserved for extention. */
    UINT32  Reserved1;          /* Reserved for extention. */
} AMBA_DSP_IMG_CALIB_SBP_INFO_s;

typedef struct _AMBA_DSP_IMG_SBP_CORRECTION_s_ {
    UINT8   Enb;
    UINT8   Reserved[3];
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  CurrentVinSensorGeo;
    AMBA_DSP_IMG_CALIB_SBP_INFO_s       CalibSbpInfo;
} AMBA_DSP_IMG_SBP_CORRECTION_s;

typedef struct _AMBA_DSP_IMG_BYPASS_SBP_INFO_s_ {
    UINT8   Enable;
    UINT16  PixelMapWidth;
    UINT16  PixelMapHeight;
    UINT16  PixelMapPitch;
    UINT8   *pMap;
} AMBA_DSP_IMG_BYPASS_SBP_INFO_s;

typedef struct _AMBA_DSP_IMG_GRID_POINT_s_ {
    INT16   X;
    INT16   Y;
} AMBA_DSP_IMG_GRID_POINT_s;

#define    AMBA_DSP_IMG_CAWARP_VER_1_0     0x20130125

typedef struct _AMBA_DSP_IMG_CALIB_CAWARP_INFO_s_ {
    UINT32  Version;            /* 0x20130125 */
    int     HorGridNum;         /* Horizontal grid number. */
    int     VerGridNum;         /* Vertical grid number. */
    int     TileWidthExp;       /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int     TileHeightExp;      /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    INT16   RedScaleFactor;
    INT16   BlueScaleFactor;
    UINT32  Reserved;           /* Reserved for extention. */
    UINT32  Reserved1;          /* Reserved for extention. */
    AMBA_DSP_IMG_GRID_POINT_s   *pCaWarp;   /* Warp grid vector arrey. */
} AMBA_DSP_IMG_CALIB_CAWARP_INFO_s;

typedef struct _AMBA_DSP_IMG_CAWARP_CALC_INFO_s_ {
    UINT8   CaWarpEnb;
    UINT8   Control;
    UINT16  Reserved1;

    AMBA_DSP_IMG_CALIB_CAWARP_INFO_s    CalibCaWarpInfo;

    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;       /* Current Vin sensor geometry. */
    AMBA_DSP_IMG_WIN_DIMENSION_s        R2rOutWinDim;       /* Raw 2 raw scaling output window */
    AMBA_DSP_IMG_WIN_GEOMETRY_s         DmyWinGeo;          /* Cropping concept */
    AMBA_DSP_IMG_WIN_DIMENSION_s        CfaWinDim;          /* Scaling concept */
} AMBA_DSP_IMG_CAWARP_CALC_INFO_s;

typedef struct _AMBA_DSP_IMG_BYPASS_CAWARP_INFO_s_ {
    UINT8   Enable;
    UINT16  HorzWarpEnable;
    UINT16  VertWarpEnable;
    UINT8   HorzPassGridArrayWidth;
    UINT8   HorzPassGridArrayHeight;
    UINT8   HorzPassHorzGridSpacingExponent;
    UINT8   HorzPassVertGridSpacingExponent;
    UINT8   VertPassGridArrayWidth;
    UINT8   VertPassGridArrayHeight;
    UINT8   VertPassHorzGridSpacingExponent;
    UINT8   VertPassVertGridSpacingExponent;
    UINT16  RedScaleFactor;
    UINT16  BlueScaleFactor;
    INT16   *pWarpHorzTable;
    INT16   *pWarpVertTable;
} AMBA_DSP_IMG_BYPASS_CAWARP_INFO_s;

typedef struct _AMBA_DSP_IMG_CAWARP_SET_INFO_s_ {
    UINT8   ResetVertCaWarp;
    UINT8   Reserved;
    UINT16  Reserved1;
} AMBA_DSP_IMG_CAWARP_SET_INFO_s;

typedef struct _AMBA_DSP_IMG_WB_GAIN_s_ {
    UINT32  GainR;
    UINT32  GainG;
    UINT32  GainB;
    UINT32  AeGain;
    UINT32  GlobalDGain;
} AMBA_DSP_IMG_WB_GAIN_s;

typedef struct _AMBA_DSP_IMG_DGAIN_SATURATION_s_ {
    UINT32  LevelRed;       /* 14:0 */
    UINT32  LevelGreenEven;
    UINT32  LevelGreenOdd;
    UINT32  LevelBlue;
} AMBA_DSP_IMG_DGAIN_SATURATION_s;

typedef struct _AMBA_DSP_IMG_CFA_NOISE_FILTER_s_ {
    UINT8   Enb;
    UINT16  NoiseLevel[3];          /* R/G/B, 0-8192 */
    UINT16  OriginalBlendStr[3];    /* R/G/B, 0-256 */
    UINT16  ExtentRegular[3];       /* R/G/B, 0-256 */
    UINT16  ExtentFine[3];          /* R/G/B, 0-256 */
    UINT16  StrengthFine[3];        /* R/G/B, 0-256 */
    UINT16  SelectivityRegular;     /* 0-256 */
    UINT16  SelectivityFine;        /* 0-256 */
} AMBA_DSP_IMG_CFA_NOISE_FILTER_s;

typedef struct _AMBA_DSP_IMG_LOCAL_EXPOSURE_s_ {
    UINT8   Enb;
    UINT8   Radius;
    UINT8   LumaWeightRed;
    UINT8   LumaWeightGreen;
    UINT8   LumaWeightBlue;
    UINT8   LumaWeightShift;
    UINT16  GainCurveTable[AMBA_DSP_IMG_NUM_EXPOSURE_CURVE];
} AMBA_DSP_IMG_LOCAL_EXPOSURE_s;

typedef struct _AMBA_DSP_IMG_DEF_BLC_s_ {
    UINT8   Enb;
    UINT8   Reserved;
    INT16   DefAmount[3];
    INT16   DefBlack1[3];
    INT16   DefBlack2[3];
    INT16   DefBlack3[3];
} AMBA_DSP_IMG_DEF_BLC_s;

typedef struct _AMBA_DSP_IMG_DEMOSAIC_s_ {
    UINT16  ActivityThresh;
    UINT16  ActivityDifferenceThresh;
    UINT16  GradClipThresh;
    UINT16  GradNoiseThresh;
    UINT16  ZipperNoiseDifferenceAddThresh;
    UINT16  ZipperNoiseDifferenceMultThresh;
    UINT16  BlackWhiteResolutionDetail;
    UINT16  ClampDirectionalCandidates;
} AMBA_DSP_IMG_DEMOSAIC_s;

typedef struct _AMBA_DSP_IMG_COLOR_CORRECTION_s_ {
    UINT32  MatrixThreeDTableAddr;
    //UINT32  SecCcAddr;
} AMBA_DSP_IMG_COLOR_CORRECTION_s;

typedef struct _AMBA_DSP_IMG_COLOR_CORRECTION_REG_s_ {
    UINT32  RegSettingAddr;
} AMBA_DSP_IMG_COLOR_CORRECTION_REG_s;

typedef struct _AMBA_DSP_IMG_TONE_CURVE_s_ {
    UINT16  ToneCurveRed[AMBA_DSP_IMG_NUM_TONE_CURVE];
    UINT16  ToneCurveGreen[AMBA_DSP_IMG_NUM_TONE_CURVE];
    UINT16  ToneCurveBlue[AMBA_DSP_IMG_NUM_TONE_CURVE];
} AMBA_DSP_IMG_TONE_CURVE_s;

typedef struct _AMBA_DSP_IMG_SPECIFIG_CC_s_ {
    UINT32  Select;
    float   SigA;
    float   SigB;
    float   SigC;
} AMBA_DSP_IMG_SPECIFIG_CC_s;

typedef struct _AMBA_DSP_IMG_RGB_TO_YUV_s_ {
    INT16   MatrixValues[9];
    INT16   YOffset;
    INT16   UOffset;
    INT16   VOffset;
} AMBA_DSP_IMG_RGB_TO_YUV_s;

typedef struct _AMBA_DSP_IMG_CHROMA_SCALE_s_ {
    UINT8   Enb;    /* 0:disable 1:PC style VOUT 2:HDTV VOUT */
    UINT8   Reserved;
    UINT16  Reserved1;
    UINT16  GainCurve[AMBA_DSP_IMG_NUM_CHROMA_GAIN_CURVE];
} AMBA_DSP_IMG_CHROMA_SCALE_s;

typedef struct _AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s_{
    int     Enable;
    UINT16  CbAdaptiveStrength;
    UINT16  CrAdaptiveStrength;
    UINT8   CbNonAdaptiveStrength;
    UINT8   CrNonAdaptiveStrength;
    UINT16  CbAdaptiveAmount;
    UINT16  CrAdaptiveAmount;
    UINT16  Reserved;
} AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s;

typedef struct _AMBA_DSP_IMG_CDNR_INFO_s_ {
    int   CdnrMode;
    int   CdnrStrength;
} AMBA_DSP_IMG_CDNR_INFO_s;

typedef struct _AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s_{
    UINT8     Enable;
} AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s;

typedef enum _AMBA_DSP_IMG_SHP_A_SELECT_e_ {
    AMBA_DSP_IMG_SHP_A_SELECT_ASF = 0x00,
    AMBA_DSP_IMG_SHP_A_SELECT_SHP,   //0x01
    AMBA_DSP_IMG_SHP_A_SELECT_DE_EDGE//0x02
} AMBA_DSP_IMG_SHP_A_SELECT_e;

typedef struct _AMBA_DSP_IMG_ALPHA_{
    UINT8   AlphaMinus;
    UINT8   AlphaPlus;
    UINT16  SmoothAdaptation;
    UINT16  SmoothEdgeAdaptation;
    UINT8   T0;
    UINT8   T1;
} AMBA_DSP_IMG_ALPHA_s;

typedef struct _AMBA_DSP_IMG_CORING_s_ {
    UINT8   Coring[AMBA_DSP_IMG_NUM_CORING_TABLE_INDEX];
} AMBA_DSP_IMG_CORING_s;

typedef struct _AMBA_DSP_IMG_FIR_{
    UINT8  Specify;
    UINT16 PerDirFirIsoStrengths[9];
    UINT16 PerDirFirDirStrengths[9];
    UINT16 PerDirFirDirAmounts[9];
    INT16  Coefs[9][25];
    UINT16 StrengthIso;
    UINT16 StrengthDir;
    UINT16 EdgeThresh;
    UINT8  WideEdgeDetect;
} AMBA_DSP_IMG_FIR_s;

typedef struct _AMBA_DSP_IMG_LEVEL_s_ {
    UINT8   Low;
    UINT8   LowDelta;
    UINT8   LowStrength;
    UINT8   MidStrength;
    UINT8   High;
    UINT8   HighDelta;
    UINT8   HighStrength;
    UINT8   Method;
} AMBA_DSP_IMG_LEVEL_s;

typedef struct _AMBA_DSP_IMG_MAX_CHANGE_s_ {
    UINT8   Up5x5;
    UINT8   Down5x5;
    UINT8   Up;
    UINT8   Down;
} AMBA_DSP_IMG_MAX_CHANGE_s;

typedef struct _AMBA_DSP_IMG_TABLE_INDEXING_s_{
    UINT8 YToneOffset;
    UINT8 YToneShift;
    UINT8 YToneBits;
    UINT8 UToneOffset;
    UINT8 UToneShift;
    UINT8 UToneBits;
    UINT8 VToneOffset;
    UINT8 VToneShift;
    UINT8 VToneBits;
    UINT8 *pTable;
    //UINT8 MaxYToneIndex;
    //UINT8 MaxUToneIndex;
    //UINT8 MaxVToneIndex;
} AMBA_DSP_IMG_TABLE_INDEXING_s;

typedef struct _AMBA_DSP_IMG_SHARPEN_BOTH_s_{
    UINT8  Enable;
    UINT8  Mode;
    UINT16 EdgeThresh;
    UINT8  WideEdgeDetect;
    AMBA_DSP_IMG_TABLE_INDEXING_s    ThreeD;
    AMBA_DSP_IMG_MAX_CHANGE_s   MaxChange;
}AMBA_DSP_IMG_SHARPEN_BOTH_s;

typedef struct _AMBA_DSP_IMG_SHARPEN_NOISE_s_{
    UINT8 MaxChangeUp;
    UINT8 MaxChangeDown;
    AMBA_DSP_IMG_FIR_s          SpatialFir;
    AMBA_DSP_IMG_LEVEL_s        LevelStrAdjust;// 1 //Fir2OutScale
}AMBA_DSP_IMG_SHARPEN_NOISE_s;

typedef struct _AMBA_DSP_IMG_FULL_ADAPTATION_s_{
    UINT8                AlphaMinUp;
    UINT8                AlphaMaxUp;
    UINT8                T0Up;
    UINT8                T1Up;
    UINT8                AlphaMinDown;
    UINT8                AlphaMaxDown;
    UINT8                T0Down;
    UINT8                T1Down;
    AMBA_DSP_IMG_TABLE_INDEXING_s   ThreeD;
} AMBA_DSP_IMG_FULL_ADAPTATION_s;


typedef struct _AMBA_DSP_IMG_ASF_INFO_s_ {
    UINT8                   Enable;
    AMBA_DSP_IMG_FIR_s      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    AMBA_DSP_IMG_FULL_ADAPTATION_s      Adapt;
    AMBA_DSP_IMG_LEVEL_s    LevelStrAdjust;
    AMBA_DSP_IMG_LEVEL_s    T0T1Div;
    UINT8                   MaxChangeUp;
    UINT8                   MaxChangeDown;
    UINT8                   Reserved;   /* to keep 32 alignment */  //not sure
    UINT16                  Reserved1;  /* to keep 32 alignment */
} AMBA_DSP_IMG_ASF_INFO_s;

typedef struct _AMBA_DSP_IMG_CHROMA_ASF_INFO_s_ {
    UINT8                   Enable;
    AMBA_DSP_IMG_FIR_s      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    UINT8                   AlphaMin;
    UINT8                   AlphaMax;
    UINT8                   T0;
    UINT8                   T1;
    AMBA_DSP_IMG_TABLE_INDEXING_s   ThreeD;
    AMBA_DSP_IMG_LEVEL_s    LevelStrAdjust;
    AMBA_DSP_IMG_LEVEL_s    T0T1Div;
    UINT8                   MaxChange;
    UINT8                   Reserved;   /* to keep 32 alignment */  //not sure
    UINT16                  Reserved1;  /* to keep 32 alignment */
} AMBA_DSP_IMG_CHROMA_ASF_INFO_s;


typedef struct _AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s_ {
    UINT8 EdgeStartCb;
    UINT8 EdgeStartCr;
    UINT8 EdgeEndCb;
    UINT8 EdgeEndCr;
} AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s;

typedef struct _AMBA_DSP_IMG_HISO_CHROMA_FILTER_s_ {
    UINT8   Enable;
    UINT8   NoiseLevelCb;          /* 0-255 */
    UINT8   NoiseLevelCr;          /* 0-255 */
    UINT16  OriginalBlendStrengthCb; /* Cb 0-256  */
    UINT16  OriginalBlendStrengthCr; /* Cr 0-256  */
    UINT8   Reserved;
} AMBA_DSP_IMG_HISO_CHROMA_FILTER_s;

typedef struct _AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s_ {
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
    AMBA_DSP_IMG_TABLE_INDEXING_s    ThreeD;
} AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s_ {
    UINT8 T0;
    UINT8 T1;
    UINT8 AlphaMax;
    UINT8 AlphaMin;
    UINT8 MaxChangeNotT0T1LevelBased;
    UINT8 MaxChange;
    AMBA_DSP_IMG_LEVEL_s    EitherMaxChangeOrT0T1AddLevel;
    UINT8 SignalPreserve;
    AMBA_DSP_IMG_TABLE_INDEXING_s    ThreeD;
} AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_COMBINE_s_ {
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
    AMBA_DSP_IMG_TABLE_INDEXING_s    ThreeD;
} AMBA_DSP_IMG_HISO_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_FREQ_RECOVER_s_ {
    AMBA_DSP_IMG_FIR_s      Fir;
    UINT8  MaxDown;
    UINT8  MaxUp;
    AMBA_DSP_IMG_LEVEL_s    Level;
} AMBA_DSP_IMG_HISO_FREQ_RECOVER_s;

typedef struct _AMBA_DSP_IMG_HISO_LUMA_BLEND_s_ {
    UINT8 Enable;
} AMBA_DSP_IMG_HISO_LUMA_BLEND_s;

typedef struct _AMBA_DSP_IMG_HISO_BLEND_s_ {
    AMBA_DSP_IMG_LEVEL_s    LumaLevel;
} AMBA_DSP_IMG_HISO_BLEND_s;


#define AMBA_DSP_IMG_RESAMP_COEFF_RECTWIN           0x1
#define AMBA_DSP_IMG_RESAMP_COEFF_M2                0x2
#define AMBA_DSP_IMG_RESAMP_COEFF_M4                0x4
#define AMBA_DSP_IMG_RESAMP_COEFF_LP_MEDIUM         0x8
#define AMBA_DSP_IMG_RESAMP_COEFF_LP_STRONG         0x10

#define AMBA_DSP_IMG_RESAMP_SELECT_CFA              0x1
#define AMBA_DSP_IMG_RESAMP_SELECT_MAIN             0x2
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_A            0x4
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_B            0x8
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_C            0x10

#define AMBA_DSP_IMG_RESAMP_COEFF_MODE_ALWAYS       0
#define AMBA_DSP_IMG_RESAMP_COEFF_MODE_ONE_FRAME    1

typedef struct _AMBA_DSP_IMG_RESAMPLER_COEF_ADJ_s_ {
    UINT32  ControlFlag;
    UINT16  ResamplerSelect;
    UINT16  Mode;
} AMBA_DSP_IMG_RESAMPLER_COEF_ADJ_s;

typedef struct _AMBA_DSP_IMG_CHROMA_FILTER_s_ {
    UINT8  Enable;
    UINT8  NoiseLevelCb;          /* 0-255 */
    UINT8  NoiseLevelCr;          /* 0-255 */
    UINT16  OriginalBlendStrengthCb; /* Cb 0-256  */
    UINT16  OriginalBlendStrengthCr; /* Cr 0-256  */
    UINT16  Radius;                 /* 32-64-128 */
    UINT8   Reserved;
    UINT16  Reserved1;
} AMBA_DSP_IMG_CHROMA_FILTER_s;

#define    AMBA_DSP_IMG_WARP_VER_1_0     0x20130101
typedef struct _AMBA_DSP_IMG_CALIB_WARP_INFO_s_ {
    UINT32                              Version;        /* 0x20130101 */
    int                                 HorGridNum;     /* Horizontal grid number. */
    int                                 VerGridNum;     /* Vertical grid number. */
    int                                 TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int                                 TileHeightExp;  /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    UINT32                              Reserved;       /* Reserved for extention. */
    UINT32                              Reserved1;      /* Reserved for extention. */
    UINT32                              Reserved2;      /* Reserved for extention. */
    AMBA_DSP_IMG_GRID_POINT_s           *pWarp;         /* Warp grid vector arrey. */
} AMBA_DSP_IMG_CALIB_WARP_INFO_s;

#define AMBA_DSP_IMG_CALC_WARP_CONTROL_SEC2_SCALE      0x1  /* section 2 done all scaling. section 3 does not do scaling. */
#define AMBA_DSP_IMG_CALC_WARP_CONTROL_VERT_FLIP       0x2  /* vertical flip. */

typedef struct _AMBA_DSP_IMG_WARP_CALC_INFO_s_ {
    /* Warp related settings */
    UINT32                              WarpEnb;
    UINT32                              Control;

    AMBA_DSP_IMG_CALIB_WARP_INFO_s      CalibWarpInfo;

    AMBA_DSP_IMG_VIN_SENSOR_GEOMETRY_s  VinSensorGeo;           /* Current Vin sensor geometry. */
    AMBA_DSP_IMG_WIN_DIMENSION_s        R2rOutWinDim;           /* Raw 2 raw scaling output window */
    AMBA_DSP_IMG_WIN_GEOMETRY_s         DmyWinGeo;              /* Cropping concept */
    AMBA_DSP_IMG_WIN_DIMENSION_s        CfaWinDim;              /* Scaling concept */
    AMBA_DSP_IMG_WIN_COORDINTATES_s     ActWinCrop;             /* Cropping concept */
    AMBA_DSP_IMG_WIN_DIMENSION_s        MainWinDim;             /* Scaling concept */
    AMBA_DSP_IMG_WIN_DIMENSION_s        PrevWinDim[2];          /* 0:PrevA 1: PrevB */
    AMBA_DSP_IMG_WIN_DIMENSION_s        ScreennailDim;
    AMBA_DSP_IMG_WIN_DIMENSION_s        ThumbnailDim;
    int                                 HorSkewPhaseInc;        /* For EIS */
    UINT32                              ExtraVertOutMode;       /* To support warp table that reference dummy window margin pixels */
} AMBA_DSP_IMG_WARP_CALC_INFO_s;

//typedef struct _AMBA_DSP_IMG_WARP_SET_INFO_s_ {
//    UINT8   ResetVertWarp;
//    UINT8   Reserved;
//    UINT16  Reserved1;
//} AMBA_DSP_IMG_WARP_SET_INFO_s;

typedef struct _AMBA_DSP_IMG_BYPASS_WARP_DZOOM_INFO_s_ {
    // Warp part
    UINT8   Enable;
    UINT32  WarpControl;
    UINT8   GridArrayWidth;
    UINT8   GridArrayHeight;
    UINT8   HorzGridSpacingExponent;
    UINT8   VertGridSpacingExponent;
    UINT8   VertWarpEnable;
    UINT8   VertWarpGridArrayWidth;
    UINT8   VertWarpGridArrayHeight;
    UINT8   VertWarpHorzGridSpacingExponent;
    UINT8   VertWarpVertGridSpacingExponent;
    INT16   *pWarpHorizontalTable;
    INT16   *pWarpVerticalTable;

    // Dzoom part
    UINT8   DzoomEnable;
    UINT32  ActualLeftTopX;
    UINT32  ActualLeftTopY;
    UINT32  ActualRightBotX;
    UINT32  ActualRightBotY;
    UINT32  ZoomX;
    UINT32  ZoomY;
    UINT32  XCenterOffset;
    UINT32  YCenterOffset;
    INT32   HorSkewPhaseInc;
    UINT8   ForceV4tapDisable;
    UINT16  DummyWindowXLeft;
    UINT16  DummyWindowYTop;
    UINT16  DummyWindowWidth;
    UINT16  DummyWindowHeight;
    UINT16  CfaOutputWidth;
    UINT16  CfaOutputHeight;
} AMBA_DSP_IMG_BYPASS_WARP_DZOOM_INFO_s;

typedef struct _AMBA_DSP_IMG_GMV_INFO_s_ {
    UINT16  Enb;
    INT16   MvX;
    INT16   MvY;
    UINT16  Reserved1;
} AMBA_DSP_IMG_GMV_INFO_s;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_ONE_CHAN_s_ {
    UINT8   TemporalAlpha0;
    UINT8   TemporalAlpha1;
    UINT8   TemporalAlpha2;
    UINT8   TemporalAlpha3;
    UINT8   TemporalT0;
    UINT8   TemporalT1;
    UINT8   TemporalT2;
    UINT8   TemporalT3;
    UINT8   TemporalMaxChange;
    UINT16  Radius;         /* 0-256 */
    UINT16  StrengthThreeD;      /* 0-256 */
    UINT16  StrengthSpatial;     /* 0-256 */
    UINT16  LevelAdjust;    /* 0-256 */
} AMBA_DSP_IMG_VIDEO_MCTF_ONE_CHAN_s;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_ONE_CHAN_s_ {
	INT32					TemporalMaxChangeNotT0T1LevelBased;
	AMBA_DSP_IMG_LEVEL_s	TemporalEitherMaxChangeOrT0T1Add;
} AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_ONE_CHAN_s;


typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s_ {
    UINT8  Enable;
    UINT8  MotionDetectionDelay;
    UINT8  FramesCombineNum;
    UINT8  FramesCombineThresh;
    UINT8  FramesCombineMinAboveThresh;
    UINT8  FramesCombineValBelowThresh;
    UINT8  Max1;
    UINT8  Min1;
    UINT8  Max2;
    UINT8  Min2;
    UINT16 Mul;
    UINT16 Sub;
    UINT8  Shift;
} AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_INFO_s_ {
    UINT8                                 Enable;
    AMBA_DSP_IMG_VIDEO_MCTF_ONE_CHAN_s    ChanInfo[3];    /* YCbCr */
    UINT16                                YCombinedStrength;   /* 0-256 */
    UINT8                                 Reserved;
} AMBA_DSP_IMG_VIDEO_MCTF_INFO_s;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s_ {
    UINT8   Y;
    UINT8   Cb;
    UINT8   Cr;
    UINT8   Reserved;
} AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s;

typedef struct _AMBA_DSP_IMG_GBGR_MISMATCH_s_ {
    UINT8  NarrowEnable;
    UINT8  WideEnable;
    UINT16 WideSafety;
    UINT16 WideThresh;
} AMBA_DSP_IMG_GBGR_MISMATCH_s;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s_ {
    AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_ONE_CHAN_s    ChanLevel[3];    /* YCbCr */
} AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s;


int AmbaDSP_ImgSetVinSensorInfo(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SENSOR_INFO_s *pVinSensorInfo);
int AmbaDSP_ImgGetVinSensorInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SENSOR_INFO_s *pVinSensorInfo);

/* CFA domain filters */
int AmbaDSP_ImgSetStaticBlackLevel(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_BLACK_CORRECTION_s *pBlackCorr);
int AmbaDSP_ImgGetStaticBlackLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_BLACK_CORRECTION_s *pBlackCorr);

int AmbaDSP_ImgCalcVignetteCompensation(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s *pVignetteCalcInfo);
int AmbaDSP_ImgSetVignetteCompensation(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode);
int AmbaDSP_ImgGetVignetteCompensation(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIGNETTE_CALC_INFO_s *pVignetteCalcInfo);
int AmbaDSP_ImgSetVignetteCompensationByPass(AMBA_DSP_IMG_MODE_CFG_s * pMode,AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s * pVigCorrByPass);

int AmbaDSP_ImgSetCfaLeakageFilter(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *pCfaLeakage);
int AmbaDSP_ImgGetCfaLeakageFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *pCfaLeakage);

int AmbaDSP_ImgSetAntiAliasing(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 Strength);
int AmbaDSP_ImgGetAntiAliasing(AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 *pStrength);

int AmbaDSP_ImgSetDynamicBadPixelCorrection(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DBP_CORRECTION_s *pDbpCorr);
int AmbaDSP_ImgGetDynamicBadPixelCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DBP_CORRECTION_s *pDbpCorr);

int AmbaDSP_ImgSetStaticBadPixelCorrection(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SBP_CORRECTION_s *pSbpCorr);
int AmbaDSP_ImgGetStaticBadPixelCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SBP_CORRECTION_s *pSbpCorr);

int AmbaDSP_ImgSetWbGain(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_WB_GAIN_s *pWbGains);
int AmbaDSP_ImgGetWbGain(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_WB_GAIN_s *pWbGains);

int AmbaDSP_ImgSetDgainSaturationLevel(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DGAIN_SATURATION_s *pDgainSat);
int AmbaDSP_ImgGetDgainSaturationLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DGAIN_SATURATION_s *pDgainSat);

int AmbaDSP_ImgSetCfaNoiseFilter(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_NOISE_FILTER_s *pCfaNoise);
int AmbaDSP_ImgGetCfaNoiseFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_NOISE_FILTER_s *pCfaNoise);

int AmbaDSP_ImgSetLocalExposure(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LOCAL_EXPOSURE_s *pLocalExposure);
int AmbaDSP_ImgGetLocalExposure(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LOCAL_EXPOSURE_s *pLocalExposure);

int AmbaDSP_ImgSetDeferredBlackLevel(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEF_BLC_s *pDefBlc);
int AmbaDSP_ImgGetDeferredBlackLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEF_BLC_s *pDefBlc);

/* RGB domain filters */
int AmbaDSP_ImgSetDemosaic(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEMOSAIC_s *pDemosaic);
int AmbaDSP_ImgGetDemosaic(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEMOSAIC_s *pDemosaic);

int AmbaDSP_ImgSetColorCorrectionReg(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_COLOR_CORRECTION_REG_s *pColorCorrReg);
int AmbaDSP_ImgGetColorCorrectionReg(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_COLOR_CORRECTION_REG_s *pColorCorrReg);
int AmbaDSP_ImgSetColorCorrection(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_COLOR_CORRECTION_s *pColorCorr);
int AmbaDSP_ImgGetColorCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_COLOR_CORRECTION_s *pColorCorr);

int AmbaDSP_ImgSetToneCurve(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_TONE_CURVE_s *pToneCurve);
int AmbaDSP_ImgGetToneCurve(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_TONE_CURVE_s  *pToneCurve);


/* Y domain filters */
int AmbaDSP_ImgSetRgbToYuvMatrix(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_RGB_TO_YUV_s *pRgbToYuv);
int AmbaDSP_ImgGetRgbToYuvMatrix(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_RGB_TO_YUV_s *pRgbToYuv);

int AmbaDSP_ImgSetChromaScale(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_SCALE_s *pChromaScale);
int AmbaDSP_ImgGetChromaScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_SCALE_s *pChromaScale);

int AmbaDSP_ImgSetChromaMedianFilter(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *pChromaMedian);
int AmbaDSP_ImgGetChromaMedianFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *pChromaMedian);

int AmbaDSP_ImgSetColorDependentNoiseReduction(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CDNR_INFO_s *pCdnr);
int AmbaDSP_ImgGetColorDependentNoiseReduction(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CDNR_INFO_s *pCdnr);

int AmbaDSP_ImgSet1stLumaProcessingMode(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHP_A_SELECT_e Select);
int AmbaDSP_ImgGet1stLumaProcessingMode(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHP_A_SELECT_e *pSelect);

int AmbaDSP_ImgSetAdvanceSpatialFilter(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);


int AmbaDSP_ImgSet1stSharpenNoiseBoth(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGet1stSharpenNoiseBoth( AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSet1stSharpenNoiseNoise(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGet1stSharpenNoiseNoise( AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSet1stSharpenNoiseSharpenFir(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);
int AmbaDSP_ImgGet1stSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);

int AmbaDSP_ImgSet1stSharpenNoiseSharpenCoring(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGet1stSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSet1stSharpenNoiseSharpenCoringIndexScale(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGet1stSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSet1stSharpenNoiseSharpenMinCoringResult(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGet1stSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSet1stSharpenNoiseSharpenScaleCoring(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGet1stSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetFinalSharpenNoiseBoth(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGetFinalSharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSetFinalSharpenNoiseNoise(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGetFinalSharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoring(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGetFinalSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSetFinalSharpenNoiseSharpenFir(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pFir);
int AmbaDSP_ImgGetFinalSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pFir);

int AmbaDSP_ImgSetFinalSharpenNoiseSharpenMinCoringResult(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);
int AmbaDSP_ImgGetFinalSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);

int AmbaDSP_ImgSetFinalSharpenNoiseSharpenCoringIndexScale(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);
int AmbaDSP_ImgGetFinalSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);

int AmbaDSP_ImgSetFinalSharpenNoiseSharpenScaleCoring(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);
int AmbaDSP_ImgGetFinalSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pUserLevel);

int AmbaDSP_ImgSetResamplerCoefAdj(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_RESAMPLER_COEF_ADJ_s *pResamplerCoefAdj);
int AmbaDSP_ImgGetResamplerCoefAdj(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_RESAMPLER_COEF_ADJ_s *pResamplerCoefAdj);

int AmbaDSP_ImgSetChromaFilter(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_FILTER_s *pChromaFilter);
int AmbaDSP_ImgGetChromaFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_FILTER_s *pChromaFilter);

int AmbaDSP_ImgSetGbGrMismatch(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_GBGR_MISMATCH_s *pGbGrMismatch);
int AmbaDSP_ImgGetGbGrMismatch(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_GBGR_MISMATCH_s *pGbGrMismatch);

/* Warp and MCTF related filters */

int AmbaDSP_ImgSetVideoMctf(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *pMctfInfo);
int AmbaDSP_ImgGetVideoMctf(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_INFO_s *pMctfInfo);

int AmbaDSP_ImgSetVideoMctfLevel(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s *pMctfLevel);
int AmbaDSP_ImgGetVideoMctfLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_LEVEL_s *pMctfLevel);

int   AmbaDSP_ImgSetVideoMctfMbTemporal(AMBA_DSP_IMG_MODE_CFG_s  *pMode, AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s *pMctfMbTemporal);
int   AmbaDSP_ImgGetVideoMctfMbTemporal(AMBA_DSP_IMG_MODE_CFG_s  *pMode, AMBA_DSP_IMG_VIDEO_MCTF_MB_TEMPORAL_s *pMctfMbTemporal);


int AmbaDSP_ImgSetVideoMctfGhostPrv(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s *pMctfGpInfo);
int AmbaDSP_ImgGetVideoMctfGhostPrv(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s *pMctfGpInfo);

int AmbaDSP_ImgSetVideoMctfCompressionEnable(int fd_iav, AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 Enb);
int AmbaDSP_ImgGetVideoMctfCompressionEnable(AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 *pEnb);

//int img_dsp_get_statistics(int fd_iav,AMBA_DSP_IMG_MODE_CFG_s *pMode,
//	img_aaa_stat_t *p_stat, aaa_tile_report_t *p_act_tile,int work_mode);

#endif  /* _AMBA_DSP_IMG_FILTER_H_ */
