/*******************************************************************************
 * am_config_audio.cpp
 *
 * Histroy:
 *  2012-8-2 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_audio.h"

static const char* audio_format_to_str[] =
{
 "NONE",
 "AAC",
 "OPUS",
 "PCM",
 "BPCM",
 "G726"
};

static const char* aac_format_to_str[] =
{
 "AAC",
 "AACPlus",
 "AAC", /* This value is invalid, reset to AAC */
 "AACPlusPs"
};

static const char* aac_quantizer_to_str[] =
{
 "Low",
 "High",
 "Highest"
};

static const char* audio_channel_to_str[] =
{
 "",
 "Mono",
 "Stereo"
};

static const char* g726_law_to_str[] =
{
 "Ulaw",
 "Alaw",
 "PCM16"
};

static const char* g726_rate_to_str[] =
{
 "",
 "",
 "16kb",
 "24kb",
 "32kb",
 "40kb"
};

bool AmConfigAudio::get_adev_config()
{
  bool ret = false;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->adevice_param) {
      mAudioParameters->adevice_param = new ADeviceParameters();
    }
    if (AM_LIKELY(mAudioParameters->adevice_param)) {
      mAudioParameters->adevice_param->audio_channel_num =
          (uint32_t)get_int("AMADEVICE:AudioChannelNumber", 2);
      mAudioParameters->adevice_param->audio_sample_freq =
          (uint32_t)get_int("AMADEVICE:AudioSampleFrequency", 48000);
      if (mAudioParameters->adevice_param->config_changed) {
        set_adev_config(mAudioParameters->adevice_param);
      }
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_adev_config(ADeviceParameters* config)
{
  if (AM_LIKELY(config && init())) {
    set_value("AMADEVICE:AudioChannelNumber", config->audio_channel_num);
    set_value("AMADEVICE:AudioSampleFrequency", config->audio_sample_freq);
    config->config_changed = 0;
    save_config();
  }
}

AudioParameters* AmConfigAudio::get_audio_config()
{
  AudioParameters *ret = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters) {
      mAudioParameters = new AudioParameters();
    }
    if (AM_LIKELY(mAudioParameters)) {
      char *string = get_string("AUDIO:AudioFormat", "AAC");
      if (is_str_equal(string, "AAC")) {
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_AAC;
      } else if (is_str_equal(string, "OPUS")) {
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_OPUS;
      } else if (is_str_equal(string, "PCM")) {
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_PCM;
      } else if (is_str_equal(string, "BPCM")) {
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_BPCM;
      } else if (is_str_equal(string, "G726") ||
                 is_str_equal(string, "G.726")) {
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_G726;
      } else {
        WARN("Unknown audio type %s, use AAC by default!", string);
        mAudioParameters->audio_format = AM_AUDIO_FORMAT_AAC;
        mAudioParameters->config_changed = 1;
      }

      for (uint32_t i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
        char section[16] = {0};
        sprintf(section, "STREAM:Stream%u", i);
        if (AM_LIKELY(get_boolean(section, true))) {
          mAudioParameters->audio_stream_map |= 1 << i;
        }
      }

      switch(mAudioParameters->audio_format) {
        case AM_AUDIO_FORMAT_AAC: {
          get_aac_config();
        }break;
        case AM_AUDIO_FORMAT_OPUS: {
          get_opus_config();
        }break;
        case AM_AUDIO_FORMAT_PCM: {
          get_pcm_config();
        }break;
        case AM_AUDIO_FORMAT_BPCM: {
          get_bpcm_config();
        }break;
        case AM_AUDIO_FORMAT_G726: {
          get_g726_config();
        }break;
        case AM_AUDIO_FORMAT_NONE:
        default: {
          delete mAudioParameters->codec;
          mAudioParameters->codec = NULL;
        }break;
      }

      get_adev_config();
      get_audio_alert_config ();
      if (mAudioParameters->config_changed) {
        set_audio_config(mAudioParameters);
      }
      ret = mAudioParameters;
    }
  }

  return ret;
}

void AmConfigAudio::set_audio_config(AudioParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("AUDIO:AudioFormat",
              audio_format_to_str[config->audio_format]);
    save_config();
    for (uint32_t i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
      char section[16] = {0};
      sprintf(section, "STREAM:Stream%u", i);
      set_value((const char*)section,
                (const char*)((config->audio_stream_map & (1 << i)) ?
                    "Yes" : "No"));
    }
    switch(config->audio_format) {
      case AM_AUDIO_FORMAT_AAC: {
        if (config->codec->aac.config_changed) {
          set_aac_config(&config->codec->aac);
        }break;
      }
      case AM_AUDIO_FORMAT_OPUS: {
        if (config->codec->opus.config_changed) {
          set_opus_config(&config->codec->opus);
        }
      }break;
      case AM_AUDIO_FORMAT_PCM: {
        if (config->codec->pcm.config_changed) {
          set_pcm_config(&config->codec->pcm);
        }
      }break;
      case AM_AUDIO_FORMAT_BPCM: {
        if (config->codec->bpcm.config_changed) {
          set_bpcm_config(&config->codec->bpcm);
        }
      }break;
      case AM_AUDIO_FORMAT_G726: {
        if (config->codec->g726.config_changed) {
          set_g726_config(&config->codec->g726);
        }
      }break;
      case AM_AUDIO_FORMAT_NONE:
      default: {
        ERROR("Should not be here!");
      }break;
    }
    config->config_changed = 0;
    save_config();
    if (config->adevice_param->config_changed) {
      set_adev_config(config->adevice_param);
    }
    if (config->audio_alert->config_changed) {
      set_audio_alert_config(config->audio_alert);
    }
  }
}

