/*******************************************************************************
 * am_vin.cpp
 *
 * Histroy:
 *  2012-3-6 2012 - [ypchang] created file
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
#include "am_vin.h"

AmVin::AmVin(VinType type, int iav) :
  mVinType(type),
  mVinIav(iav),
  mVinParams(NULL),
  mIsVinStarted(false)
{
  mNeedCloseIav = (mVinIav < 0);
  if (mNeedCloseIav && ((mVinIav = ::open("/dev/iav", O_RDWR)) < 0)) {
    PERROR("Failed to open IAV device");
  }
  DEBUG("VIN IAV fd is %d", mVinIav);
}

AmVin::~AmVin()
{
  if (mNeedCloseIav && (mVinIav >= 0)) {
    ::close(mVinIav);
  }
  DEBUG("AmVin deleted!");
}

void AmVin::set_vin_config(VinParameters *config)
{
  mVinParams = config;
}

bool AmVin::get_current_vin_mode(amba_video_mode &mode)
{
  bool ret = false;
  mode = AMBA_VIDEO_MODE_AUTO;
  if (AM_UNLIKELY(ioctl(mVinIav, IAV_IOC_VIN_SRC_GET_VIDEO_MODE, &mode) < 0)) {
    PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_MODE");
  } else {
    ret = true;
  }

  return ret;
}

bool AmVin::get_current_vin_fps(uint32_t *fps)
{
  bool ret = false;
  *fps = AMBA_VIDEO_FPS_AUTO;
  if (AM_UNLIKELY(ioctl(mVinIav, IAV_IOC_VIN_SRC_GET_FRAME_RATE, fps) < 0)) {
    PERROR("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
  } else {
    ret = true;
  }

  return ret;
}

bool AmVin::get_current_vin_size(Resolution &size)
{
  bool ret = false;
  struct amba_video_info video_info;
  if (AM_UNLIKELY(ioctl(mVinIav, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0)) {
    PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
  } else {
    size.width = video_info.width;
    size.height = video_info.height;
    ret = true;
  }

  return ret;
}

bool AmVin::start(bool force)
{
  if (AM_LIKELY((mVinIav != -1) && check_vin_parameters())) {

    if (!mIsVinStarted || force) {
      bool                 ret = false;
      amba_vin_source_info sourceInfo;
      amba_video_info      videoInfo;
      do {
        if (AM_LIKELY(mVinParams)) {
          if (AM_UNLIKELY(ioctl(mVinIav, IAV_IOC_VIN_SET_CURRENT_SRC,
                                &mVinParams->source) < 0)) {
            PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
            break;
          }

          if (AM_UNLIKELY(ioctl(mVinIav,
                                IAV_IOC_VIN_SRC_SET_VIDEO_MODE,
                                mVinParams->video_mode) < 0)) {
            PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
            break;
          }

          if ((mVinParams->mirror_mode.pattern <= 3) &&
              (mVinParams->mirror_mode.bayer_pattern <= 3)) {
            if (AM_UNLIKELY(ioctl(mVinIav,
                                  IAV_IOC_VIN_SRC_SET_MIRROR_MODE,
                                  &mVinParams->mirror_mode) < 0)) {
              PERROR("IAV_IOC_VIN_SRC_SET_MIRROR_MODE");
              break;
            }
          }

          if (AM_UNLIKELY(ioctl(mVinIav,
                                IAV_IOC_VIN_SRC_GET_INFO, &sourceInfo) < 0)) {
            PERROR("IAV_IOC_VIN_SRC_GET_INFO");
            break;
          }

          if (AM_UNLIKELY(ioctl(mVinIav,
                                IAV_IOC_VIN_SRC_GET_VIDEO_INFO,
                                &videoInfo) < 0)) {
            PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
            break;
          }

          if (sourceInfo.dev_type != AMBA_VIN_SRC_DEV_TYPE_DECODER) {
            if (AM_UNLIKELY(ioctl(mVinIav,
                                  IAV_IOC_VIN_SRC_SET_FRAME_RATE,
                                  mVinParams->framerate))) {
              PERROR("IAV_IOC_VIN_SRC_SET_FRAME_RATE");
              break;
            }

            if (videoInfo.type == AMBA_VIDEO_TYPE_RGB_RAW) {
              if (AM_UNLIKELY(ioctl(mVinIav, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME,
                                    (512000000 /
                                     mVinParams->vin_eshutter_time)) < 0)) {
                PERROR("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
                break;
              }
              if (AM_UNLIKELY(ioctl(mVinIav,
                                    IAV_IOC_VIN_SRC_SET_AGC_DB,
                                    (mVinParams->vin_agc_db << 24)) < 0)) {
                PERROR("IAV_IOC_VIN_SRC_SET_AGC_DB");
                break;
              }
            }
          }
          ret = true;
        } /* if (config)*/
      } while (0);
      mIsVinStarted = ret;
    }
  } else if (mVinIav == -1) {
    ERROR("Invalid IAV fd!");
  } else {
    ERROR("VIN parameters checking failed!");
  }

  return mIsVinStarted;
}

