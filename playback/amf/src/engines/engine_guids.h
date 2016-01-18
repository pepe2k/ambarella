
/**
 * engine_guids.h
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __ENGINE_GUIDS_H__
#define __ENGINE_GUIDS_H__


// media type
extern const AM_GUID GUID_None;
extern const AM_GUID GUID_Video;
extern const AM_GUID GUID_Audio;
extern const AM_GUID GUID_Subtitle;
extern const AM_GUID GUID_Pridata;


extern const AM_GUID GUID_Decoded_Video;
extern const AM_GUID GUID_Decoded_Audio;
extern const AM_GUID GUID_Decoded_Subtitle;
extern const AM_GUID GUID_Decoded_Pridata;
extern const AM_GUID GUID_Amba_Decoded_Video;

// video coding GUIDs

extern const AM_GUID GUID_Video_MPEG12;
extern const AM_GUID GUID_Video_MPEG4;
extern const AM_GUID GUID_Video_XVID;
extern const AM_GUID GUID_Video_H264;

extern const AM_GUID GUID_Video_RV10;
extern const AM_GUID GUID_Video_RV20;
extern const AM_GUID GUID_Video_RV30;
extern const AM_GUID GUID_Video_RV40;

extern const AM_GUID GUID_Video_MSMPEG4_V1;
extern const AM_GUID GUID_Video_MSMPEG4_V2;
extern const AM_GUID GUID_Video_MSMPEG4_V3;

extern const AM_GUID GUID_Video_WMV1;
extern const AM_GUID GUID_Video_WMV2;
extern const AM_GUID GUID_Video_WMV3;

extern const AM_GUID GUID_Video_H263P;
extern const AM_GUID GUID_Video_H263I;
extern const AM_GUID GUID_Video_FLV1;

extern const AM_GUID GUID_Video_VC1;
extern const AM_GUID GUID_Video_VP8;

extern const AM_GUID GUID_AmbaVideoAVC;
extern const AM_GUID GUID_AmbaVideoDecoder;

//temp for ffmpeg, to add each codec in future
extern const AM_GUID GUID_Video_FF_OTHER_CODECS;//ffmpeg support's other video codecs
// audio coding GUIDs

extern const AM_GUID GUID_Audio_MP2;
extern const AM_GUID GUID_Audio_MP3;
extern const AM_GUID GUID_Audio_AAC;
extern const AM_GUID GUID_Audio_AC3;
extern const AM_GUID GUID_Audio_DTS;

extern const AM_GUID GUID_Audio_WMAV1;
extern const AM_GUID GUID_Audio_WMAV2;

extern const AM_GUID GUID_Audio_COOK;
extern const AM_GUID GUID_Audio_VORBIS;

extern const AM_GUID GUID_Audio_PCM;

//temp for ffmpeg, to add each codec in future
extern const AM_GUID GUID_Audio_FF_OTHER_CODECS;//ffmpeg support's other audio codecs

// video format GUIDs

extern const AM_GUID GUID_Video_YUV420SP;
extern const AM_GUID GUID_Video_YUV420NV12;

// audio format GUIDs

//subtitle decoding GUIDs
extern const AM_GUID GUID_Subtitle_DVD;
extern const AM_GUID GUID_Subtitle_DVB;
extern const AM_GUID GUID_Subtitle_TEXT;
extern const AM_GUID GUID_Subtitle_XSUB;
extern const AM_GUID GUID_Subtitle_SSA;
extern const AM_GUID GUID_Subtitle_MOV_TEXT;
extern const AM_GUID GUID_Subtitle_HDMV_PGS_SUBTITLE;
extern const AM_GUID GUID_Subtitle_DVB_TELETEXT;
extern const AM_GUID GUID_Subtitle_SRT;
extern const AM_GUID GUID_Subtitle_MICRODVD;
extern const AM_GUID GUID_Subtitle_OTHER_CODECS;
// format type

extern const AM_GUID GUID_Format_FFMPEG_Stream;
extern const AM_GUID GUID_Format_FFMPEG_Media;


#endif

