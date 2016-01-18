/*******************************************************************************
 * am_webcam.cpp
 *
 * History:
 *  Nov 28, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_vin.h"
#include "am_vout.h"
#include "am_vout_lcd.h"
#include "am_vout_hdmi.h"

static const char* vout_type_to_str[] = {
  "None", "Lcd", "Hdmi", "Cvbs"
};

AmEncodeDevice::AmEncodeDevice(VDeviceParameters *vDeviceConfig) :
  AmVideoDevice()
{
  if (!create_video_device(vDeviceConfig)) {
    ERROR("Failed to create the components of this video device!");
  }
  instance_pm_dptz = new AmPrivacyMaskDPTZ(mIav, mVinParamList);
}

AmEncodeDevice::~AmEncodeDevice()
{
  delete instance_pm_dptz;
  DEBUG("AmEncodeDevice deleted!");
}

bool AmEncodeDevice:: goto_idle()
{
  return false;
}

bool AmEncodeDevice::enter_preview()
{
  return false;
}
bool AmEncodeDevice:: enter_decode_mode()
{
  return false;
}

bool AmEncodeDevice::leave_decode_mode()
{
  return false;
}

bool AmEncodeDevice::enter_photo_mode()
{
  return false;
}

bool AmEncodeDevice::leave_photo_mode()
{
  return false;
}

bool AmEncodeDevice::change_iav_state(IavState target)
{
  return false;
}

bool AmEncodeDevice::encode(uint32_t* streams)
{
  bool ret = true;
  uint32_t stop_streams = (~(*streams)) & ((1 << mStreamNumber) - 1);
  uint32_t start_streams = (*streams) & ((1 << mStreamNumber) - 1);

  for (uint32_t id = 0; id < mStreamNumber; ++id) {
   if (is_stream_encoding(id)) {
      start_streams &= ~(1 << id);
    } else {
      stop_streams &= ~(1 << id);
    }
   if (mStreamParamList[id]->encode_params.encode_format.encode_type ==
       IAV_ENCODE_NONE) {
     start_streams &= ~(1 << id);
   }
  }

  DEBUG("streams %d, start %d, stop %d", *streams, start_streams, stop_streams);

  if (start_streams > 0) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_START_ENCODE_EX, start_streams) < 0)) {
      ERROR("IAV_IOC_START_ENCODE_EX: streams [%d] %s", start_streams,
          strerror(errno));
      ret = false;
    } else {
      INFO("Start encoding for Streams [%d] successfully!", start_streams);
    }
  }

  if (stop_streams > 0) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_STOP_ENCODE_EX, stop_streams) < 0)) {
      PERROR("IAV_IOC_STOP_ENCODE_EX");
      ret = false;
    } else {
      INFO("Stop encoding for Streams [%d] successfully!", stop_streams);
    }
  }
  *streams = start_streams;
  return ret;
}

bool AmEncodeDevice::update_stream_h264_qp(uint32_t streamId)
{
  mStreamParamList[streamId]->h264_qp.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_CHANGE_QP_LIMIT_EX,
      &(mStreamParamList[streamId]->h264_qp)) < 0)) {
    PERROR("IAV_IOC_CHANGE_QP_LIMIT_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_CHANGE_QP_LIMIT_EX", streamId);
  return true;
}



bool AmEncodeDevice::apply_encoder_parameters()
{
  return AmVideoDevice:: apply_encoder_parameters();
}

bool AmEncodeDevice::apply_stream_parameters()
{
  if (!check_system_performance())
    return false;

  for (uint32_t sId = 0; sId < mStreamNumber; ++sId) {
    if (mStreamParamList[sId]->encode_params.encode_format.encode_type
        != IAV_ENCODE_NONE) {
      if (!update_stream_format(sId)
          || !update_stream_framerate(sId)
          || !update_stream_overlay(sId))
        return false;

      if (mStreamParamList[sId]->encode_params.encode_format.encode_type
          == IAV_ENCODE_H264) {
        if (!check_stream_h264_config(sId)
            || !update_stream_h264_config(sId)
            || !update_stream_h264_bitrate(sId)\
            // IAV driver resets QP when changing bitrate.
            // QP must be applied after setting bitrate.
            || !eval_qp_mode(sId)
            || !update_stream_h264_qp(sId))
          return false;
      } else {
        if (!update_stream_mjpeg_config(sId))
          return false;
      }
    }
  }
  return true;
}

bool AmEncodeDevice::init_device(int voutInitMode, bool force)
{
  bool ret = true;
  do {
    for (uint32_t i = 0; i < mVoutNumber; ++ i) {
      if (mVoutTypeList[i] != AM_VOUT_TYPE_NONE) {
        ret = mVoutList[i]->start((AmVout::VoutInitMode)voutInitMode, force);
        if (!ret) {
          ERROR("Failed to start %s", vout_type_to_str[mVoutTypeList[i]]);
          break;
        }
      }
    }
    if (ret) {
      for (uint32_t i = 0; i < mVinNumber; ++ i) {
        if (mVinList[i]) {
          if (false == mVinList[i]->start(force)) {
            ERROR("Failed to start VIN%u", i);
            ret = false;
            break;
          }
        }
      }
    }
  } while(0);

  return ret;
}

#ifdef CONFIG_ARCH_S2
int AmEncodeDevice::disable_pm()
{
  return instance_pm_dptz->disable_pm();
}

int AmEncodeDevice::reset_pm()
{
  //reset DPTZ and restore Privacy Mask!
#if 0
  DPTZParam dptz_set;
  dptz_set.source_buffer = BUFFER_1;
  dptz_set.zoom_factor= 0;
  dptz_set.offset_x = 0;
  dptz_set.offset_y = 0;

  return instance_pm_dptz->pm_reset_all(1, &dptz_set);
#else
  RECT main_buffer;
  main_buffer.width = mEncoderParams->buffer_format_info.main_width;
  main_buffer.height = mEncoderParams->buffer_format_info.main_height;
  return instance_pm_dptz->pm_reset_all(&main_buffer);
#endif
}
#endif

bool AmEncodeDevice::change_stream_state(IavState target, uint32_t streams)
{
  bool ret = false;
  bool error = false;
  IavState currentState = get_iav_status();


  switch (target) {
    case AM_IAV_INIT:
    case AM_IAV_IDLE:
    case AM_IAV_PREVIEW:
      streams = 0;
      /* no breaks */
    case AM_IAV_ENCODING:
      break;
    default:
      ERROR("IPCam not support IAV state [%s],", iav_state_to_str(target));
      return ret;
  }

  do {
    DEBUG("Target IAV state is %s.", iav_state_to_str(target));
    DEBUG("Current IAV state is %s.", iav_state_to_str(currentState));
    switch (currentState) {
      case AM_IAV_INIT:
        /* If current state is INIT, should initialize device first */
        if (idle() && init_device(AmVout::AM_VOUT_INIT_DISABLE_VIDEO)) {
          switch (target) {
            case AM_IAV_IDLE:
            case AM_IAV_PREVIEW:
            case AM_IAV_ENCODING:
              /* If current state is INIT,
               * need to enter preview to boot up dsp
               */
              if (apply_encoder_parameters() && preview()) {
                /* Turn on video layer of VOUT0 and VOUT1 */
                for (uint32_t i = 0; i < mVoutNumber; ++i) {
                  if (mVoutTypeList[i] != AM_VOUT_TYPE_NONE) {
                    mVoutList[i]->video_layer_switch(true);
                  }
                }
              } else {
                ret = false;
                error = true;
              }
              break;
            default:
              ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
              ret = false;
              error = true;
              break;
          }
        } else {
          ret = false;
          error = true;
          ERROR("Initialize camera device failed!");
        }
        break;

      case AM_IAV_IDLE:
        if (init_device(AmVout::AM_VOUT_INIT_ENABLE_VIDEO)) {
          switch (target) {
            case AM_IAV_PREVIEW:
            case AM_IAV_ENCODING:
              if (!apply_encoder_parameters() || !preview()) {
                ret = false;
                error = true;
              } else if (target == AM_IAV_PREVIEW) {
                ret = true;
              }
              break;
            case AM_IAV_IDLE:
              ret = true;
              break;
            default:
              ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
              ret = false;
              error = true;
              break;
          }
        } else {
          ret = false;
          error = true;
          ERROR("Failed to initialize device!");
        }
        break;

      case AM_IAV_PREVIEW:
        switch (target) {
          case AM_IAV_PREVIEW:
            ret = true;
            break;
          case AM_IAV_IDLE:
            if (
#ifdef CONFIG_ARCH_S2
                disable_pm() ||
#endif
                !idle()) {
              ret = false;
              error = true;
            } else if (target == AM_IAV_IDLE) {
              ret = true;
            }
            break;
          case AM_IAV_ENCODING:
            if (streams == 0) {
              error = true;
              ret = false;
          } else if (!apply_stream_parameters() || !encode(&streams)
#ifdef CONFIG_ARCH_S2
              || reset_pm()
#endif
              ) {
           // } else if (!apply_stream_parameters() || !encode(&streams)) {
              error = true;
              ret = false;
            } else if (target == AM_IAV_ENCODING) {
              ret = true;
            }
            break;
          default:
            ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            ret = false;
            error = true;
            break;
        }
        break;

      case AM_IAV_ENCODING:
        switch (target) {
          case AM_IAV_ENCODING:
            ret = true;
            break;
          case AM_IAV_PREVIEW:
          case AM_IAV_IDLE:
            if (AM_UNLIKELY(!encode(&streams))) {
              ret = false;
              error = true;
            } else if (target == AM_IAV_PREVIEW) {
              ret = true;
            }
            break;
          default:
            ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            ret = false;
            error = true;
            break;
        }
        break;

      default:
        ERROR("Wrong IAV status!");
        ret = false;
        error = true;

        break;
    }
  }while (!error && (currentState = get_iav_status()) != target);
  DEBUG("Target IAV state is %s.", iav_state_to_str(target));
  DEBUG("Current IAV state is %s.", iav_state_to_str(currentState));

  return ret;
}

