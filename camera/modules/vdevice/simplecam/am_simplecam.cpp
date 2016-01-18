/*******************************************************************************
 * am_simplecam.cpp
 *
 * Histroy:
 *  2012-3-9 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
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

AmSimpleCam::AmSimpleCam(VDeviceParameters *vDeviceConfig)
  : AmVideoDevice(),
    mYdataBuffer(NULL)
{
  if (!create_video_device(vDeviceConfig)) {
    ERROR("Failed to create the components of this video device!");
  }
}

AmSimpleCam::~AmSimpleCam()
{
  delete[] mYdataBuffer;
  DEBUG("AmSimpleCam deleted!");
}

bool AmSimpleCam::goto_idle()
{
  return change_iav_state(AM_IAV_IDLE);
}

bool AmSimpleCam::enter_preview()
{
  return change_iav_state(AM_IAV_PREVIEW);
}

bool AmSimpleCam::start_encode()
{
  return change_iav_state(AM_IAV_ENCODING);
}

bool AmSimpleCam::stop_encode()
{
  return change_iav_state(AM_IAV_PREVIEW);
}

bool AmSimpleCam::enter_decode_mode()
{
  return change_iav_state(AM_IAV_DECODING);
}

bool AmSimpleCam::leave_decode_mode()
{
  return change_iav_state(AM_IAV_IDLE);
}

bool AmSimpleCam::enter_photo_mode()
{
  return change_iav_state(AM_IAV_STILL_CAPTURE);
}

bool AmSimpleCam::leave_photo_mode()
{
  ERROR("Not supported in SimpleCam!"
        "Use SimplePhoto instead!");
  return false;
}

bool AmSimpleCam::apply_encoder_parameters(IavState targetIavState)
{
  switch(targetIavState) {
    case AM_IAV_DECODING: {
      for (uint32_t i = 0; i < mVoutNumber; ++ i) {
        if ((mVoutTypeList[i] == AM_VOUT_TYPE_HDMI) ||
            (mVoutTypeList[i] == AM_VOUT_TYPE_CVBS)) {
          mEncoderParams->buffer_type_info.third_buffer_type =
                      IAV_SOURCE_BUFFER_TYPE_PREVIEW;
        } else if (mVoutTypeList[i] == AM_VOUT_TYPE_LCD) {
          mEncoderParams->buffer_type_info.fourth_buffer_type =
                      IAV_SOURCE_BUFFER_TYPE_PREVIEW;
        }
      }
    }break;
    case AM_IAV_PREVIEW:
    case AM_IAV_ENCODING:
    case AM_IAV_STILL_CAPTURE: {
      iav_source_buffer_type_all_ex_t &bufferType =
          mEncoderParams->buffer_type_info;
      for (uint32_t i = 0; i < mStreamNumber; ++ i) {
        StreamParameters *streamParam = mStreamParamList[i];
        if (streamParam->encode_params.encode_format.encode_type > 0) {
          if (streamParam->encode_params.encode_format.source == 2) {
            bufferType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
          }
          if (streamParam->encode_params.encode_format.source == 3) {
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
          }
        }
      }
    }break;
    default: break;
  }
  return AmVideoDevice::apply_encoder_parameters();
}

bool AmSimpleCam::apply_stream_parameters()
{
  return AmVideoDevice::apply_stream_parameters();
}

bool AmSimpleCam::ready_for_encode()
{
  bool ret = false;
  if (change_iav_state(AM_IAV_IDLE) && change_iav_state(AM_IAV_PREVIEW) &&
      apply_stream_parameters()) {
    /* VOUT 1 should be disabled when using Mixer B for OSD blending */
    if ((mEncoderParams->system_setup_info.voutA_osd_blend_enable == 0) &&
        (mEncoderParams->system_setup_info.voutB_osd_blend_enable == 1)) {
      for (uint32_t i = 0; i < mVoutNumber; ++ i) {
        if ((mVoutTypeList[i] == AM_VOUT_TYPE_HDMI) ||
            (mVoutTypeList[i] == AM_VOUT_TYPE_CVBS)) {
          mVoutList[i]->stop();
        }
      }
    }
    ret = true;
  }

  return ret;
}