uint32_t AmVin::get_vin_fps()
{
  uint32_t fps = mVinParams->framerate;

  if (fps == AMBA_VIDEO_FPS_AUTO) {
    amba_video_info      videoInfo;
    if (AM_UNLIKELY(ioctl(mVinIav,
                          IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &videoInfo) < 0)) {
      PERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    }
    fps = videoInfo.fps;
  }

  return fps;
}

bool AmVin::get_vin_size(Resolution &size)
{
  bool ret = false;
  amba_video_mode video_mode = mVinParams->video_mode;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == video_mode) {
      size.width = gVideoModeList[i].width;
      size.height = gVideoModeList[i].height;
      ret = true;
      break;
    }
  }
  return ret;
}

bool AmVin::check_vin_parameters()
{
  bool ret = false;
  if (AM_LIKELY((mVinIav != -1) && mVinParams)) {
    do {
      uint32_t num = 0;
      if (ioctl(mVinIav, IAV_IOC_VIN_GET_SOURCE_NUM, &num) < 0) {
        PERROR("IAV_IOC_VIN_GET_SOURCE_NUM");
        break;
      }
      if (num < 1) {
        ERROR("No VIN devices found, "
              "you probably need to load source driver!");
        break;
      }
      if (mVinParams->source >= num) {
        WARN("Invalid source ID %u, which is not in range [0, %u], reset to 0",
             mVinParams->source, (num - 1));
        mVinParams->source = 0;
      }
      if (ioctl(mVinIav,
                IAV_IOC_VIN_SET_CURRENT_SRC,
                &mVinParams->source) < 0) {
        PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
        break;
      } else {
        uint32_t                  currentVinFps  = AMBA_VIDEO_FPS_AUTO;
        amba_video_mode           currentVinMode = AMBA_VIDEO_MODE_AUTO;
        amba_vin_source_info      sourceInfo;
        amba_vin_source_mode_info modeInfo;
        uint32_t                  fpsList[256] = {0};
        modeInfo.mode = (uint32_t)mVinParams->video_mode;
        modeInfo.fps_table = fpsList;
        modeInfo.fps_table_size = sizeof(fpsList) / sizeof(uint32_t);

        if (ioctl(mVinIav, IAV_IOC_VIN_SRC_GET_INFO, &sourceInfo) < 0) {
          PERROR("IAV_IOC_VIN_SRC_GET_INFO");
          break;
        }

        INFO("Find Vin Source %s!", sourceInfo.name);

        if (sourceInfo.dev_type == AMBA_VIN_SRC_DEV_TYPE_DECODER) {
          /* This source is from Decoder device */
          if (ioctl(mVinIav,
                    IAV_IOC_SELECT_CHANNEL,
                    sourceInfo.active_channel_id) < 0) {
            PERROR("IAV_IOC_SELECT_CHANNEL");
            break;
          }
        }
        if (ioctl(mVinIav, IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE, &modeInfo) < 0) {
          PERROR("IAV_IOC_VIN_SRC_CHECK_VIDEO_MODE");
          break;
        }

        if (AM_UNLIKELY(!modeInfo.is_supported)) {
          ERROR("VIN mode %s is not supported by %s!",
                video_mode_string(mVinParams->video_mode),
                sourceInfo.name);
          break;
        } else {
          if ((mVinParams->framerate != AMBA_VIDEO_FPS_AUTO) &&
              (mVinParams->framerate < modeInfo.video_info.fps)) {
            WARN("Vin fps %s in mode %s is not supported by %s, "
                "reset fps to %s",
                video_fps_string(mVinParams->framerate),
                video_mode_string(mVinParams->video_mode),
                sourceInfo.name,
                video_fps_string(modeInfo.video_info.fps));
            mVinParams->framerate = modeInfo.video_info.fps;
          }
        }
        if (get_current_vin_mode(currentVinMode) &&
            get_current_vin_fps(&currentVinFps)) {
          NOTICE("VIN current mode is %s, config's mode is %s, "
                 "VIN current fps is %s, config's fps is %s.",
                 video_mode_string(currentVinMode),
                 video_mode_string(mVinParams->video_mode),
                 video_fps_string(currentVinFps),
                 video_fps_string(mVinParams->framerate));
          if ((currentVinMode != AMBA_VIDEO_MODE_AUTO) &&
              (currentVinMode != mVinParams->video_mode) &&
              (currentVinFps != AMBA_VIDEO_FPS_AUTO) &&
              (currentVinFps != mVinParams->framerate)) {
            mIsVinStarted = false; //VIN's video mode is changed, need restart
            NOTICE("VIN mode changed, need re-initialize VIN device!");
          } else if ((currentVinMode != AMBA_VIDEO_MODE_AUTO) &&
                     (currentVinMode == mVinParams->video_mode) &&
                     (currentVinFps != AMBA_VIDEO_FPS_AUTO) &&
                     (currentVinFps == mVinParams->framerate)) {
            mIsVinStarted = true;
            NOTICE("VIN has already been initialized to target mode: %s %s!",
                   video_mode_string(currentVinMode),
                   video_fps_string(currentVinFps));
          } else {
            mIsVinStarted = false;
            NOTICE("VIN device needs to be initialized!");
          }
        } else {
          mIsVinStarted = false;
          DEBUG("VIN device needs to be initialized!");
        }
        ret = true;
      }
    } while (0);
  } else if (!mVinParams) {
    ERROR("Invalid VIN parameters!");
  } else {
    ERROR("Invalid IAV fd!");
  }

  return ret;
}

const char* AmVin::video_fps_string(uint32_t fps)
{
  char *videoFps = NULL;
  char string[64] = {0};

  for (uint32_t i = 0; i < sizeof(gFpsList) / sizeof(CameraVinFPS); ++ i) {
    if (gFpsList[i].fpsValue == fps) {
      videoFps = (char *)gFpsList[i].fpsName;
      DEBUG("%u is converted to %s", fps, videoFps);
      break;
    }
  }

  if (AM_UNLIKELY(videoFps == NULL)) {
    if (fps != 0xffffffff) {
      sprintf(string, "%.2lf", (double)(512000000/fps));
      videoFps = string;
    } else {
      videoFps = (char *)gFpsList[0].fpsName;
      WARN("Invalid fps value %u, reset to auto!", fps);
    }
  }

  return videoFps;
}

const char* AmVin::video_mode_string(int32_t mode)
{
  for (uint32_t i = 0;
       i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
       ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      return gVideoModeList[i].videoMode;
    }
  }

  return gVideoModeList[0].videoMode;
}
