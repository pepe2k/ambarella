/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaDSP_ImgHighIsoFilter.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Ambarella DSP Image Kernel HISO APIs
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/12/2012  Steve Chen  Created
 *
 *  $LastChangedDate: 2013-10-21 18:21:41 +0800 (2013.10.21) $
 *  $LastChangedRevision: 6823 $ 
 *  $LastChangedBy: hfwang $ 
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/DSP/inc/AmbaDSP_ImgHighIsoFilter.h $
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_DSP_IMG_HISO_FILTER_H_
#define _AMBA_DSP_IMG_HISO_FILTER_H_

#include "AmbaDSP_ImgDef.h"

int AmbaDSP_ImgSetHighIsoAntiAliasing(AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 Strength);
int AmbaDSP_ImgGetHighIsoAntiAliasing(AMBA_DSP_IMG_MODE_CFG_s *pMode, UINT8 *pStrength);

int AmbaDSP_ImgSetHighIsoCfaLeakageFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *pCfaLeakage);
int AmbaDSP_ImgGetHighIsoCfaLeakageFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_LEAKAGE_FILTER_s *pCfaLeakage);

int AmbaDSP_ImgSetHighIsoDynamicBadPixelCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DBP_CORRECTION_s *pDbpCorr);
int AmbaDSP_ImgGetHighIsoDynamicBadPixelCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DBP_CORRECTION_s *pDbpCorr);

int AmbaDSP_ImgSetHighIsoCfaNoiseFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_NOISE_FILTER_s *pCfaNoise);
int AmbaDSP_ImgGetHighIsoCfaNoiseFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFA_NOISE_FILTER_s *pCfaNoise);

int AmbaDSP_ImgSetHighIsoGbGrMismatch(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_GBGR_MISMATCH_s *pGbGrMismatch);
int AmbaDSP_ImgGetHighIsoGbGrMismatch(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_GBGR_MISMATCH_s *pGbGrMismatch);

int AmbaDSP_ImgSetHighIsoDemosaic(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEMOSAIC_s *pDemosaic);
int AmbaDSP_ImgGetHighIsoDemosaic(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEMOSAIC_s *pDemosaic);

int AmbaDSP_ImgSetHighIsoChromaMedianFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *pChromaMedian);
int AmbaDSP_ImgGetHighIsoChromaMedianFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_MEDIAN_FILTER_s *pChromaMedian);

int AmbaDSP_ImgSetHighIsoDeferColorCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *pDeferColorCorrection);
int AmbaDSP_ImgGetHighIsoDeferColorCorrection(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s *pDeferColorCorrection);

int AmbaDSP_ImgSetHighIsoColorDependentNoiseReduction(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CDNR_INFO_s *pHisoCdnr);
int AmbaDSP_ImgGetHighIsoColorDependentNoiseReduction(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CDNR_INFO_s *pHisoCdnr);

//Sharpen A
int AmbaDSP_ImgSetHighIsoHighSharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoHighSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoHighSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoMedSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoMedSharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

//Sharpen B
int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoLiso1SharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso1SharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseBoth(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_BOTH_s *pSharpenBoth);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseNoise(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SHARPEN_NOISE_s *pSharpenNoise);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CORING_s *pCoring);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenFir(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_FIR_s *pSharpenFir);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenCoringIndexScale(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenMinCoringResult(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);

int AmbaDSP_ImgSetHighIsoLiso2SharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);
int AmbaDSP_ImgGetHighIsoLiso2SharpenNoiseSharpenScaleCoring(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_LEVEL_s *pLevel);


//Advance spatial filter
int AmbaDSP_ImgSetHighIsoAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoHighAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoHighAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoLowAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoLowAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoMed1AdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoMed1AdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoMed2AdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoMed2AdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoLi2ndAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighisoLi2ndAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_ASF_INFO_s *pAsf);

