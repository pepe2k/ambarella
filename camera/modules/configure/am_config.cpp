/*******************************************************************************
 * am_config.cpp
 *
 * Histroy:
 *  2012-3-30 2012 - [ypchang] created file
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
#include "am_data.h"
#include "am_utility.h"

/* Interface */
#include "am_config.h"

/* Base class*/
#include "am_config_base.h"

/* Video device module related */
#include "am_config_vin.h"
#include "am_config_vout.h"
#include "am_config_vdevice.h"
#include "am_config_vdev_stream.h"
#include "am_config_vdev_encoder.h"

/* Stream module related */
#include "am_config_record.h"

/* Photo */
#include "am_config_photo.h"

/* Audio */
#include "am_config_audio.h"

/* Wifi */
#include "am_config_wifi.h"

/* AudioDetect */
#include "am_config_audiodetect.h"

/* MotionDetect */
#include "am_config_motiondetect.h"

/* LBR */
#include "am_config_lbrcontrol.h"

#ifdef CONFIG_ARCH_S2
/* Fisheye */
#include "am_config_fisheye.h"
#endif

static void set_config_path(const char *path, char *&confPath)
{
  delete[] confPath;
  confPath = (path ? amstrdup(path) : NULL);
}

AmConfig::AmConfig()
: mVinConfig(NULL),
  mVoutConfig(NULL),
  mVdevConfig(NULL),
  mEncoderConfig(NULL),
  mStreamConfig(NULL),
  mRecordConfig(NULL),
  mPhotoConfig(NULL),
  mAudioConfig(NULL),
  mWifiConfig (NULL),
  mAudioDetectConfig (NULL),
  mMotionDetectConfig(NULL),
  mLBRConfig(NULL),
  mVinParamList(NULL),
  mVoutParamList(NULL),
  mStreamParamList(NULL),
  mEncoderParams(NULL),
  mVdevParams(NULL),
  mRecordParams(NULL),
  mPhotoParams(NULL),
  mAdevParams(NULL),
  mAudioParams(NULL),
  mWifiParams (NULL),
  mAudioDetectParams (NULL),
  mMotionDetectParams(NULL),
  mLBRParams(NULL),
  mIavFd(-1),
  mVdevConfigPath(NULL),
  mVinConfigPath(NULL),
  mVoutConfigPath(NULL),
  mRecordConfigPath(NULL),
  mPhotoConfigPath(NULL),
  mAudioConfigPath(NULL),
  mWifiConfigPath (NULL),
  mAudioDetectConfigPath (NULL),
  mMotionDetectConfigPath(NULL),
  mLBRConfigPath(NULL)
#ifdef CONFIG_ARCH_S2
,mFisheyeConfig (NULL)
,mFisheyeParams (NULL)
,mFisheyeConfigPath (NULL)
#endif
{
  if ((mIavFd = ::open("/dev/iav", O_RDWR)) < 0) {
    PERROR("Failed to open IAV device");
  }
  DEBUG("AmConfig IAV fd is %d", mIavFd);
}

AmConfig::~AmConfig()
{
  delete   mVinConfig;
  delete   mVoutConfig;
  delete   mVdevConfig;
  delete   mEncoderConfig;
  delete   mStreamConfig;
  delete   mRecordConfig;
  delete   mPhotoConfig;
  delete   mAudioConfig;
  delete   mWifiConfig;
  delete   mAudioDetectConfig;
  delete   mMotionDetectConfig;
  delete   mLBRConfig;
  delete[] mVinParamList;
  delete[] mVoutParamList;
  delete[] mStreamParamList;
  delete[] mVdevConfigPath;
  delete[] mVinConfigPath;
  delete[] mVoutConfigPath;
  delete[] mRecordConfigPath;
  delete[] mPhotoConfigPath;
  delete[] mAudioConfigPath;
  delete[] mWifiConfigPath;
  delete[] mAudioDetectConfigPath;
  delete[] mMotionDetectConfigPath;
  delete[] mLBRConfigPath;
#ifdef CONFIG_ARCH_S2
  delete   mFisheyeConfig;
  delete[] mFisheyeConfigPath;
#endif
  if (mIavFd >= 0) {
    ::close(mIavFd);
  }
}

VDeviceParameters* AmConfig::vdevice_config()
{
  return mVdevParams;
}

VinParameters* AmConfig::vin_config(uint32_t vinId)
{
  if (mVdevParams) {
    if (vinId < mVdevParams->vin_number) {
      return mVinParamList[vinId];
    } else {
      ERROR("No such VIN: %u", vinId);
    }
  } else {
    ERROR("Please load video device's paramters first!");
  }

  return NULL;
}

VoutParameters* AmConfig::vout_config(uint32_t voutId)
{
  if (mVdevParams) {
    if (voutId < mVdevParams->vout_number) {
      return mVoutParamList[voutId];
    } else {
      ERROR("No such VOUT: %u", voutId);
    }
  } else {
    ERROR("Please load video device's paramters first!");
  }

  return NULL;
}

StreamParameters* AmConfig::stream_config(uint32_t streamId)
{
  if (mVdevParams) {
    if (streamId < mVdevParams->stream_number) {
      return mStreamParamList[streamId];
    } else {
      ERROR("No such Stream: %u", streamId);
    }
  } else {
    ERROR("Please load video device's paramters first!");
  }
  return NULL;
}

EncoderParameters* AmConfig::encoder_config()
{
  return mEncoderParams;
}

RecordParameters* AmConfig::record_config()
{
  return mRecordParams;
}

PhotoParameters* AmConfig::photo_config()
{
  return mPhotoParams;
}

AudioParameters* AmConfig::audio_config()
{
  return mAudioParams;
}

WifiParameters* AmConfig::wifi_config ()
{
  return mWifiParams;
}

AudioDetectParameters *AmConfig::audiodetect_config ()
{
  return mAudioDetectParams;
}

MotionDetectParameters *AmConfig::motiondetect_config()
{
  return mMotionDetectParams;
}

LBRParameters *AmConfig::lbr_config()
{
  return mLBRParams;
}

#ifdef CONFIG_ARCH_S2
FisheyeParameters* AmConfig::fisheye_config ()
{
  return mFisheyeParams;
}
#endif


void AmConfig::set_vin_config_path(const char *path)
{
  set_config_path(path, mVinConfigPath);
  INFO("VIN config path set to %s", (path ? path : DEFAULT_VIN_CONFIG_PATH));
}

void AmConfig::set_vout_config_path(const char *path)
{
  set_config_path(path, mVoutConfigPath);
  INFO("VOUT config path set to %s", (path ? path : DEFAULT_VOUT_CONFIG_PATH));
}

void AmConfig::set_vdevice_config_path(const char *path)
{
  set_config_path(path, mVdevConfigPath);
  INFO("VideoDevice config path set to %s",
       (path ? path : DEFAULT_VDEV_CONFIG_PATH));
}

void AmConfig::set_record_config_path(const char *path)
{
  set_config_path(path, mRecordConfigPath);
  INFO("StreamRecord config path set to %s",
       (path ? path : DEFAULT_RECORD_CONFIG_PATH));
}

void AmConfig::set_photo_config_path(const char *path)
{
  set_config_path(path, mPhotoConfigPath);
  INFO("Photo config path set to %s",
       (path ? path : DEFAULT_PHOTO_CONFIG_PATH));
}

void AmConfig::set_audio_config_path(const char *path)
{
  set_config_path(path, mAudioConfigPath);
  INFO("Audio config path set to %s",
       (path ? path : DEFAULT_PHOTO_CONFIG_PATH));
}

void AmConfig::set_wifi_config_path(const char *path)
{
  set_config_path(path, mWifiConfigPath);
  INFO("Wifi config path set to %s",
       (path ? path : DEFAULT_WIFI_CONFIG_PATH));
}

void AmConfig::set_audiodetect_config_path(const char *path)
{
  set_config_path(path, mAudioDetectConfigPath);
  INFO("Wifi config path set to %s",
       (path ? path : DEFAULT_AUDIODETECT_CONFIG_PATH));
}

void AmConfig::set_motiondetect_config_path(const char *path)
{
  set_config_path(path, mMotionDetectConfigPath);
  INFO("Motion Detection config path set to %s",
       (path ? path : DEFAULT_MOTIONDETECT_CONFIG_PATH));
}

