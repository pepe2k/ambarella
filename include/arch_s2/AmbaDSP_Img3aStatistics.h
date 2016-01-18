/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaDSP_Img3aStatistics.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Ambarella DSP Image Kernel Aaa Stat APIs
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/12/2012  Steve Chen  Created
 *
 *  $LastChangedDate: 2013-10-22 17:05:12 +0800 (週二, 22 十月 2013) $
 *  $LastChangedRevision: 6855 $
 *  $LastChangedBy: hfwang $
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/DSP/inc/AmbaDSP_Img3aStatistics.h $
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_DSP_IMG_AAA_STAT_H_
#define _AMBA_DSP_IMG_AAA_STAT_H_

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"
//#include "AmbaDSP_EventInfo.h"

typedef struct _AMBA_DSP_IMG_AAA_STAT_CTRL_s_ {
    UINT8   AeAwbEnable;
    UINT8   AfEnable;
    UINT8   HisEnable;
} AMBA_DSP_IMG_AAA_STAT_ENB_s;      /* TODO: should be better to changed it into AMBA_DSP_IMG_AAA_STAT_CTRL_s */

typedef struct _AMBA_DSP_IMG_AE_STAT_INFO_s_ {
    UINT16  AeTileNumCol;
    UINT16  AeTileNumRow;
    UINT16  AeTileColStart;
    UINT16  AeTileRowStart;
    UINT16  AeTileWidth;
    UINT16  AeTileHeight;
    UINT16  AePixMinValue;
    UINT16  AePixMaxValue;
} AMBA_DSP_IMG_AE_STAT_INFO_s;

typedef struct _AMBA_DSP_IMG_AWB_STAT_INFO_s_ {
    UINT16  AwbTileNumCol;
    UINT16  AwbTileNumRow;
    UINT16  AwbTileColStart;
    UINT16  AwbTileRowStart;
    UINT16  AwbTileWidth;
    UINT16  AwbTileHeight;
    UINT16  AwbTileActiveWidth;
    UINT16  AwbTileActiveHeight;
    UINT32  AwbPixMinValue;
    UINT32  AwbPixMaxValue;
} AMBA_DSP_IMG_AWB_STAT_INFO_s;

typedef struct _AMBA_DSP_IMG_AF_STAT_INFO_s_ {
    UINT16  AfTileNumCol;
    UINT16  AfTileNumRow;
    UINT16  AfTileColStart;
    UINT16  AfTileRowStart;
    UINT16  AfTileWidth;
    UINT16  AfTileHeight;
    UINT16  AfTileActiveWidth;
    UINT16  AfTileActiveHeight;
} AMBA_DSP_IMG_AF_STAT_INFO_s;

typedef struct _AMBA_DSP_IMG_AF_STAT_EX_INFO_s_ {
    UINT8   AfHorizontalFilter1Mode;
    UINT8   AfHorizontalFilter1Stage1Enb;
    UINT8   AfHorizontalFilter1Stage2Enb;
    UINT8   AfHorizontalFilter1Stage3Enb;
    UINT16  AfHorizontalFilter1Gain[7];
    UINT16  AfHorizontalFilter1Shift[4];
    UINT16  AfHorizontalFilter1BiasOff;
    UINT16  AfHorizontalFilter1Thresh;
    UINT16  AfVerticalFilter1Thresh;
    UINT8   AfHorizontalFilter2Mode;
    UINT8   AfHorizontalFilter2Stage1Enb;
    UINT8   AfHorizontalFilter2Stage2Enb;
    UINT8   AfHorizontalFilter2Stage3Enb;
    UINT16  AfHorizontalFilter2Gain[7];
    UINT16  AfHorizontalFilter2Shift[4];
    UINT16  AfHorizontalFilter2BiasOff;
    UINT16  AfHorizontalFilter2Thresh;
    UINT16  AfVerticalFilter2Thresh;
    UINT16  AfTileFv1HorizontalShift;
    UINT16  AfTileFv1VerticalShift;
    UINT16  AfTileFv1HorizontalWeight;
    UINT16  AfTileFv1VerticalWeight;
    UINT16  AfTileFv2HorizontalShift;
    UINT16  AfTileFv2VerticalShift;
    UINT16  AfTileFv2HorizontalWeight;
    UINT16  AfTileFv2VerticalWeight;
} AMBA_DSP_IMG_AF_STAT_EX_INFO_s;