int AmbaDSP_ImgSetHighIsoChromaAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_ASF_INFO_s *pAsf);
int AmbaDSP_ImgGetHighIsoChromaAdvanceSpatialFilter(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_ASF_INFO_s *pAsf);

//chroma filter
int AmbaDSP_ImgHighIsoSetChromaFilterHigh(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_FILTER_s *pChromaFilterHigh);
int AmbaDSP_ImgHighIsoGetChromaFilterHigh(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CHROMA_FILTER_s *pChromaFilterHigh);

int AmbaDSP_ImgHighIsoSetChromaFilterLowVeryLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *pChromaFilterLowVeryLow);
int AmbaDSP_ImgHighIsoGetChromaFilterLowVeryLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s *pChromaFilterLowVeryLow);

int AmbaDSP_ImgHighIsoSetChromaFilterPre(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterPre);
int AmbaDSP_ImgHighIsoGetChromaFilterPre(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterPre);

int AmbaDSP_ImgHighIsoSetChromaFilterMed(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterMed);
int AmbaDSP_ImgHighIsoGetChromaFilterMed(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterMed);

int AmbaDSP_ImgHighIsoSetChromaFilterLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterLow);
int AmbaDSP_ImgHighIsoGetChromaFilterLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterLow);

int AmbaDSP_ImgHighIsoSetChromaFilterVeryLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterVeryLow);
int AmbaDSP_ImgHighIsoGetChromaFilterVeryLow(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_s *pChromaFilterVeryLow);

//combine
int AmbaDSP_ImgHighIsoSetLumaNoiseCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *pLumaNoiseCombine);
int AmbaDSP_ImgHighIsoGetLumaNoiseCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *pLumaNoiseCombine);

int AmbaDSP_ImgHighIsoSetLowASFCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *pLowASFCombine);
int AmbaDSP_ImgHighIsoGetLowASFCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s *pLowASFCombine);

int AmbaDSP_ImgHighIsoSetChromaFilterMedCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterMedCombine);
int AmbaDSP_ImgHighIsoGetChromaFilterMedCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterMedCombine);

int AmbaDSP_ImgHighIsoSetChromaFilterLowCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterLowCombine);
int AmbaDSP_ImgHighIsoGetChromaFilterLowCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterLowCombine);

int AmbaDSP_ImgHighIsoSetChromaFilterVeryLowCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterVeryLowCombine);
int AmbaDSP_ImgHighIsoGetChromaFilterVeryLowCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pChromaFilterVeryLowCombine);

int AmbaDSP_ImgHighIsoSetHighIsoFreqRecover(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *pHighIsoFreqRecover);
int AmbaDSP_ImgHighIsoGetHighIsoFreqRecover(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_FREQ_RECOVER_s *pHighIsoFreqRecover);

int AmbaDSP_ImgHighIsoSetHighIsoCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_COMBINE_s *pHighIsoCombine);
int AmbaDSP_ImgHighIsoGetHighIsoCombine(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_COMBINE_s *pHighIsoCombine);

int AmbaDSP_ImgHighIsoSetHighIsoLumaBlend(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_BLEND_s *pHighIsoLumaBlend);
int AmbaDSP_ImgHighIsoGetHighIsoLumaBlend(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_LUMA_BLEND_s *pHighIsoLumaBlend);

int AmbaDSP_ImgHighIsoSetHighIsoBlendLumaLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_BLEND_s *pHighIsoBlendLumaLevel);
int AmbaDSP_ImgHighIsoGetHighIsoBlendLumaLevel(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_HISO_BLEND_s *pHighIsoBlendLumaLevel);

//Others
//int AmbaDSP_ImgHighIsoSetSizeInfo(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_SIZE_INFO_s *pSizeInfo);
int AmbaDSP_ImgHighIsoPrintCfg(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_CFG_INFO_s CfgInfo);
int AmbaDSP_ImgHighIsoDumpCfg(AMBA_DSP_IMG_CFG_INFO_s CfgInfo, char* dir);

#endif  /* _AMBA_DSP_IMG_HISO_FILTER_H_ */