void AmConfig::set_lbr_config_path(const char *path)
{
  set_config_path(path, mLBRConfigPath);
  INFO("LBR config path set to %s",
       (path ? path : DEFAULT_LBR_CONFIG_PATH));
}

#ifdef CONFIG_ARCH_S2
void AmConfig::set_fisheye_config_path(const char* path)
{
  set_config_path(path, mFisheyeConfigPath);
  INFO("Fisheye config path set to %s",
       (path ? path : DEFAULT_FISHEYE_CONFIG_PATH));
}
#endif


bool AmConfig::load_vdev_config()
{
  bool ret = false;
  do {
    mVdevParams = get_vdev_config();
    if (mVdevParams) {
      if (mVdevParams->vin_number > 0) {
        mVinParamList = new VinParameters* [mVdevParams->vin_number];
        if (mVinParamList) {
          for (uint32_t i = 0; i < mVdevParams->vin_number; ++ i) {
            mVinParamList[i] = get_vin_config(mVdevParams->vin_list[i]);
          }
        }
      } else {
        ERROR("VIN device is disabled in VideoDevice's configuration!");
        break;
      }
      if (mVdevParams->vout_number > 0) {
        mVoutParamList = new VoutParameters* [mVdevParams->vout_number];
        if (mVoutParamList) {
          bool checked = true;
          for (uint32_t i = 0; i < mVdevParams->vout_number; ++ i) {
            mVoutParamList[i] = get_vout_config(mVdevParams->vout_list[i]);
            if (mVdevParams->vout_list[i] != AM_VOUT_TYPE_NONE) {
              INFO("Vout%u is enabled!", i);
              if (!check_vout_parameters(mVoutParamList[i],
                                         mVdevParams->vout_list[i])) {
                checked = false;
                ERROR("Vout%u parameters checking failed!", i);
                break;
              }
            } else {
              WARN("Vout%u is set to None, disabled!", i);
            }
          }
          if (false == checked) {
            break;
          }
        }
      }
      if (mVdevParams->stream_number > 0) {
        mStreamParamList = new StreamParameters* [mVdevParams->stream_number];
        if (mStreamParamList) {
          for (uint32_t i = 0; i < mVdevParams->stream_number; ++i) {
            if (NULL == (mStreamParamList[i] = get_stream_config(i))) {
              ERROR("Load Stream%u configuration failed.", i);
            } else {
              INFO("Stream%u is enabled!", i);
            }
          }
        }
      }
    } else {
      ERROR("Failed to get VideoDevice's configuration!");
      break;
    }

    if (NULL == (mEncoderParams = get_encoder_config())) {
      ERROR("Failed to get Encoder's configuration!");
      break;
    }

    /* Following calling sequence should not be changed*/
    for (uint32_t i = 0; i < mVdevParams->vin_number; ++ i) {
      if (mVdevParams->vin_list[i] != AM_VIN_TYPE_NONE) {
        cross_check_vin_encoder(mVinParamList[i]);
      }
    }

    /* Check VOUT and buffer parameters, VOUT parameters will affact buffer*/
    for (uint32_t i = 0; i < mVdevParams->vout_number; ++ i) {
      if (mVdevParams->vout_list[i] != AM_VOUT_TYPE_NONE) {
        cross_check_vout_encoder(mVoutParamList[i],
                                 mVdevParams->vout_list[i]);
      }
    }
    cross_check_encoder_resource(mVdevParams->stream_number);

    if (mEncoderParams->config_changed) {
      DEBUG("EncoderParameter has changed, save it back!");
      set_encoder_config(mEncoderParams);
    }
    for (uint32_t i = 0; i < mVdevParams->stream_number; ++ i) {
      if (mStreamParamList && mStreamParamList[i] &&
          mStreamParamList[i]->config_changed) {
        set_stream_config(mStreamParamList[i], i);
        DEBUG("StreamParameter %u has changed, save it back!", i);
      }
    }

    ret = true;
  } while (0);

  return ret;
}

bool AmConfig::load_record_config()
{
  bool ret = (load_audio_config() && check_audio_parameters());
  if (AM_LIKELY(ret)) {
    mRecordParams = get_record_config();
    if (mRecordParams) {
      mRecordParams->set_audio_parameters(mAudioParams);
      ret = check_record_parameters();
    } else {
      ERROR("Failed to get Record's configuration!");
    }
  } else {
    ERROR("Failed to get audio's configuration!");
  }

  return ret;
}

bool AmConfig::load_photo_config()
{
  bool ret = false;
  mPhotoParams = get_photo_config();
  if (mPhotoParams) {
    ret = check_photo_parameters();
  } else {
    ERROR("Failed to get Photo configuration!");
  }

  return ret;
}

bool AmConfig::load_audio_config()
{
  mAudioParams = get_audio_config();
  return (NULL != mAudioParams);
}

bool AmConfig::load_wifi_config ()
{
  mWifiParams = get_wifi_config ();
  return (NULL != mWifiParams);
}

bool AmConfig::load_audiodetect_config ()
{
  mAudioDetectParams = get_audiodetect_config ();
  return (NULL != mAudioDetectParams);
}

bool AmConfig::load_motiondetect_config()
{
  mMotionDetectParams = get_motiondetect_config();
  return (NULL != mMotionDetectParams);
}

bool AmConfig::load_lbr_config()
{
  mLBRParams = get_lbr_config();
  return (NULL != mLBRParams);
}

#ifdef CONFIG_ARCH_S2
bool AmConfig::load_fisheye_config()
{
  mFisheyeParams = get_fisheye_config();
  return (NULL != mFisheyeParams);
}
#endif

void AmConfig::print_vin_config()
{
  if (mVinConfig) {
    mVinConfig->dump_ini_file();
  }
}

void AmConfig::print_vout_config()
{
  if (mVoutConfig) {
    mVoutConfig->dump_ini_file();
  }
}

void AmConfig::print_vdev_config()
{
  if (mVdevConfig) {
    mVdevConfig->dump_ini_file();
  }
}

void AmConfig::print_encoder_config()
{
  if (mEncoderConfig) {
    mEncoderConfig->dump_ini_file();
  }
}

void AmConfig::print_record_config()
{
  if (mRecordConfig) {
    mRecordConfig->dump_ini_file();
  }
}

void AmConfig::print_photo_config()
{
  if (mPhotoConfig) {
    mPhotoConfig->dump_ini_file();
  }
}

void AmConfig::print_audio_config()
{
  if (mPhotoConfig) {
    mAudioConfig->dump_ini_file();
  }
}

void AmConfig::print_wifi_config()
{
  if (mWifiConfig) {
    mWifiConfig->dump_ini_file();
  }
}

void AmConfig::print_audiodetect_config()
{
  if (mAudioDetectConfig) {
    mAudioDetectConfig->dump_ini_file();
  }
}

void AmConfig::print_motiondetect_config()
{
  if (mMotionDetectConfig) {
    mMotionDetectConfig->dump_ini_file();
  }
}

void AmConfig::print_lbr_config()
{
  if (mLBRConfig) {
    mLBRConfig->dump_ini_file();
  }
}

#ifdef CONFIG_ARCH_S2
void AmConfig::print_fisheye_config()
{
  if (mFisheyeConfig) {
    mFisheyeConfig->dump_ini_file();
  }
}
#endif