uint32_t AmEncodeDevice::linear_scale(uint32_t in, uint32_t in_min, uint32_t in_max,
                                  uint32_t out_min, uint32_t out_max)
{
  uint32_t out = out_max;
  if ((in_min != in_max) && (out_min != out_max) && (in <= in_min) && (in >= in_max))
    out = out_min + (out_max - out_min) * (in - in_min) / (in_max - in_min);
  return out;
}

bool AmEncodeDevice::eval_qp_mode(uint32_t streamId)
{
  // init QP value for CBR(SCBR) mode only.
  if (mStreamParamList[streamId]->bitrate_info.rate_control_mode != IAV_CBR)
    return true;

  StreamEncodeFormat &stream_format = mStreamParamList[streamId]->encode_params;
  iav_change_qp_limit_ex_t &qp = mStreamParamList[streamId]->h264_qp;
  uint32_t vinFPS = round_div(512000000, mVinList[0]->get_vin_fps());
  uint32_t macroblocks = round_up(stream_format.encode_format.encode_width, 16)
          * round_up(stream_format.encode_format.encode_height , 16) / 256;
  uint32_t kbps_for_30fps =
      (mStreamParamList[streamId]->bitrate_info.cbr_avg_bitrate >> 10)
      * 30 * stream_format.encode_framerate.ratio_denominator / vinFPS
      / stream_format.encode_framerate.ratio_numerator;
  uint32_t bitrateModeListNum = sizeof(gBitrateModeList)
      / sizeof(gBitrateModeList[0]);
  uint32_t minIdx = 0, maxIdx = bitrateModeListNum - 1;

  for (uint32_t i = 0; i < bitrateModeListNum; ++ i) {
    if (macroblocks < gBitrateModeList[i].macroblocks) {
      minIdx = i;
    }
    if (macroblocks >= gBitrateModeList[i].macroblocks) {
      maxIdx = i;
      break;
    }
  }

  mStreamParamList[streamId]->h264_qp.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_QP_LIMIT_EX,
      &(mStreamParamList[streamId]->h264_qp)) < 0)) {
    PERROR("IAV_IOC_GET_QP_LIMIT_EX");
    return false;
  }

  BitrateMode &min = gBitrateModeList[minIdx], &max = gBitrateModeList[maxIdx];
  if (kbps_for_30fps <= linear_scale(macroblocks, min.macroblocks,
      max.macroblocks, min.extremely_low_kbps, max.extremely_low_kbps)) {
    // Extremely Low Bitrate
    qp.qp_min_on_I = 1;
    qp.qp_max_on_I = 40;
    qp.qp_min_on_P = 45;
    qp.qp_max_on_P = 51;
    qp.qp_min_on_B = 45;
    qp.qp_max_on_B = 51;
    qp.i_qp_reduce = 10;
    qp.p_qp_reduce = 5;
    qp.adapt_qp = 0;
  } else if (kbps_for_30fps <= linear_scale(macroblocks, min.macroblocks,
      max.macroblocks, min.low_kbps, max.low_kbps)) {
    // Low Bitrate
    qp.qp_min_on_I = 1;
    qp.qp_max_on_I = 40;
    qp.qp_min_on_P = 17;
    qp.qp_max_on_P = 51;
    qp.qp_min_on_B = 17;
    qp.qp_max_on_B = 51;
    qp.i_qp_reduce = 10;
    qp.p_qp_reduce = 5;
    qp.adapt_qp = 0;
  } else if (kbps_for_30fps <= linear_scale(macroblocks, min.macroblocks,
      max.macroblocks, min.medium_kbps, max.medium_kbps)) {
    // Medium Bitrate
    qp.qp_min_on_I = 1;
    qp.qp_max_on_I = 51;
    qp.qp_min_on_P = 1;
    qp.qp_max_on_P = 51;
    qp.qp_min_on_B = 1;
    qp.qp_max_on_B = 51;
    qp.i_qp_reduce = 5;
    qp.p_qp_reduce = 2;
    qp.adapt_qp = 2;
  } else {
    // High Bitrate
    qp.qp_min_on_I = 1;
    qp.qp_max_on_I = 51;
    qp.qp_min_on_P = 1;
    qp.qp_max_on_P = 51;
    qp.qp_min_on_B = 1;
    qp.qp_max_on_B = 51;
    qp.i_qp_reduce = 1;
    qp.p_qp_reduce = 1;
    qp.adapt_qp = 4;
  }

  DEBUG("Stream%u QP: I %u~%u, P %u~%u, B %u~%u, IP diff %u, PB diff %u, aqp %u.",
        streamId, qp.qp_min_on_I, qp.qp_max_on_I, qp.qp_min_on_P,
        qp.qp_max_on_P, qp.qp_min_on_B, qp.qp_max_on_B, qp.i_qp_reduce,
        qp.p_qp_reduce, qp.adapt_qp);
  return true;
}

