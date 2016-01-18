/*******************************************************************************
 * am_warp_device.cpp
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
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_vin.h"
#include "am_vout.h"
#include "am_vout_lcd.h"
#include "am_vout_hdmi.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

static const char* vout_type_to_str[] = { "None", "Lcd", "Hdmi", "Cvbs" };

AmWarpDevice::AmWarpDevice(VDeviceParameters *vDeviceSize) :
    AmVideoDevice(),
    mIsWarpMapCreated(false),
    mIsDeviceStarted(false)
{
  for (uint32_t i = 0; i < MAX_WARP_AREA_NUM; ++i) {
    for (uint32_t j = 0; j < WARP_MAP_NUM; ++j) {
      mWarpMap[i][j] = NULL;
    }
  }
  if (!create_video_device(vDeviceSize)) {
    ERROR("Failed to create the components of this video device!");
  }
  instance_pm_dptz = new AmPrivacyMaskDPTZ(mIav, mVinParamList);
}

AmWarpDevice::~AmWarpDevice()
{
  destroy_warp_map();
  delete instance_pm_dptz;
  DEBUG("AmWarpDevice deleted!");
}

bool AmWarpDevice::goto_idle()
{
  return false;
}

bool AmWarpDevice::enter_preview()
{
  return false;
}
bool AmWarpDevice::enter_decode_mode()
{
  return false;
}

bool AmWarpDevice::leave_decode_mode()
{
  return false;
}

bool AmWarpDevice::enter_photo_mode()
{
  return false;
}

bool AmWarpDevice::leave_photo_mode()
{
  return false;
}

bool AmWarpDevice::change_iav_state(IavState target)
{
  return false;
}

bool AmWarpDevice::create_warp_map()
{
  if (!mIsWarpMapCreated) {
    for (uint32_t i = 0; i < MAX_WARP_AREA_NUM; ++i) {
      for (uint32_t j = 0; j < WARP_MAP_NUM; ++j) {
        mWarpMap[i][j] = new int16_t[MAX_GRID_HEIGHT * MAX_GRID_WIDTH];
//        DEBUG("Area%u: warp_map%u = 0x%x", i, j, (uint32_t) mWarpMap[i][j]);
      }
    }
    mIsWarpMapCreated = true;
  }
  return mIsWarpMapCreated;
}

bool AmWarpDevice::destroy_warp_map()
{
  if (mIsWarpMapCreated) {
    for (uint32_t i = 0; i < MAX_WARP_AREA_NUM; ++i) {
      for (uint32_t j = 0; j < WARP_MAP_NUM; ++j) {
        delete mWarpMap[i][j];
        mWarpMap[i][j] = NULL;
      }
    }
    mIsWarpMapCreated = false;
  }
  return !mIsWarpMapCreated;
}

void AmWarpDevice::set_warp_config(WarpParameters *config)
{
  iav_system_resource_setup_ex_t &resource =
      mEncoderParams->system_resource_info;

  // Clear dptz, warp
  memset(&mEncoderParams->warp_control_info, 0, sizeof(mEncoderParams->warp_control_info));
  memset(&mEncoderParams->dptz_main_info, 0, sizeof(mEncoderParams->dptz_main_info));

  /*for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    mStreamParamList[i]->encode_params.encode_format.encode_type = IAV_ENCODE_NONE;
  }*/

  if (resource.encode_mode != IAV_ENCODE_WARP_MODE) {
    resource.encode_mode = IAV_ENCODE_WARP_MODE;
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0)) {
      PERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX: WARP");
    }
  }

  if (config) {
    memcpy(&mEncoderParams->warp_layout, config, sizeof(WarpParameters));
  } else {
    ERROR("Invalid warp layout config.");
  }

 }

bool AmWarpDevice::start(bool force)
{
  if (!mIsDeviceStarted || force) {
    mIsDeviceStarted = change_stream_state(AM_IAV_IDLE)
        && init_device(AmVout::AM_VOUT_INIT_ENABLE_VIDEO, true)
        && change_stream_state(AM_IAV_PREVIEW);
    if (mIsDeviceStarted) {
      INFO("Warp device starts.");
    } else {
      ERROR("Warp device failed to start.");
    }
  } else if (mIsDeviceStarted) {
    INFO("Warp device has been started.");

  }
  return  mIsDeviceStarted;
}

bool AmWarpDevice::encode(uint32_t* streams)
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
    if (mStreamParamList[id]->encode_params.encode_format.encode_type
        == IAV_ENCODE_NONE) {
      start_streams &= ~(1 << id);
    }
  }

  DEBUG("streams %d, start %d, stop %d", *streams, start_streams, stop_streams);

  if (start_streams > 0) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_START_ENCODE_EX, start_streams) < 0)) {
      ERROR("IAV_IOC_START_ENCODE_EX: streams [%d] %s", start_streams, strerror(errno));
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

bool AmWarpDevice::update_encoder_warp()
{
  uint32_t used_area_num = 0;
  for (uint32_t i = 0; i < MAX_WARP_AREA_NUM; ++i) {
    if (mEncoderParams->warp_control_info.area[i].enable) {
      mEncoderParams->warp_control_info.area[i].hor_map.enable =
          mEncoderParams->warp_control_info.area[i].hor_map.output_grid_col ?
              1 : 0;
      mEncoderParams->warp_control_info.area[i].ver_map.enable =
          mEncoderParams->warp_control_info.area[i].ver_map.output_grid_col ?
              1 : 0;
      ++used_area_num;
      DEBUG("Area%u: hor_map %u, ver_map %u: input %ux%u (%u, %u),"
            "output %ux%u (%u, %u), rotate %u", i,
            mEncoderParams->warp_control_info.area[i].hor_map.enable,
            mEncoderParams->warp_control_info.area[i].ver_map.enable,
            mEncoderParams->warp_control_info.area[i].input.width,
            mEncoderParams->warp_control_info.area[i].input.height,
            mEncoderParams->warp_control_info.area[i].input.x,
            mEncoderParams->warp_control_info.area[i].input.y,
            mEncoderParams->warp_control_info.area[i].output.width,
            mEncoderParams->warp_control_info.area[i].output.height,
            mEncoderParams->warp_control_info.area[i].output.x,
            mEncoderParams->warp_control_info.area[i].output.y,
            mEncoderParams->warp_control_info.area[i].rotate_flip);
    }
  }

  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_WARP_CONTROL_EX,
          &mEncoderParams->warp_control_info) < 0)) {
    PERROR("IAV_IOC_SET_WARP_CONTROL_EX");
    return false;
  }
  INFO("IAV_IOC_SET_WARP_CONTROL_EX: number %u, keep_dptz[1] %u, "
		  "keep_dptz[3] %u", used_area_num,
       mEncoderParams->warp_control_info.keep_dptz[1],
       mEncoderParams->warp_control_info.keep_dptz[3]);

  return true;
}

