/**
 * am_external_define.h
 *
 * History:
 *  2011/05/23 - [Zhi He] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_EXTERNAL_DEFINE_H__
#define __AM_EXTERNAL_DEFINE_H__

//-----------------------------------------------------------------------
//
//  from dsp/udec
//
//-----------------------------------------------------------------------

//error code's decoder type
#if 0
enum
{
    EH_DecoderType_Common = 0,
    EH_DecoderType_H264 = 1,
    EH_DecoderType_MPEG12 = 2,
    EH_DecoderType_MPEG4_HW = 4,
    EH_DecoderType_MPEG4_Hybird = 8,
    EH_DecoderType_VC1 = 16,
    EH_DecoderType_RV40_Hybird = 32,
    EH_DecoderType_JPEG = 64,
    EH_DecoderType_SW = 128,
    EH_DecoderType_Cnt = 8,
};
#else
enum
{
    EH_DecoderType_Common = 0,
    EH_DecoderType_H264,
    EH_DecoderType_MPEG12,
    EH_DecoderType_MPEG4_HW,
    EH_DecoderType_MPEG4_Hybird,
    EH_DecoderType_VC1,
    EH_DecoderType_RV40_Hybird,
    EH_DecoderType_JPEG,
    EH_DecoderType_SW,
    EH_DecoderType_Cnt,
};
#endif

//error code's level
enum
{
    EH_ErrorLevel_NoError = 0,
    EH_ErrorLevel_Warning,
    EH_ErrorLevel_Recoverable,
    EH_ErrorLevel_Fatal,
    EH_ErrorLevel_Last,
};

//error code's error type, decoder type==common
enum
{
    EH_CommonErrorType_None = 0,
    EH_CommonErrorType_HardwareHang = 1,
    EH_CommonErrorType_FrameBufferLeak,
    EH_CommonErrorType_Last,
};

typedef struct _UDECErrorCodeDetail
{
    AM_U16 error_type;
    AM_U8 error_level;
    AM_U8 decoder_type;
} UDECErrorCodeDetail;

typedef union _UDECErrorCode
{
    UDECErrorCodeDetail detail;
    AM_UINT mu32;
} UDECErrorCode;

//mw's behavior
enum
{
    MW_Bahavior_Ignore = 0,
    MW_Bahavior_Ignore_AndPostAppMsg = 1,
    MW_Bahavior_TrySeekToSkipError,
    MW_Bahavior_ExitPlayback,
    MW_Bahavior_HaltForDebug,//debug mode, do not send stop cmd to dsp, just make the env un-touched, for debug(ucode)
};

extern const char GUdecStringDecoderType[EH_DecoderType_Cnt][20];
extern const char GUdecStringErrorLevel[EH_ErrorLevel_Last][20];
extern const char GUdecStringCommonErrorType[EH_CommonErrorType_Last][20];

#endif