bool AmConfig::check_audio_parameters()
{
  bool ret = true;
  if (mAudioParams) {
    switch(mAudioParams->audio_format) {
      case AM_AUDIO_FORMAT_AAC: {
        AacEncoderParameters *aac = &mAudioParams->codec->aac;
        switch(aac->aac_format) {
          case AM_AAC_FORMAT_AAC: {
            if (aac->aac_bitrate > 160000) {
              WARN("%u is greater than AAC maximum bit rate 160000, "
                  "reset to 160000", aac->aac_bitrate);
              aac->aac_bitrate = 160000;
              mAudioParams->config_changed = 1;
            } else if (aac->aac_bitrate < 16000) {
              WARN("%u is less than AAC minimum bit rate 16000, "
                  "reset to 16000", aac->aac_bitrate);
              aac->aac_bitrate = 16000;
              mAudioParams->config_changed = 1;
            }
          }break;
          case AM_AAC_FORMAT_AACPLUS: {
            if (aac->aac_bitrate > 64000) {
              WARN("%u is greater than AACPlus maximum bit rate 64000, "
                  "reset to 64000", aac->aac_bitrate);
              aac->aac_bitrate = 64000;
              mAudioParams->config_changed = 1;
            } else if (aac->aac_bitrate < 14000) {
              WARN("%u is less than AACPlus minimum bit rate 14000, "
                  "reset to 14000", aac->aac_bitrate);
              aac->aac_bitrate = 14000;
              mAudioParams->config_changed = 1;
            }
          }break;
          case AM_AAC_FORMAT_AACPLUSPS: {
            if (aac->aac_bitrate > 64000) {
              WARN("%u is greater than AACPlus_PS maximum bit rate 64000, "
                  "reset to 64000", aac->aac_bitrate);
              aac->aac_bitrate = 64000;
              mAudioParams->config_changed = 1;
            } else if (aac->aac_bitrate < 16000) {
              WARN("%u is less than AACPlus_S minimum bit rate 16000, "
                  "reset to 16000", aac->aac_bitrate);
              aac->aac_bitrate = 16000;
              mAudioParams->config_changed = 1;
            }
            if (mAudioParams->adevice_param->audio_channel_num != 2) {
              WARN("AACPlus_PS need stereo audio input, "
                   "reset audio channel to 2");
              mAudioParams->adevice_param->audio_channel_num = 2;
              mAudioParams->adevice_param->config_changed = 1;
            }
          }break;
          default: {
            ERROR("Unknown AAC format!");
            ret = false;
          }
        }
        if (mAudioParams->adevice_param->audio_sample_freq != 48000) {
          WARN("AAC should use 48000 sample rate, "
               "reset audio sample rate to 48000!");
          mAudioParams->adevice_param->audio_sample_freq = 48000;
          mAudioParams->adevice_param->config_changed = 1;
        }
      }break;
      case AM_AUDIO_FORMAT_OPUS: {
        if (mAudioParams->adevice_param->audio_sample_freq != 48000) {
          WARN("Opus should use 48000 sample rate, "
               "reset audio sample rate to 48000!");
          mAudioParams->adevice_param->audio_sample_freq = 48000;
          mAudioParams->adevice_param->config_changed = 1;
        }
      }break;
      case AM_AUDIO_FORMAT_PCM:
      case AM_AUDIO_FORMAT_BPCM: {/* todo */
        ret = true;
      }break;
      case AM_AUDIO_FORMAT_G726: {
        if (mAudioParams->adevice_param->audio_channel_num > 1) {
          WARN("G726 only supports MONO audio, reset audio channel to 1!");
          mAudioParams->adevice_param->audio_channel_num = 1;
          mAudioParams->adevice_param->config_changed = 1;
        }
        if (mAudioParams->adevice_param->audio_sample_freq != 8000) {
          WARN("G726 only supports 8000 sample rate, "
               "reset audio sample rate to 8000!");
          mAudioParams->adevice_param->audio_sample_freq = 8000;
          mAudioParams->adevice_param->config_changed = 1;
        }
        ret = true;
      }break;
      default: {
        ERROR("Unknown audio format!");
        ret = false;
      }break;
    }
    if (mAudioParams->config_changed ||
        mAudioParams->adevice_param->config_changed) {
      set_audio_config(mAudioParams);
    }
  }

  return ret;
}

bool AmConfig::check_record_parameters()
{
  bool ret = false;
  if (mRecordParams) {
    if ((NULL == mRecordParams->file_name_prefix) ||
        (mRecordParams->file_name_prefix &&
            (strlen(mRecordParams->file_name_prefix) == 0))) {
      WARN("Video file name prefix is not set, use A5s instead!");
      mRecordParams->set_file_name_prefix("A5s");
      mRecordParams->config_changed = 1;
    }
    if ((NULL == mRecordParams->file_store_location) ||
        (mRecordParams->file_store_location &&
            (strlen(mRecordParams->file_store_location) == 0))) {
      mRecordParams->set_file_store_location(".");
      WARN("\nVideo file store location is set to current directory!"
          "\nIt's not recommended to store files in current location,"
          "\nunless you know the actual path of current location!");
    }
    if (mRecordParams->\
        file_store_location[strlen(mRecordParams->file_store_location) - 1]
                            == '/') {
      mRecordParams->\
      file_store_location[strlen(mRecordParams->file_store_location) - 1]
                          = '\0';
    }
    ret = AmFile::create_path(mRecordParams->file_store_location);
    if (!ret) {
      ERROR("Failed to create path %s!", mRecordParams->file_store_location);
    }
    if (AM_LIKELY(ret && mEncoderParams)) {
      for (uint32_t i = 0; i < mVdevParams->stream_number; ++ i) {
        iav_encode_format_ex_t &format =
            mStreamParamList[i]->encode_params.encode_format;
        if (AM_LIKELY((format.encode_type == 1) ||
                      (format.encode_type == 2))) {
          mRecordParams->stream_map |= (1 << i);
          mRecordParams->stream_type[i] = format.encode_type;
        }
      }
      if (AM_UNLIKELY(mRecordParams->stream_map == 0)) {
        ERROR("No streams are enabled!");
        ret = false;
      }
    } else if (ret) {
      ERROR("Encoder parameters and stream parameters are not loaded!");
      ret = false;
    }
    if (mRecordParams->config_changed) {
      set_record_config(mRecordParams);
    }
  } else {
    ERROR("Record configuration is not loaded!");
  }

  return ret;
}

bool AmConfig::check_photo_parameters()
{
  bool ret = false;
  if (mPhotoParams) {
    if ((NULL == mPhotoParams->file_name_prefix) ||
        (mPhotoParams->file_name_prefix &&
            (strlen(mPhotoParams->file_name_prefix) == 0))) {
      WARN("Photo file name prefix is not set!");;
    }

    if ((NULL == mPhotoParams->file_store_location) ||
        (mPhotoParams->file_store_location &&
            (strlen(mPhotoParams->file_store_location) == 0))) {
      mRecordParams->set_file_store_location(".");
      WARN("\nPhoto file store location is set to current directory!"
          "It's not recommanded to store files in current location,"
          "\nunlesss you know the actual path of current location!");
    }
    if (mPhotoParams->\
        file_store_location[strlen(mPhotoParams->file_store_location) - 1] ==
            '/') {
      mPhotoParams->\
      file_store_location[strlen(mPhotoParams->file_store_location) - 1] = '\0';
    }
    ret = AmFile::create_path(mPhotoParams->file_store_location);
    if (!ret) {
      ERROR("Failed to create path %s!", mPhotoParams->file_store_location);
    }
  } else {
    ERROR("Photo configuration is not loaded!");
  }

  return ret;
}

bool AmConfig::check_wifi_parameters ()
{
  bool ret = true;
  if (mWifiParams) {
    if ((NULL == mWifiParams->wifi_mode) ||
        (mWifiParams->wifi_mode &&
            strlen (mWifiParams->wifi_mode) == 0)) {
      WARN ("Wifi mode is not set!");
      ret = false;
    }

    if ((NULL == mWifiParams->wifi_ssid) ||
        (mWifiParams->wifi_ssid &&
            strlen (mWifiParams->wifi_ssid) == 0)) {
      WARN ("Wifi ssid is not set!");
      ret = false;
    }

    if ((NULL == mWifiParams->wifi_key) ||
        (mWifiParams->wifi_key &&
            strlen (mWifiParams->wifi_key) == 0)) {
      WARN ("Wifi key is not set!");
      ret = false;
    }
  } else {
    ERROR ("Wifi configuration is not loaded!");
    ret = false;
  }

  return ret;
}

bool AmConfig::check_audiodetect_parameters ()
{
   bool ret = true;

   if (mAudioDetectParams) {
      if (mAudioDetectParams->audio_channel_number != 1 ||
          mAudioDetectParams->audio_channel_number != 2) {
         WARN ("Audio channel is not set correctly: %d",
               mAudioDetectParams->audio_channel_number);
         ret = false;
      }

      if (mAudioDetectParams->audio_alert_sensitivity > 5) {
         WARN ("Audio alert sensitivity is not set correctly[1 -5]: %d",
               mAudioDetectParams->audio_alert_sensitivity);
         ret = false;
      }
   } else {
      ERROR ("Audio Detect configuration is not loaded!");
      ret = false;
   }

   return ret;
}