bool AmWarpDevice:: update_encoder_dptz_warp()
{
  for (uint32_t i = 0; i < MAX_ENCODE_BUFFER_NUM; ++i) {
    for (uint32_t areaId = 0; areaId < MAX_WARP_AREA_NUM; ++areaId) {
      if (mEncoderParams->dptz_warp_info[i][areaId].buffer_id == 1
          || mEncoderParams->dptz_warp_info[i][areaId].buffer_id == 3) {
        if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_WARP_AREA_DPTZ_EX,
                &mEncoderParams->dptz_warp_info[i][areaId]) < 0)) {
          PERROR("IAV_IOC_SET_WARP_AREA_DPTZ_EX");
          return false;
        }
        INFO("IAV_IOC_SET_WARP_AREA_DPTZ_EX: buffer %u region %u",
             mEncoderParams->dptz_warp_info[i][areaId].buffer_id, areaId);
      }
    }
  }
  return true;
}

bool AmWarpDevice::update_stream_h264_qp(uint32_t streamId)
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

bool AmWarpDevice::assign_buffer_to_stream()
{
  iav_system_resource_setup_ex_t &resource =
      mEncoderParams->system_resource_info;
  iav_source_buffer_format_all_ex_t &bufferFormat =
      mEncoderParams->buffer_format_info;
  iav_source_buffer_type_all_ex_t &bufferType =
      mEncoderParams->buffer_type_info;
  bool isBufferUsed[MAX_ENCODE_BUFFER_NUM] = {false};
  bool assignedBuffer = false;

  isBufferUsed[3] = (bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW);

  for (uint32_t i = 0; i < mStreamNumber; ++i) {
    iav_encode_format_ex_t &format =
            mStreamParamList[i]->encode_params.encode_format;
    Rect &src_win = mStreamParamList[i]->encode_params.src_window;
    assignedBuffer = false;
    if (format.encode_type != IAV_ENCODE_NONE) {
      DEBUG("stream%u: %ux%u, unwarp %u, window %ux%u",
            i, format.encode_width, format.encode_height,
            mStreamParamList[i]->encode_params.src_unwarp,
            src_win.width, src_win.height);

      /*Set stream max size */
      resource.stream_max_size[i].width = format.encode_width;
      resource.stream_max_size[i].height = format.encode_height;

      /* Unwarp */
      if (mStreamParamList[i]->encode_params.src_unwarp) {
        if (!isBufferUsed[1]) {
          if ((format.encode_width <= resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width)
              && (format.encode_height
                  <= resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height)) {
            format.source = 1;
            bufferFormat.second_unwarp = 1;
            bufferFormat.second_width = format.encode_width;
            bufferFormat.second_height = format.encode_height;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            isBufferUsed[1] = true;
            assignedBuffer = true;
            DEBUG("Stream%u is from Buffer 1 (Unwarp).", i);

            // Fix me: work around for second/fourth buffer cannot be turned
            // on once unwarp is enabled
            if (!isBufferUsed[3]) {
              bufferFormat.fourth_unwarp = 0;
              bufferFormat.fourth_width = 0;
              bufferFormat.fourth_height = 0;
              bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
              isBufferUsed[3] = true;
            }
          }
        }
        if (!assignedBuffer && !isBufferUsed[3]) {
          if ((format.encode_width <= resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width)
              && (format.encode_height
                  <= resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height)) {
            format.source = 3;
            bufferFormat.fourth_unwarp = 1;
            bufferFormat.fourth_width = format.encode_width;
            bufferFormat.fourth_height = format.encode_height;
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            isBufferUsed[3] = true;
            assignedBuffer = true;
            DEBUG("Stream%u is from Buffer 3 (Unwarp).", i);
            // Fix me: work around for second/fourth buffer cannot be turned
            // on once unwarp is enabled
            bufferFormat.second_unwarp = 0;
            bufferFormat.second_width = 0;
            bufferFormat.second_height = 0;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_OFF;
            isBufferUsed[1] = true;

          }
        }
        if (!assignedBuffer) {
          ERROR("Cannot assign unwarp buffer for stream%u [%ux%u] (disable warp). "
              "Buffer1 supports max [%ux%u]. "
              "Buffer3 is for [%s] and supports max [%ux%u].",
              i, format.encode_width, format.encode_height,
              resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width,
              resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height,
              bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW ?
              "Preview" : "Encode", resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width,
              resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height);
          return false;
        }
      } else {
        /* Warp */
        if (!src_win.width || !src_win.height
            || (format.encode_width == src_win.width
                && format.encode_height == src_win.height)) {
          format.source = 0;
          assignedBuffer = true;
          DEBUG("Stream%u is from Buffer0", i);
        }

        if (!assignedBuffer && !isBufferUsed[1]) {
          if ((format.encode_width <= resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width)
              && (format.encode_height
                  <= resource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height)) {
            format.source = 1;
            bufferFormat.second_unwarp = 0;
            bufferFormat.second_width = format.encode_width;
            bufferFormat.second_height = format.encode_height;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            isBufferUsed[1] = true;
            assignedBuffer = true;
            DEBUG("Stream%u is from Buffer1", i);
          }
        }
        if (!assignedBuffer && !isBufferUsed[3]) {
          if ((format.encode_width <= resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width)
              && (format.encode_height
                  <= resource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height)) {
            format.source = 3;
            bufferFormat.fourth_unwarp = 0;
            bufferFormat.fourth_width = format.encode_width;
            bufferFormat.fourth_height = format.encode_height;
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            isBufferUsed[3] = true;
            assignedBuffer = true;
            DEBUG("Stream%u is from Buffer3", i);
          }
        }

        if (!assignedBuffer)  {
          ERROR("Cannot find available warp buffer for stream%u [%ux%u], "
              "window [%ux%u]. ",
              i, format.encode_width, format.encode_height,
              src_win.width, src_win.height);
          return false;
        }
      }
    }
  }

  /* Set bsize to 0 if the buffer is unused */
  for (uint32_t bufId = 1; bufId < MAX_ENCODE_BUFFER_NUM; ++bufId) {
    uint16_t *pBufWidth = NULL, *pBufHeight = NULL;
    switch (bufId) {
      case 1:
        pBufWidth = &bufferFormat.second_width;
        pBufHeight = &bufferFormat.second_height;
        if (bufferFormat.second_unwarp) {
          /* Unwarp */
          bufferFormat.second_input_width = bufferFormat.pre_main_width;
          bufferFormat.second_input_height = bufferFormat.pre_main_height;
        } else {
          /* Warp */
          bufferFormat.second_input_width = bufferFormat.main_width;
          bufferFormat.second_input_height = bufferFormat.main_height;
        }
        break;
      case 2:
        pBufWidth = &bufferFormat.third_width;
        pBufHeight = &bufferFormat.third_height;
        if (bufferFormat.third_unwarp) {
        /* Unwarp */
          bufferFormat.third_input_width = bufferFormat.pre_main_width;
          bufferFormat.third_input_width = bufferFormat.pre_main_height;
        } else {
          /* Warp */
          bufferFormat.third_input_width = bufferFormat.main_width;
          bufferFormat.third_input_width = bufferFormat.main_height;
        }
        break;
      case 3:
        pBufWidth = &bufferFormat.fourth_width;
        pBufHeight = &bufferFormat.fourth_height;
         if (bufferFormat.fourth_unwarp) {
        /* Unwarp */
          bufferFormat.fourth_input_width = bufferFormat.pre_main_width;
          bufferFormat.fourth_input_height = bufferFormat.pre_main_height;
        } else {
          /* Warp */
          bufferFormat.fourth_input_width = bufferFormat.main_width;
          bufferFormat.fourth_input_height = bufferFormat.main_height;
        }
        break;
      default:
        pBufWidth = &bufferFormat.main_width;
        pBufHeight = &bufferFormat.main_height;
        break;
    }
    if (!isBufferUsed[bufId]) {
      *pBufWidth = 0;
      *pBufHeight = 0;
      DEBUG("Buffer %u is unused. Set to %ux%u.", bufId, *pBufWidth,
            *pBufHeight);
    }
  }
  return true;
}