bool AmEncodeDevice::start_encode()
{
  uint32_t streams = (1 << mStreamNumber) - 1;
  return ready_for_encode() && change_stream_state(AM_IAV_ENCODING, streams);
}

bool AmEncodeDevice::stop_encode()
{
  bool ret = true;
  uint32_t streams = 0;
  for (uint32_t i = 0; i < mStreamNumber; i++) {
    if (is_stream_encoding(i)) {
      streams |= (1 << i);
  }
  }
  if (streams)
    ret = change_stream_state(AM_IAV_PREVIEW);
  return ret;
}

bool AmEncodeDevice::ready_for_encode()
{
  return change_stream_state(AM_IAV_IDLE) && change_stream_state(AM_IAV_PREVIEW);
}

bool AmEncodeDevice::start_encode_stream(uint32_t streamId)
{
  bool ret = true;
  if (!is_stream_encoding(streamId)) {
    uint32_t streams = 0;
    for (uint32_t i = 0; i < mStreamNumber; i++) {
      if (is_stream_encoding(i)) {
        streams |= (1 << i);
      }
    }
    streams |= (1 << streamId);
    ret = change_stream_state(AM_IAV_ENCODING, streams);
  }
  return ret;
}

bool AmEncodeDevice::stop_encode_stream(uint32_t streamId)
{
  bool ret = true;
  if (is_stream_encoding(streamId)) {
    uint32_t streams = 0;
    for (uint32_t i = 0; i < mStreamNumber; i++) {
      if (is_stream_encoding(i)) {
        streams |= (1 << i);
      }
    }
    streams &= ~(1 << streamId);
    if (streams)
      ret = change_stream_state(AM_IAV_ENCODING, streams);
    else
      ret = change_stream_state(AM_IAV_PREVIEW);
  }
  return ret;
}