bool AmConfig::check_motiondetect_parameters()
{
  bool ret = false;
  if (mMotionDetectParams) {
    uint32_t *params = NULL;

    if (strcmp(mMotionDetectParams->algorithm, "MOG2") == 0) {
      for (int i = 0; i < MAX_MOTION_DETECT_ROI_NUM; i++) {
        params = mMotionDetectParams->value[i];
        if (params[MD_ROI_LEFT] > 15) {
          WARN("Invalid roi left value[0~15]: %d, reset to 0\n",
              params[MD_ROI_LEFT]);
          params[MD_ROI_LEFT] = 0;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_RIGHT] > 15 ||
            params[MD_ROI_RIGHT] < params[MD_ROI_LEFT]) {
          WARN("Invalid roi right value[%d~15]: %d, should be larger than roi left"
              "value, reset to 15\n",
              params[MD_ROI_LEFT], params[MD_ROI_RIGHT]);
          params[MD_ROI_RIGHT] = 15;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_TOP] > 15) {
          WARN("Invalid roi top value[0~15]: %d, reset to 0\n",
              params[MD_ROI_TOP]);
          params[MD_ROI_TOP] = 0;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_BOTTOM] > 15 ||
            params[MD_ROI_BOTTOM] < params[MD_ROI_TOP]) {
          WARN("Invalid roi botton value[%d~15]: %d, should be larger than roi"
              "top value, reset to 15\n",
              params[MD_ROI_TOP], params[MD_ROI_BOTTOM]);
          params[MD_ROI_BOTTOM] = 15;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_THRESHOLD] > 200) {
          WARN("Invalid roi threshold[0~200]: %d, reset to 50\n",
              params[MD_ROI_THRESHOLD]);
          params[MD_ROI_THRESHOLD] = 50;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_SENSITIVITY] > 100) {
          WARN("Invalid roi sensitivity[0~100]: %d, reset to 100\n",
              params[MD_ROI_SENSITIVITY]);
          params[MD_ROI_SENSITIVITY] = 100;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_VALID] > 1) {
          WARN("Invalid roi valid[0~1]: %d, reset to 1\n",
              params[MD_ROI_VALID]);
          params[MD_ROI_VALID] = 1;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_OUTPUT_MOTION] > 1) {
          WARN("Invalid roi ouput_motion[0~1]: %d, reset to 1\n",
              params[MD_ROI_OUTPUT_MOTION]);
          params[MD_ROI_OUTPUT_MOTION] = 1;
          mMotionDetectParams->config_changed = 1;
        }
      }
    } else {
      if (strcmp(mMotionDetectParams->algorithm, "MSE") != 0) {
        strncpy(mMotionDetectParams->algorithm, "MSE",
                sizeof(mMotionDetectParams->algorithm));
      }
      for (int i = 0; i < MAX_MOTION_DETECT_ROI_NUM; i++) {
        params = mMotionDetectParams->value[i];
        if (params[MD_ROI_LEFT] > 11) {
          WARN("Invalid roi left value[0~11]: %d, reset to 0\n",
              params[MD_ROI_LEFT]);
          params[MD_ROI_LEFT] = 0;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_RIGHT] > 11 ||
            params[MD_ROI_RIGHT] < params[MD_ROI_LEFT]) {
          WARN("Invalid roi right value[%d~11]: %d, should be larger than roi left"
              "value, reset to 11\n",
              params[MD_ROI_LEFT], params[MD_ROI_RIGHT]);
          params[MD_ROI_RIGHT] = 11;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_TOP] > 7) {
          WARN("Invalid roi top value[0~7]: %d, reset to 0\n",
              params[MD_ROI_TOP]);
          params[MD_ROI_TOP] = 0;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_BOTTOM] > 7 ||
            params[MD_ROI_BOTTOM] < params[MD_ROI_TOP]) {
          WARN("Invalid roi botton value[%d~7]: %d, should be larger than roi"
              "top value, reset to 7\n",
              params[MD_ROI_TOP], params[MD_ROI_BOTTOM]);
          params[MD_ROI_BOTTOM] = 7;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_THRESHOLD] > 200) {
          WARN("Invalid roi threshold[0~200]: %d, reset to 50\n",
              params[MD_ROI_THRESHOLD]);
          params[MD_ROI_THRESHOLD] = 50;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_SENSITIVITY] > 100) {
          WARN("Invalid roi sensitivity[0~100]: %d, reset to 100\n",
              params[MD_ROI_SENSITIVITY]);
          params[MD_ROI_SENSITIVITY] = 100;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_VALID] > 1) {
          WARN("Invalid roi valid[0~1]: %d, reset to 1\n",
              params[MD_ROI_VALID]);
          params[MD_ROI_VALID] = 1;
          mMotionDetectParams->config_changed = 1;
        }
        if (params[MD_ROI_OUTPUT_MOTION] > 1) {
          WARN("Invalid roi ouput_motion[0~1]: %d, reset to 1\n",
              params[MD_ROI_OUTPUT_MOTION]);
          params[MD_ROI_OUTPUT_MOTION] = 1;
          mMotionDetectParams->config_changed = 1;
        }
      }
    }

    if (mMotionDetectParams->config_changed) {
      set_motiondetect_config(mMotionDetectParams);
    }

    ret = true;
  } else {
    ERROR("Motion Detect configuration is not loaded!");
    return ret;
  }

  return ret;
}

bool AmConfig::check_vout_parameters(VoutParameters *voutConfig, VoutType type)
{
  bool ret = false;
  if (voutConfig) {
    if (type == AM_VOUT_TYPE_HDMI) {
      if ((voutConfig->video_mode == AMBA_VIDEO_MODE_HDMI_NATIVE) ||
          (voutConfig->video_mode == AMBA_VIDEO_MODE_AUTO)) {
        voutConfig->video_mode = hdmi_native_mode();
      }
    } else if (type == AM_VOUT_TYPE_CVBS) {
      if ((voutConfig->video_mode != AMBA_VIDEO_MODE_480I) &&
          (voutConfig->video_mode != AMBA_VIDEO_MODE_576I)) {
        WARN("CVBS can only support 480i or 576i, reset to 576i");
        voutConfig->video_mode = AMBA_VIDEO_MODE_576I;
      }
    } else if (type == AM_VOUT_TYPE_LCD) {
      if ((voutConfig->video_mode == AMBA_VIDEO_MODE_AUTO)) {
        ERROR("Cannot detect LCD's resolution! Please specify it manully!");
        return ret;
      }
    }
    if (voutConfig->vout_video_size.specified) {
      int32_t w = video_mode_width(voutConfig->video_mode);
      int32_t h = video_mode_height(voutConfig->video_mode);
      uint16_t &voutW = voutConfig->vout_video_size.vout_width;
      uint16_t &voutH = voutConfig->vout_video_size.vout_height;
      uint16_t &videoW = voutConfig->vout_video_size.video_width;
      uint16_t &videoH = voutConfig->vout_video_size.video_height;
      int16_t  &offsetX = voutConfig->video_offset.offset_x;
      int16_t  &offsetY = voutConfig->video_offset.offset_y;

      if ((voutW > w) || (voutW <= 0)) {
        WARN("Vout Width %hu is invalid, reset to %d!", voutW, w);
        voutW = w;
        voutConfig->config_changed = 1;
      }
      if ((voutH > h) || (voutH <= 0)) {
        WARN("Vout Height %hu is invalid, reset to %d!", voutH, h);
        voutH = h;
        voutConfig->config_changed = 1;
      }
      if ((videoW > w) || (videoW <= 0)) {
        WARN("Video Width %hu is invalid, reset to %d!", videoW, w);
        videoW = w;
        voutConfig->config_changed = 1;
      }
      if ((videoH > h) || (videoH <= 0)) {
        WARN("Video Height %hu is invalid, reset to %d!", videoH, h);
        videoH = h;
        voutConfig->config_changed = 1;
      }
      /* offset should be calculated by customer
       * default is to set to center */
      offsetX = (int16_t)((w - voutW) / 2);
      offsetY = (int16_t)((h - voutH) / 2);
      if ((offsetX > 0) || (offsetY > 0)) {
        voutConfig->video_offset.specified = 1;
        voutConfig->config_changed = 1;
      }
    }
    if (voutConfig->video_offset.specified) {
      if (voutConfig->video_offset.offset_x < 0) {
        WARN("Invalid video offset X value: %d!",
             voutConfig->video_offset.offset_x);
        voutConfig->video_offset.offset_x = 0;
        voutConfig->config_changed = 1;
      }
      if (voutConfig->video_offset.offset_y < 0) {
        WARN("Invalid video offset Y value: %d!",
             voutConfig->video_offset.offset_y);
        voutConfig->video_offset.offset_y = 0;
        voutConfig->config_changed = 1;
      }
      if ((voutConfig->video_offset.offset_x > 0) &&
          (voutConfig->video_offset.offset_y > 0)) {
        if (voutConfig->video_offset.offset_x % 4 != 0) {
          voutConfig->video_offset.offset_x =
              round_up(voutConfig->video_offset.offset_x, 4);
          voutConfig->config_changed = 1;
          WARN("Video offset X must be multiple of 4, reset to %hd!",
               voutConfig->video_offset.offset_x);
        }
        if (voutConfig->video_offset.offset_y % 4 != 0) {
          voutConfig->video_offset.offset_y =
              round_up(voutConfig->video_offset.offset_y, 4);
          voutConfig->config_changed = 1;
          WARN("Video offset Y must be multiple of 4, reset to %hd!",
               voutConfig->video_offset.offset_y);
        }
      }
    } else {
      voutConfig->video_offset.offset_x = 0;
      voutConfig->video_offset.offset_y = 0;
      voutConfig->config_changed = 1;
    }
    if (voutConfig->config_changed) {
      set_vout_config(voutConfig, type);
    }
    ret = true;
  } else {
    const char * type_to_str[] =
    { "None", "LCD", "HDMI", "CVBS"};
    ERROR("%s's configuration is not loaded!", type_to_str[(uint32_t)type]);
  }

  return ret;
}