bool AmWarpDevice::assign_dptz_warp()
{
  iav_warp_control_ex_t &warp_info = mEncoderParams->warp_control_info;
  iav_warp_dptz_ex_t *dptz = NULL;
  Rect warp_area[MAX_WARP_AREA_NUM];
  bool is_area_used[MAX_WARP_AREA_NUM] = {false};

  for (uint32_t areaId = 0; areaId < MAX_WARP_AREA_NUM; ++areaId) {
    warp_area[areaId].width = warp_info.area[areaId].output.width;
    warp_area[areaId].height = warp_info.area[areaId].output.height;
    warp_area[areaId].x = warp_info.area[areaId].output.x;
    warp_area[areaId].y = warp_info.area[areaId].output.y;
  }

  memset(mEncoderParams->dptz_warp_info, 0, sizeof(mEncoderParams->dptz_warp_info));
  for (uint32_t i = 0; i < MAX_ENCODE_STREAM_NUM; ++i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[i]->encode_params.encode_format;
    Rect &src_win = mStreamParamList[i]->encode_params.src_window;
    if (format.encode_type != IAV_ENCODE_NONE) {
      DEBUG("Stream%u %ux%u, source window %ux%u + offset %ux%u",
            i, format.encode_width, format.encode_height, src_win.width, src_win.height,
            src_win.x, src_win.y);
      Vertex vertex[] = {
          Vertex(src_win.x, src_win.y, true, false, true, false),
          Vertex(src_win.x + src_win.width, src_win.y, false, true, true,
                 false),
          Vertex(src_win.x, src_win.y + src_win.height, true, false, false,
                 true),
          Vertex(src_win.x + src_win.width, src_win.y + src_win.height,
                 false, true, false, true) };
      if (format.source == 0) {
        format.encode_x = src_win.x;
        format.encode_y = src_win.y;
      } else if (format.source == 1 || format.source == 3) {
        format.encode_x = format.encode_y = 0;
        bool is_warp = (format.source == 1 ?
                (mEncoderParams->buffer_format_info.second_unwarp == 0) :
                (mEncoderParams->buffer_format_info.fourth_unwarp == 0));
        if (is_warp) {
          for (uint32_t vId = 0; vId < sizeof(vertex) / sizeof(vertex[0]);
              ++vId) {
            for (uint32_t areaId = 0; areaId < MAX_WARP_AREA_NUM; ++areaId) {
              if (warp_area[areaId].width && warp_area[areaId].height) {
                dptz = &mEncoderParams->dptz_warp_info[format.source][areaId];
                dptz->buffer_id = format.source;
                if (is_vertex_inside_rectangle(vertex[vId], warp_area[areaId])
                    && !is_area_used[areaId]) {
                  DEBUG("Vertex (%u, %u) is inside Rectangle%u %ux%u + (%u, %u).",
                        vertex[vId].x, vertex[vId].y, areaId,
                        warp_area[areaId].width, warp_area[areaId].height,
                        warp_area[areaId].x, warp_area[areaId].y);
                  if (vertex[vId].is_left) {
                    dptz->dptz[areaId].input.x = vertex[vId].x - warp_area[areaId].x;
                    dptz->dptz[areaId].input.width = min((int)(warp_area[areaId].width - dptz->dptz[areaId].input.x),
                        (int)src_win.width);
                  } else if (vertex[vId].is_right) {
                    dptz->dptz[areaId].input.width = min((int)(vertex[vId].x - warp_area[areaId].x),
                        (int)src_win.width);
                    dptz->dptz[areaId].input.x = vertex[vId].x - dptz->dptz[areaId].input.width
                        - warp_area[areaId].x;
                  }

                  if (vertex[vId].is_upper) {
                    dptz->dptz[areaId].input.y = vertex[vId].y - warp_area[areaId].y;
                    dptz->dptz[areaId].input.height =
                        min((int)(warp_area[areaId].height - dptz->dptz[areaId].input.y),
                            (int)src_win.height);
                  } else if (vertex[vId].is_lower) {
                    dptz->dptz[areaId].input.height = min((int)(vertex[vId].y - warp_area[areaId].y),
                        (int)src_win.height);
                    dptz->dptz[areaId].input.y = vertex[vId].y - dptz->dptz[areaId].input.height
                        - warp_area[areaId].y;
                  }

                  dptz->dptz[areaId].output.width = round_up(dptz->dptz[areaId].input.width * format.encode_width
                      / src_win.width, 2);
                  dptz->dptz[areaId].output.height = round_up(dptz->dptz[areaId].input.height * format.encode_height
                      / src_win.height, 4);
                  dptz->dptz[areaId].output.x = round_down(
                      vertex[vId].is_left ? 0 :
                      format.encode_width - dptz->dptz[areaId].output.width, 2);
                  dptz->dptz[areaId].output.y = round_down(
                      vertex[vId].is_upper ? 0 :
                      format.encode_height - dptz->dptz[areaId].output.height, 4);
                  is_area_used[areaId] = true;
                  warp_info.keep_dptz[format.source] = 1;
                  DEBUG("Buffer %u Warp Area%u: "
                        "input %ux%u + offset %ux%u, output %ux%u + offset %ux%u",
                        dptz->buffer_id, areaId, dptz->dptz[areaId].input.width,
                        dptz->dptz[areaId].input.height, dptz->dptz[areaId].input.x, dptz->dptz[areaId].input.y,
                        dptz->dptz[areaId].output.width, dptz->dptz[areaId].output.height, dptz->dptz[areaId].output.x,
                        dptz->dptz[areaId].output.y);
                }
              }
            }
          }
        }
      }
    }
  }
  return true;
}