bool AmSimpleCam::reset_device()
{
  return (change_iav_state(AM_IAV_IDLE) &&
          init_device(AmVout::AM_VOUT_INIT_ENABLE_VIDEO, true));
}

bool AmSimpleCam::get_y_data(YBufferFormat *yBufferFormat)
{
  bool ret = false;
  AmVout *lcd = NULL;
  VoutParameters *lcdParams = NULL;
  /* Find LCD configurations */
  for (uint32_t i = 0; i < mVoutNumber; ++ i) {
    if (mVoutTypeList[i] == AM_VOUT_TYPE_LCD) {
      lcd = mVoutList[i];
      lcdParams = mVoutParamList[i];
      break;
    }
  }
  if (yBufferFormat && mEncoderParams->yuv_buffer_id < MAX_ENCODE_BUFFER_NUM) {
    IavState iavStatus = get_iav_status();
    if (AM_LIKELY((iavStatus == AM_IAV_PREVIEW) ||
                  (iavStatus == AM_IAV_ENCODING))) {
      if (map_dsp()) {
        iav_yuv_buffer_info_ex_t yuvInfo = {0};
        uint32_t count = 0;
        yuvInfo.source = mEncoderParams->yuv_buffer_id;
        do {
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_YUV_BUFFER_INFO_EX,
                                &yuvInfo) < 0)) {
            if ((errno == EAGAIN) && (++ count < 10)) {
              usleep(100000);
              continue;
            }
            PERROR("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
          } else {
            if (!mYdataBuffer) {
              mYdataBuffer = new uint8_t[MAX_YUV_BUFFER_SIZE];
            }
            if (AM_LIKELY(mYdataBuffer)) {
              if (true == (ret = save_y_data(mYdataBuffer, &yuvInfo))) {
                yBufferFormat->y_addr = mYdataBuffer;
                if ((mEncoderParams->yuv_buffer_id == 3) &&
                    (mEncoderParams->buffer_type_info.fourth_buffer_type ==
                        IAV_SOURCE_BUFFER_TYPE_PREVIEW) &&
                    (lcd && lcdParams->video_rotate == AMBA_VOUT_ROTATE_90)) {
                  yBufferFormat->y_width = yuvInfo.height;
                  yBufferFormat->y_height = yuvInfo.width;
                } else {
                  yBufferFormat->y_width = yuvInfo.width;
                  yBufferFormat->y_height = yuvInfo.height;
                }
                DEBUG("Y buffer width %u, Y buffer height %u",
                      yuvInfo.width, yuvInfo.height);
              }
            } else {
              ERROR("Not enough memory for Y data capture!");
            }
          }
        } while (!ret && (count < 10) &&
                 ((errno == EINTR) || (errno == EAGAIN)));
        if (!unmap_dsp()) {
          ERROR("Failed to unmap DSP!");
        }
      } else {
        ERROR("Failed to map DSP!");
      }
    } else {
      ERROR("To get YUV data, IAV must be in encoding or preview state!");
    }
  } else {
    if (!yBufferFormat) {
      ERROR("Invalid input parameter!");
    }
    if (mEncoderParams->yuv_buffer_id >= MAX_ENCODE_BUFFER_NUM) {
      ERROR("No source assigned to YUV data.");
    }
  }

  return ret;
}