void AmConfig::cross_check_vin_encoder(VinParameters *vin)
{
  if (vin && mEncoderParams) {
    uint16_t vinWidth = (uint16_t)video_mode_width(vin->video_mode);
    uint16_t vinHeight = (uint16_t)video_mode_height(vin->video_mode);

    /* Main buffer max size <= Vin size */
    if (vinWidth && mEncoderParams->max_source_buffer[0].width > vinWidth) {
      WARN("Main buffer max width [%d] SHOULD NOT be greater than VIN width "
           "[%d]. Reset it to VIN width.",
          mEncoderParams->max_source_buffer[0].width, vinWidth);
      mEncoderParams->max_source_buffer[0].width = vinWidth;
      mEncoderParams->config_changed = 1;
    }
    if (vinHeight && mEncoderParams->max_source_buffer[0].height > vinHeight) {
      WARN("Main buffer max height [%d] SHOULD NOT be greater than VIN height "
           "[%d]. Reset it to VIN height.",
          mEncoderParams->max_source_buffer[0].height, vinHeight);
      mEncoderParams->max_source_buffer[0].height = vinHeight;
      mEncoderParams->config_changed = 1;
    }
  } else {
    ERROR("Invalid %s configurations!", (!vin ? (!mEncoderParams ?
        "VIN and Encoder" : "VIN") : "Encoder") );
  }
}

#ifdef CONFIG_ARCH_S2
void AmConfig::cross_check_vout_encoder(VoutParameters *vout,
                                        VoutType type)
{
  if (vout && mEncoderParams) {
    iav_system_resource_setup_ex_t          &sysRes =
        mEncoderParams->system_resource_info;
    iav_source_buffer_format_all_ex_t &bufFormatAll =
        mEncoderParams->buffer_format_info;
    iav_source_buffer_type_all_ex_t        &bufType =
        mEncoderParams->buffer_type_info;
    int32_t voutW = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
        video_mode_height(vout->video_mode) :
        video_mode_width(vout->video_mode);
    int32_t voutH = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
        video_mode_width(vout->video_mode) :
        video_mode_height(vout->video_mode);
    int32_t maxW  = 0;
    int32_t maxH  = 0;
    if (vout->vout_video_size.specified) {
      voutW = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
          vout->vout_video_size.video_height :
          vout->vout_video_size.video_width;
      voutH = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
          vout->vout_video_size.video_width :
          vout->vout_video_size.video_height;
    }
    switch(type) {
      case AM_VOUT_TYPE_LCD: {
        maxW = BUFFER_SUB3_MAX_W;
        maxH = BUFFER_SUB3_MAX_H;
      }break;
      case AM_VOUT_TYPE_HDMI: {
        maxW = BUFFER_SUB2_MAX_W;
        maxH = BUFFER_SUB2_MAX_H;
      }break;
      case AM_VOUT_TYPE_CVBS: {
        maxW = BUFFER_SUB1_MAX_W;
        maxH = BUFFER_SUB1_MAX_H;
      }break;
      default: break;
    }
    bufType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
    bufType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;

    if ((type == AM_VOUT_TYPE_HDMI) || (type == AM_VOUT_TYPE_CVBS)) {
      bufType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
      DEBUG("Third buffer is used for preview!");
      /* Set bmaxsize */
      sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width =
          round_up((voutW <= 0) ? maxW : ((voutW <= maxW) ? voutW : maxW), 4);
      sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height =
          round_up((voutH <= 0) ? maxH : ((voutH <= maxH) ? voutH : maxH), 4);
      /* Set bsize */
      if ((voutW <= maxW) && (voutH <= maxH)) {
        bufFormatAll.third_width  = round_up((voutW <= 0) ? maxW : voutW, 4);
        bufFormatAll.third_height = round_up((voutH <= 0) ? maxH : voutH, 4);
      } else {
        if (voutW > voutH) {
          bufFormatAll.third_width = round_up(maxW, 4);
          bufFormatAll.third_height = round_up(((voutH > 0) && (voutW > 0)) ?
              maxW * voutH / voutW : maxH, 4);
        } else {
          bufFormatAll.third_width = round_up(((voutW > 0) && (voutH > 0)) ?
              maxH * voutW / voutH : maxW, 4);
          bufFormatAll.third_height = round_up(maxH, 4);
        }
      }
      DEBUG("%s: buffer max width: %hu, buffer max height: %hu",
            (type == AM_VOUT_TYPE_CVBS ? "CVBS" : "HDMI"),
            sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width,
            sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height);
      DEBUG("      buffer     width: %hu, buffer     height: %hu",
            bufFormatAll.third_width, bufFormatAll.third_height);

      if(sysRes.encode_mode == IAV_ENCODE_WARP_MODE) {
        DEBUG("Fourth buffer is used for preview!");
        bufType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
        sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width = sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width;
        sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height = sysRes.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height;
        bufFormatAll.fourth_width = bufFormatAll.third_width;
        bufFormatAll.fourth_height = bufFormatAll.third_height;
        DEBUG("%s: buffer max width: %hu, buffer max height: %hu",
              (type == AM_VOUT_TYPE_CVBS ? "CVBS" : "HDMI"),
              sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width,
              sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height);
        DEBUG("      buffer     width: %hu, buffer     height: %hu",
              bufFormatAll.fourth_width, bufFormatAll.fourth_height);
      }

      mEncoderParams->config_changed = 1;
    } else if (type == AM_VOUT_TYPE_LCD) {
      bufType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
      DEBUG("Fourth buffer is used for preview!");
      /* Set bmaxsize */
      sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width =
          round_up((voutW <= 0) ? maxW : ((voutW <= maxW) ? voutW : maxW), 4);
      sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height =
          round_up((voutH <= 0) ? maxH : ((voutH <= maxH) ? voutH : maxH), 4);
      /* Set bsize */
      if ((voutW <= maxW) && (voutH <= maxH)) {
        bufFormatAll.fourth_width  = round_up((voutW <= 0) ? maxW : voutW, 4);
        bufFormatAll.fourth_height = round_up((voutH <= 0) ? maxH : voutH, 4);
      } else {
        if (voutW > voutH) {
          bufFormatAll.fourth_width  = round_up(maxW, 4);
          bufFormatAll.fourth_height = round_up(((voutH > 0) && (voutW > 0))
                                                ? maxW * voutH / voutW : maxH, 4);
        } else {
          bufFormatAll.fourth_width  =
              round_up(((voutW > 0) && (voutH > 0)) ?
                  maxH * voutW / voutH : maxW, 4);
          bufFormatAll.fourth_height = round_up(maxH, 4);
        }
      }
      DEBUG("LCD: buffer max width: %hu, buffer max height: %hu",
            sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width,
            sysRes.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height);
      DEBUG("     buffer     width: %hu, buffer     height: %hu",
            bufFormatAll.fourth_width, bufFormatAll.fourth_height);
      mEncoderParams->config_changed = 1;
    }
  } else {
    ERROR("Invalid %s configurations!", (!vout ?
        (!mEncoderParams ? "VOUT and Encoder" : "VOUT") : "Encoder") );
  }

}