bool AmWarpDevice::apply_encoder_parameters()
{
  iav_source_buffer_format_all_ex_t &buffer = mEncoderParams->buffer_format_info;
  iav_system_resource_setup_ex_t &resource =
      mEncoderParams->system_resource_info;
  WarpParameters &layout = mEncoderParams->warp_layout;
  // buffer_format_all and system_resource
  buffer.pre_main_width = layout.unwarp.width;
  buffer.pre_main_height = layout.unwarp.height;
  buffer.pre_main_input.width = layout.unwarp_window.width;
  buffer.pre_main_input.height = layout.unwarp_window.height;
  buffer.pre_main_input.x = layout.unwarp_window.x;
  buffer.pre_main_input.y = layout.unwarp_window.y;
  buffer.main_width = resource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width =
      layout.warp.width;
  buffer.main_height = resource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height =
      layout.warp.height;
  resource.max_warp_input_width = layout.unwarp.width;
  resource.max_warp_output_width = layout.unwarp.width;
  // setup
  mEncoderParams->system_setup_info.vout_swap = 1;
  if (!assign_buffer_to_stream() || !check_encoder_parameters()
      || !update_encoder_parameters())
    return false;
  return true;
}

bool AmWarpDevice::apply_encoder_parameters_in_preview()
{
  if (!update_encoder_warp() || !assign_dptz_warp()
      || !update_encoder_dptz_warp())
    return false;
  return true;
}

bool AmWarpDevice::apply_stream_parameters()
{
  if (!check_system_performance())
    return false;

  for (uint32_t sId = 0; sId < mStreamNumber; ++sId) {
    if (mStreamParamList[sId]->encode_params.encode_format.encode_type
        != IAV_ENCODE_NONE) {
      // Not support rotation in warp mode
      mStreamParamList[sId]->encode_params.encode_format.rotate_clockwise = 0;
      mStreamParamList[sId]->encode_params.encode_format.vflip = 0;
      mStreamParamList[sId]->encode_params.encode_format.hflip = 0;
      if (!update_stream_format(sId) || !update_stream_framerate(sId)
          || !update_stream_overlay(sId))
        return false;

      if (mStreamParamList[sId]->encode_params.encode_format.encode_type
          == IAV_ENCODE_H264) {
        if (!check_stream_h264_config(sId) || !update_stream_h264_config(sId)
            || !update_stream_h264_bitrate(sId)\
            // IAV driver resets QP when changing bitrate.
            // QP must be applied after setting bitrate.
            || !eval_qp_mode(sId) || !update_stream_h264_qp(sId))
          return false;
      } else {
        if (!update_stream_mjpeg_config(sId))
          return false;
      }
    }
  }
  return true;
}