bool AmConfigAudio::get_aac_config()
{
  bool ret = false;
  AacEncoderParameters* aacEnc = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->codec) {
      mAudioParameters->codec = new AudioCodecParameters;
    }
    aacEnc = (AacEncoderParameters*)mAudioParameters->codec;
    if (AM_LIKELY(aacEnc)) {
      char *string = get_string("AAC:AacFormat", "AAC");
      aacEnc->aac_format =
          (is_str_equal(string, "AAC") ? AM_AAC_FORMAT_AAC :
           (is_str_equal(string, "AACPlus") ? AM_AAC_FORMAT_AACPLUS :
            (is_str_equal(string, "AACPlusPs") ? AM_AAC_FORMAT_AACPLUSPS :
             (WARN("Unknown AAC format %s, reset to AAC!", string),
                 aacEnc->config_changed = 1,
                 AM_AAC_FORMAT_AAC))));
      aacEnc->aac_bitrate = get_int("AAC:AacBitrate", 128000);
      string = get_string("AAC:AacQuantizerQuality", "High");
      aacEnc->aac_quantizer_quality =
          (is_str_equal(string, "Low") ? AM_AAC_QUANTIZER_QUALITY_LOW  :
           (is_str_equal(string, "High") ? AM_AAC_QUANTIZER_QUALITY_HIGH :
            (is_str_equal(string, "Highest") ? AM_AAC_QUANTIZER_QUALITY_HIGHEST:
             (WARN("Unknown quantizer quality %s, reset to High!", string),
                 aacEnc->config_changed = 1,
                 AM_AAC_QUANTIZER_QUALITY_HIGH))));
      string = get_string("AAC:AacOutputChannelNumber", "Mono");
      aacEnc->aac_output_channel =
          (is_str_equal(string, "Mono") ? AM_AUDIO_OUTPUT_CHANNEL_MONO :
           (is_str_equal(string, "Stereo") ? AM_AUDIO_OUTPUT_CHANNEL_STERO :
            (WARN("Unknown output channel %s, reset to mono!", string),
                aacEnc->config_changed = 1,
                AM_AUDIO_OUTPUT_CHANNEL_MONO)));
      if (aacEnc->config_changed) {
        set_aac_config(aacEnc);
      }
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_aac_config(AacEncoderParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("AAC:AacFormat", aac_format_to_str[config->aac_format]);
    set_value("AAC:AacBitrate", config->aac_bitrate);
    set_value("AAC:AacQuantizerQuality",
              aac_quantizer_to_str[config->aac_quantizer_quality]);
    set_value("AAC:AacOutputChannelNumber",
              audio_channel_to_str[config->aac_output_channel]);
    config->config_changed = 0;
    save_config();
  }
}

bool AmConfigAudio::get_opus_config()
{
  bool ret = false;
  OpusEncoderParameters* opusEnc = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->codec) {
      mAudioParameters->codec = new AudioCodecParameters;
    }
    opusEnc = (OpusEncoderParameters*)mAudioParameters->codec;
    if (AM_LIKELY(opusEnc)) {
      int value = get_int("OPUS:OpusComplexity", 0);
      if (AM_UNLIKELY((value < 0) || (value > 10))) {
        WARN("Invalid Opus complexity value: %d, reset to 0!", value);
        value = 0;
        opusEnc->config_changed = 1;
      }
      opusEnc->opus_complexity = value;

      value = get_int("OPUS:OpusAvgBitrate", 32000);
      if (AM_UNLIKELY(value < 0)) {
        WARN("Invalid Opus Average bitrate value: %d, reset to 32000", value);
        value = 32000;
        opusEnc->config_changed = 1;
      }
      opusEnc->opus_avg_bitrate = value;
      if (opusEnc->config_changed) {
        set_opus_config(opusEnc);
      }
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_opus_config(OpusEncoderParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("OPUS:OpusComplexity", config->opus_complexity);
    set_value("OPUS:OpusAvgBitrate", config->opus_avg_bitrate);
    config->config_changed = 0;
    save_config();
  }
}