#if CONFIG_ARCH_S2
int AmEncodeDevice::set_pm_param(PRIVACY_MASK * pm_in)
{
  RECT main_buffer;
  main_buffer.width = mEncoderParams->buffer_format_info.main_width;
  main_buffer.height = mEncoderParams->buffer_format_info.main_height;
  //ERROR("set_pm_param in encodedev id:%d height:%d w:%d left:%d top:%d",pm_in->id, pm_in->height,pm_in->width,pm_in->left,pm_in->top);
  return instance_pm_dptz->set_pm_param(pm_in, &main_buffer);
}

int AmEncodeDevice::set_dptz_param(DPTZParam   *dptz_set)
{
  //ERROR("dptz in set_dptz_param buffer:%d zoom:%d x:%d y:%d",dptz_set->source_buffer, dptz_set->zoom_factor,dptz_set->offset_x,dptz_set->offset_y);
  if(dptz_set->source_buffer == BUFFER_1) {
    return instance_pm_dptz->set_dptz_param_mainbuffer(dptz_set);
  } else {
    return instance_pm_dptz->set_dptz_param(dptz_set);
  }
  return -1;
}

int AmEncodeDevice::get_pm_param(uint32_t * pm_in)
{
  return instance_pm_dptz->get_pm_param(pm_in);
}