void AmConfig::cross_check_encoder_resource(uint32_t streamNum)
{
  if (mEncoderParams) {
    iav_system_resource_setup_ex_t &sysResource =
        mEncoderParams->system_resource_info;

    /* Second buffer max size <= Main buffer max size */
    if (mEncoderParams->max_source_buffer[1].width >
    mEncoderParams->max_source_buffer[0].width) {
      WARN("Second buffer max width [%d] should be no greater than main buffer"
           "width [%d]. Reset it to the main buffer width.",
           mEncoderParams->max_source_buffer[1].width,
           mEncoderParams->max_source_buffer[0].width);
      mEncoderParams->config_changed = 1;
    }
    if (mEncoderParams->max_source_buffer[1].height >
    mEncoderParams->max_source_buffer[0].height) {
      WARN("Second buffer max width [%d] should be no greater than main buffer"
           "width [%d]. Reset it to the main buffer width.",
           mEncoderParams->max_source_buffer[1].height,
           mEncoderParams->max_source_buffer[0].height);
      mEncoderParams->config_changed = 1;
    }

    sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width =
        mEncoderParams->max_source_buffer[0].width;
    sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height =
        mEncoderParams->max_source_buffer[0].height;
    sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width =
        mEncoderParams->max_source_buffer[1].width;
    sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height =
        mEncoderParams->max_source_buffer[1].height;

    /* Disable third and fourth buffer if the max size is 0x0 */
    if (mEncoderParams->buffer_type_info.third_buffer_type !=
        IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
      if (!mEncoderParams->max_source_buffer[2].width ||
          !mEncoderParams->max_source_buffer[2].height) {
        mEncoderParams->buffer_type_info.third_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_OFF;
      } else {
        mEncoderParams->buffer_type_info.third_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_ENCODE;
        sysResource.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width =
            mEncoderParams->max_source_buffer[2].width;
        sysResource.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height =
            mEncoderParams->max_source_buffer[2].height;
      }
    }

    if (mEncoderParams->buffer_type_info.fourth_buffer_type !=
        IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
      if (!mEncoderParams->max_source_buffer[3].width ||
          !mEncoderParams->max_source_buffer[3].height) {
        mEncoderParams->buffer_type_info.fourth_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_OFF;
      } else {
        mEncoderParams->buffer_type_info.fourth_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_ENCODE;
        sysResource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width =
            mEncoderParams->max_source_buffer[3].width;
        sysResource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height =
            mEncoderParams->max_source_buffer[3].height;
      }
    }

    /* Disable streams if id < streamNum */
    for (uint32_t i = streamNum; i < MAX_ENCODE_STREAM_NUM; ++ i) {
      sysResource.stream_max_size[i].width =
          sysResource.stream_max_size[i].height = 0;
    }
  } else {
    ERROR("Invalid Encoder configurations!");
  }
}

#else
void AmConfig::cross_check_vout_encoder(VoutParameters *vout,
                                        VoutType type)
{
  if (vout && mEncoderParams) {
    iav_system_resource_setup_ex_t          &sysRes =
        mEncoderParams->system_resource_info;
    iav_source_buffer_format_all_ex_t &bufFormatAll =
        mEncoderParams->buffer_format_info;
    iav_source_buffer_type_all_ex_t        &bufType =
        mEncoderParams->buffer_type_info;
    int32_t voutW = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
        video_mode_height(vout->video_mode) :
        video_mode_width(vout->video_mode);
    int32_t voutH = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
        video_mode_width(vout->video_mode) :
        video_mode_height(vout->video_mode);
    int32_t maxW  = 0;
    int32_t maxH  = 0;
    if (vout->vout_video_size.specified) {
      voutW = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
          vout->vout_video_size.video_height :
          vout->vout_video_size.video_width;
      voutH = (vout->video_rotate == AMBA_VOUT_ROTATE_90) ?
          vout->vout_video_size.video_width :
          vout->vout_video_size.video_height;
    }
    switch(type) {
      case AM_VOUT_TYPE_LCD: {
        maxW = BUFFER_SUB3_MAX_W;
        maxH = BUFFER_SUB3_MAX_H;
      }break;
      case AM_VOUT_TYPE_HDMI: {
        maxW = BUFFER_SUB2_MAX_W;
        maxH = BUFFER_SUB2_MAX_H;
      }break;
      case AM_VOUT_TYPE_CVBS: {
        maxW = BUFFER_SUB1_MAX_W;
        maxH = BUFFER_SUB1_MAX_H;
      }break;
      default: break;
    }
    bufType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
    bufType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;

    if ((type == AM_VOUT_TYPE_HDMI) || (type == AM_VOUT_TYPE_CVBS)) {
      bufType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
      DEBUG("Third buffer is used for preview!");
      /* Set bmaxsize */
      sysRes.third_source_buffer_max_width =
          round_up((voutW <= 0) ? maxW : ((voutW <= maxW) ? voutW : maxW), 4);
      sysRes.third_source_buffer_max_height =
          round_up((voutH <= 0) ? maxH : ((voutH <= maxH) ? voutH : maxH), 4);
      /* Set bsize */
      if ((voutW <= maxW) && (voutH <= maxH)) {
        bufFormatAll.third_width  = round_up((voutW <= 0) ? maxW : voutW, 4);
        bufFormatAll.third_height = round_up((voutH <= 0) ? maxH : voutH, 4);
      } else {
        if (voutW > voutH) {
          bufFormatAll.third_width = round_up(maxW, 4);
          bufFormatAll.third_height = round_up(((voutH > 0) && (voutW > 0)) ?
              maxW * voutH / voutW : maxH, 4);
        } else {
          bufFormatAll.third_width = round_up(((voutW > 0) && (voutH > 0)) ?
              maxH * voutW / voutH : maxW, 4);
          bufFormatAll.third_height = round_up(maxH, 4);
        }
      }
      DEBUG("%s: buffer max width: %hu, buffer max height: %hu",
            (type == AM_VOUT_TYPE_CVBS ? "CVBS" : "HDMI"),
            sysRes.third_source_buffer_max_width,
            sysRes.third_source_buffer_max_height);
      DEBUG("      buffer     width: %hu, buffer     height: %hu",
            bufFormatAll.third_width, bufFormatAll.third_height);

      mEncoderParams->config_changed = 1;
    } else if (type == AM_VOUT_TYPE_LCD) {
      bufType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
      DEBUG("Fourth buffer is used for preview!");
      /* Set bmaxsize */
      sysRes.fourth_source_buffer_max_width =
          round_up((voutW <= 0) ? maxW : ((voutW <= maxW) ? voutW : maxW), 4);
      sysRes.fourth_source_buffer_max_height =
          round_up((voutH <= 0) ? maxH : ((voutH <= maxH) ? voutH : maxH), 4);
      /* Set bsize */
      if ((voutW <= maxW) && (voutH <= maxH)) {
        bufFormatAll.fourth_width  = round_up((voutW <= 0) ? maxW : voutW, 4);
        bufFormatAll.fourth_height = round_up((voutH <= 0) ? maxH : voutH, 4);
      } else {
        if (voutW > voutH) {
          bufFormatAll.fourth_width  = round_up(maxW, 4);
          bufFormatAll.fourth_height = round_up(((voutH > 0) && (voutW > 0))
                                                ? maxW * voutH / voutW : maxH, 4);
        } else {
          bufFormatAll.fourth_width  =
              round_up(((voutW > 0) && (voutH > 0)) ?
                  maxH * voutW / voutH : maxW, 4);
          bufFormatAll.fourth_height = round_up(maxH, 4);
        }
      }
      DEBUG("LCD: buffer max width: %hu, buffer max height: %hu",
            sysRes.fourth_source_buffer_max_width,
            sysRes.fourth_source_buffer_max_height);
      DEBUG("     buffer     width: %hu, buffer     height: %hu",
            bufFormatAll.fourth_width, bufFormatAll.fourth_height);
      mEncoderParams->config_changed = 1;
    }
  } else {
    ERROR("Invalid %s configurations!", (!vout ?
        (!mEncoderParams ? "VOUT and Encoder" : "VOUT") : "Encoder") );
  }

}