typedef struct _AMBA_DSP_IMG_AAA_STAT_INFO_s_ {
    UINT16  AwbTileNumCol;
    UINT16  AwbTileNumRow;
    UINT16  AwbTileColStart;
    UINT16  AwbTileRowStart;
    UINT16  AwbTileWidth;
    UINT16  AwbTileHeight;
    UINT16  AwbTileActiveWidth;
    UINT16  AwbTileActiveHeight;
    UINT16  AwbPixMinValue;
    UINT16  AwbPixMaxValue;
    UINT16  AeTileNumCol;
    UINT16  AeTileNumRow;
    UINT16  AeTileColStart;
    UINT16  AeTileRowStart;
    UINT16  AeTileWidth;
    UINT16  AeTileHeight;
    UINT16  AePixMinValue;
    UINT16  AePixMaxValue;
    UINT16  AfTileNumCol;
    UINT16  AfTileNumRow;
    UINT16  AfTileColStart;
    UINT16  AfTileRowStart;
    UINT16  AfTileWidth;
    UINT16  AfTileHeight;
    UINT16  AfTileActiveWidth;
    UINT16  AfTileActiveHeight;
} AMBA_DSP_IMG_AAA_STAT_INFO_s;

typedef struct _AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s_ {
    UINT16  FloatTileColStart:13;
    UINT16  FloatTileRowStart:14;
    UINT16  FloatTileWidth:13;
    UINT16  FloatTileHeight:14;
    UINT16  FloatTileShift:10;
} AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s;

typedef struct _AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s_ {
    UINT16                              FrameSyncId;
    UINT16                              NumberOfTiles;
    AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s    FloatTileConfig[32];
} AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s;

typedef struct _AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s_ {
    UINT32  RgbAaaDataAddr;
    UINT32  CfaAaaDataAddr;
    UINT32  SrcRgbAaaDataAddr;
    UINT32  SrcCfaAaaDataAddr;
} AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s;

typedef struct _AMBA_DSP_AAA_STATISTIC_DATA_s_ {
    UINT32  RgbAaaDataAddr;
    UINT32  CfaAaaDataAddr;
    UINT32  SensorDataAddr;

    UINT32   RgbDataValid;
    UINT32   CfaDataValid;
    UINT32   SensorDataValid;

} AMBA_DSP_AAA_STATISTIC_DATA_s;

int AmbaDSP_Img3AConfigAaaStat(int fd_iav,
					int enable,
					AMBA_DSP_IMG_MODE_CFG_s* pMode,
					AMBA_DSP_IMG_AE_STAT_INFO_s *pAeStat,
					AMBA_DSP_IMG_AWB_STAT_INFO_s *pAwbStat,
					AMBA_DSP_IMG_AF_STAT_INFO_s *pAfStat);

int AmbaDSP_Img3aGetAaaStat(int fd_iav,
					AMBA_DSP_IMG_MODE_CFG_s* pMode,
					AMBA_DSP_AAA_STATISTIC_DATA_s* pAAAData);

#if 0
/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaDSP_Img3aStatistics.c
\*-----------------------------------------------------------------------------------------------*/
/* enable/disable various 3A statistics. AmbaDSP_Img3aTransferAaaStatData will not*/
/* function properly if statistics is not enabled.                           */
int AmbaDSP_Img3aEnbAaaStat(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AAA_STAT_ENB_s *pEnbInfo);

/* provides the AE, AWB, AF and histogram statistics collection data          */
int AmbaDSP_Img3aTransferAaaStatData(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s *pTAaaStatData);

/* define tile configuration for AE statistics calculation. The parameters    */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAeStatInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AE_STAT_INFO_s *pAeStat);

/* define tile configuration for AWB statistics calculation. The parameters   */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAwbStatInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AWB_STAT_INFO_s *pAwbStat);

/* define tile configuration for AF statistics calculation. The parameters    */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAfStatInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AF_STAT_INFO_s *pAfStat);

/* define tile advanced configuration for AF statistics calculation.          */
int AmbaDSP_Img3aSetAfStatExInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AF_STAT_EX_INFO_s *pAfStatEx);

/* Get tile advanced configuration for AF statistics calculation.          */
int AmbaDSP_Img3aGetAfStatExInfo( AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AF_STAT_EX_INFO_s *pAfStatEx);

/* define tile configuration for AWB/AE/AF statistics calculation. The param  */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAaaStatInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AAA_STAT_INFO_s *pAaaStat);

/* Get the AAA statistic information settings*/
int AmbaDSP_Img3aGetAaaStatInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AAA_STAT_INFO_s *pAaaStat);

/* define tile configuration for floating tile statistics calculation.        */
int AmbaDSP_Img3aSetAaaFloatTileInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s *pAaaFloatTileStat);

/* Get tile configuration for floating tile statistics calculation  */
int AmbaDSP_Img3aGetAaaFloatTileInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s *pAaaFloatTileStat);

int AmbaDSP_Img3aGetAaaData (int fd_iav,AMBA_DSP_IMG_MODE_CFG_s* pMode,AMBA_DSP_AAA_STATISTIC_DATA_s* pAAAData);
#endif
#endif  /* _AMBA_DSP_IMG_AAA_STAT_H_ */