bool AmWarpDevice::init_device(int voutInitMode, bool force)
{
  bool ret = true;
  do {
    for (uint32_t i = 0; i < mVoutNumber; ++i) {
      if (mVoutTypeList[i] != AM_VOUT_TYPE_NONE) {
        ret = mVoutList[i]->start((AmVout::VoutInitMode) voutInitMode, force);
        if (!ret) {
          ERROR("Failed to start %s", vout_type_to_str[mVoutTypeList[i]]);
          break;
        }
      }
    }
    if (ret) {
      for (uint32_t i = 0; i < mVinNumber; ++i) {
        if (mVinList[i]) {
          if (false == mVinList[i]->start(force)) {
            ERROR("Failed to start VIN%u", i);
            ret = false;
            break;
          } else {
            INFO("start VIN%u", i);
          }
        }
      }
    }
  } while (0);

  return ret;
}

bool AmWarpDevice::disable_pm()
{
  return instance_pm_dptz->disable_pm();
}

bool AmWarpDevice::reset_pm()
{
  RECT premain_buffer;//pre-main input window
  premain_buffer.width = mEncoderParams->buffer_format_info.pre_main_width;
  premain_buffer.height = mEncoderParams->buffer_format_info.pre_main_height;
  return instance_pm_dptz->pm_reset_all(&premain_buffer);
}

bool AmWarpDevice::change_stream_state(IavState target, uint32_t streams)
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
              if (apply_encoder_parameters() && preview()
                  && apply_encoder_parameters_in_preview()) {
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
              if (!apply_encoder_parameters() || !preview()
                  || !apply_encoder_parameters_in_preview()) {
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
            if (disable_pm() || !idle()) {
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
            } else if (!apply_stream_parameters() || !encode(&streams)|| reset_pm()) {
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
  } while (!error && (currentState = get_iav_status()) != target);
  DEBUG("Target IAV state is %s.", iav_state_to_str(target));
  DEBUG("Current IAV state is %s.", iav_state_to_str(currentState));

  return ret;
}

uint32_t AmWarpDevice::linear_scale(uint32_t in, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max)
{
  uint32_t out = out_max;
  if ((in_min != in_max) && (out_min != out_max) && (in <= in_min)
      && (in >= in_max))
    out = out_min + (out_max - out_min) * (in - in_min) / (in_max - in_min);
  return out;
}

bool AmWarpDevice::eval_qp_mode(uint32_t streamId)
{
  // init QP value for CBR(SCBR) mode only.
  if (mStreamParamList[streamId]->bitrate_info.rate_control_mode != IAV_CBR)
    return true;

  StreamEncodeFormat &stream_format = mStreamParamList[streamId]->encode_params;
  iav_change_qp_limit_ex_s &qp = mStreamParamList[streamId]->h264_qp;
  uint32_t vinFPS = round_div(512000000, mVinList[0]->get_vin_fps());
  uint32_t macroblocks = round_up(stream_format.encode_format.encode_width, 16)
      * round_up(stream_format.encode_format.encode_height , 16) / 256;
  uint32_t kbps_for_30fps =
      (mStreamParamList[streamId]->bitrate_info.cbr_avg_bitrate >> 10) * 30
          * stream_format.encode_framerate.ratio_denominator / vinFPS
          / stream_format.encode_framerate.ratio_numerator;
  uint32_t bitrateModeListNum = sizeof(gBitrateModeList)
      / sizeof(gBitrateModeList[0]);
  uint32_t minIdx = 0, maxIdx = bitrateModeListNum - 1;

  for (uint32_t i = 0; i < bitrateModeListNum; ++i) {
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
  if (kbps_for_30fps
      <= linear_scale(macroblocks, min.macroblocks, max.macroblocks, min.extremely_low_kbps, max.extremely_low_kbps)) {
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
  } else if (kbps_for_30fps
      <= linear_scale(macroblocks, min.macroblocks, max.macroblocks, min.low_kbps, max.low_kbps)) {
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
  } else if (kbps_for_30fps
      <= linear_scale(macroblocks, min.macroblocks, max.macroblocks, min.medium_kbps, max.medium_kbps)) {
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

  DEBUG("Stream%u QP: I %u~%u, P %u~%u, B %u~%u, IP diff %u, PB diff %u, aqp %u.", streamId, qp.qp_min_on_I, qp.qp_max_on_I, qp.qp_min_on_P, qp.qp_max_on_P, qp.qp_min_on_B, qp.qp_max_on_B, qp.i_qp_reduce, qp.p_qp_reduce, qp.adapt_qp);
  return true;
}

uint32_t AmWarpDevice::get_grid_exponent(uint32_t spacing)
{
  uint32_t exponent = 0;
  switch (spacing) {
  case 16:
    exponent = GRID_SPACING_PIXEL_16;
    break;
  case 32:
    exponent = GRID_SPACING_PIXEL_32;
    break;
  case 64:
    exponent = GRID_SPACING_PIXEL_64;
    break;
  case 128:
    exponent = GRID_SPACING_PIXEL_128;
    break;
  default:
    ERROR("NOT supported spacing [%d]. Use 64 pixels.!\n",
      spacing);
    exponent = GRID_SPACING_PIXEL_64;
    break;
  }
  return exponent;
}

uint32_t AmWarpDevice::get_grid_spacing(uint32_t exponent)
{
  uint32_t spacing = 0;
  switch (exponent) {
  case GRID_SPACING_PIXEL_16:
    spacing = 16;
    break;
  case GRID_SPACING_PIXEL_32:
    spacing = 32;
    break;
  case GRID_SPACING_PIXEL_64:
    spacing = 64;
    break;
  case GRID_SPACING_PIXEL_128:
    spacing = 128;
    break;
  default:
    spacing = 64;
    ERROR("NOT supported spacing [%d]. Use 64 pixels.!\n",
          exponent);
    spacing = GRID_SPACING_PIXEL_64;
    break;
  }
  return spacing;
}

bool AmWarpDevice::is_vertex_inside_rectangle(Vertex &v, Rect &r)
{
  return (v.is_left ? v.x >= r.x : v.x <= (r.x + (int)r.width))
      && (v.is_left ? v.x < (r.x + (int)r.width) : v.x > r.x)
      && (v.is_upper ? v.y >= r.y : v.y <= (r.y + (int)r.height))
      && (v.is_upper ? v.y < (r.y + (int)r.height) : v.y > r.y);
}

bool AmWarpDevice::start_encode()
{
  uint32_t streams = (1 << mStreamNumber) - 1;
  return ready_for_encode() && change_stream_state(AM_IAV_ENCODING, streams);
}

bool AmWarpDevice::stop_encode()
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

bool AmWarpDevice::ready_for_encode()
{
  return change_stream_state(AM_IAV_IDLE) && change_stream_state(AM_IAV_PREVIEW);
}

bool AmWarpDevice::start_encode_stream(uint32_t streamId)
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

bool AmWarpDevice::stop_encode_stream(uint32_t streamId)
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

void AmWarpDevice::get_max_stream_num(uint32_t *pMaxNum)
{
  if (pMaxNum)
    *pMaxNum = mStreamNumber;
}

void AmWarpDevice::get_max_stream_size(Resolution *pMaxSize)
{
  if (pMaxSize) {
    pMaxSize->width = mEncoderParams->buffer_format_info.main_width;
    pMaxSize->width = mEncoderParams->buffer_format_info.main_height;
  }
}

bool AmWarpDevice::get_stream_size(uint32_t streamId, EncodeSize *pSize)
{
  if (!check_stream_id(streamId))
    return false;

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }

  StreamEncodeFormat &param =  mStreamParamList[streamId]->encode_params;

  pSize->width = param.encode_format.encode_width;
  pSize->height = param.encode_format.encode_height;
  pSize->src_unwarp = param.src_unwarp;
  pSize->src_window.width = param.src_window.width;
  pSize->src_window.height = param.src_window.height;
  pSize->src_window.x = param.src_window.x;
  pSize->src_window.y = param.src_window.y;

  return true;
}

bool AmWarpDevice::set_stream_size_id(uint32_t streamId, const EncodeSize *pSize)
{
  if (streamId > MAX_ENCODE_STREAM_NUM) {
    ERROR("Invalid stream id %d. Max number is %d.",
        streamId, MAX_ENCODE_STREAM_NUM);
    return false;
  }

  if (!pSize) {
    ERROR("Invalid warp encode size.");
    return false;
  }

  StreamEncodeFormat &param = mStreamParamList[streamId]->encode_params;
  param.encode_format.encode_width = pSize->width;
  param.encode_format.encode_height = pSize->height;
  param.src_unwarp = pSize->src_unwarp;
  param.src_window.width = pSize->src_window.width;
  param.src_window.height = pSize->src_window.height;
  param.src_window.x = pSize->src_window.x;
  param.src_window.y = pSize->src_window.y;
  mStreamParamList[streamId]->config_changed = 1;

  return true;
}

bool AmWarpDevice::set_stream_size_all(uint32_t totalNum, const EncodeSize *pSize)
{
  if (totalNum > MAX_ENCODE_STREAM_NUM) {
    ERROR("Invalid total stream number %d. Max number is %d.",
        totalNum, MAX_ENCODE_STREAM_NUM);
    return false;
  }

  if (!pSize) {
    ERROR("Invalid warp encode size.");
    return false;
  }

  for (uint32_t streamId = 0; streamId < totalNum; ++ streamId) {
    StreamEncodeFormat &param = mStreamParamList[streamId]->encode_params;
    param.encode_format.encode_width = pSize[streamId].width;
    param.encode_format.encode_height = pSize[streamId].height;
    param.src_unwarp = pSize[streamId].src_unwarp;
    param.src_window.width = pSize[streamId].src_window.width;
    param.src_window.height = pSize[streamId].src_window.height;
    param.src_window.x = pSize[streamId].src_window.x;
    param.src_window.y = pSize[streamId].src_window.y;
    mStreamParamList[streamId]->config_changed = 1;
  }

  return true;
}

bool AmWarpDevice::get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate)
{
  uint32_t vinFPS = 0;

  if (!check_stream_id(streamId))
    return false;

  if (!pFrameRate) {
    ERROR("Stream%u frame rate ptr is NULL.", streamId);
    return false;
  }
  StreamEncodeFormat &encode_params = mStreamParamList[streamId]->encode_params;
  encode_params.encode_framerate.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_FRAMERATE_FACTOR_EX,
          &encode_params.encode_framerate) < 0)) {
    PERROR("IAV_IOC_GET_FRAMERATE_FACTOR_EX");
    return false;
  }
  vinFPS = round_div(512000000, mVinList[0]->get_vin_fps());
  *pFrameRate = vinFPS * encode_params.encode_framerate.ratio_numerator
      / encode_params.encode_framerate.ratio_denominator;
  return true;
}

