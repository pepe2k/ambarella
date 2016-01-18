/*******************************************************************************
 * am_fisheyecam.cpp
 *
 * History:
 *  Mar 20, 2013 2013 - [qianshen] created file
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
#include "am_dewarp.h"
#include "am_middleware.h"


static const char *mountMode[AM_FISHEYE_MOUNT_TOTAL_NUM] = { "Wall",
    "Ceiling", "Desktop", };

static const char *dewarpMode[AM_TRANSFORM_MODE_TOTAL_NUM] = { "No Transform",
    "Normal", "Panorama", "Subregion", };

AmFisheyeCam::AmFisheyeCam(AmConfig *config) :
        AmCam(),
        mUsedWarpAreaNumber(0),
        mWarpDevice(NULL),
        mTransform(NULL)
{
  memset(mRegionToArea, 0, sizeof(RegionToArea));

  if ((false == mIsCamCreated) && config) {
    if (config->load_vdev_config() && config->load_fisheye_config()) {
      mWarpDevice = new AmWarpDevice(config->vdevice_config());
      mVideoDevice = mWarpDevice;
      mTransform = new AmFisheyeTransform();
      mConfig = config;
      mIsCamCreated = (mWarpDevice && mTransform ? true : false);
    } else {
      ERROR("Failed to load VideoDevice or fisheye's config.");
    }
  } else {
     ERROR("Invalid AmConfig!");
  }
}

AmFisheyeCam::~AmFisheyeCam()
{
  delete mTransform;

  DEBUG("AmFisheyeCam deleted!");
}

bool AmFisheyeCam:: init(void)
{
  if (!mIsCamInited) {
	if (AmCam::init()) {
      mWarpDevice->get_max_stream_num(&mStreamMaxNumber);
   }
  }
  return mIsCamInited;
}

bool AmFisheyeCam::start(FisheyeParameters *fisheye_config)
{
  bool ret = true;
  if (mIsCamCreated) {
    memset(mRegionToArea, 0, sizeof(mRegionToArea));
    mUsedWarpAreaNumber = 0;
    mWarpDevice->set_warp_config(&fisheye_config->layout);
    mTransform->set_transform_config(mConfig->fisheye_config());
    if (!mWarpDevice->start(true)) {
        ERROR("warp device failed to start.");
        ret = false;
    } else if (!mTransform->init()) {
        ERROR("dewarp failed to init.");
        ret = false;
    } else {
        INFO("Fisheyecam starts.");
    }
  } else {
    ret = false;
    ERROR("AmFisheyeCam not initialized.");
  }

  return ret;
}

bool AmFisheyeCam::start_encode()
{
  return init() && stop_encode() && mWarpDevice->start_encode();
}

bool AmFisheyeCam::stop_encode()
{
  bool ret = false;
  if (init()) {
    if (mWarpDevice->stop_encode()) {
      for (uint32_t streamId = 0; streamId < mStreamMaxNumber; ++streamId) {
        for (uint32_t areaId = 0; areaId < MAX_OVERLAY_AREA_NUM; ++areaId) {
          remove_overlay(streamId, areaId);
        }
      }
      destroy_overlaytime_task();
      ret = true;
    }
  }
  return ret;
}

bool AmFisheyeCam::get_stream_type(uint32_t streamId, EncodeType *pType)
{
  return init() && mWarpDevice->get_stream_type(streamId, pType);
}

bool AmFisheyeCam::set_stream_type(uint32_t streamId, EncodeType type)
{
  return init() && mWarpDevice->set_stream_type(streamId, type);
}

bool AmFisheyeCam::get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate)
{
  return init() && mWarpDevice->get_stream_framerate(streamId, pFrameRate);
}

bool AmFisheyeCam::set_stream_framerate(uint32_t streamId, const uint32_t frameRate)
{
  return init() && mWarpDevice->set_stream_framerate(streamId, frameRate);
}

bool AmFisheyeCam::set_stream_N(uint32_t streamId, uint16_t n)
{
  return init() && mWarpDevice->set_stream_n(streamId, n);
}

bool AmFisheyeCam::get_stream_N(uint32_t streamId, uint16_t* n)
{
  return init() && mWarpDevice->get_stream_n(streamId, n);
}

bool AmFisheyeCam::set_stream_idr(uint32_t streamId, uint8_t idr_interval)
{
  return init() && mWarpDevice->set_stream_idr(streamId, idr_interval);
}

bool AmFisheyeCam::get_stream_idr(uint32_t streamId, uint8_t *idr_interval)
{
  return init() && mWarpDevice->get_stream_idr(streamId, idr_interval);
}

bool AmFisheyeCam::set_mjpeg_quality(uint32_t streamId, uint8_t quality)
{
  return init() && mWarpDevice->set_mjpeg_quality(streamId, quality);
}

bool AmFisheyeCam::get_mjpeg_quality(uint32_t streamId, uint8_t* quality)
{
  return init() && mWarpDevice->get_mjpeg_quality(streamId, quality);
}

bool AmFisheyeCam::set_stream_profile(uint32_t streamId, uint8_t profile)
{
  return init() && mWarpDevice->set_stream_profile(streamId, profile);
}

bool AmFisheyeCam::get_stream_profile(uint32_t streamId, uint8_t *profile)
{
  return init() && mWarpDevice->get_stream_profile(streamId, profile);
}

bool AmFisheyeCam::set_cbr_bitrate(uint32_t streamId, uint32_t bitrate)
{
  return init() && mWarpDevice->set_cbr_bitrate(streamId, bitrate);
}

bool AmFisheyeCam::get_cbr_bitrate(uint32_t streamId, uint32_t* pBitrate)
{
  return init() && mWarpDevice->get_cbr_bitrate(streamId, pBitrate);
}

bool AmFisheyeCam::set_privacy_mask(PRIVACY_MASK * pm_in)
{
  return init() ? mWarpDevice->set_pm_param(pm_in) : false;
}

bool AmFisheyeCam::get_privacy_mask(uint32_t * pm_in)
{
  return init() ? mWarpDevice->get_pm_param(pm_in) : false;
}

bool AmFisheyeCam::set_digital_ptz(DPTZParam *dptz_set)
{
  return init() ? mWarpDevice->set_dptz_param(dptz_set) : false;
}

bool AmFisheyeCam::set_stream_size_all(uint32_t totalNum, const EncodeSize *pSize)
{
  return init() && mWarpDevice->set_stream_size_all(totalNum, pSize);
}

bool AmFisheyeCam::get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get)
{
  return false;
}

bool AmFisheyeCam::set_transform_mode_all(uint32_t total_num, TransformMode *modes)
{
  uint32_t total_area_num = MAX_WARP_AREA_NUM + 1;
  if (init()) {
    if (total_num <= MAX_FISHEYE_REGION_NUM) {
      total_area_num = 0;
      for (uint32_t i = 0; i < total_num; ++i) {
        total_area_num += mTransform->get_max_area_num(modes[i]);
      }
      if (total_area_num > MAX_WARP_AREA_NUM) {
        ERROR("Total %d warp areas exceeds the max warp area number %d.",
              total_area_num,
              MAX_WARP_AREA_NUM);
      } else {
        for (uint32_t i = 0; i < total_num; ++i) {
          mRegionToArea[i].mode = modes[i];
          mRegionToArea[i].area_map = 0;
          mRegionToArea[i].area_num = 0;
          DEBUG("Region %u: %s", i, dewarpMode[mRegionToArea[i].mode]);
        }
        mUsedWarpAreaNumber = 0;
      }
    }
  }
  return (total_area_num <= MAX_WARP_AREA_NUM);
}

bool AmFisheyeCam::set_transform_region(uint32_t total_num,
                                      TransformParameters *pTrans)
{
  bool                ret = false;
  if (init()) {
    uint32_t total_area_num = 0;
    uint32_t assign_area_id = mUsedWarpAreaNumber;
    uint32_t active_area_num = 0;
    uint32_t ctrl_id = 0;
    uint32_t first_ctrl_id[MAX_WARP_AREA_NUM];
    WarpControl warp_ctrl[MAX_WARP_AREA_NUM];
    TransformParameters *trans = NULL;

    memset(first_ctrl_id, MAX_WARP_AREA_NUM, sizeof(first_ctrl_id));

    for (uint32_t i = 0; i < total_num; ++i) {
      trans = &pTrans[i];
      if (trans != NULL && is_valid_region_id(trans->id)) {
        if (AM_UNLIKELY(!mRegionToArea[trans->id].area_num)) {
          mRegionToArea[trans->id].area_num =
              mTransform->get_max_area_num(mRegionToArea[trans->id].mode);
        }
        total_area_num += mRegionToArea[trans->id].area_num;
        if (!mRegionToArea[trans->id].area_map) {
          for (uint32_t id = 0; id < mRegionToArea[trans->id].area_num; ++id) {
            mRegionToArea[trans->id].area_map |= (1 << assign_area_id);
//          DEBUG("Region%u: assign area id %u, map %x.",
//                trans->id,  assign_area_id,
//                mRegionToArea[trans->id].area_map);
            ++assign_area_id;
          }
        }
        uint32_t area_id = 0;
        while (area_id < MAX_WARP_AREA_NUM) {
          if (mRegionToArea[trans->id].area_map & (1 << area_id)) {
            if (first_ctrl_id[trans->id] >= MAX_WARP_AREA_NUM) {
              first_ctrl_id[trans->id] = ctrl_id;
            }
            warp_ctrl[ctrl_id].id = area_id;
//          DEBUG("warp_ctrl%u: id = %u", ctrl_id, area_id);
            mWarpDevice->get_warp_control(&warp_ctrl[ctrl_id]);
            ctrl_id++;
          }
          ++area_id;
        }
//      DEBUG("Region%u: first ctrl id %u, area_id %u, hor %x, ver %x.",
//            trans->id, first_ctrl_id[trans->id],
//            warp_ctrl[first_ctrl_id[trans->id]].id,
//            (uint32_t )warp_ctrl[first_ctrl_id[trans->id]].hor_map.addr,
//            (uint32_t )warp_ctrl[first_ctrl_id[trans->id]].ver_map.addr);
        if ((active_area_num =
            mTransform->create_warp_control(&warp_ctrl[first_ctrl_id[trans->id]],
                                          mRegionToArea[trans->id].mode,
                                          trans))
            == 0) {
          ERROR("Region%d [%s %s]: Failed to create warp control.", trans->id,
                mountMode[mTransform->get_mount_mode()],
                dewarpMode[mRegionToArea[trans->id].mode]);
          break;
        } else {
          for (uint32_t j = active_area_num;
              j < mRegionToArea[trans->id].area_num; ++j) {
            uint32_t unused_ctrl_id = j + first_ctrl_id[trans->id];
            uint32_t tmp = warp_ctrl[unused_ctrl_id].id;
            // Erase the content of warp area expect area id if unused.
            memset(&warp_ctrl[unused_ctrl_id], 0,
                   sizeof(warp_ctrl[unused_ctrl_id]));
            warp_ctrl[unused_ctrl_id].id = tmp;
          }

          INFO("Region%d [%s %s]: Create warp control %u areas.", trans->id,
               mountMode[mTransform->get_mount_mode()],
               dewarpMode[mRegionToArea[trans->id].mode], active_area_num);
        }
      } else if (!trans) {
        ERROR("Invalid %uth Transform parameter.",
              i);
        return ret;
      } else {
        ERROR("Invalid dewarp region ID %u. Max is %u.",
              trans->id,
              MAX_FISHEYE_REGION_NUM - 1);
        return ret;
      }
    }
    mUsedWarpAreaNumber = assign_area_id;

    if (mWarpDevice->set_warp_control(total_area_num, warp_ctrl)) {
      ret = true;
    }
  }
  return ret;
}

bool AmFisheyeCam::is_valid_region_id(uint32_t regionId)
{
  return init() && (regionId < MAX_FISHEYE_REGION_NUM);
}