#else
int AmEncodeDevice::set_pm_param(PRIVACY_MASK * pm_in)
{
  return instance_pm_dptz->set_pm_param(pm_in);
}
int AmEncodeDevice::set_dptz_param(DPTZParam   *dptz_set)
{
  return instance_pm_dptz->set_dptz_param(dptz_set);
}

int AmEncodeDevice::get_dptz_param(uint32_t stream_id, DPTZParam *dptz_get)
{
  return instance_pm_dptz->get_dptz_param(stream_id, dptz_get);
}

#endif

//DPTZ end

void AmEncodeDevice::get_max_stream_num(uint32_t *pMaxNum)
{
  if (pMaxNum)
    *pMaxNum = mStreamNumber;
}

void AmEncodeDevice::get_max_stream_size(Resolution *pMaxSize)
{
  if (pMaxSize) {
    pMaxSize->width = mEncoderParams->buffer_format_info.main_width;
    pMaxSize->height = mEncoderParams->buffer_format_info.main_height;
  }
}

bool AmEncodeDevice::get_stream_source(uint32_t streamId, u8 *source)
{
  if (!check_stream_id(streamId)) {
    return false;
  }
  iav_encode_format_ex_t encode_format;
  encode_format.id = (1 << streamId);
  if (ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX, &encode_format) < 0) {
    ERROR("IAV_IOC_GET_ENCODE_FORMAT_EX: %s", strerror(errno));
    return false;
  }

  *source = encode_format.source;
  return true;
}

