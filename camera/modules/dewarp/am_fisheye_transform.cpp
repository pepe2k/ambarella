/*******************************************************************************
 * am_fisheye_transform.cpp
 *
 * History:
 *  Mar 21, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
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
#include "lib_dewarp.h"
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_dewarp.h"

AmFisheyeTransform::AmFisheyeTransform():
    IsInited(false)
{
  mFisheyeParams = new FisheyeParameters();
  memset(&mConfig, 0, sizeof(fisheye_config_t));
  memset(mVector, 0, sizeof(mVector));
  memset(&mRegion, 0, sizeof(mRegion));
}

AmFisheyeTransform::~AmFisheyeTransform()
{
  DEBUG("~AmFisheyeTransform");
}

void AmFisheyeTransform::set_transform_config(FisheyeParameters *config)
{
  if (config) {
    mFisheyeParams = config;
  } else if (!config) {
    ERROR("Invalid fisheye config.");
  }
}

bool AmFisheyeTransform::init()
{
  dewarp_init_t setup;
  IsInited = false;
  switch (mFisheyeParams->projection) {
    case AM_FISHEYE_PROJECTION_FTAN:
      setup.projection_mode = PROJECTION_STEREOGRAPHIC;
      break;
    case AM_FISHEYE_PROJECTION_FTHETA:
    default:
      setup.projection_mode = PROJECTION_EQUIDISTANT;
      break;
  }

  setup.max_fov = (degree_t) mFisheyeParams->max_fov;
  setup.max_radius = mFisheyeParams->max_circle
      * mFisheyeParams->layout.unwarp.width
      / mFisheyeParams->layout.unwarp_window.width / 2;
  setup.max_input_width = mFisheyeParams->layout.unwarp.width;
  setup.max_input_height = mFisheyeParams->layout.unwarp.height;
  setup.lens_center_in_max_input.x = mFisheyeParams->layout.unwarp.width / 2;
  setup.lens_center_in_max_input.y = mFisheyeParams->layout.unwarp.height / 2;
  IsInited = (dewarp_init(&setup) == 0);
  fisheye_get_config(&mConfig);
  if (IsInited)
    INFO("fisheye transform initialized.");
  else
    ERROR("Failed to initialize fisheye transform.");
  return IsInited;
}

uint32_t AmFisheyeTransform::get_max_area_num(TransformMode mode)
{
  uint32_t required_area_num = 0;
  if (IsInited) {
    switch (mFisheyeParams->mount) {
      case AM_FISHEYE_MOUNT_WALL:
        switch (mode) {
          case AM_TRANSFORM_MODE_NONE:
          case AM_TRANSFORM_MODE_SUBREGION:
            required_area_num = 1;
            break;
          case AM_TRANSFORM_MODE_NORMAL:
              required_area_num =
                      mConfig.wall_normal_max_area_num ;
            break;
          case AM_TRANSFORM_MODE_PANORAMA:
            required_area_num = mConfig.wall_panor_max_area_num;
            break;
          default:
            ERROR("Unknown fisheye mode %d.", mode);
            break;
        }
        break;
      case AM_FISHEYE_MOUNT_CEILING:
        switch (mode) {
          case AM_TRANSFORM_MODE_NONE:
            required_area_num = 1;
            break;
          case AM_TRANSFORM_MODE_SUBREGION:
            required_area_num =  mConfig.ceiling_sub_max_area_num;
            break;
          case AM_TRANSFORM_MODE_NORMAL:
              required_area_num = mConfig.ceiling_normal_max_area_num;
            break;
          case AM_TRANSFORM_MODE_PANORAMA:
            required_area_num = mConfig.ceiling_panor_max_area_num;
            break;
          default:
            ERROR("Unknown fisheye mode %d.", mode);
            break;
        }
        break;
      case AM_FISHEYE_MOUNT_DESKTOP:
        switch (mode) {
          case AM_TRANSFORM_MODE_NONE:
            required_area_num = 1;
            break;
          case AM_TRANSFORM_MODE_SUBREGION:
            required_area_num =  mConfig.desktop_sub_max_area_num;
            break;
          case AM_TRANSFORM_MODE_NORMAL:
              required_area_num = mConfig.desktop_normal_max_area_num;
            break;
          case AM_TRANSFORM_MODE_PANORAMA:
            required_area_num = mConfig.desktop_panor_max_area_num;
            break;
          default:
            ERROR("Unknown fisheye mode %d.", mode);
            break;
        }
        break;
      default:
        ERROR("Unknow mount mode %d.", mFisheyeParams->mount);
        break;
    }

  } else {
    ERROR("AmFisheyeTransform not initialized.");
  }
  return required_area_num;
}

FisheyeMount AmFisheyeTransform::get_mount_mode()
{
  return mFisheyeParams->mount;
}

uint32_t AmFisheyeTransform:: create_warp_control(WarpControl *ctrl,
                                                const TransformMode mode,
                                                TransformParameters *trans)
{
  uint32_t area_num = 0;
  if (IsInited) {
    switch (mode) {
      case AM_TRANSFORM_MODE_NONE:
        area_num = create_lib_notrans(ctrl, trans);
        break;
      case AM_TRANSFORM_MODE_NORMAL:
        area_num = create_lib_normal(ctrl, trans);
        break;
      case AM_TRANSFORM_MODE_PANORAMA:
        area_num = create_lib_panorama(ctrl, trans);
        break;
      case AM_TRANSFORM_MODE_SUBREGION:
        area_num = create_lib_subregion(ctrl, trans);
        break;
      default:
        ERROR("Invalid transform mode %d.", mode);
        break;
    }

      for (uint32_t i = 0; i < area_num; ++i) {
        WarpControl *p = &ctrl[i];
        p->input.width = mVector[i].input.width;
        p->input.height = mVector[i].input.height;
        p->input_offset.x = mVector[i].input.upper_left.x;
        p->input_offset.y = mVector[i].input.upper_left.y;
        p->output.width = mVector[i].output.width;
        p->output.height = mVector[i].output.height;
        p->output_offset.x = mVector[i].output.upper_left.x;
        p->output_offset.y = mVector[i].output.upper_left.y;
        p->rotate = (mVector[i].rotate_flip & ROTATE ? AM_ROTATE_90 : 0)
            | (mVector[i].rotate_flip & HFLIP ? AM_HORIZONTAL_FLIP : 0)
            | (mVector[i].rotate_flip & VFLIP ? AM_VERTICAL_FLIP: 0);
        p->hor_map.rows = mVector[i].hor_map.rows;
        p->hor_map.cols = mVector[i].hor_map.cols;
        p->hor_map.ver_spacing= mVector[i].hor_map.grid_height;
        p->hor_map.hor_spacing= mVector[i].hor_map.grid_width;
        p->ver_map.rows = mVector[i].ver_map.rows;
        p->ver_map.cols = mVector[i].ver_map.cols;
        p->ver_map.ver_spacing= mVector[i].ver_map.grid_height;
        p->ver_map.hor_spacing= mVector[i].ver_map.grid_width;
      }
  } else {
    ERROR("AmFisheyeTransform not initialized.");
  }
  return area_num;
}

uint32_t AmFisheyeTransform::create_lib_notrans(WarpControl *ctrl,
                                              const TransformParameters *trans)
{
  int32_t used_area_num = 0;

  rect_in_main_t lib_roi;
  lib_roi.width = trans->source.width;
  lib_roi.height = trans->source.height;
  lib_roi.upper_left.x = trans->source.x;
  lib_roi.upper_left.y = trans->source.y;
  used_area_num = fisheye_no_transform(
      get_lib_region(&trans->region, &trans->zoom), &lib_roi,
      get_lib_vector(ctrl, 1));

  return used_area_num < 0 ? 0 : used_area_num;
}

uint32_t AmFisheyeTransform::create_lib_normal(WarpControl *ctrl,
                                             const TransformParameters *normal)
{
  int32_t used_area_num = 0;
  switch (mFisheyeParams->mount) {
    case AM_FISHEYE_MOUNT_WALL:
      used_area_num = fisheye_wall_normal(
           get_lib_region(&normal->region, &normal->zoom),
           get_lib_vector(ctrl, mConfig.wall_normal_max_area_num));
      break;
    case AM_FISHEYE_MOUNT_CEILING:
      used_area_num =
          fisheye_ceiling_normal(
              get_lib_region(&normal->region, &normal->zoom),
              (degree_t) normal->roi_top_angle, get_lib_orient(normal->orient),
              get_lib_vector(ctrl, mConfig.ceiling_normal_max_area_num));
      break;
    case AM_FISHEYE_MOUNT_DESKTOP:
      used_area_num =
          fisheye_desktop_normal(
              get_lib_region(&normal->region, &normal->zoom),
              (degree_t) normal->roi_top_angle, get_lib_orient(normal->orient),
              get_lib_vector(ctrl, mConfig.desktop_normal_max_area_num));
      break;
    default:
      used_area_num = 0;
      break;
  }

  return used_area_num < 0 ? 0 : used_area_num;
}

uint32_t AmFisheyeTransform::create_lib_panorama(WarpControl *ctrl,
                                               const TransformParameters *panor)
{
  int32_t used_area_num = 0;
  switch (mFisheyeParams->mount) {
    case AM_FISHEYE_MOUNT_WALL:
      used_area_num = fisheye_wall_panorama(
          get_lib_region(&panor->region, &panor->zoom),
          (degree_t)panor->hor_angle_range,
          get_lib_vector(ctrl, mConfig.wall_panor_max_area_num));
      break;
    case AM_FISHEYE_MOUNT_CEILING:
      used_area_num =
          fisheye_ceiling_panorama(
              get_lib_region(&panor->region, &panor->zoom),
              (degree_t) panor->roi_top_angle, (degree_t) panor->hor_angle_range,
              get_lib_orient(panor->orient),
              get_lib_vector(ctrl, mConfig.ceiling_panor_max_area_num));
      break;
    case AM_FISHEYE_MOUNT_DESKTOP:
      used_area_num =
          fisheye_desktop_panorama(
              get_lib_region(&panor->region, &panor->zoom),
              (degree_t) panor->roi_top_angle, (degree_t) panor->hor_angle_range,
              get_lib_orient(panor->orient),
              get_lib_vector(ctrl, mConfig.desktop_panor_max_area_num));
      break;
    default:
      used_area_num = 0;
      break;
  }

  return used_area_num < 0 ? 0 : used_area_num;
}

uint32_t AmFisheyeTransform::create_lib_subregion(WarpControl *ctrl,
                                                TransformParameters *sub)
{
  int32_t used_area_num = 0;
  fpoint_t  lib_roi;
  pantilt_angle_t lib_angle = {0};
  lib_roi.x = sub->roi_center.x;
  lib_roi.y = sub->roi_center.y;
  lib_angle.pan = sub->pantilt_angle.pan;
  lib_angle.tilt = sub->pantilt_angle.tilt;
  switch (mFisheyeParams->mount) {
    case AM_FISHEYE_MOUNT_WALL:
      if (sub->pantilt_angle.tilt >= 360) {
        used_area_num =
            fisheye_wall_subregion_roi(get_lib_region(&sub->region, &sub->zoom),
                                       &lib_roi, get_lib_vector(ctrl, 1),
                                       &lib_angle);
        sub->pantilt_angle.tilt = (int)lib_angle.tilt;
        sub->pantilt_angle.pan = (int)lib_angle.pan;
      } else {
        used_area_num =
            fisheye_wall_subregion_angle(get_lib_region(&sub->region, &sub->zoom),
                                         &lib_angle, get_lib_vector(ctrl, 1),
                                         &lib_roi);
        sub->roi_center.x = lib_roi.x;
        sub->roi_center.y = lib_roi.y;
      }
      break;
    case AM_FISHEYE_MOUNT_CEILING:
      if (sub->pantilt_angle.tilt >= 360) {
        used_area_num =
            fisheye_ceiling_subregion_roi(
                get_lib_region(&sub->region, &sub->zoom),
                &lib_roi,
                get_lib_vector(ctrl, mConfig.ceiling_sub_max_area_num),
                &lib_angle);
        sub->pantilt_angle.tilt = (int)lib_angle.tilt;
        sub->pantilt_angle.pan = (int)lib_angle.pan;
      } else {
        used_area_num =
            fisheye_ceiling_subregion_angle(
                get_lib_region(&sub->region, &sub->zoom),
                &lib_angle,
                get_lib_vector(ctrl, mConfig.ceiling_sub_max_area_num),
                &lib_roi);
        sub->roi_center.x = lib_roi.x;
        sub->roi_center.y = lib_roi.y;
      }
      break;
    case AM_FISHEYE_MOUNT_DESKTOP:
      if (sub->pantilt_angle.tilt >= 360) {
        used_area_num =
            fisheye_desktop_subregion_roi(
                get_lib_region(&sub->region, &sub->zoom),
                &lib_roi,
                get_lib_vector(ctrl, mConfig.desktop_sub_max_area_num),
                &lib_angle);
        sub->pantilt_angle.tilt = (int) lib_angle.tilt;
        sub->pantilt_angle.pan = (int)lib_angle.pan;
      } else {
        used_area_num =
            fisheye_desktop_subregion_angle(
                get_lib_region(&sub->region, &sub->zoom),
                &lib_angle,
                get_lib_vector(ctrl, mConfig.desktop_sub_max_area_num),
                &lib_roi);
        sub->roi_center.x = lib_roi.x;
        sub->roi_center.y = lib_roi.y;
      }
      break;
    default:
      used_area_num = 0;
      break;
  }

  return used_area_num < 0 ? 0 : used_area_num;
}

warp_region_t* AmFisheyeTransform::get_lib_region(const Rect *region,
                                                const Fraction *zoom)
{
  mRegion.output.width = region->width;
  mRegion.output.height = region->height;
  mRegion.output.upper_left.x = region->x;
  mRegion.output.upper_left.y = region->y;
  mRegion.zoom.num = (zoom ? zoom->numer : 1);
  mRegion.zoom.denom = (zoom ? zoom->denom : 1);
  return &mRegion;
}

warp_vector_t* AmFisheyeTransform::get_lib_vector(WarpControl *ctrl,
                                                uint32_t areaNum)
{
  for (uint32_t i = 0; i < areaNum; ++i) {
    mVector[i].hor_map.addr = ctrl[i].hor_map.addr;
    mVector[i].ver_map.addr = ctrl[i].ver_map.addr;
  }
  return mVector;
}

ORIENTATION AmFisheyeTransform::get_lib_orient(const FisheyeOrient orient)
{
  ORIENTATION ret;
  switch (orient) {
    case AM_FISHEYE_ORIENT_SOUTH:
      ret = CEILING_SOUTH;
      break;
    case AM_FISHEYE_ORIENT_EAST:
      ret = CEILING_EAST;
      break;
    case AM_FISHEYE_ORIENT_WEST:
      ret = CEILING_WEST;
      break;
    case AM_FISHEYE_ORIENT_NORTH:
    default:
      ret = CEILING_NORTH;
      break;
  }
  return ret;
}
