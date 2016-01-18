/*******************************************************************************
 * am_config_encoder.cpp
 *
 * Histroy:
 *  2012-3-19 2012 - [ypchang] created file
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
#include "am_config_vdev_encoder.h"

#ifdef CONFIG_ARCH_S2

EncoderParameters* AmConfigEncoder::get_encoder_config(int mIav)
{
  EncoderParameters* ret = NULL;

  if (AM_LIKELY(mIav >= 0)) {
    if (init()) {
      if (NULL == mEncoderParams) {
        mEncoderParams = new EncoderParameters();
      }
      if (mEncoderParams) {
        do {
          int assignMethod = get_int("ENCODER:BufferAssignMethod", 0);

          if ((assignMethod < AM_BUFFER_TO_RESOLUTION) ||
              (assignMethod > AM_BUFFER_TO_STREAM_YUV)) {
            WARN("Invalid buffer assign method, reset to 0!");
            assignMethod = AM_BUFFER_TO_RESOLUTION;
          }
          mEncoderParams->yuv_buffer_size.width =
              get_int("ENCODER:YUVDataWidth", 0);
          mEncoderParams->yuv_buffer_size.height =
              get_int("ENCODER:YUVDataHeight", 0);
          mEncoderParams->buffer_assign_method =
              (BufferAssignMethod)assignMethod;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX,
                                &(mEncoderParams->system_setup_info)) < 0)) {
            PERROR("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
            break;
          }
          get_sys_setup_config(mEncoderParams->system_setup_info);

          get_sys_limit_config(mEncoderParams->system_resource_info, mIav);
          mEncoderParams->max_source_buffer[0].width =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width;
          mEncoderParams->max_source_buffer[0].height =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height;
          mEncoderParams->max_source_buffer[1].width =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width;
          mEncoderParams->max_source_buffer[1].height =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height;
          mEncoderParams->max_source_buffer[2].width =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width;
          mEncoderParams->max_source_buffer[2].height =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height;
          mEncoderParams->max_source_buffer[3].width =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width;
          mEncoderParams->max_source_buffer[3].height =
              mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX,
                                &(mEncoderParams->buffer_type_info)) < 0)) {
            PERROR("IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX");
            break;
          }
          mEncoderParams->buffer_type_info.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
          mEncoderParams->buffer_type_info.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX,
                                &(mEncoderParams->buffer_format_info)) < 0)) {
            PERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
            break;
          }
          get_buffer_format_config(mEncoderParams->buffer_format_info);

          if (mEncoderParams->config_changed) {
            set_encoder_config(mEncoderParams);
          }
          ret = mEncoderParams;
        } while (0);
      }
    } else {
      WARN("Failed opening %s, use system default value for encoder parameter!",
           mConfigFile);
    }
  }

  return ret;
}

#else
EncoderParameters* AmConfigEncoder::get_encoder_config(int mIav)
{
  EncoderParameters* ret = NULL;

  if (AM_LIKELY(mIav >= 0)) {
    if (init()) {
      if (NULL == mEncoderParams) {
        mEncoderParams = new EncoderParameters();
      }
      if (mEncoderParams) {
        do {
          int assignMethod = get_int("ENCODER:BufferAssignMethod", 0);

          if ((assignMethod < AM_BUFFER_TO_RESOLUTION) ||
              (assignMethod > AM_BUFFER_TO_STREAM_YUV)) {
            WARN("Invalid buffer assign method, reset to 0!");
            assignMethod = AM_BUFFER_TO_RESOLUTION;
          }
          mEncoderParams->yuv_buffer_size.width =
              get_int("ENCODER:YUVDataWidth", 0);
          mEncoderParams->yuv_buffer_size.height =
              get_int("ENCODER:YUVDataHeight", 0);
          mEncoderParams->buffer_assign_method =
              (BufferAssignMethod)assignMethod;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SYSTEM_SETUP_INFO_EX,
                                &(mEncoderParams->system_setup_info)) < 0)) {
            PERROR("IAV_IOC_GET_SYSTEM_SETUP_INFO_EX");
            break;
          }
          get_sys_setup_config(mEncoderParams->system_setup_info);

          get_sys_limit_config(mEncoderParams->system_resource_info, mIav);
          mEncoderParams->max_source_buffer[0].width =
              mEncoderParams->system_resource_info.main_source_buffer_max_width;
          mEncoderParams->max_source_buffer[0].height =
              mEncoderParams->system_resource_info.main_source_buffer_max_height;
          mEncoderParams->max_source_buffer[1].width =
              mEncoderParams->system_resource_info.second_source_buffer_max_width;
          mEncoderParams->max_source_buffer[1].height =
              mEncoderParams->system_resource_info.second_source_buffer_max_height;
          mEncoderParams->max_source_buffer[2].width =
              mEncoderParams->system_resource_info.third_source_buffer_max_width;
          mEncoderParams->max_source_buffer[2].height =
              mEncoderParams->system_resource_info.third_source_buffer_max_height;
          mEncoderParams->max_source_buffer[3].width =
              mEncoderParams->system_resource_info.fourth_source_buffer_max_width;
          mEncoderParams->max_source_buffer[3].height =
              mEncoderParams->system_resource_info.fourth_source_buffer_max_height;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX,
                                &(mEncoderParams->buffer_type_info)) < 0)) {
            PERROR("IAV_IOC_GET_SOURCE_BUFFER_TYPE_ALL_EX");
            break;
          }
          mEncoderParams->buffer_type_info.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
          mEncoderParams->buffer_type_info.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;

          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX,
                                &(mEncoderParams->buffer_format_info)) < 0)) {
            PERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX");
            break;
          }
          get_buffer_format_config(mEncoderParams->buffer_format_info);

          if (mEncoderParams->config_changed) {
            set_encoder_config(mEncoderParams);
          }
          ret = mEncoderParams;
        } while (0);
      }
    } else {
      WARN("Failed opening %s, use system default value for encoder parameter!",
           mConfigFile);
    }
  }

  return ret;
}
#endif
void AmConfigEncoder::set_encoder_config(EncoderParameters *config)
{
  if (AM_LIKELY(config)) {
    if (init()) {
      set_value("ENCODER:YUVDataWidth", config->yuv_buffer_size.width);
      set_value("ENCODER:YUVDataHeight", config->yuv_buffer_size.height);
      set_value("ENCODER:BufferAssignMethod", config->buffer_assign_method);
      set_sys_setup_config(config->system_setup_info);
      set_sys_limit_config(config->system_resource_info);
      set_buffer_format_config(config->buffer_format_info);
      config->config_changed = 0;
      save_config();
    } else {
      WARN("Failed opening %s, config file NOT saved!", mConfigFile);
    }
  }
}

void AmConfigEncoder::get_sys_setup_config(
    iav_system_setup_info_ex_t &config)
{
  int value = get_int("ENCODER:Mixer", 0);
  switch(value) {
    case 1: {
      config.voutA_osd_blend_enable = 1;
      config.voutB_osd_blend_enable = 0;
    }break;
    case 2: {
      config.voutA_osd_blend_enable = 0;
      config.voutB_osd_blend_enable = 1;
    }break;
    case 0: {
      config.voutA_osd_blend_enable = 0;
      config.voutB_osd_blend_enable = 0;
    }break;
    default: {
      WARN("Invalid Mixer type: %d, Disable OSD blend!", value);
      config.voutA_osd_blend_enable = 0;
      config.voutB_osd_blend_enable = 0;
      mEncoderParams->config_changed = 1;
    }break;
  }
  value = get_int("ENCODER:CodedBitsInterrupt", -1);
  if (value >= 0) {
    config.coded_bits_interrupt_enable = (uint16_t)value;
  }
  value = get_int("ENCODER:PipSize", -1);
  if (value >= 0) {
    config.pip_size_enable = (uint8_t)value;
  }
  value = get_int("ENCODER:LowDelayCap", -1);
  if (value >= 0) {
    config.low_delay_cap_enable = (uint8_t)value;
  }
  value = get_int("ENCODER:AudioClkFreq", -1);
  if (value >= 0) {
    config.audio_clk_freq = (uint32_t)value;
  }
}

void AmConfigEncoder::set_sys_setup_config(
    iav_system_setup_info_ex_t &config)
{
  if ((0 == config.voutA_osd_blend_enable) &&
      (0 == config.voutB_osd_blend_enable)) {
    set_value("ENCODER:Mixer", 0);
  } else if ((1 == config.voutA_osd_blend_enable) &&
             (0 == config.voutB_osd_blend_enable)) {
    set_value("ENCODER:Mixer", 1);
  } else if ((0 == config.voutA_osd_blend_enable) &&
             (1 == config.voutB_osd_blend_enable)) {
    set_value("ENCODER:Mixer", 2);
  }

  set_value("ENCODER:CodedBitsInterrupt", config.coded_bits_interrupt_enable);
  set_value("ENCODER:PipSize", config.pip_size_enable);
  set_value("ENCODER:LowDelayCap", config.low_delay_cap_enable);
  set_value("ENCODER:AudioClkFreq", config.audio_clk_freq);
}

#ifdef CONFIG_ARCH_S2

void AmConfigEncoder::get_sys_limit_config(
    iav_system_resource_setup_ex_t &config, int iav)
{
  int value = -1;

  value = get_int("ENCODER:EncodeMode", -1);
  if (value >= 0 && value < IAV_ENCODE_MODE_TOTAL_NUM) {
    config.encode_mode = (uint8_t)value;
  } else {
    config.encode_mode = IAV_ENCODE_CURRENT_MODE;
  }

  if (AM_UNLIKELY(ioctl(iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX,
                        &config) < 0)) {
    PERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
  }

  value = get_int("ENCODER:SharpenB", -1);
  config.sharpen_b_enable = (value < 0 ? config.sharpen_b_enable : value);

  value = get_int("ENCODER:CavlcMaxBitrate", -1);

  if ((get_int("ENCODER:BufferMainMaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferMainMaxHeight", -1) >= 0)) {
    config.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width =
        (uint16_t)get_int("ENCODER:BufferMainMaxWidth", 0);
    config.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height =
        (uint16_t)get_int("ENCODER:BufferMainMaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub1MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub1MaxHeight", -1) >= 0)) {
    config.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width =
        (uint16_t)get_int("ENCODER:BufferSub1MaxWidth", 0);
    config.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height =
        (uint16_t)get_int("ENCODER:BufferSub1MaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub2MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub2MaxHeight", -1) >= 0)) {
    config.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width =
        (uint16_t)get_int("ENCODER:BufferSub2MaxWidth", 0);
    config.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height =
        (uint16_t)get_int("ENCODER:BufferSub2MaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub3MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub3MaxHeight", -1) >= 0)) {
    config.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width =
        (uint16_t)get_int("ENCODER:BufferSub3MaxWidth", 0);
    config.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height =
        (uint16_t)get_int("ENCODER:BufferSub3MaxHeight", 0);
  }

  for (int i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    char m[32] = {0};
    char n[32] = {0};
    sprintf(m, "ENCODER:Stream%dMaxM", i);
    sprintf(n, "ENCODER:Stream%dMaxN", i);

    value = get_int(m, -1);
    if (value >= 0) {
      config.stream_max_GOP_M[i] = (uint8_t)value;
    }

    value = get_int(n, -1);
    if (value >= 0) {
      config.stream_max_GOP_N[i] = (uint8_t)value;
    }
  }
}

void AmConfigEncoder::set_sys_limit_config(
    iav_system_resource_setup_ex_t &config)
{
  set_value("ENCODER:EncodeMode", config.encode_mode);
  set_value("ENCODER:SharpenB", config.sharpen_b_enable);

  set_value("ENCODER:BufferMainMaxWidth",
            config.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width);
  set_value("ENCODER:BufferMainMaxHeight",
            config.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height);
  set_value("ENCODER:BufferSub1MaxWidth",
            config.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width);
  set_value("ENCODER:BufferSub1MaxHeight",
            config.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height);
  set_value("ENCODER:BufferSub2MaxWidth",
            config.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width);
  set_value("ENCODER:BufferSub2MaxHeight",
            config.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height);
  set_value("ENCODER:BufferSub3MaxWidth",
            config.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width);
  set_value("ENCODER:BufferSub3MaxHeight",
            config.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height);

  for (int i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    char m[32] = {0};
    char n[32] = {0};

    sprintf(m, "ENCODER:Stream%dMaxM", i);
    sprintf(n, "ENCODER:Stream%dMaxN", i);

    set_value(m, config.stream_max_GOP_M[i]);
    set_value(n, config.stream_max_GOP_N[i]);
  }
}

#else

void AmConfigEncoder::get_sys_limit_config(
    iav_system_resource_setup_ex_t &config, int iav)
{
  int value = -1;

  if (AM_UNLIKELY(ioctl(iav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX,
                        &config) < 0)) {
    PERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
  }

  value = get_int("ENCODER:AdvOverSampling", 0);
  config.adv_oversampling = (value ? 1 : 0);

  value = get_int("ENCODER:MCTF", -1);
  if (value >= 0) {
    config.MCTF_possible = (uint8_t)value;
  }

  value = get_int("ENCODER:CavlcMaxBitrate", -1);
  if (value >= 0) {
    config.cavlc_max_bitrate = (uint32_t)value;
  }

  if ((get_int("ENCODER:BufferMainMaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferMainMaxHeight", -1) >= 0)) {
    config.main_source_buffer_max_width =
        (uint16_t)get_int("ENCODER:BufferMainMaxWidth", 0);
    config.main_source_buffer_max_height =
        (uint16_t)get_int("ENCODER:BufferMainMaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub1MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub1MaxHeight", -1) >= 0)) {
    config.second_source_buffer_max_width =
        (uint16_t)get_int("ENCODER:BufferSub1MaxWidth", 0);
    config.second_source_buffer_max_height =
        (uint16_t)get_int("ENCODER:BufferSub1MaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub2MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub2MaxHeight", -1) >= 0)) {
    config.third_source_buffer_max_width =
        (uint16_t)get_int("ENCODER:BufferSub2MaxWidth", 0);
    config.third_source_buffer_max_height =
        (uint16_t)get_int("ENCODER:BufferSub2MaxHeight", 0);
  }

  if ((get_int("ENCODER:BufferSub3MaxWidth", -1) >= 0) &&
      (get_int("ENCODER:BufferSub3MaxHeight", -1) >= 0)) {
    config.fourth_source_buffer_max_width =
        (uint16_t)get_int("ENCODER:BufferSub3MaxWidth", 0);
    config.fourth_source_buffer_max_height =
        (uint16_t)get_int("ENCODER:BufferSub3MaxHeight", 0);
  }

  for (int i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    char m[32] = {0};
    char n[32] = {0};
    sprintf(m, "ENCODER:Stream%dMaxM", i);
    sprintf(n, "ENCODER:Stream%dMaxN", i);

    value = get_int(m, -1);
    if (value >= 0) {
      config.stream_max_GOP_M[i] = (uint8_t)value;
    }

    value = get_int(n, -1);
    if (value >= 0) {
      config.stream_max_GOP_N[i] = (uint8_t)value;
    }
  }
}

void AmConfigEncoder::set_sys_limit_config(
    iav_system_resource_setup_ex_t &config)
{
  set_value("ENCODER:AdvOverSampling", config.adv_oversampling);
  set_value("ENCODER:MCTF", config.MCTF_possible);
  set_value("ENCODER:CavlcMaxBitrate", config.cavlc_max_bitrate);

  set_value("ENCODER:BufferMainMaxWidth",
            config.main_source_buffer_max_width);
  set_value("ENCODER:BufferMainMaxHeight",
            config.main_source_buffer_max_height);
  set_value("ENCODER:BufferSub1MaxWidth",
            config.second_source_buffer_max_width);
  set_value("ENCODER:BufferSub1MaxHeight",
            config.second_source_buffer_max_height);
  set_value("ENCODER:BufferSub2MaxWidth",
            config.third_source_buffer_max_width);
  set_value("ENCODER:BufferSub2MaxHeight",
            config.third_source_buffer_max_height);
  set_value("ENCODER:BufferSub3MaxWidth",
            config.fourth_source_buffer_max_width);
  set_value("ENCODER:BufferSub3MaxHeight",
            config.fourth_source_buffer_max_height);

  for (int i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
    char m[32] = {0};
    char n[32] = {0};

    sprintf(m, "ENCODER:Stream%dMaxM", i);
    sprintf(n, "ENCODER:Stream%dMaxN", i);

    set_value(m, config.stream_max_GOP_M[i]);
    set_value(n, config.stream_max_GOP_N[i]);
  }
}

#endif
void AmConfigEncoder::get_buffer_format_config(
    iav_source_buffer_format_all_ex_t &config)
{
  int wValue = -1;
  wValue = get_int("ENCODER:BufferMainIsInterlaced", -1);
  if (wValue >= 0) {
    config.main_deintlc_for_intlc_vin = (uint16_t)wValue;
  }

  wValue = get_int("ENCODER:BufferSub1IsInterlaced", -1);
  if (wValue >= 0) {
    config.second_deintlc_for_intlc_vin = (uint16_t)wValue;
  }

  wValue = get_int("ENCODER:BufferSub2IsInterlaced", -1);
  if (wValue >= 0) {
    config.third_deintlc_for_intlc_vin = (uint16_t)wValue;
  }

  wValue = get_int("ENCODER:BufferSub3IsInterlaced", -1);
  if (wValue >= 0) {
    config.fourth_deintlc_for_intlc_vin = (uint16_t)wValue;
  }
}

void AmConfigEncoder::set_buffer_format_config(
    iav_source_buffer_format_all_ex_t &config)
{
  set_value("ENCODER:BufferMainIsInterlaced",config.main_deintlc_for_intlc_vin);
  set_value("ENCODER:BufferSub1IsInterlaced",
            config.second_deintlc_for_intlc_vin);
  set_value("ENCODER:BufferSub2IsInterlaced",
            config.third_deintlc_for_intlc_vin);
  set_value("ENCODER:BufferSub3IsInterlaced",
            config.fourth_deintlc_for_intlc_vin);
}

#ifdef CONFIG_ARCH_S2

void AmConfigEncoder::get_warp_config(WarpParameters &config)
{
  int  value;

  value = get_int("Encoder:UnwarpWidth", 0);
  if (value <= 0) {
   config.unwarp.width = 1920;
    WARN("Invalid UnwarpWidth value: %d < 0, reset to %d.", value,
         config.unwarp.width);
    mEncoderParams->config_changed = 1;
  } else {
   config.unwarp.width = value;
  }

  value = get_int("Encoder:UnwarpHeight", 0);
  if (value <= 0) {
   config.unwarp.height = 1080;
    WARN("Invalid UnwarpHeight value: %d < 0, reset to %d.", value,
         config.unwarp.height);
    mEncoderParams->config_changed = 1;
  } else {
   config.unwarp.height = value;
  }

  value = get_int("Encoder:WarpWidth", -1);
  if (value <= 0) {
   config.warp.width = config.unwarp.width;
    WARN("Invalid WarpWidth value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.warp.width);
    mEncoderParams->config_changed = 1;
  } else {
   config.warp.width = value;
  }

  value = get_int("Encoder:WarpHeight", -1);
  if (value <= 0) {
   config.warp.height = config.unwarp.height;
    WARN("Invalid WarpHeight value: %d < 0, reset to %d (UnwarpHeight).",
         value, config.warp.height);
    mEncoderParams->config_changed = 1;
  } else {
   config.warp.height = value;
  }

  value = get_int("Encoder:UnwarpWindowWidth", -1);
  if (value <= 0) {
   config.unwarp_window.width = config.unwarp.width;
    WARN("Invalid UnwarpWindowWidth value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.unwarp_window.width);
    mEncoderParams->config_changed = 1;
  } else {
    config.unwarp_window.width = value;
  }

  value = get_int("Encoder:UnwarpWindowHeight", -1);
  if (value <= 0) {
   config.unwarp_window.height = config.unwarp.height;
    WARN("Invalid UnwarpWindowHeight value: %d < 0, reset to %d (UnwarpWidth).",
         value, config.unwarp_window.height);
    mEncoderParams->config_changed = 1;
  } else {
    config.unwarp_window.height = value;
  }

  value = get_int("Encoder:UnwarpWindowOffsetX", -1);
  if (value < 0) {
   config.unwarp_window.x = 0;
    WARN("Invalid UnwarpWindowOffsetX value: %d < 0, reset to %d.", value,
         config.unwarp_window.x);
    mEncoderParams->config_changed = 1;
  } else {
   config.unwarp_window.x = value;
  }

  value = get_int("Encoder:UnwarpWindowOffsetY", -1);
  if (value < 0) {
   config.unwarp_window.y = 0;
    WARN("Invalid UnwarpWindowOffsetY value: %d < 0, reset to %d.", value,
         config.unwarp_window.y);
    mEncoderParams->config_changed = 1;
  } else {
   config.unwarp_window.y = value;
  }
}

void AmConfigEncoder::set_warp_config(WarpParameters &config)
{
  set_value("Encoder:UnwarpWidth", config.unwarp.width);
  set_value("Encoder:UnwarpHeight", config.unwarp.height);
  set_value("Encoder:WarpWidth", config.warp.width);
  set_value("Encoder:WarpHeight", config.warp.height);

  set_value("Encoder:UnwarpWindowWidth", config.unwarp_window.width);
  set_value("Encoder:UnwarpWindowHeight", config.unwarp_window.height);
  set_value("Encoder:UnwarpWindowOffsetX", config.unwarp_window.x);
  set_value("Encoder:UnwarpWindowOffsetY", config.unwarp_window.y);
}

#endif