bool AmEncodeDevice::get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate)
{
  uint32_t vinFPS = 0;

  if (!check_stream_id(streamId))
    return false;

  if (!pFrameRate) {
    ERROR("Stream%u frame rate ptr is NULL.", streamId);
    return false;
  }
  mStreamParamList[streamId]->encode_params.encode_framerate.id =  1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_FRAMERATE_FACTOR_EX,
      &mStreamParamList[streamId]->encode_params.encode_framerate) < 0)) {
    PERROR("IAV_IOC_GET_FRAMERATE_FACTOR_EX");
    return false;
  }
  vinFPS = round_div(512000000, mVinList[0]->get_vin_fps());
  *pFrameRate = vinFPS *
      mStreamParamList[streamId]->encode_params.encode_framerate.ratio_numerator /
      mStreamParamList[streamId]->encode_params.encode_framerate.ratio_denominator;
  return true;
}

bool AmEncodeDevice::set_stream_framerate(uint32_t streamId, uint32_t frameRate)
{
  uint32_t vinFPS = 0;
  EncodeType type = AM_ENCODE_TYPE_NONE;

  if (!check_stream_id(streamId))
    return false;

  vinFPS = mVinList[0]->get_vin_fps();
  switch(vinFPS) {
    case AMBA_VIDEO_FPS_29_97:
      vinFPS = AMBA_VIDEO_FPS_30;
      break;
    case AMBA_VIDEO_FPS_59_94:
      vinFPS = AMBA_VIDEO_FPS_60;
      break;
    default:
      break;
  }

  vinFPS = round_div(512000000, vinFPS);

    if (frameRate > vinFPS) {
    ERROR("Stream%u frame rate [%u] cannot be greater than VIN frame rate"
    "[%u].", streamId, frameRate, vinFPS);
    return false;
  }

  mStreamParamList[streamId]->encode_params.encode_framerate.ratio_numerator =
      frameRate;
  mStreamParamList[streamId]->encode_params.encode_framerate.ratio_denominator =
      vinFPS;

  // Update Framerate if the stream is in encoding.
  if (is_stream_encoding(streamId) && get_stream_type(streamId, &type)
      && (type != AM_ENCODE_TYPE_NONE)) {
    if (!check_system_performance() || !update_stream_framerate(streamId))
      return false;
    // Reset H.264 qp
    if (type == AM_ENCODE_TYPE_H264){
      eval_qp_mode(streamId);
      update_stream_h264_qp(streamId);
    }
  }
  mStreamParamList[streamId]->config_changed = 1;
  return true;
}

bool AmEncodeDevice::get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate)
{
  if (!check_stream_id(streamId))
    return false;

  if (!pBitrate) {
    ERROR("Stream%u bitrate ptr is NULL.", streamId);
    return false;
  }
  mStreamParamList[streamId]->bitrate_info.id =  1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_BITRATE_EX,
        &mStreamParamList[streamId]->bitrate_info) < 0)) {
      PERROR("IAV_IOC_GET_BITRATE_EX");
      return false;
    }

  *pBitrate = mStreamParamList[streamId]->bitrate_info.cbr_avg_bitrate;
  return true;
}

bool AmEncodeDevice::set_cbr_bitrate(uint32_t streamId, uint32_t bitrate)
{
  EncodeType type = AM_ENCODE_TYPE_NONE;
  if (!check_stream_id(streamId))
    return false;
  if (!bitrate) {
    ERROR("Stream%u bitrate is 0.", streamId);
    return false;
  }
  mStreamParamList[streamId]->bitrate_info.cbr_avg_bitrate = bitrate;

  // Update bitrate if the stream is in encoding.
  if (is_stream_encoding(streamId) && get_stream_type(streamId, &type)
      && (type == AM_ENCODE_TYPE_H264)) {
    if (!update_stream_h264_bitrate(streamId))
      return false;
    // Reset H.264 qp
    eval_qp_mode(streamId);
    update_stream_h264_qp(streamId);
  }
  mStreamParamList[streamId]->config_changed = 1;
  return true;
}