void AmConfig::cross_check_encoder_resource(uint32_t streamNum)
{
  if (mEncoderParams) {
    iav_system_resource_setup_ex_t &sysResource =
        mEncoderParams->system_resource_info;
    if (sysResource.adv_oversampling) {
      if (sysResource.stream_max_GOP_M[0] > 1) {
        WARN("When advanced over sampling is enabled, cannot enable B frame!"
            "Reset stream0 max GOP M to 1");
        sysResource.stream_max_GOP_M[0] = 1;
        mEncoderParams->config_changed = 1;
      }
      if (mStreamParamList && mStreamParamList[0] &&
          (mStreamParamList[0]->h264_config.M > 1)) {
        WARN("When advanced over sampling is enabled, cannot enable B frame!"
             "Reset stream0 H.264 M to 1");
        mStreamParamList[0]->h264_config.M = 1;
        mStreamParamList[0]->config_changed = 1;
      }
    }

    /* Second buffer max size <= Main buffer max size */
    if (mEncoderParams->max_source_buffer[1].width >
    mEncoderParams->max_source_buffer[0].width) {
      WARN("Second buffer max width [%d] should be no greater than main buffer"
           "width [%d]. Reset it to the main buffer width.",
           mEncoderParams->max_source_buffer[1].width,
           mEncoderParams->max_source_buffer[0].width);
      mEncoderParams->config_changed = 1;
    }
    if (mEncoderParams->max_source_buffer[1].height >
    mEncoderParams->max_source_buffer[0].height) {
      WARN("Second buffer max width [%d] should be no greater than main buffer"
           "width [%d]. Reset it to the main buffer width.",
           mEncoderParams->max_source_buffer[1].height,
           mEncoderParams->max_source_buffer[0].height);
      mEncoderParams->config_changed = 1;
    }

    sysResource.main_source_buffer_max_width =
        mEncoderParams->max_source_buffer[0].width;
    sysResource.main_source_buffer_max_height =
        mEncoderParams->max_source_buffer[0].height;
    sysResource.second_source_buffer_max_width =
        mEncoderParams->max_source_buffer[1].width;
    sysResource.second_source_buffer_max_height =
        mEncoderParams->max_source_buffer[1].height;

    /* Disable third and fourth buffer if the max size is 0x0 */
    if (mEncoderParams->buffer_type_info.third_buffer_type !=
        IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
      if (!mEncoderParams->max_source_buffer[2].width ||
          !mEncoderParams->max_source_buffer[2].height) {
        mEncoderParams->buffer_type_info.third_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_OFF;
      } else {
        mEncoderParams->buffer_type_info.third_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_ENCODE;
        sysResource.third_source_buffer_max_width =
            mEncoderParams->max_source_buffer[2].width;
        sysResource.third_source_buffer_max_height =
            mEncoderParams->max_source_buffer[2].height;
      }
    }

    if (mEncoderParams->buffer_type_info.fourth_buffer_type !=
        IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
      if (!mEncoderParams->max_source_buffer[3].width ||
          !mEncoderParams->max_source_buffer[3].height) {
        mEncoderParams->buffer_type_info.fourth_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_OFF;
      } else {
        mEncoderParams->buffer_type_info.fourth_buffer_type =
            IAV_SOURCE_BUFFER_TYPE_ENCODE;
        sysResource.fourth_source_buffer_max_width =
            mEncoderParams->max_source_buffer[3].width;
        sysResource.fourth_source_buffer_max_height =
            mEncoderParams->max_source_buffer[3].height;
      }
    }

    /* Disable streams if id < streamNum */
    for (uint32_t i = streamNum; i < MAX_ENCODE_STREAM_NUM; ++ i) {
      sysResource.stream_max_encode_size[i].width =
          sysResource.stream_max_encode_size[i].height = 0;
    }
  } else {
    ERROR("Invalid Encoder configurations!");
  }
}

#endif

/* VIN */
VinParameters *AmConfig::get_vin_config(VinType type)
{
  if (!mVinConfig) {
    mVinConfig = new AmConfigVin(mVinConfigPath ? mVinConfigPath :
        DEFAULT_VIN_CONFIG_PATH);
  }

  return (mVinConfig ? mVinConfig->get_vin_config(type) : NULL);
}

void AmConfig::set_vin_config(VinParameters *config, VinType type)
{
  if (!mVinConfig) {
    mVinConfig = new AmConfigVin(mVinConfigPath ? mVinConfigPath :
        DEFAULT_VIN_CONFIG_PATH);
  }
  if (mVinConfig) {
    mVinConfig->set_vin_config(config, type);
  }
}

/* VOUT */
VoutParameters *AmConfig::get_vout_config(VoutType type)
{
  if (!mVoutConfig) {
    mVoutConfig = new AmConfigVout(mVoutConfigPath ? mVoutConfigPath :
        DEFAULT_VOUT_CONFIG_PATH);
  }

  return (mVoutConfig ? mVoutConfig->get_vout_config(type) : NULL);
}

void AmConfig::set_vout_config(VoutParameters *config, VoutType type)
{
  if (!mVoutConfig) {
    mVoutConfig = new AmConfigVout(mVoutConfigPath ? mVoutConfigPath :
        DEFAULT_VOUT_CONFIG_PATH);
  }
  if (mVoutConfig) {
    mVoutConfig->set_vout_config(config, type);
  }
}

