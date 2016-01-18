/*******************************************************************************
 * AmMdAlgoMog2.cpp
 *
 * Histroy:
 *2014-1-6[HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <string>

#include "am_include.h"
#include "am_utility.h"
#include "am_data.h"
#include "am_vdevice.h"
#include "am_config.h"


#include "mdet_lib.h"
#include "AmMdCam.h"

#include "am_motion_detect.h"
#include "AmMdAlgoMog2.h"

#define CAMERA_VIN_CONFIG "/etc/camera/video_vin.conf"

static inline void debug_mog2_cfg(am_md_mog2_cfg_t*);
static inline void dump_mog2_roi(int roi_idx, mdet_roi_t* p);

static mog2_attr_t default_mog2_attr = {
  MDET_T_DEFAULT,     //T
};

static mog2_roi_attr_t default_roi_attr = {
  1,                                                //output motion
  AM_MD_SENSITIVITY_HIGH,                         //sensitivity
  AM_MD_THRESHOLD_LOW,                           //threshold
  (AM_MD_THRESHOLD_LOW * AM_MD_THRESHOLD_LOW),//mog2_thres
  1,                                                //valid
};

static mog2_roi_scale_t default_roi_scale = {
  0, MD_MOG2_MAX_SCALE,
  0, MD_MOG2_MAX_SCALE,
};

#define MOG2_SKIP_CNT 10

#if 0
static am_md_mog2_cfg_t initial_mog2_cfg = {
  0,    //output motion
};
static am_md_roi_info_t initial_roi = {
  4,12,
  12,4, //Note: this different than mse, since mog2's y grows upward
  AM_MD_THRESHOLD_MEDIUM,
  AM_MD_SENSITIVITY_MEDIUM,
  1,
  &initial_mog2_cfg,
};
#endif

AmMdAlgoMog2::AmMdAlgoMog2(AmMdCam* cam): AmMdAlgo(AM_MD_ALGO_MOG2),
                                              mdCam(cam),
                                              ybuf(NULL)
{
#if defined(CONFIG_ARCH_S2) || defined(CONFIG_ARCH_A9)
  bufType = AM_IAV_BUF_ME1_MAIN;
#else
  bufType = AM_IAV_BUF_ME1_SECOND;
#endif

  memset(&mog2_roi, 0, sizeof(mog2_roi));
  memset(&motion_status, 0, sizeof(uint8_t)*AM_MD_MAX_ROI_NUM);
  memset(&mdet_session, 0, sizeof(mdet_session));

  //mog2 attr
  memcpy(&attr, &default_mog2_attr, sizeof(mog2_attr_t));
  for (int i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    memcpy(&mog2_roi.scale[i], &default_roi_scale, sizeof(mog2_roi_scale_t));
    memcpy(&mog2_roi.attr[i], &default_roi_attr, sizeof(mog2_roi_attr_t));
    if (i != 0) {
      mog2_roi.attr[i].valid = 0;
    }
  }

  createCam = (mdCam == NULL);
}
AmMdAlgoMog2::~AmMdAlgoMog2()
{
  if (status == AM_MD_ALGO_RUNNING) {
    stop();
  }
}

int AmMdAlgoMog2::start(void)
{
  if (status == AM_MD_ALGO_RUNNING) {
    NOTICE("MOG2 is already running\n");
    return 0;
  }

  int ret = 0;
  do {
    if (createCam && get_mdcam() < 0) {
      ERROR("failed to create simple camera\n");
      ret = -1;
      break;
    }
    if (mdCam) {
      if (!(mdCam->get_yuv_data_init())) {
        ERROR("failed to do get_yuv_data_init\n");
        ret = -1;
        break;
      }
    }
    if (!ybuf) {
      ybuf = new YRawFormat;
      memset(ybuf, 0, sizeof(YRawFormat));
    }
    if (get_ybuf() < 0) {
      ERROR("failed to get Y buffer\n");
      ret = -1;
      break;
    }
    if (start_mog2() == 0) {
      status = AM_MD_ALGO_RUNNING;
      NOTICE("MOG2 is created\n");
    }
    else {
      status = AM_MD_ALGO_STOPPED;
      ret = -1;
    }
  } while(0);

  if (ret < 0) {
    ERROR("failed to start mog2\n");
    if (ybuf) {
      delete ybuf;
      ybuf = NULL;
    }
    if (mdCam) {
      mdCam->get_yuv_data_deinit();
    }
    if (createCam) {
      put_mdcam();
    }
  }
  return ret;
}
int AmMdAlgoMog2::stop(void)
{
  if (status == AM_MD_ALGO_STOPPED) {
    NOTICE("MOG2 is not running\n");
    return 0;
  }

  stop_mog2();
  if (ybuf) {
    delete ybuf;
    ybuf = NULL;
  }
  if (mdCam) {
    mdCam->get_yuv_data_deinit();
  }
  if (createCam && put_mdcam() < 0) {
    ERROR("failed to delete simple camera\n");
  }
  status = AM_MD_ALGO_STOPPED;

  NOTICE("MOG2 is stopped\n");
  return 0;
}

inline int AmMdAlgoMog2::start_mog2(uint8_t ref, uint8_t roi)
{
  if (refresh(ref, roi) < 0) {
    ERROR("refresh failed\n");
    return -1;
  }

  if ((ref & DIMENTION)) {
  }

  mdet_session.roi_info.num_roi  = AM_MD_MAX_ROI_NUM;
  if (mdet_start(&mdet_session) < 0) {
    ERROR("mdet_start failed\n");
    return -1;
  }

  return 0;
}
inline int AmMdAlgoMog2::stop_mog2(void)
{
  mdet_stop(&mdet_session);
  return 0;
}
int AmMdAlgoMog2::get_mdcam(void)
{
  int ret = 0;

  if (mdCam) {
    return ret;
  }

  AmConfig* config = new AmConfig();
  config->set_vin_config_path(CAMERA_VIN_CONFIG);

  if (config && config->load_vdev_config()) {
    VDeviceParameters *vdevConfig = config->vdevice_config();

    if (vdevConfig) {
      mdCam = new AmMdCam(vdevConfig);
      for (uint32_t i = 0; i < vdevConfig->vin_number; ++ i) {
        mdCam->set_vin_config(config->vin_config(i), i);
      }
      for (uint32_t i = 0; i < vdevConfig->vout_number; ++ i) {
        mdCam->set_vout_config(config->vout_config(i), i);
      }
      mdCam->set_encoder_config(config->encoder_config());
      for (uint32_t i = 0; i < vdevConfig->stream_number; ++ i) {
        mdCam->set_stream_config(config->stream_config(i), i);
      }

      ret = 0;
    } else {
      ERROR("Faild to get VideoDevice's configurations!");
      ret = -1;
    }
  } else {
    ERROR("Failed to load configurations!");
    ret = -1;
  }

  delete config;
  return ret;
}
inline int AmMdAlgoMog2::put_mdcam(void)
{
  if (mdCam) {
    delete mdCam;
    mdCam = NULL;
  }
  return 0;
}
inline int AmMdAlgoMog2::get_ybuf(void)
{
  if (check_pointer(ybuf) < 0) {
    return -1;
  }
  if (mdCam && mdCam->get_y_raw_data(ybuf, (IavBufType) bufType)) {
    if (ybuf->width  != mdet_session.fm_dim.width  ||
        ybuf->height != mdet_session.fm_dim.height ||
        ybuf->pitch  != mdet_session.fm_dim.pitch) {
      //yuv buffer size changed, need to refresh DIMENTION && ROI_AREA
      return 1;
    }
    return 0;
  }
  else {
    return -1;
  }
}


am_md_event_msg_t* AmMdAlgoMog2::check_motion(int* msg_num)
{
  if (check_pointer(msg_num) < 0) {
    return NULL;
  }

  uint8_t motion_type[AM_MD_MAX_ROI_NUM];
  int ret = 0;
  static int skip_cnt = MOG2_SKIP_CNT;

  if (skip_cnt-- > 0) {
    *msg_num = 0;
    return NULL;
  } else {
    skip_cnt = MOG2_SKIP_CNT;
  }

  memset(motion_type, 0, sizeof(uint8_t)*AM_MD_MAX_ROI_NUM);
  *msg_num = 0;

  if ((ret = get_ybuf()) != 0) {
    if (ret == 1) {
      stop_mog2();
      if (start_mog2(DIMENTION, AM_MD_MAX_ROI_NUM) < 0) {
        return NULL;
      }
    }
    else {
      ERROR("failed to get Y buffer\n");
      return NULL;
    }
  }

  mdet_update_frame(&mdet_session, ybuf->y_addr, attr.T);
  for (int i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    if (mog2_roi.attr[i].valid == 0) {
      continue;
    }

    if (mog2_roi.attr[i].mog2_motion_output_enable) {
      (*msg_num)++;
    }

    mdet_session.motion[i] *= MAX_MOTION_INDICATOR;
    mdet_session.motion[i] =
      (mdet_session.motion[i] * mog2_roi.attr[i].sensitivity)
      / AM_MD_MAX_SENSITIVITY;

    if (motion_status[i] == MD_NO_MOTION) {
      if (mdet_session.motion[i] >= mog2_roi.attr[i].mog2_thres) {
        motion_status[i]  = MD_IN_MOTION;
        motion_type[i]    = AM_MD_EVENT_MOTION_START;
        (*msg_num)++;
      }
    }
    else {
      if (mdet_session.motion[i] < mog2_roi.attr[i].mog2_thres) {
        motion_status[i]  = MD_NO_MOTION;
        motion_type[i]    = AM_MD_EVENT_MOTION_END;
        (*msg_num)++;
      }
    }
  }

  am_md_event_msg_t* p = &md_msgs[0];

  for (int i = 0; i < AM_MD_MAX_ROI_NUM; i++) {
    if (mog2_roi.attr[i].valid == 0) {
      continue;
    }

    if (mog2_roi.attr[i].mog2_motion_output_enable) {
      p->algo_type  = AM_MD_ALGO_MOG2;
      p->roi_idx    = i;
      p->motion     = mdet_session.motion[i];
      p->timestamp  = time(NULL);
      p->motion_type= AM_MD_EVENT_MOTION_DIFF;
      p++;
    }

    if (motion_type[i] == AM_MD_EVENT_MOTION_START ||
        motion_type[i] == AM_MD_EVENT_MOTION_END) {
      p->algo_type    = AM_MD_ALGO_MOG2;
      p->roi_idx      = i;
      p->motion       = mdet_session.motion[i];
      p->timestamp    = time(NULL);
      p->motion_type  = motion_type[i];
      p++;
    }
  }

  return &md_msgs[0];
}
int AmMdAlgoMog2::set_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi_info)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  if (check_pointer(roi_info) < 0) {
    return -1;
  }

  debug_md_roi_info(roi_info);
  debug_mog2_cfg(&roi_info->mog2_cfg);

  mog2_roi_scale_t* scale = &mog2_roi.scale[roi_idx];
  scale->s_left   = roi_info->left;
  scale->s_right  = roi_info->right;
  scale->s_low    = roi_info->top;
  scale->s_high   = roi_info->bottom;

  mog2_roi_attr_t* roi_attr = &mog2_roi.attr[roi_idx];
  roi_attr->valid       = roi_info->valid;

  check_threshold(&roi_info->threshold);
  roi_attr->threshold   = roi_info->threshold;

  check_sensitivity(&roi_info->sensitivity);
  roi_attr->sensitivity = roi_info->sensitivity;

  roi_attr->mog2_motion_output_enable =
    roi_info->mog2_cfg.mog2_motion_output_enable != 0? 1:0;

  if (stop_mog2() < 0) {
    return -1;
  }
  if (start_mog2(ROI_AREA | ROI_ATTR, roi_idx) < 0) {
    return -1;
  }

  return 0;
}
int AmMdAlgoMog2::get_roi_info(uint8_t roi_idx, am_md_roi_info_t* roi_info)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  if (check_pointer(roi_info) < 0) {
    return -1;
  }

  mog2_roi_scale_t* scale = &mog2_roi.scale[roi_idx];
  mog2_roi_attr_t* roi_attr   = &mog2_roi.attr[roi_idx];

  roi_info->left          = scale->s_left;
  roi_info->right         = scale->s_right;
  roi_info->top           = scale->s_low;
  roi_info->bottom        = scale->s_high;
  roi_info->threshold     = roi_attr->threshold;
  roi_info->sensitivity   = roi_attr->sensitivity;
  roi_info->valid         = roi_attr->valid;
  roi_info->mog2_cfg.mog2_motion_output_enable
                          = roi_attr->mog2_motion_output_enable;

  debug_md_roi_info(roi_info);
  debug_mog2_cfg(&roi_info->mog2_cfg);
  return 0;
}
int AmMdAlgoMog2::set_sensitivity(uint8_t roi_idx, uint32_t sensitivity)
{
  if (check_roi_idx(roi_idx) < 0) {
      return -1;
  }

  check_sensitivity(&sensitivity);

  mog2_roi_attr_t* roi_attr   = &mog2_roi.attr[roi_idx];
  roi_attr->sensitivity       = sensitivity;

  return 0;
}
int AmMdAlgoMog2::get_sensitivity(uint8_t roi_idx, uint32_t* sensitivity)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }
  if (check_pointer(sensitivity) < 0) {
    return -1;
  }

  mog2_roi_attr_t* roi_attr   = &mog2_roi.attr[roi_idx];
  *sensitivity                = roi_attr->sensitivity;

  DEBUG("MOG2:get ROI[%d] sensitivity %d\n", roi_idx, *sensitivity);
  return 0;
}
int AmMdAlgoMog2::set_threshold(uint8_t roi_idx, uint32_t threshold)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }

  check_threshold(&threshold);

  mog2_roi_attr_t* roi_attr   = &mog2_roi.attr[roi_idx];
  roi_attr->threshold         = threshold;
  roi_attr->mog2_thres        = threshold * threshold;

  return 0;
}
int AmMdAlgoMog2::get_threshold(uint8_t roi_idx, uint32_t* threshold)
{
  if (check_roi_idx(roi_idx) < 0) {
    return -1;
  }
  if (check_pointer(threshold) < 0) {
    return -1;
  }

  mog2_roi_attr_t* roi_attr   = &mog2_roi.attr[roi_idx];
  *threshold                  = roi_attr->threshold;

  DEBUG("MOG2:get ROI[%d] threshold %d\n", roi_idx, *threshold);
  return 0;
}

int AmMdAlgoMog2::refresh(uint8_t scope, uint8_t r)
{
  if (check_dimension(&mdet_session.fm_dim) < 0) {
    return -1;
  }

  int i = 0;

  scope &= REFRESH_ALL;

  if (scope & MOG2_ATTR) {
  }
  if (scope & DIMENTION) {
    mdet_session.fm_dim.width   = ybuf->width;
    mdet_session.fm_dim.height  = ybuf->height;
    mdet_session.fm_dim.pitch   = ybuf->pitch;
    //update ROI_AREA of all rois
    scope |= ROI_AREA;
    r = AM_MD_MAX_ROI_NUM;
    DEBUG("dimension: w %d, h %d, p %d\n", ybuf->width, ybuf->height,
                                           ybuf->pitch);
  }

  do {
    if (r < AM_MD_MAX_ROI_NUM && i != r) {
      i++;
      continue;
    }

    if (scope & ROI_AREA) {
      if (check_scale(&mog2_roi.scale[i]) < 0) {
        return -1;
      }
      calc_roi_area(&mdet_session.fm_dim, &mog2_roi.scale[i],
                    &mdet_session.roi_info.roi[i]);
    }
    if (scope & ROI_ATTR) {
      mog2_roi.attr[i].mog2_thres =
        (mog2_roi.attr[i].threshold) * (mog2_roi.attr[i].threshold);
    }

    dump_mog2_roi(i, &mdet_session.roi_info.roi[i]);
    i++;
  } while (i < AM_MD_MAX_ROI_NUM);

  return 0;
}
inline int AmMdAlgoMog2::check_dimension(mdet_dimension_t* d)
{
  if (check_pointer(d) < 0) {
    return -1;
  }
  return 0;
}
inline int AmMdAlgoMog2::check_scale(mog2_roi_scale_t* s)
{
  if (check_pointer(s) < 0) {
    return -1;
  }

  if (s->s_left > s->s_right || s->s_right > MD_MOG2_MAX_SCALE) {
    ERROR("scale not valid: left %d, right %d (max scale = %d)\n", s->s_left,
          s->s_right, MD_MOG2_MAX_SCALE);
    return -1;
  }
  if (s->s_low > s->s_high || s->s_high > MD_MOG2_MAX_SCALE) {
    ERROR("scale not valid: low %d, high %d (max scale = %d)\n", s->s_low,
          s->s_high, MD_MOG2_MAX_SCALE);
    return -1;
  }

  return 0;
}
inline void AmMdAlgoMog2::calc_roi_area(mdet_dimension_t* d,
                                        mog2_roi_scale_t* s, mdet_roi_t* roi)
{
  uint32_t left   = d->width *  s->s_left  / MD_MOG2_MAX_SCALE;
  uint32_t right  = d->width  * s->s_right / MD_MOG2_MAX_SCALE;
  uint32_t top    = d->height * s->s_low   / MD_MOG2_MAX_SCALE;
  uint32_t bottom = d->height * s->s_high  / MD_MOG2_MAX_SCALE;

  right = (right >= d->width ? (d->width - 1) : right);
  bottom = (bottom >= d->height ? (d->height - 1) : bottom);

  roi->num_points = 4;

  roi->points[0].x = left;
  roi->points[0].y = top;

  roi->points[1].x = right;
  roi->points[1].y = top;

  roi->points[2].x = right;
  roi->points[2].y = bottom;

  roi->points[3].x = left;
  roi->points[3].y = bottom;
}

static inline void debug_mog2_cfg(am_md_mog2_cfg_t* p)
{
  if (AM_LIKELY(p)) {
    DEBUG("MD::mog2 cfg {motion_output_enable %d}\n",
          p->mog2_motion_output_enable);
  }
}
static inline void dump_mog2_roi(int roi_idx, mdet_roi_t* p)
{
  if (AM_LIKELY(p)) {
    DEBUG("MD::mog2 roi[%d] {left %u right %u top %u bottom %u}\n",
          roi_idx, p->points[0].x, p->points[1].x, p->points[0].y,
          p->points[3].y);
  }
}

AmMdAlgo* get_instance(void)
{
  return new AmMdAlgoMog2;
}

void free_instance(AmMdAlgo* algo)
{
  if (algo) {
    delete algo;
    algo = NULL;
  }
}