bool AmWarpDevice::set_stream_framerate(uint32_t streamId, uint32_t frameRate)
{
  uint32_t vinFPS = 0;
  EncodeType type = AM_ENCODE_TYPE_NONE;

  if (!check_stream_id(streamId))
    return false;

  vinFPS = mVinList[0]->get_vin_fps();
  switch (vinFPS) {
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
    if (type == AM_ENCODE_TYPE_H264) {
      eval_qp_mode(streamId);
      update_stream_h264_qp(streamId);
    }
  }
  mStreamParamList[streamId]->config_changed = 1;
  return true;
}

bool AmWarpDevice::get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate)
{
  if (!check_stream_id(streamId))
    return false;

  if (!pBitrate) {
    ERROR("Stream%u bitrate ptr is NULL.", streamId);
    return false;
  }
  mStreamParamList[streamId]->bitrate_info.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_BITRATE_EX,
          &mStreamParamList[streamId]->bitrate_info) < 0)) {
    PERROR("IAV_IOC_GET_BITRATE_EX");
    return false;
  }

  *pBitrate = mStreamParamList[streamId]->bitrate_info.cbr_avg_bitrate;
  return true;
}

bool AmWarpDevice::set_cbr_bitrate(uint32_t streamId, uint32_t bitrate)
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

