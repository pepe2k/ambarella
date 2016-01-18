/*******************************************************************************
 * am_config_stream.cpp
 *
 * Histroy:
 *  2012-3-20 2012 - [ypchang] created file
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
#include "am_config_vdev_stream.h"

#ifdef CONFIG_ARCH_S2

StreamParameters* AmConfigStream::get_stream_config(int mIav,
                                                    uint32_t streamID)
{
  StreamParameters* ret = NULL;

  if (AM_LIKELY((mIav >= 0) && (streamID < AM_STREAM_ID_MAX))) {
    if (NULL == mStreamParamsList[streamID]) {
      mStreamParamsList[streamID] = new StreamParameters;
      DEBUG("Stream%u configurations created!", streamID);
    }
    if (mStreamParamsList[streamID]) {
      memset(mStreamParamsList[streamID], 0, sizeof(StreamParameters));
      if (init()) {
        do {
          mStreamParamsList[streamID]->bitrate_info.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_BITRATE_EX,
                                &(mStreamParamsList[streamID]->\
                                    bitrate_info)) < 0)) {
            ERROR("IAV_IOC_GET_BITRATE_EX: %s", strerror(errno));
            break;
          }
          get_bitrate_config(mStreamParamsList[streamID]->bitrate_info,
                             streamID);

          mStreamParamsList[streamID]->encode_params.encode_format.id =
              (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX,
                                &(mStreamParamsList[streamID]->\
                                    encode_params.encode_format)) < 0)) {
            ERROR("IAV_IOC_GET_ENCODE_FORMAT_EX: %s", strerror(errno));
            break;
          }
          get_encode_format_config(mStreamParamsList[streamID]->encode_params, streamID);

          mStreamParamsList[streamID]->h264_config.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_H264_CONFIG_EX,
                  &(mStreamParamsList[streamID]->h264_config)) < 0)) {
            ERROR("IAV_IOC_GET_H264_CONFIG_EX: %s", strerror(errno));
            break;
          }

          mStreamParamsList[streamID]->mjpeg_config.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_JPEG_CONFIG_EX,
                  &(mStreamParamsList[streamID]->mjpeg_config)) < 0)) {
            ERROR("IAV_IOC_GET_JPEG_CONFIG_EX: %s", strerror(errno));
            break;
          }

          get_h264_config(mStreamParamsList[streamID]->h264_config, \
            mStreamParamsList[streamID]->encode_params.encode_format, streamID);
          get_mjpeg_config(mStreamParamsList[streamID]->mjpeg_config, \
            mStreamParamsList[streamID]->encode_params.encode_format, streamID);

          if (mStreamParamsList[streamID]->config_changed) {
            set_stream_config(mStreamParamsList[streamID], streamID);
          }
          ret = mStreamParamsList[streamID];
        } while (0);
      } else {
        WARN("Failed opening %s, use system default value for stream info!",
             mConfigFile);
      }
    }
  } else {
    if (mIav < 0) {
      ERROR("Invalid IAV file descriptor!");
    }
    if (streamID >= AM_STREAM_ID_MAX) {
      ERROR("Invalid stream ID: %d!", streamID);
    }
  }

  return ret;
}

void AmConfigStream::set_stream_config(StreamParameters *config,
                                       uint32_t streamID)
{
  if (init()) {
    if (AM_LIKELY(config)) {
      set_bitrate_config(config->bitrate_info, streamID);
      set_encode_format_config(config->encode_params, streamID);
      set_h264_config(config->h264_config, \
        mStreamParamsList[streamID]->encode_params.encode_format, streamID);
      set_mjpeg_config(config->mjpeg_config, \
        mStreamParamsList[streamID]->encode_params.encode_format, streamID);
      config->config_changed = 0;
      save_config();
    }
  } else {
    WARN("Failed opening %s, stream configuration NOT saved!", mConfigFile);
  }
}


#else
StreamParameters* AmConfigStream::get_stream_config(int mIav,
                                                    uint32_t streamID)
{
  StreamParameters* ret = NULL;

  if (AM_LIKELY((mIav >= 0) && (streamID < AM_STREAM_ID_MAX))) {
    if (NULL == mStreamParamsList[streamID]) {
      mStreamParamsList[streamID] = new StreamParameters;
      DEBUG("Stream%u configurations created!", streamID);
    }
    if (mStreamParamsList[streamID]) {
      memset(mStreamParamsList[streamID], 0, sizeof(StreamParameters));
      if (init()) {
        do {
          mStreamParamsList[streamID]->bitrate_info.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_BITRATE_EX,
                                &(mStreamParamsList[streamID]->\
                                    bitrate_info)) < 0)) {
            ERROR("IAV_IOC_GET_BITRATE_EX: %s", strerror(errno));
            break;
          }
          get_bitrate_config(mStreamParamsList[streamID]->bitrate_info,
                             streamID);

          mStreamParamsList[streamID]->encode_params.encode_format.id =
              (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX,
                                &(mStreamParamsList[streamID]->\
                                    encode_params.encode_format)) < 0)) {
            ERROR("IAV_IOC_GET_ENCODE_FORMAT_EX: %s", strerror(errno));
            break;
          }
          get_encode_format_config(mStreamParamsList[streamID]->encode_params,
                                   streamID);

          mStreamParamsList[streamID]->h264_config.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_H264_CONFIG_EX,
                  &(mStreamParamsList[streamID]->h264_config)) < 0)) {
            ERROR("IAV_IOC_GET_H264_CONFIG_EX: %s", strerror(errno));
            break;
          }

          mStreamParamsList[streamID]->mjpeg_config.id = (1 << streamID);
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_JPEG_CONFIG_EX,
                  &(mStreamParamsList[streamID]->mjpeg_config)) < 0)) {
            ERROR("IAV_IOC_GET_JPEG_CONFIG_EX: %s", strerror(errno));
            break;
          }

          get_h264_config(mStreamParamsList[streamID]->h264_config, streamID);
          get_mjpeg_config(mStreamParamsList[streamID]->mjpeg_config, streamID);

          if (mStreamParamsList[streamID]->config_changed) {
            set_stream_config(mStreamParamsList[streamID], streamID);
          }
          ret = mStreamParamsList[streamID];
        } while (0);
      } else {
        WARN("Failed opening %s, use system default value for stream info!",
             mConfigFile);
      }
    }
  } else {
    if (mIav < 0) {
      ERROR("Invalid IAV file descriptor!");
    }
    if (streamID >= AM_STREAM_ID_MAX) {
      ERROR("Invalid stream ID: %d!", streamID);
    }
  }

  return ret;
}

void AmConfigStream::set_stream_config(StreamParameters *config,
                                       uint32_t streamID)
{
  if (init()) {
    if (AM_LIKELY(config)) {
      set_bitrate_config(config->bitrate_info, streamID);
      set_encode_format_config(config->encode_params, streamID);
      set_h264_config(config->h264_config, streamID);
      set_mjpeg_config(config->mjpeg_config, streamID);
      config->config_changed = 0;
      save_config();
    }
  } else {
    WARN("Failed opening %s, stream configuration NOT saved!", mConfigFile);
  }
}

#endif

void AmConfigStream::get_bitrate_config(iav_bitrate_info_ex_t &config,
                                        uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:BitrateCtrlMode", streamID);
  section[ret] = '\0';
  value = get_int(section, -1);
  if ((value >= 0) && (value <= 5)) {
    config.rate_control_mode = (iav_rate_control_mode)value;
  } else if (value > 5) {
    WARN("Invalid value for Bitrate control mode: %d, use system default!",
         value);
    mStreamParamsList[streamID]->config_changed = 1;
  }

  ret = sprintf(section, "Stream%u:CbrAvgBitrate", streamID);
  section[ret] = '\0';
  value = get_int(section, 10000000);
  config.cbr_avg_bitrate = (uint32_t)value;

  ret = sprintf(section, "Stream%u:VbrMin", streamID);
  section[ret] = '\0';
  value = get_int(section, 4000000);
  config.vbr_min_bitrate = (uint32_t)value;

  ret = sprintf(section, "Stream%u:VbrMax", streamID);
  section[ret] = '\0';
  value = get_int(section, 8000000);
  config.vbr_max_bitrate = (uint32_t)value;
}

void AmConfigStream::set_bitrate_config(iav_bitrate_info_ex_t &config,
                                        uint32_t streamID)
{
  char section[32] = {0};
  config.id = 1 << streamID;

  sprintf(section, "Stream%u:BitrateCtrlMode", streamID);
  set_value(section, (uint32_t)config.rate_control_mode);

  sprintf(section, "Stream%u:CbrAvgBitrate", streamID);
  set_value(section, (uint32_t)config.cbr_avg_bitrate);

  sprintf(section, "Stream%u:VbrMin", streamID);
  set_value(section, (uint32_t)config.vbr_min_bitrate);

  sprintf(section, "Stream%u:VbrMax", streamID);
  set_value(section, (uint32_t)config.vbr_max_bitrate);
}

void AmConfigStream::get_encode_format_config(StreamEncodeFormat &config,
                                              uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.encode_format.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:Type", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value < 0) || (value > 2)) {
    WARN("Invalid value for stream type: %d, reset to 1(H.264)", value);
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.encode_format.encode_type = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Source", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value < 0) || (value > 3)) {
    WARN("Invalid value for stream source: %d, "
        "reset to 0(Main source buffer", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.encode_format.source = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Width", streamID);
  section[ret] = '\0';
  value = get_int(section, 1920);
#ifdef CONFIG_ARCH_A5S
  if ((value <= 0) || (value > 2592)) {
    WARN("Invalid stream width: %d, reset to 1920!", value);
    value = 1920;
    mStreamParamsList[streamID]->config_changed = 1;
  }
#endif
#ifdef CONFIG_ARCH_S2
  if ((value <= 0) || (value > 5120)) {
    WARN("Invalid stream width: %d, reset to 1920!", value);
    value = 1920;
    mStreamParamsList[streamID]->config_changed = 1;
  }
#endif
  config.encode_format.encode_width = (uint16_t)value;

  ret = sprintf(section, "Stream%u:Height", streamID);
  section[ret] = '\0';
  value = get_int(section, 1080);
#ifdef CONFIG_ARCH_A5S
  if ((value <= 0) || (value > 1944)) {
    WARN("Invalid stream width: %d, reset to 1080!", value);
    value = 1080;
    mStreamParamsList[streamID]->config_changed = 1;
  }
#endif
#ifdef CONFIG_ARCH_S2
  if ((value <= 0) || (value > 4096)) {
    WARN("Invalid stream width: %d, reset to 1080!", value);
    value = 1080;
    mStreamParamsList[streamID]->config_changed = 1;
  }
#endif
  config.encode_format.encode_height = (uint16_t)value;

  ret = sprintf(section, "Stream%u:OffsetX", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 1920)) {
    WARN("Invalid offset x: %d, reset to 0!", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.encode_format.encode_x = (uint16_t)value;

  ret = sprintf(section, "Stream%u:OffsetY", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 1080)) {
    WARN("Invalid offset y: %d, reset to 0!", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.encode_format.encode_y = (uint16_t)value;

  config.encode_framerate.id = config.encode_format.id;
  ret = sprintf(section, "Stream%u:RatioNumerator", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value <= 0) || (value > 255)) {
    WARN("Invalid ratio numerator: %d, reset to 1!");
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.encode_framerate.ratio_numerator = (uint8_t)value;

  ret = sprintf(section, "Stream%u:RatioDenominator", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value <= 0) || (value > 255)) {
    WARN("Invalid ratio denominator: %d, reset to 1!");
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  } else if (value < config.encode_framerate.ratio_numerator) {
    WARN("Ratio_numerator/ Ratio_denominator == %hhu/%hhu is greator than 1, "
        "reset frame factor to 1!", config.encode_framerate.ratio_numerator,
        value);
    config.encode_framerate.ratio_numerator = 1;
    value = 1;
  }
  config.encode_framerate.ratio_denominator = (uint8_t)value;
}
void AmConfigStream::set_encode_format_config(StreamEncodeFormat &config,
                                              uint32_t streamID)
{
  char section[32] = {0};
  config.encode_format.id = 1 << streamID;
  config.encode_framerate.id = 1 << streamID;
  sprintf(section, "Stream%u:Type", streamID);
  set_value(section, config.encode_format.encode_type);

  sprintf(section, "Stream%u:Source", streamID);
  set_value(section, config.encode_format.source);

  sprintf(section, "Stream%u:Width", streamID);
  set_value(section, config.encode_format.encode_width);

  sprintf(section, "Stream%u:Height", streamID);
  set_value(section, config.encode_format.encode_height);

  sprintf(section, "Stream%u:OffsetX", streamID);
  set_value(section, config.encode_format.encode_x);

  sprintf(section, "Stream%u:OffsetY", streamID);
  set_value(section, config.encode_format.encode_y);

  sprintf(section, "Stream%u:RatioNumerator", streamID);
  set_value(section, config.encode_framerate.ratio_numerator);

  sprintf(section, "Stream%u:RatioDenominator", streamID);
  set_value(section, config.encode_framerate.ratio_denominator);
}

#ifdef CONFIG_ARCH_S2

void AmConfigStream::get_warp_window_config(Rect &config,
                                            uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;

  ret = sprintf(section, "Stream%u:WarpWindowWidth", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if (value < 0) {
    value = 0;
  }
  config.width = (value < 0 ? 0 : (uint8_t)value);

  ret = sprintf(section, "Stream%u:WarpWindowHeight", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if (value < 0) {
    value = 0;
  }
  config.height = (value < 0 ? 0 : (uint8_t)value);

  ret = sprintf(section, "Stream%u:WarpWindowOffsetX", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  config.x = value;

  ret = sprintf(section, "Stream%u:WarpWindowOffsetY", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  config.y = value;
}

void AmConfigStream::set_warp_window_config(Rect &config,
                                            uint32_t streamID)
{
  char section[32] = {0};
  int ret = -1;

  ret = sprintf(section, "Stream%u:WarpWindowWidth", streamID);
  section[ret] = '\0';
  set_value(section, config.width);

  ret = sprintf(section, "Stream%u:WarpWindowHeight", streamID);
  section[ret] = '\0';
  set_value(section, config.height);

  ret = sprintf(section, "Stream%u:WarpWindowOffsetX", streamID);
  section[ret] = '\0';
  set_value(section, config.x);

  ret = sprintf(section, "Stream%u:WarpWindowOffsetY", streamID);
  section[ret] = '\0';
  set_value(section, config.y);
}

void AmConfigStream::get_h264_config(iav_h264_config_ex_t &config,
                                     iav_encode_format_ex_t &encode_format,
                                     uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:M", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value <= 0) || (value > 3)) {
    WARN("Invalid M value: %d, reset to 1!", value);
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  } else if (value > 1) {
    if (streamID > 1) {
      WARN("M value greater than 1 is not valid for stream %u, reset to 1!",
           streamID);
      value = 1;
      mStreamParamsList[streamID]->config_changed = 1;
    }
  }
  config.M = (uint16_t)value;

  ret = sprintf(section, "Stream%u:N", streamID);
  section[ret] = '\0';
  value = get_int(section, 30);
  if ((value <= 0) || (value > 255)) {
    WARN("Invalid N value: %d, reset to 30!", value);
    value = 30;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.N = (uint16_t)value;

  ret = sprintf(section, "Stream%u:GopModel", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 1)) {
    WARN("Invalid GOP model value: %d, reset to 0", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.gop_model = (uint8_t)value;

  ret = sprintf(section, "Stream%u:DeblockingEnable", streamID);
  section[ret] = '\0';
  value = get_int(section, 2);
  if ((value < 0) || (value > 2)) {
    WARN("Invalid  value for Deblocking enable: %d, reset to 2!", value);
    value = 2;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.deblocking_filter_enable = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Hflip", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  encode_format.hflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Vflip", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  encode_format.vflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Rotate", streamID);
  section[ret] = '\0';
  value = (get_int(section , 0) == 0) ? 0 : 1;
  encode_format.rotate_clockwise = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264ChromaFormat", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  config.chroma_format = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Profile", streamID);
  section[ret] = '\0';
  switch(get_int(section, 0)) {
    case 2: {
      config.entropy_codec = 0;
      config.high_profile  = 1;
    }break;
    case 1: {
      config.entropy_codec = 1;
      config.high_profile  = 0;
    }break;
    case 0: {
      config.entropy_codec = 0;
      config.high_profile  = 0;
    }break;
    default: {
      WARN("Invalid value for Profile, reset to Main profile");
      config.entropy_codec = 0;
      config.high_profile  = 0;
    }break;
  }

  ret = sprintf(section, "Stream%u:AuType", streamID);
  section[ret] = '\0';
  value = get_int(section , 2);
  if ((value < 0) || (value > 3)) {
    WARN("Invalid value for AuType: %d, reset to 2!", value);
    value = 2;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.au_type = (uint8_t)value;
}

void AmConfigStream::set_h264_config(iav_h264_config_ex_t &config,
                                     iav_encode_format_ex_t &encode_format,
                                     uint32_t streamID)
{
  char section[32] = {0};
  config.id = 1 << streamID;

  sprintf(section, "Stream%u:M", streamID);
  set_value(section, config.M);

  sprintf(section, "Stream%u:N", streamID);
  set_value(section, config.N);

  sprintf(section, "Stream%u:GopModel", streamID);
  set_value(section, config.gop_model);

  sprintf(section, "Stream%u:DeblockingEnable", streamID);
  set_value(section, config.deblocking_filter_enable);

  sprintf(section, "Stream%u:H264Hflip", streamID);
  set_value(section, encode_format.hflip);

  sprintf(section, "Stream%u:H264Vflip", streamID);
  set_value(section, encode_format.vflip);

  sprintf(section, "Stream%u:H264Rotate", streamID);
  set_value(section, encode_format.rotate_clockwise);

  sprintf(section, "Stream%u:H264ChromaFormat", streamID);
  set_value(section, config.chroma_format);

  sprintf(section, "Stream%u:Profile", streamID);
  if ((config.entropy_codec == 0) && config.high_profile) {
    set_value(section, 2); /* CABAC High Profile */
  } else if ((config.entropy_codec == 0) && (config.high_profile == 0)) {
    set_value(section, 0); /* CABAC Main Profile */
  } else {
    set_value(section, 1); /* CAVLC Baseline Profile */
  }
  sprintf(section, "Stream%u:AuType", streamID);
  set_value(section, config.au_type);
}