bool AmConfigAudio::get_pcm_config()
{
  bool ret = false;
  PcmEncoderParameters *pcmEnc = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->codec) {
      mAudioParameters->codec = new AudioCodecParameters;
    }
    pcmEnc = (PcmEncoderParameters*)mAudioParameters->codec;
    if (AM_LIKELY(pcmEnc)) {
      char *string = get_string("PCM:PcmOutputChannelNumber", "Mono");
      pcmEnc->pcm_output_channel =
          (is_str_equal(string, "Mono") ? AM_AUDIO_OUTPUT_CHANNEL_MONO :
           (is_str_equal(string, "Stereo") ? AM_AUDIO_OUTPUT_CHANNEL_STERO :
            (WARN("Unknown output channel %s, reset to mono!", string),
                pcmEnc->config_changed = 1,
                AM_AUDIO_OUTPUT_CHANNEL_MONO)));
      if (pcmEnc->config_changed) {
        set_pcm_config(pcmEnc);
      }
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_pcm_config(PcmEncoderParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("PCM:PcmOutputChannelNumber",
              audio_channel_to_str[config->pcm_output_channel]);
    config->config_changed = 0;
    save_config();
  }
}

bool AmConfigAudio::get_bpcm_config()
{
  bool ret = false;
  BpcmEncoderParameters *bpcmEnc = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->codec) {
      mAudioParameters->codec = new AudioCodecParameters;
    }
    bpcmEnc = (BpcmEncoderParameters*)mAudioParameters->codec;
    if (AM_LIKELY(bpcmEnc)) {
      char *string = get_string("BPCM:BpcmOutputChannelNumber", "Mono");
      bpcmEnc->bpcm_output_channel =
          (is_str_equal(string, "Mono") ? AM_AUDIO_OUTPUT_CHANNEL_MONO :
           (is_str_equal(string, "Stereo") ? AM_AUDIO_OUTPUT_CHANNEL_STERO :
            (WARN("Unknown output channel %s, reset to mono!", string),
                bpcmEnc->config_changed = 1,
                AM_AUDIO_OUTPUT_CHANNEL_MONO)));
      if (bpcmEnc->config_changed) {
        set_bpcm_config(bpcmEnc);
      }
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_bpcm_config(BpcmEncoderParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("BPCM:BpcmOutputChannelNumber",
              audio_channel_to_str[config->bpcm_output_channel]);
    config->config_changed = 0;
    save_config();
  }
}

bool AmConfigAudio::get_g726_config()
{
  bool ret = false;
  G726EncoderParameters* g726Enc = NULL;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->codec) {
      mAudioParameters->codec = new AudioCodecParameters;
    }
    g726Enc = (G726EncoderParameters*)mAudioParameters->codec;
    if (AM_LIKELY(g726Enc)) {
      char* string = get_string("G726:G726Law", "PCM16");
      g726Enc->g726_law =
          (is_str_equal(string, "PCM16") ? AM_G726_PCM16 :
              (is_str_equal(string, "Ulaw") ? AM_G726_ULAW :
                  (is_str_equal(string, "Alaw") ? AM_G726_ALAW :
                      (WARN("Unknown G726 law %s, reset to PCM16", string),
                          g726Enc->config_changed = 1,
                          AM_G726_PCM16))));
      string = get_string("G726:G726Rate", "40kb");
      g726Enc->g726_rate =
          (is_str_equal(string, "40kb") ? AM_G726_40K :
              (is_str_equal(string, "32kb") ? AM_G726_32K :
                  (is_str_equal(string, "24kb") ? AM_G726_24K :
                      (is_str_equal(string, "16kb") ? AM_G726_16K :
                          (WARN("Unknown G726 rate %s, reset to 40kb", string),
                              g726Enc->config_changed = 1,
                              AM_G726_40K)))));
      if (g726Enc->config_changed) {
        set_g726_config(g726Enc);
      }
      ret = true;
    }
  }

  return ret;
}
void AmConfigAudio::set_g726_config(G726EncoderParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("G726:G726Law", g726_law_to_str[config->g726_law]);
    set_value("G726:G726Rate", g726_rate_to_str[config->g726_rate]);
    config->config_changed = 0;
    save_config();
  }
}

bool AmConfigAudio::get_audio_alert_config ()
{
  bool ret = false;
  if (AM_LIKELY(init())) {
    if (!mAudioParameters->audio_alert) {
      mAudioParameters->audio_alert = new AudioAlertParameters();
    }
    if (AM_LIKELY(mAudioParameters->audio_alert)) {
      mAudioParameters->audio_alert->audio_alert_enable =
          get_int("AUDIO_ALERT:EnableAudioAlert", 0);
      mAudioParameters->audio_alert->audio_alert_sen =
          get_int ("AUDIO_ALERT:AudioAlertSensitivity", 2);
      mAudioParameters->audio_alert->audio_alert_direction =
          get_int ("AUDIO_ALERT:AudioAlertDirection", 1);
      ret = true;
    }
  }

  return ret;
}

void AmConfigAudio::set_audio_alert_config(AudioAlertParameters *config)
{
  if (AM_LIKELY(config && init())) {
    set_value("AUDIO_ALERT:EnableAudioAlert", config->audio_alert_enable);
    set_value("AUDIO_ALERT:AudioAlertSensitivity", config->audio_alert_sen);
    config->config_changed = 0;
    save_config();
  }
}