bool AmWarpDevice::get_stream_idr(uint32_t streamId, uint8_t *idr_interval)
{
  if (!check_stream_id(streamId))
    return false;

  *idr_interval = mStreamParamList[streamId]->h264_config.idr_interval;

  return true;
}

bool AmWarpDevice::set_stream_idr(uint32_t streamId, uint8_t idr_interval)
{
  if (!check_stream_id(streamId))
    return false;

  mStreamParamList[streamId]->h264_config.idr_interval = idr_interval;

  mStreamParamList[streamId]->config_changed = 1;

  return update_stream_h264_config(streamId);
}

bool AmWarpDevice::get_stream_n(uint32_t streamId, uint16_t *n)
{
  if (!check_stream_id(streamId))
    return false;

  *n = mStreamParamList[streamId]->h264_config.N;

  return true;
}

bool AmWarpDevice::set_stream_n(uint32_t streamId, uint16_t n)
{
  if (!check_stream_id(streamId))
    return false;

  mStreamParamList[streamId]->h264_config.N = n;

  mStreamParamList[streamId]->config_changed = 1;

  return update_stream_h264_config(streamId);
}

bool AmWarpDevice::get_stream_profile(uint32_t streamId, uint8_t *profile)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  *profile = mStreamParamList[streamId]->h264_config.profile;

  return true;
}

bool AmWarpDevice::set_stream_profile(uint32_t streamId, uint8_t profile)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  mStreamParamList[streamId]->h264_config.profile = profile;

  mStreamParamList[streamId]->config_changed = 1;

  return true;

}

bool AmWarpDevice::get_mjpeg_quality(uint32_t streamId, uint8_t *quality)
{
  if (!check_stream_id(streamId)){
    return false;
  }

  *quality = mStreamParamList[streamId]->mjpeg_config.quality;

  return true;
}

bool AmWarpDevice::set_mjpeg_quality(uint32_t streamId, uint8_t quality)
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



bool AmWarpDevice::get_stream_type(uint32_t streamId, EncodeType *pType)
{
  if (!check_stream_id(streamId))
    return false;

  if (!pType) {
    ERROR("EncodeType ptr is NULL.");
    return false;
  }

#if 0
  mStreamParamList[streamId]->encode_params.encode_format.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX,
          &mStreamParamList[streamId]->encode_params.encode_format) < 0)) {
    PERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
    return false;
  }
