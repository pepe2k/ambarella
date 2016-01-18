/*******************************************************************************
 * am_webcam.cpp

 *
 * History:
 *  Dec 25, 2012 2012 - [qianshen] created file
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
#include "am_configure.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_overlay.h"
#include "am_middleware.h"

AmWebCam::AmWebCam(AmConfig *config):
  AmCam(),
  mEncodeDevice(NULL)
{
  if ((false == mIsCamCreated) && config) {
    if (config->load_vdev_config()) {
      mEncodeDevice = new AmEncodeDevice(config->vdevice_config());
      mVideoDevice = mEncodeDevice;
      mConfig = config;
      mIsCamCreated = (mEncodeDevice ? true : false);
    } else {
      ERROR("Failed to load VideoDevice's config.");
    }
  } else if (!config) {
    ERROR("Invalid AmConfig!");
  }
}

AmWebCam::~AmWebCam()
{
  DEBUG("AmWebCam deleted!");
}

bool AmWebCam::init(void)
{
  if (AmCam::init()) {
    mEncodeDevice->get_max_stream_num(&mStreamMaxNumber);
  }

  return mIsCamInited;
}

void AmWebCam::get_streamMaxSize(Resolution *pMaxSize)
{
  if (init()) {
    mEncodeDevice->get_max_stream_size(pMaxSize);
  }
}

bool AmWebCam::start_encode()
{
  return init() && stop_encode() && mEncodeDevice->start_encode();
}

bool AmWebCam::stop_encode()
{
  bool ret = false;
  if (init() && mEncodeDevice->stop_encode()) {
    for (uint32_t streamId = 0; streamId < mStreamMaxNumber; ++streamId) {
      for (uint32_t areaId = 0; areaId < MAX_OVERLAY_AREA_NUM; ++ areaId) {
        remove_overlay(streamId, areaId);
      }
    }
    destroy_overlaytime_task();
    ret = true;
  }
  return ret;
}

bool AmWebCam::get_stream_type(uint32_t streamId, EncodeType *pType)
{
  return init() && mEncodeDevice->get_stream_type(streamId, pType);
}

bool AmWebCam::set_stream_type(uint32_t streamId, EncodeType type)
{
  return init() && mEncodeDevice->set_stream_type(streamId, type);
}

bool AmWebCam::get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate)
{
  return init() ? mEncodeDevice->get_stream_framerate(streamId, pFrameRate):false;
}

bool AmWebCam::set_stream_framerate(uint32_t streamId, const uint32_t frameRate)
{
  return init() ? mEncodeDevice->set_stream_framerate(streamId, frameRate) : false;
}

bool AmWebCam::set_privacy_mask(PRIVACY_MASK * pm_in)
{
  return init() ? mEncodeDevice->set_pm_param(pm_in) : false;
}

bool AmWebCam::set_digital_ptz(DPTZParam *dptz_set)
{
  return init() ? mEncodeDevice->set_dptz_param(dptz_set) : false;
}

#ifdef CONFIG_ARCH_S2
bool AmWebCam::get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get)
{
  return true;
}

bool AmWebCam::get_privacy_mask(uint32_t * pm_in)
{
  return init() ? mEncodeDevice->get_pm_param(pm_in) : false;
}
int AmWebCam::reset_dptz_pm()
{
  return init() ? mEncodeDevice->reset_pm() : false;
}

#else
bool AmWebCam::get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get)
{
  return init() ? mEncodeDevice->get_dptz_param(stream_id, dptz_get) : false;
}
#endif

bool AmWebCam::get_stream_idr(uint32_t streamId, uint8_t *idr_interval)
{
  return init() ? mEncodeDevice->get_stream_idr(streamId, idr_interval) : false;
}

bool AmWebCam::set_stream_idr(uint32_t streamId, uint8_t idr_interval)
{
  return init() ? mEncodeDevice->set_stream_idr(streamId, idr_interval) : false;
}

bool AmWebCam::get_stream_N(uint32_t streamId, uint16_t *n)
{
  return init() ? mEncodeDevice->get_stream_n(streamId, n) : false;
}

bool AmWebCam::set_stream_N(uint32_t streamId, uint16_t n)
{
  return init() ? mEncodeDevice->set_stream_n(streamId, n) : false;
}

bool AmWebCam::get_stream_profile(uint32_t streamId, uint8_t *profile)
{
  return init() ? mEncodeDevice->get_stream_profile(streamId, profile) : false;
}

bool AmWebCam::set_stream_profile(uint32_t streamId, uint8_t profile)
{
  return init() ? mEncodeDevice->set_stream_profile(streamId, profile) : false;
}

bool AmWebCam::get_mjpeg_quality(uint32_t streamId, uint8_t *quality)
{
  return init() ? mEncodeDevice->get_mjpeg_quality(streamId, quality) : false;
}

bool AmWebCam::set_mjpeg_quality(uint32_t streamId, uint8_t quality)
{
  return init() ? mEncodeDevice->set_mjpeg_quality(streamId, quality) : false;
}
bool AmWebCam::set_cbr_bitrate(uint32_t streamId, uint32_t bitrate)
{
  return init() && mEncodeDevice->set_cbr_bitrate(streamId, bitrate);
}

bool AmWebCam::get_cbr_bitrate(uint32_t streamId, uint32_t* pBitrate)
{
  return init() && mEncodeDevice->get_cbr_bitrate(streamId, pBitrate);
}

