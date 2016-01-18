/*******************************************************************************
 * audio_codec_info.h
 *
 * Histroy:
 *   2012-10-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_CODEC_INFO_H_
#define AUDIO_CODEC_INFO_H_

struct AudioCodecAacInfo {
    AM_U32 bitrate;
    AM_U32 quantizer_quality; /* 0: Low, 1: High, 2: Highest */
    AM_U16 enc_mode;          /* 0: AAC, 1: AAC_Plus, 3: AAC_Plus_PS */
    AM_U16 out_channel_num;   /* 1: Mono, 2: Stereo */
    AM_U8  tns;               /* 0: off, 1: on */
    AM_S8  ff_type;
};

struct AudioCodecOpusInfo {
    AM_U32 opus_complexity;
    AM_U32 opus_avg_bitrate;
};

struct AudioCodecBpcmInfo {
    AM_U32 out_channel_num;
};

struct AudioCodecPcmInfo {
    AM_U32 out_channel_num;
};

struct AudioCodecG726Info {
    AM_U32 g726_law;
    AM_U32 g726_rate;
};

struct AudioCodecInfo {
    AM_AUDIO_INFO audio_info;
    union {
        AudioCodecAacInfo  aac;
        AudioCodecOpusInfo opus;
        AudioCodecPcmInfo  pcm;
        AudioCodecBpcmInfo bpcm;
        AudioCodecG726Info g726;
    }codec;
#define codec_aac     codec.aac
#define codec_opus    codec.opus
#define codec_pcm     codec.pcm
#define codec_bpcm    codec.bpcm
#define codec_g726    codec.g726
};

enum AmAudioCodecType {
  AM_AUDIO_CODEC_NONE = 0,
  AM_AUDIO_CODEC_AAC  = 1,
  AM_AUDIO_CODEC_OPUS = 2,
  AM_AUDIO_CODEC_PCM  = 3,
  AM_AUDIO_CODEC_BPCM = 4,
  AM_AUDIO_CODEC_G726 = 5,
};

#endif /* AUDIO_CODEC_INFO_H_ */
