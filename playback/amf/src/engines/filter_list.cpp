
/**
 * filter_list.cpp
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "string.h"

#include "am_types.h"
#include "am_if.h"
#include "am_mw.h"
#include "filter_list.h"

//only build some filters now on linux, fixe me
#if 0
#else
//extern filter_entry g_simple_demuxer;
extern filter_entry g_audio_renderer;
//extern filter_entry g_amba_audio_decoder;
extern filter_entry g_audio_effecter;
extern filter_entry g_subtitle_renderer;
#endif

extern filter_entry g_ffmpeg_demuxer;
extern filter_entry g_ffmpeg_decoder;
//extern filter_entry g_audio_decoder_hw;
//extern filter_entry g_amba_vdec;
extern filter_entry g_video_renderer;
extern filter_entry g_amba_video_decoder;
//extern filter_entry g_amba_video_renderer;
//extern filter_entry g_general_video_decoder;
extern filter_entry g_pridata_parser;
extern filter_entry g_amba_video_sink;

filter_entry *g_filter_table[] = {

//#ifdef CONFIG_SIMPLE_DEMUXER
//	&g_simple_demuxer,
//#endif

//#ifdef CONFIG_FFMPEG_DEMUXER
    &g_ffmpeg_demuxer,
//#endif

//	&g_amba_audio_decoder,

//#if AM_ENABLE_GENERAL_DEOCDER
    //&g_general_video_decoder,
//#endif

    &g_amba_video_decoder,

//    &g_audio_decoder_hw,
//#ifdef CONFIG_FFMPEG_DECODER
    &g_ffmpeg_decoder,
//#endif

//#ifdef CONFIG_AMBA_VDEC
//	&g_amba_vdec,
//#endif

//#ifdef CONFIG_VIDEO_RENDERER
    &g_video_renderer,
//#endif

//#if PLATFORM_ANDROID
     &g_audio_effecter,
//#endif

//#ifdef CONFIG_AUDIO_RENDERER
    &g_audio_renderer,
//#endif

#if PLATFORM_ANDROID
    &g_subtitle_renderer,
#endif

    &g_pridata_parser,
//	&g_amba_video_renderer,

    &g_amba_video_sink,

    NULL,
};