void AmConfigStream::get_mjpeg_config(iav_jpeg_config_ex_t &config,
                                      iav_encode_format_ex_t &encode_format,
                                      uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:MjpegChromaFormat", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 2)) {
    WARN("Invalid value for MjpegChromaFormat: %d, reset to 0!", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.chroma_format = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Quality", streamID);
  section[ret] = '\0';
  value = get_int(section, 50);
  if ((value < 0) || (value > 100)) {
    WARN("Invalid value for Quality: %d, reset to 50", value);
    value = 50;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.quality = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegHflip", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  encode_format.hflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegVflip", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  encode_format.vflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegRotate", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  encode_format.rotate_clockwise = (uint8_t)value;
}

void AmConfigStream::set_mjpeg_config(iav_jpeg_config_ex_t &config,
                                      iav_encode_format_ex_t &encode_format,
                                      uint32_t streamID)
{
  char section[32] = {0};
  config.id = 1 << streamID;

  sprintf(section, "Stream%u:MjpegChromaFormat", streamID);
  set_value(section, config.chroma_format);

  sprintf(section, "Stream%u:Quality", streamID);
  set_value(section, config.quality);

  sprintf(section, "Stream%u:MjpegHflip", streamID);
  set_value(section, encode_format.hflip);

  sprintf(section, "Stream%u:MjpegVflip", streamID);
  set_value(section, encode_format.vflip);

  sprintf(section, "Stream%u:MjpegRotate", streamID);
  set_value(section, encode_format.rotate_clockwise);
}

#else
void AmConfigStream::get_h264_config(iav_h264_config_ex_t &config,
                                     uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:M", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value <= 0) || (value > 3)) {
    WARN("Invalid M value: %d, reset to 1!", value);
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  } else if (value > 1) {
    if (streamID > 1) {
      WARN("M value greater than 1 is not valid for stream %u, reset to 1!",
           streamID);
      value = 1;
      mStreamParamsList[streamID]->config_changed = 1;
    }
  }
  config.M = (uint16_t)value;

  ret = sprintf(section, "Stream%u:N", streamID);
  section[ret] = '\0';
  value = get_int(section, 30);
  if ((value <= 0) || (value > 255)) {
    WARN("Invalid N value: %d, reset to 30!", value);
    value = 30;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.N = (uint16_t)value;

  ret = sprintf(section, "Stream%u:IdrInterval", streamID);
  section[ret] = '\0';
  value = get_int(section, 1);
  if ((value < 1) || (value > 255)) {
    WARN("Invalid IDR interval value: %d, reset to 1!", value);
    value = 1;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.idr_interval = (uint8_t)value;

  ret = sprintf(section, "Stream%u:GopModel", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 1)) {
    WARN("Invalid GOP model value: %d, reset to 0", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.gop_model = (uint8_t)value;

  ret = sprintf(section, "Stream%u:DeblockingEnable", streamID);
  section[ret] = '\0';
  value = get_int(section, 2);
  if ((value < 0) || (value > 2)) {
    WARN("Invalid  value for Deblocking enable: %d, reset to 2!", value);
    value = 2;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.deblocking_filter_enable = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Hflip", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  config.hflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Vflip", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  config.vflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264Rotate", streamID);
  section[ret] = '\0';
  value = (get_int(section , 0) == 0) ? 0 : 1;
  config.rotate_clockwise = (uint8_t)value;

  ret = sprintf(section, "Stream%u:H264ChromaFormat", streamID);
  section[ret] = '\0';
  value = (get_int(section, 0) == 0) ? 0 : 1;
  config.chroma_format = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Profile", streamID);
  section[ret] = '\0';
  switch(get_int(section, 0)) {
    case 2: {
      config.entropy_codec = 0;
      config.high_profile  = 1;
    }break;
    case 1: {
      config.entropy_codec = 1;
      config.high_profile  = 0;
    }break;
    case 0: {
      config.entropy_codec = 0;
      config.high_profile  = 0;
    }break;
    default: {
      WARN("Invalid value for Profile, reset to Main profile");
      config.entropy_codec = 0;
      config.high_profile  = 0;
    }break;
  }

  ret = sprintf(section, "Stream%u:AuType", streamID);
  section[ret] = '\0';
  value = get_int(section , 2);
  if ((value < 0) || (value > 3)) {
    WARN("Invalid value for AuType: %d, reset to 2!", value);
    value = 2;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.au_type = (uint8_t)value;
}

void AmConfigStream::set_h264_config(iav_h264_config_ex_t &config,
                                     uint32_t streamID)
{
  char section[32] = {0};
  config.id = 1 << streamID;

  sprintf(section, "Stream%u:M", streamID);
  set_value(section, config.M);

  sprintf(section, "Stream%u:N", streamID);
  set_value(section, config.N);

  sprintf(section, "Stream%u:GopModel", streamID);
  set_value(section, config.gop_model);

  sprintf(section, "Stream%u:DeblockingEnable", streamID);
  set_value(section, config.deblocking_filter_enable);

  sprintf(section, "Stream%u:H264Hflip", streamID);
  set_value(section, config.hflip);

  sprintf(section, "Stream%u:H264Vflip", streamID);
  set_value(section, config.vflip);

  sprintf(section, "Stream%u:H264Rotate", streamID);
  set_value(section, config.rotate_clockwise);

  sprintf(section, "Stream%u:H264ChromaFormat", streamID);
  set_value(section, config.chroma_format);

  sprintf(section, "Stream%u:Profile", streamID);
  if ((config.entropy_codec == 0) && config.high_profile) {
    set_value(section, 2); /* CABAC High Profile */
  } else if ((config.entropy_codec == 0) && (config.high_profile == 0)) {
    set_value(section, 0); /* CABAC Main Profile */
  } else {
    set_value(section, 1); /* CAVLC Baseline Profile */
  }
  sprintf(section, "Stream%u:AuType", streamID);
  set_value(section, config.au_type);
}

void AmConfigStream::get_mjpeg_config(iav_jpeg_config_ex_t &config,
                                      uint32_t streamID)
{
  int value   = -1;
  char section[32] = {0};
  int ret = 0;
  config.id = 1 << streamID;

  ret = sprintf(section, "Stream%u:MjpegChromaFormat", streamID);
  section[ret] = '\0';
  value = get_int(section, 0);
  if ((value < 0) || (value > 2)) {
    WARN("Invalid value for MjpegChromaFormat: %d, reset to 0!", value);
    value = 0;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.chroma_format = (uint8_t)value;

  ret = sprintf(section, "Stream%u:Quality", streamID);
  section[ret] = '\0';
  value = get_int(section, 50);
  if ((value < 0) || (value > 100)) {
    WARN("Invalid value for Quality: %d, reset to 50", value);
    value = 50;
    mStreamParamsList[streamID]->config_changed = 1;
  }
  config.quality = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegHflip", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  config.hflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegVflip", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  config.vflip = (uint8_t)value;

  ret = sprintf(section, "Stream%u:MjpegRotate", streamID);
  section[ret] = '\0';
  value = get_int(section, 0) == 0 ? 0 : 1;
  config.rotate_clockwise = (uint8_t)value;
}

void AmConfigStream::set_mjpeg_config(iav_jpeg_config_ex_t &config,
                                      uint32_t streamID)
{
  char section[32] = {0};
  config.id = 1 << streamID;

  sprintf(section, "Stream%u:MjpegChromaFormat", streamID);
  set_value(section, config.chroma_format);

  sprintf(section, "Stream%u:Quality", streamID);
  set_value(section, config.quality);

  sprintf(section, "Stream%u:MjpegHflip", streamID);
  set_value(section, config.hflip);

  sprintf(section, "Stream%u:MjpegVflip", streamID);
  set_value(section, config.vflip);

  sprintf(section, "Stream%u:MjpegRotate", streamID);
  set_value(section, config.rotate_clockwise);
}
#endif