inline bool AmSimpleCam::save_y_data(uint8_t                  *yBuffer,
                                     iav_yuv_buffer_info_ex_t *yuvInfo)
{
  int ret = false;
  AmVout *lcd = NULL;
  VoutParameters *lcdParams = NULL;
  /* Find LCD configurations */
  for (uint32_t i = 0; i < mVoutNumber; ++ i) {
    if (mVoutTypeList[i] == AM_VOUT_TYPE_LCD) {
      lcd = mVoutList[i];
      lcdParams = mVoutParamList[i];
      break;
    }
  }
  if (yBuffer && yuvInfo && yuvInfo->y_addr) {
    DEBUG("Pitch is %u, Width is %u, Height is %u.",
          yuvInfo->pitch, yuvInfo->width, yuvInfo->height);
    if ((mEncoderParams->yuv_buffer_id == 3) &&
        (mEncoderParams->buffer_type_info.fourth_buffer_type ==
            IAV_SOURCE_BUFFER_TYPE_PREVIEW) &&
        (lcd && lcdParams->video_rotate == AMBA_VOUT_ROTATE_90)) {
      /* Buffer has 90 degrees rotation */
      NOTICE("4th buffer has 90 degrees rotation!");
      if (yuvInfo->pitch == yuvInfo->height) {
        memcpy(yBuffer, yuvInfo->y_addr, yuvInfo->width * yuvInfo->height);
      } else if (yuvInfo->pitch > yuvInfo->height) {
        uint8_t *yAddr = yuvInfo->y_addr;
        for (uint32_t line = 0; line < yuvInfo->width; ++ line) {
          memcpy(yBuffer, yAddr, yuvInfo->height);
          yAddr += yuvInfo->pitch;
          yBuffer += yuvInfo->height;
        }
      } else {
        ERROR("Pitch %u is smaller than height %u!",
              yuvInfo->pitch, yuvInfo->height);
      }
    } else {
      if (yuvInfo->pitch == yuvInfo->width) {
        memcpy(yBuffer, yuvInfo->y_addr, yuvInfo->width * yuvInfo->height);
      } else if (yuvInfo->pitch > yuvInfo->width) {
        uint8_t *yAddr = yuvInfo->y_addr;
        for (uint32_t row = 0; row < yuvInfo->height; ++ row) {
          memcpy(yBuffer, yAddr, yuvInfo->width);
          yAddr += yuvInfo->pitch;
          yBuffer += yuvInfo->width;
        }
      } else {
        ERROR("Pitch %u is smaller than width %u!",
              yuvInfo->pitch, yuvInfo->width);
      }
    }

    ret = true;
  }

  return ret;
}

bool AmSimpleCam::init_device(int voutInitMode, bool force)
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