/* Video Device */
VDeviceParameters *AmConfig::get_vdev_config()
{
  if (!mVdevConfig) {
    mVdevConfig = new AmConfigVDevice(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }

  return (mVdevConfig ? mVdevConfig->get_vdev_config() : NULL);
}

void AmConfig::set_vdev_config(VDeviceParameters *vdevConfig)
{
  if (!mVdevConfig) {
    mVdevConfig = new AmConfigVDevice(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }
  if (mVdevConfig) {
    mVdevConfig->set_vdev_config(vdevConfig);
  }
}

EncoderParameters *AmConfig::get_encoder_config()
{
  if (!mEncoderConfig) {
    mEncoderConfig = new AmConfigEncoder(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }
  return (mEncoderConfig ? mEncoderConfig->get_encoder_config(mIavFd) : NULL);
}

void AmConfig::set_encoder_config(EncoderParameters *encoderConfig)
{
  if (!mEncoderConfig) {
    mEncoderConfig = new AmConfigEncoder(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }

  if (mEncoderConfig && encoderConfig) {
    mEncoderConfig->set_encoder_config(encoderConfig);
  }
}

StreamParameters *AmConfig::get_stream_config(uint32_t streamId)
{
  if (!mStreamConfig) {
    mStreamConfig = new AmConfigStream(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }
  return (mStreamConfig ? mStreamConfig->get_stream_config(mIavFd, streamId) :
      NULL);
}

void AmConfig::set_stream_config(StreamParameters *config, uint32_t streamID)
{
  if (!mStreamConfig) {
    mStreamConfig = new AmConfigStream(mVdevConfigPath ? mVdevConfigPath :
        DEFAULT_VDEV_CONFIG_PATH);
  }
  if (mStreamConfig && config) {
    mStreamConfig->set_stream_config(config, streamID);
  }

}

/* Record */
RecordParameters *AmConfig::get_record_config()
{
  if (!mRecordConfig) {
    mRecordConfig = new AmConfigRecord(mRecordConfigPath ? mRecordConfigPath :
        DEFAULT_RECORD_CONFIG_PATH);
  }

  return (mRecordConfig ? mRecordConfig->get_record_config() : NULL);
}

void AmConfig::set_record_config(RecordParameters *config)
{
  if (!mRecordConfig) {
    mRecordConfig = new AmConfigRecord(mRecordConfigPath ? mRecordConfigPath :
        DEFAULT_RECORD_CONFIG_PATH);
  }

  if (mRecordConfig && config) {
    mRecordConfig->set_record_config(config);
  }
}

/* Photo */
PhotoParameters *AmConfig::get_photo_config()
{
  if (!mPhotoConfig) {
    mPhotoConfig = new AmConfigPhoto(mPhotoConfigPath ? mPhotoConfigPath :
        DEFAULT_PHOTO_CONFIG_PATH);
  }

  return (mPhotoConfig ? mPhotoConfig->get_photo_config() : NULL);
}

void AmConfig::set_photo_config(PhotoParameters *config)
{
  if (!mPhotoConfig) {
    mPhotoConfig = new AmConfigPhoto(mPhotoConfigPath ? mPhotoConfigPath :
        DEFAULT_PHOTO_CONFIG_PATH);
  }
  if (mPhotoConfig && config) {
    mPhotoConfig->set_photo_config(config);
  }
}

/* Wifi */
WifiParameters *AmConfig::get_wifi_config ()
{
  if (!mWifiConfig) {
    mWifiConfig = new AmConfigWifi (mWifiConfigPath ? mWifiConfigPath :
        DEFAULT_WIFI_CONFIG_PATH);
  }

  return (mWifiConfig ? mWifiConfig->get_wifi_config () : NULL);
}

void AmConfig::set_wifi_config (WifiParameters *config)
{
  if (!mWifiConfig) {
    mWifiConfig = new AmConfigWifi (mWifiConfigPath ? mWifiConfigPath :
        DEFAULT_WIFI_CONFIG_PATH);
  }

  if (mWifiConfig && config) {
    mWifiConfig->set_wifi_config (config);
  }
}

/* AudioDetect */
AudioDetectParameters *AmConfig::get_audiodetect_config ()
{
  if (!mAudioDetectConfig) {
    mAudioDetectConfig = new AmConfigAudioDetect (mAudioDetectConfigPath ? mAudioDetectConfigPath :
        DEFAULT_WIFI_CONFIG_PATH);
  }

  return (mAudioDetectConfig ? mAudioDetectConfig->get_audiodetect_config () : NULL);
}

void AmConfig::set_audiodetect_config (AudioDetectParameters *config)
{
  if (!mAudioDetectConfig) {
    mAudioDetectConfig = new AmConfigAudioDetect (mAudioDetectConfigPath ? mAudioDetectConfigPath :
        DEFAULT_WIFI_CONFIG_PATH);
  }

  if (mAudioDetectConfig && config) {
    mAudioDetectConfig->set_audiodetect_config (config);
  }
}

/* MotionDetect */
MotionDetectParameters *AmConfig::get_motiondetect_config()
{
  if (!mMotionDetectConfig) {
    mMotionDetectConfig =
      new AmConfigMotionDetect(mMotionDetectConfigPath ?
          mMotionDetectConfigPath : DEFAULT_MOTIONDETECT_CONFIG_PATH);
  }

  return (mMotionDetectConfig ?
          mMotionDetectConfig->get_motiondetect_config() : NULL);
}

void AmConfig::set_motiondetect_config(MotionDetectParameters *config)
{
  if (!mMotionDetectConfig) {
    mMotionDetectConfig =
      new AmConfigMotionDetect(mMotionDetectConfigPath ?
          mMotionDetectConfigPath : DEFAULT_MOTIONDETECT_CONFIG_PATH);
  }

  if (mMotionDetectConfig && config) {
    mMotionDetectConfig->set_motiondetect_config(config);
  }
}

AudioParameters *AmConfig::get_audio_config()
{
  if (!mAudioConfig) {
    mAudioConfig = new AmConfigAudio(mAudioConfigPath ? mAudioConfigPath :
        DEFAULT_AUDIO_CONFIG_PATH);
  }
  return (mAudioConfig ? mAudioConfig->get_audio_config() : NULL);
}

void AmConfig::set_audio_config(AudioParameters *config)
{
  if (AM_LIKELY(config)) {
    if (!mAudioConfig) {
      mAudioConfig = new AmConfigAudio(mAudioConfigPath ? mAudioConfigPath :
          DEFAULT_AUDIO_CONFIG_PATH);
    }
    if (AM_LIKELY(mAudioConfig)) {
      mAudioConfig->set_audio_config(config);
    }
  }
}

LBRParameters *AmConfig::get_lbr_config()
{
  if (!mLBRConfig) {
    mLBRConfig = new AmConfigLBR(mLBRConfigPath ? mLBRConfigPath :
        DEFAULT_LBR_CONFIG_PATH);
  }
  return (mLBRConfig ? mLBRConfig->get_lbr_config() : NULL);
}

void AmConfig::set_lbr_config(LBRParameters *config)
{
  if (AM_LIKELY(config)) {
    if (!mLBRConfig) {
      mLBRConfig = new AmConfigLBR(mLBRConfigPath ? mLBRConfigPath :
          DEFAULT_LBR_CONFIG_PATH);
    }
    if (AM_LIKELY(mLBRConfig)) {
      mLBRConfig->set_lbr_config(config);
    }
  }
}

#ifdef CONFIG_ARCH_S2
FisheyeParameters *AmConfig::get_fisheye_config()
{
  if (!mFisheyeConfig) {
    mFisheyeConfig = new AmConfigFisheye(mFisheyeConfigPath ?
        mFisheyeConfigPath : DEFAULT_FISHEYE_CONFIG_PATH);
  }
  return (mFisheyeConfig ? mFisheyeConfig->get_fisheye_config() : NULL);
}

void AmConfig::set_fisheye_config(FisheyeParameters *config)
{
  if (AM_LIKELY(config)) {
    if (!mFisheyeConfig) {
      mFisheyeConfig = new AmConfigFisheye(mFisheyeConfigPath ?
          mFisheyeConfigPath : DEFAULT_FISHEYE_CONFIG_PATH);
    }
    if (AM_LIKELY(mFisheyeConfig)) {
      mFisheyeConfig->set_fisheye_config(config);
    }
  }
}
#endif

int32_t AmConfig::video_mode_width(amba_video_mode mode)
{
  int32_t w = gVideoModeList[0].width;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      w = gVideoModeList[i].width;
      break;
    }
  }

  return w;
}

int32_t AmConfig::video_mode_height(amba_video_mode mode)
{
  int32_t h = gVideoModeList[0].height;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      h = gVideoModeList[i].height;
      break;
    }
  }

  return h;
}

amba_video_mode AmConfig::hdmi_native_mode()
{
  amba_vout_sink_info sinkInfo;
  int32_t num = 0;
  amba_video_mode mode = AMBA_VIDEO_MODE_AUTO;
  memset(&sinkInfo, 0, sizeof(sinkInfo));

  if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0)) {
    PERROR("IAV_IOC_GET_SINK_NUM");
    WARN("HDMI will use default mode 480P!");
    mode = AMBA_VIDEO_MODE_D1_NTSC;
  } else {
    for (int32_t i = 0; i < num; ++ i) {
      sinkInfo.id = i;
      if (AM_UNLIKELY(ioctl(mIavFd, IAV_IOC_VOUT_GET_SINK_INFO,
                            &sinkInfo) < 0)) {
        PERROR("IAV_IOC_VOUT_GET_SINK_INFO");
        WARN("HDMI will use default mode 480P!");
        mode = AMBA_VIDEO_MODE_D1_NTSC;
        break;
      } else if ((sinkInfo.sink_type == AMBA_VOUT_SINK_TYPE_HDMI) &&
          (sinkInfo.source_id == 1)) {
        if (sinkInfo.hdmi_plug) {
          mode = sinkInfo.hdmi_native_mode;
          DEBUG("Found HDMI's native mode %4dx%4d!",
                video_mode_width(mode), video_mode_height(mode));
        } else {
          WARN("HDMI device has not been initialized! "
              "Cannot determine it's native resolution, use 480p instead!");
          mode = AMBA_VIDEO_MODE_D1_NTSC;
        }
        break;
      }
    }
    if (mode == AMBA_VIDEO_MODE_AUTO) {
      WARN("Cannot find HDMI's native mode, use 480P instead!");
      mode = AMBA_VIDEO_MODE_D1_NTSC;
    }
  }
  DEBUG("HDMI's native mode is set to %4dx%4d!",
        video_mode_width(mode), video_mode_height(mode));

  return mode;
}