bool AmEncodeDevice::get_stream_type(uint32_t streamId, EncodeType *pType)
{
  if (!check_stream_id(streamId))
    return false;

  if (!pType) {
    ERROR("EncodeType ptr is NULL.");
    return false;
  }

#if 0
  mStreamParamList[streamId]->encode_params.encode_format.id =  1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX,
      &mStreamParamList[streamId]->encode_params.encode_format) < 0)) {
    PERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
    return false;
  }
#endif

  switch(mStreamParamList[streamId]->encode_params.encode_format.encode_type) {
    case IAV_ENCODE_H264:
      *pType = AM_ENCODE_TYPE_H264;
      break;
    case IAV_ENCODE_MJPEG:
      *pType = AM_ENCODE_TYPE_MJPEG;
      break;
    default:
      *pType = AM_ENCODE_TYPE_NONE;
      break;
  }
  return true;
}

bool AmEncodeDevice::set_stream_type(uint32_t streamId, const EncodeType type)
{
  if (!check_stream_id(streamId))
    return false;

  switch (type) {
    case AM_ENCODE_TYPE_H264:
      mStreamParamList[streamId]->encode_params.encode_format.encode_type =
          IAV_ENCODE_H264;
      break;
    case AM_ENCODE_TYPE_MJPEG:
      mStreamParamList[streamId]->encode_params.encode_format.encode_type =
          IAV_ENCODE_MJPEG;
      break;
    default:
      mStreamParamList[streamId]->encode_params.encode_format.encode_type =
          IAV_ENCODE_NONE;
      break;
  }
  mStreamParamList[streamId]->config_changed = 1;
  return true;
}

bool AmEncodeDevice::get_stream_idr(uint32_t streamId, uint8_t *idr_interval)
{
  if (!check_stream_id(streamId))
    return false;

  *idr_interval = mStreamParamList[streamId]->h264_config.idr_interval;

  return true;
}

bool AmEncodeDevice::set_stream_idr(uint32_t streamId, uint8_t idr_interval)
{
  if (!check_stream_id(streamId))
    return false;

  mStreamParamList[streamId]->h264_config.idr_interval = idr_interval;

  mStreamParamList[streamId]->config_changed = 1;

  return update_stream_h264_config(streamId);
}

bool AmEncodeDevice::get_stream_n(uint32_t streamId, uint16_t *n)
{
  if (!check_stream_id(streamId))
    return false;

  *n = mStreamParamList[streamId]->h264_config.N;

  return true;
}

bool AmEncodeDevice::set_stream_n(uint32_t streamId, uint16_t n)
{
  if (!check_stream_id(streamId))
    return false;

  mStreamParamList[streamId]->h264_config.N = n;

  mStreamParamList[streamId]->config_changed = 1;

  return update_stream_h264_config(streamId);
}

bool AmEncodeDevice::get_stream_profile(uint32_t streamId, uint8_t *profile)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  *profile = mStreamParamList[streamId]->h264_config.profile;

  return true;
}

bool AmEncodeDevice::set_stream_profile(uint32_t streamId, uint8_t profile)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  mStreamParamList[streamId]->h264_config.profile = profile;

  mStreamParamList[streamId]->config_changed = 1;

  return true;

}

bool AmEncodeDevice::get_mjpeg_quality(uint32_t streamId, uint8_t *quality)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  *quality = mStreamParamList[streamId]->mjpeg_config.quality;

  return true;
}

bool AmEncodeDevice::set_mjpeg_quality(uint32_t streamId, uint8_t quality)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  if (quality < 0 || quality >100){
    ERROR("MJPEG Quality should be between 0~100!");
    return false;
  }

  mStreamParamList[streamId]->mjpeg_config.quality = quality;
  mStreamParamList[streamId]->config_changed = 1;

  return update_stream_mjpeg_config(streamId);
}

