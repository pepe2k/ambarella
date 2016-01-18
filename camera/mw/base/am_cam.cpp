/*******************************************************************************
 * am_cam.cpp
 *
 * History:
 *  Jul 1, 2013 2013 - [qianshen] created file
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

AmCam::AmCam():
  mConfig(NULL),
  mVideoDevice(NULL),
  mStreamMaxNumber(0),
  mOverlayTimeTask(NULL),
  mIsCamCreated(false),
  mIsCamInited(false)
{
}

AmCam::~AmCam()
{
  delete mOverlayTimeTask;
  delete mVideoDevice;

  DEBUG("AmCam deleted!");
}

bool AmCam::init(void)
{
  if (AM_UNLIKELY(!mIsCamInited && mIsCamCreated)) {
    VDeviceParameters *vdevConfig = mConfig->vdevice_config();
    if (vdevConfig) {
      for (uint32_t i = 0; i < vdevConfig->vin_number; ++i) {
        mVideoDevice->set_vin_config(mConfig->vin_config(i), i);
      }

      for (uint32_t i = 0; i < vdevConfig->vout_number; ++i) {
        mVideoDevice->set_vout_config(mConfig->vout_config(i), i);
      }

      mVideoDevice->set_encoder_config(mConfig->encoder_config());

      for (uint32_t i = 0; i < vdevConfig->stream_number; ++i) {
        mVideoDevice->set_stream_config(mConfig->stream_config(i), i);
      }
      mIsCamInited = true;
    } else {
      ERROR("Failed to load VideoDevice's configurations!");
    }
  } else if (!mIsCamCreated) {
    ERROR("camera is not created yet.");
  }
  return mIsCamInited;
}

uint32_t AmCam::get_stream_max_num(void)
{
  return init() ? mStreamMaxNumber : 0;
}

bool AmCam::is_stream_encoding(uint32_t streamId)
{
  return init() ? mVideoDevice->is_stream_encoding(streamId) : false;
}

bool AmCam::get_stream_size(uint32_t streamId, EncodeSize *pSize)
{
  return init() ? mVideoDevice->get_stream_size(streamId, pSize) : false;
}

bool AmCam::set_stream_size(uint32_t id, const EncodeSize *pSize)
{
  return init() ? mVideoDevice->set_stream_size_streamid(id, pSize) : false;
}

bool AmCam::set_stream_size_all(uint32_t num, const EncodeSize *pSize)
{
  return init() ? mVideoDevice->set_stream_size_all(num, pSize) : false;
}

bool AmCam::add_overlay_bitmap(uint32_t streamId, uint32_t areaId,
                                Point offset, const char *bmpfile)
{
  bool ret = false;
  if (init()) {
    if (mOverlayTimeTask) {
      mOverlayTimeTask->remove(streamId, areaId);
    }
    EncodeSize size;
    uint32_t overlay_max_size =
        mVideoDevice->get_stream_overlay_max_size(streamId, areaId);
    mVideoDevice->get_stream_size(streamId, &size);
    AmOverlayGenerator *overlay_generator = new AmOverlayGenerator(
        overlay_max_size);
    Overlay *overlay = overlay_generator->create_bitmap(size, offset, bmpfile);
    if (overlay) {
      ret = mVideoDevice->set_stream_overlay(streamId, areaId, overlay);
    } else {
      ERROR("Stream%u Area%u: failed to create bitmap overlay.", streamId,
            areaId);
    }
    delete overlay_generator;
  }
  return ret;
}

bool AmCam::add_overlay_text(uint32_t streamId, uint32_t areaId,
                              Point offset, TextBox *textbox, char *pText)
{
  bool ret = false;
  if (init()) {
    if (mOverlayTimeTask) {
      mOverlayTimeTask->remove(streamId, areaId);
    }
    EncodeSize size;
    uint32_t overlay_max_size =
        mVideoDevice->get_stream_overlay_max_size(streamId, areaId);
    mVideoDevice->get_stream_size(streamId, &size);
    AmOverlayGenerator *overlay_generator = new AmOverlayGenerator(
        overlay_max_size);
    Overlay *pOverlay = overlay_generator->create_text(size, offset, textbox,
                                                      pText);
    if (pOverlay) {
      ret = mVideoDevice->set_stream_overlay(streamId, areaId, pOverlay);
    } else {
      ERROR("Stream%u Area%u: failed to create text OSD.", streamId, areaId);
    }
    delete overlay_generator;
  }
  return ret;
}

bool AmCam::add_overlay_time(uint32_t streamId, uint32_t areaId, Point offset,
                              TextBox *textbox)
{
  bool ret = false;
  if (init()) {
    if (mOverlayTimeTask) {
      mOverlayTimeTask->remove(streamId, areaId);
    }
    ret = create_overlaytime_task()
        && mOverlayTimeTask->add(streamId, areaId, offset, textbox)
        && mOverlayTimeTask->run();
  }
  return ret;

}

bool AmCam::remove_overlay(uint32_t streamId, uint32_t areaId)
{
  bool ret = false;
  if (init()) {
    Overlay overlay;
    if (mOverlayTimeTask) {
      mOverlayTimeTask->remove(streamId, areaId);
    }

    memset(&overlay, 0, sizeof(Overlay));
    overlay.enable = 0;
    ret = mVideoDevice->set_stream_overlay(streamId, areaId, &overlay);
  }
  return ret;
}

bool AmCam::create_overlaytime_task()
{
  if (!mOverlayTimeTask) {
    mOverlayTimeTask = new AmOverlayTimeTask(mStreamMaxNumber, mVideoDevice,
                                             NULL);
  }
  if (mOverlayTimeTask != NULL) {
    DEBUG("Create overlay time task.");
    return true;
  } else {
    DEBUG("Failed to create overlay time task.");
    return false;
  }
}

bool AmCam:: destroy_overlaytime_task()
{
  if (mOverlayTimeTask) {
    delete mOverlayTimeTask;
    mOverlayTimeTask = NULL;
  }
  DEBUG("Destroy overlay time task.");
  return mOverlayTimeTask == NULL;
}

bool AmCam::set_privacy_mask(PRIVACY_MASK * pm_in)
{
  return false;
}

bool AmCam::set_digital_ptz(DPTZParam *dptz_set)
{
  return false;
}

bool AmCam::get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get)
{
  return false;
}

bool AmCam::get_privacy_mask(uint32_t * pm_in)
{
  return false;
}