bool AmSimpleCam::change_iav_state(IavState target)
{
  bool ret   = false;
  bool error = false;
  IavState currentState = get_iav_status();

  do {
    DEBUG("Target IAV state is %s.", iav_state_to_str(target));
    DEBUG("Current IAV state is %s.", iav_state_to_str(currentState));
    switch(currentState) {
      case AM_IAV_INIT: {
        /* If current state is INIT, should initialize device first */
        if (idle() && init_device(AmVout::AM_VOUT_INIT_DISABLE_VIDEO)) {
          switch(target) {
            case AM_IAV_IDLE:
            case AM_IAV_DECODING:
            case AM_IAV_PREVIEW:
            case AM_IAV_STILL_CAPTURE:
            case AM_IAV_ENCODING: {
              /* If current state is INIT,
               * need to enter preview to boot up dsp
               */
              if (apply_encoder_parameters(target) && preview()) {
                /* Turn on video layer of VOUT0 and VOUT1 */
                for (uint32_t i = 0; i < mVoutNumber; ++ i) {
                  if (mVoutTypeList[i] != AM_VOUT_TYPE_NONE) {
                    mVoutList[i]->video_layer_switch(true);
                  }
                }
                if (target == AM_IAV_PREVIEW) {
                  ret = true;
                }
              } else {
                ret = false;
                error = true;
              }
            }break;
            default: {
              ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            }break;
          }
        } else {
          ret = false;
          error = true;
          ERROR("Initialize camera device failed!");
        }
      }break;
      case AM_IAV_IDLE: {
        if (init_device(AmVout::AM_VOUT_INIT_ENABLE_VIDEO)) {
          switch(target) {
            case AM_IAV_DECODING: {
              if (!apply_encoder_parameters(target) || !decode(true)) {
                ret = false;
                error = true;
              } else if (target == AM_IAV_DECODING) {
                ret = true;
              }
            }break;
            case AM_IAV_PREVIEW:
            case AM_IAV_ENCODING:
            case AM_IAV_STILL_CAPTURE: {
              if (!apply_encoder_parameters(target) || !preview()) {
                ret = false;
                error = true;
              } else if (target == AM_IAV_PREVIEW) {
                ret = true;
              }
            }break;
            case AM_IAV_IDLE: {
              ret = true;
            }break;
            default: {
              ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
              ret = false;
              error = true;
            }break;
          }
        } else {
          ret = false;
          error = true;
          ERROR("Failed to initialize device!");
        }
      }break;
      case AM_IAV_PREVIEW: {
        switch(target) {
          case AM_IAV_PREVIEW: {
            ret = true;
            error = false;
          }break;
          case AM_IAV_DECODING:
          case AM_IAV_IDLE: {
            if (!idle()) {
              ret = false;
              error = true;
            } else if (target == AM_IAV_IDLE) {
              ret = true;
            }
          }break;
          case AM_IAV_ENCODING: {
            if ((mEncoderParams->system_setup_info.\
                 voutA_osd_blend_enable == 0) &&
                (mEncoderParams->system_setup_info.\
                 voutB_osd_blend_enable == 1)) {
              for (uint32_t i = 0; i < mVoutNumber; ++ i) {
                if ((mVoutTypeList[i] == AM_VOUT_TYPE_HDMI) ||
                    (mVoutTypeList[i] == AM_VOUT_TYPE_CVBS)) {
                  mVoutList[i]->stop();
                }
              }
            }
            ret = encode(true);
            error = !ret;
          }break;
          case AM_IAV_STILL_CAPTURE: {
            ERROR("SimpleCam doesn't support photo mode!"
                  "Please use SimplePhoto instead!");
            ret = false;
            error = true;
          }break;
          default: {
            ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            ret = false;
            error = true;
          }break;
        }
      }break;
      case AM_IAV_ENCODING: {
        switch(target) {
          case AM_IAV_ENCODING: {
            ret = true;
          }break;
          case AM_IAV_PREVIEW:
          case AM_IAV_STILL_CAPTURE:
          case AM_IAV_IDLE:
          case AM_IAV_DECODING: {
            if (AM_UNLIKELY(!encode(false))) {
              ret = false;
              error = true;
            } else if (target == AM_IAV_PREVIEW) {
              ret = true;
            }
          }break;
          default: {
            ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            ret = false;
            error = true;
          }break;
        }
      }break;
      case AM_IAV_DECODING: {
        switch(target) {
          case AM_IAV_DECODING: {
            ret = true;
          }break;
          case AM_IAV_IDLE:
          case AM_IAV_PREVIEW:
          case AM_IAV_ENCODING:
          case AM_IAV_STILL_CAPTURE: {
            if (!decode(false) || !idle()) {
              ret = false;
              error = true;
            } else if (target == AM_IAV_IDLE) {
              ret = true;
            }
          }break;
          default: {
            ERROR("Cannot change IAV state to %s", iav_state_to_str(target));
            ret = false;
            error = true;
          }break;
        }
      }break;
      case AM_IAV_STILL_CAPTURE: {
        ERROR("Invalid mode: %s", iav_state_to_str(AM_IAV_STILL_CAPTURE));
        ret = false;
        error = true;
      }break;
      default: {
        ERROR("Wrong IAV status!");
        ret = false;
        error = true;
      }break;
    }
  } while (!error && (((currentState = get_iav_status()) != target)));
  DEBUG("Target IAV state is %s.", iav_state_to_str(target));
  DEBUG("Current IAV state is %s.", iav_state_to_str(currentState));

  return ret;
}