#endif

  switch (mStreamParamList[streamId]->encode_params.encode_format.encode_type) {
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

bool AmWarpDevice::set_stream_type(uint32_t streamId, const EncodeType type)
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

bool AmWarpDevice:: get_warp_control(WarpControl *pControl)
{
  bool ret = false;
  if (!pControl) {
    ERROR("Invalid warp control.");
    return ret;
  }

  if (create_warp_map()) {
    iav_warp_control_ex_t warp_ctrl = mEncoderParams->warp_control_info;
    if (AM_UNLIKELY(pControl->id >= MAX_WARP_AREA_NUM)) {
      ERROR("Invalid warp ID [%d]. Max area number is %d.", pControl->id, MAX_WARP_AREA_NUM - 1);
      return ret;
    }
    uint32_t area_id = pControl->id;
    pControl->input.width = warp_ctrl.area[area_id].input.width;
    pControl->input.height = warp_ctrl.area[area_id].input.height;
    pControl->input_offset.x = warp_ctrl.area[area_id].input.x;
    pControl->input_offset.y = warp_ctrl.area[area_id].input.y;
    pControl->output.width = warp_ctrl.area[area_id].output.width;
    pControl->output.height = warp_ctrl.area[area_id].output.height;
    pControl->output_offset.x = warp_ctrl.area[area_id].output.x;
    pControl->output_offset.y = warp_ctrl.area[area_id].output.y;
    pControl->rotate = (
        warp_ctrl.area[area_id].rotate_flip & ROTATE_90 ? AM_ROTATE_90 :
            0)
        | (warp_ctrl.area[area_id].rotate_flip & HORIZONTAL_FLIP ?
            AM_HORIZONTAL_FLIP : 0)
        | (warp_ctrl.area[area_id].rotate_flip & VERTICAL_FLIP ?
            AM_VERTICAL_FLIP : 0);
    pControl->hor_map.rows =
        warp_ctrl.area[area_id].hor_map.output_grid_row;
    pControl->hor_map.cols =
        warp_ctrl.area[area_id].hor_map.output_grid_col;
    pControl->hor_map.hor_spacing =
        get_grid_spacing((uint32_t) warp_ctrl.area[area_id].hor_map.horizontal_spacing);
    pControl->hor_map.ver_spacing =
        get_grid_spacing((uint32_t) warp_ctrl.area[area_id].hor_map.vertical_spacing);
    pControl->hor_map.addr = mWarpMap[area_id][0];

    pControl->ver_map.rows =
        warp_ctrl.area[area_id].ver_map.output_grid_row;
    pControl->ver_map.cols =
        warp_ctrl.area[area_id].ver_map.output_grid_col;
    pControl->ver_map.hor_spacing =
        get_grid_spacing((uint32_t) warp_ctrl.area[area_id].ver_map.horizontal_spacing);
    pControl->ver_map.ver_spacing =
        get_grid_spacing((uint32_t) warp_ctrl.area[area_id].ver_map.vertical_spacing);
    pControl->ver_map.addr = mWarpMap[area_id][1];
//    DEBUG("Area%u: hor_map 0x%x, ver_map 0x%x", area_id, (uint32_t )pControl->hor_map.addr, (uint32_t )pControl->ver_map.addr);

    ret = true;
  } else {
    ERROR("Cannot create warp map.");
  }
  return ret;
}

bool AmWarpDevice::set_warp_control(uint32_t totalNum, const WarpControl *pControl)
{
  bool ret = false;
  if (!pControl) {
    ERROR("Invalid warp control.");
    return ret;
  }
  if (totalNum > MAX_WARP_AREA_NUM) {
    ERROR("Invalid total warp area number [%d]. Max area number is %d.", totalNum, MAX_WARP_AREA_NUM);
    return ret;
  }

  iav_warp_control_ex_t& warp_ctrl = mEncoderParams->warp_control_info;
  if (create_warp_map()) {
    for (uint32_t i = 0; i < totalNum; ++i) {
      if (AM_UNLIKELY(pControl->id >= MAX_WARP_AREA_NUM)) {
        ERROR("Invalid warp ID [%d]. Max area number is %d.", pControl->id, MAX_WARP_AREA_NUM
            - 1);
        return ret;
      }
      warp_ctrl.area[pControl[i].id].enable = (pControl[i].input.width
          && pControl[i].input.height && pControl[i].output.width
          && pControl[i].output.height);
      if (warp_ctrl.area[pControl[i].id].enable) {
        warp_ctrl.area[pControl[i].id].input.width = pControl[i].input.width;
        warp_ctrl.area[pControl[i].id].input.height = pControl[i].input.height;
        warp_ctrl.area[pControl[i].id].input.x = pControl[i].input_offset.x;
        warp_ctrl.area[pControl[i].id].input.y = pControl[i].input_offset.y;
        warp_ctrl.area[pControl[i].id].output.width = pControl[i].output.width;
        warp_ctrl.area[pControl[i].id].output.height = pControl[i].output.height;
        warp_ctrl.area[pControl[i].id].output.x = pControl[i].output_offset.x;
        warp_ctrl.area[pControl[i].id].output.y = pControl[i].output_offset.y;
        warp_ctrl.area[pControl[i].id].rotate_flip = (
            pControl[i].rotate & AM_ROTATE_90 ? ROTATE_90 : 0)
            | (pControl[i].rotate & AM_HORIZONTAL_FLIP ? HORIZONTAL_FLIP : 0)
            | (pControl[i].rotate & AM_VERTICAL_FLIP ? VERTICAL_FLIP : 0);

        warp_ctrl.area[pControl[i].id].hor_map.output_grid_row =
            pControl[i].hor_map.rows;
        warp_ctrl.area[pControl[i].id].hor_map.output_grid_col =
            pControl[i].hor_map.cols;
        if (warp_ctrl.area[pControl[i].id].hor_map.output_grid_row
            && warp_ctrl.area[pControl[i].id].hor_map.output_grid_col) {
          warp_ctrl.area[pControl[i].id].hor_map.horizontal_spacing =
              get_grid_exponent(pControl[i].hor_map.hor_spacing);
          warp_ctrl.area[pControl[i].id].hor_map.vertical_spacing =
              get_grid_exponent(pControl[i].hor_map.ver_spacing);
          if (AM_UNLIKELY(pControl[i].hor_map.addr != mWarpMap[pControl[i].id][0])) {
            memcpy(mWarpMap[pControl[i].id][0], pControl[i].hor_map.addr, sizeof(int16_t)
                       * MAX_GRID_WIDTH * MAX_GRID_HEIGHT);
          }
        }
        warp_ctrl.area[pControl[i].id].hor_map.addr =
            (uint32_t) mWarpMap[pControl[i].id][0];

        warp_ctrl.area[pControl[i].id].ver_map.output_grid_row =
            pControl[i].ver_map.rows;
        warp_ctrl.area[pControl[i].id].ver_map.output_grid_col =
            pControl[i].ver_map.cols;
        if (warp_ctrl.area[pControl[i].id].ver_map.output_grid_row
            && warp_ctrl.area[pControl[i].id].ver_map.output_grid_col) {
          warp_ctrl.area[pControl[i].id].ver_map.horizontal_spacing =
              get_grid_exponent(pControl[i].ver_map.hor_spacing);
          warp_ctrl.area[pControl[i].id].ver_map.vertical_spacing =
              get_grid_exponent(pControl[i].ver_map.ver_spacing);
          if (AM_UNLIKELY(pControl[i].ver_map.addr != mWarpMap[pControl[i].id][1])) {
            memcpy(mWarpMap[pControl[i].id][0], pControl[i].ver_map.addr, sizeof(int16_t)
                       * MAX_GRID_WIDTH * MAX_GRID_HEIGHT);
          }
        }
        warp_ctrl.area[pControl[i].id].ver_map.addr =
            (uint32_t) mWarpMap[pControl[i].id][1];
      }
    }
    ret = start() && update_encoder_warp();
  } else {
    ERROR("Cannot create warp map.");
  }

  return ret;
}

bool AmWarpDevice::set_pm_param(PRIVACY_MASK * pm_in)
{
  RECT premain_buffer;//pre-main input window
  premain_buffer.width = mEncoderParams->buffer_format_info.pre_main_width;
  premain_buffer.height = mEncoderParams->buffer_format_info.pre_main_height;
  //ERROR("set_pm_param in encodedev id:%d height:%d w:%d left:%d top:%d",pm_in->id, pm_in->height,pm_in->width,pm_in->left,pm_in->top);
  return instance_pm_dptz->set_pm_param(pm_in, &premain_buffer);
}

bool AmWarpDevice::set_dptz_param(DPTZParam	 *dptz_set)
{
  //ERROR("dptz in set_dptz_param buffer:%d zoom:%d x:%d y:%d",dptz_set->source_buffer, dptz_set->zoom_factor,dptz_set->offset_x,dptz_set->offset_y);
  if(dptz_set->source_buffer == BUFFER_1) {
    return instance_pm_dptz->set_dptz_param_mainbuffer(dptz_set);
  } else {
    return instance_pm_dptz->set_dptz_param(dptz_set);
  }
  return -1;
}

bool AmWarpDevice::get_pm_param(uint32_t * pm_in)
{
  return instance_pm_dptz->get_pm_param(pm_in);
}