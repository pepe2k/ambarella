/*******************************************************************************
 * am_video_device.cpp
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
#include "am_vdevice.h"
#include "am_vin.h"
#include "am_vout.h"
#include "am_vout_lcd.h"
#include "am_vout_hdmi.h"
#include "am_vout_cvbs.h"

static const char *iavStatus[] =
{
 "Idle", "Preview", "Encoding", "Still Capture", "Decoding", "Transcoding",
 "Duplex", "Init", "Unknown"
};

#ifdef CONFIG_ARCH_A5S
static uint32_t max_macroblock_number[] =
{
 131400, /* A5s33 */
 230400, /* A5s55 */
 285300, /* A5s66 */
 285300, /* A5s70 */
 285300, /* A5s88 */
 285300, /* A5s99 */
};
static const char * chip_str[] =
{
 "Unknown",
 "A5s33",
 "A5s55",
 "A5s66",
 "A5s70",
 "A5s88",
 "A5s99"
};
#else
#ifdef CONFIG_ARCH_S2
static uint32_t max_macroblock_number[] =
{
  2048 * 1536 / 256 * 30 + 720 * 480 /256 * 30,      /* S233 */
  1920 * 1088 / 256 * 60 + 720 * 480 / 256 * 30,     /* S255 */
  1920 * 1088 / 256 * 60 + 720 * 480 / 256 * 60,     /* S266 */
  1920 * 1088 / 256 * 120,                           /* S288 */
  1920 * 1088 / 256 * 120                            /* S299 */
};
static const char * chip_str[] =
{
 "Unknown",
 "S233",
 "S255",
 "S266",
 "S288",
 "S299"
};
#endif

#endif

AmVideoDevice::AmVideoDevice()
: mVinList(NULL),
  mVoutList(NULL),
  mVinParamList(NULL),
  mVinTypeList(NULL),
  mVoutParamList(NULL),
  mVoutTypeList(NULL),
  mStreamParamList(NULL),
  mEncoderParams(NULL),
  mPhotoParams(NULL),
  mVinNumber(0),
  mVoutNumber(0),
  mStreamNumber(0),
  mIav(-1),
  mIsDevCreated(false),
  mIsDspMapped(false),
  mIsBsbMapped(false),
  mIsOverlayMapped(false)
{
  memset(&mDspMapInfo, 0, sizeof(mDspMapInfo));
  memset(&mBsbMapInfo, 0, sizeof(mBsbMapInfo));
  if (AM_UNLIKELY((mIav = open("/dev/iav", O_RDWR)) < 0)) {
    PERROR("Failed to open IAV device");
  }
  DEBUG("vDevice IAV fd is %d", mIav);
}

AmVideoDevice::~AmVideoDevice()
{
  for (uint32_t i = 0; i < mVinNumber; ++ i) {
    delete mVinList[i];
    delete mVinParamList[i];
  }
  delete[] mVinList;
  delete[] mVinParamList;
  delete[] mVinTypeList;
  for (uint32_t i = 0; i < mVoutNumber; ++ i) {
    delete mVoutList[i];
    delete mVoutParamList[i];
  }
  delete[] mVoutList;
  delete[] mVoutParamList;
  delete[] mVoutTypeList;
  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    delete mStreamParamList[i];
  }
  delete[] mStreamParamList;
  delete mEncoderParams;
  delete mPhotoParams;
  unmap_overlay();
  if (mIav >= 0) {
    close(mIav);
  }
  DEBUG("AmVideoDevice deleted!");
}

bool AmVideoDevice::create_video_device(VDeviceParameters *vdevConfig)
{
  if ((false == mIsDevCreated) && vdevConfig &&
      (vdevConfig->vin_number != 0) &&
      (vdevConfig->vout_number != 0)) {
    mVinNumber    = vdevConfig->vin_number;
    mVinList      = new AmVin* [mVinNumber];
    mVinParamList = new VinParameters* [mVinNumber];
    mVinTypeList  = new VinType [mVinNumber];
    for (uint32_t i = 0; i < mVinNumber; ++ i) {
      mVinList[i] = NULL;
      mVinParamList[i] = NULL;
      mVinTypeList[i] = vdevConfig->vin_list[i];
      switch(mVinTypeList[i]) {
        case AM_VIN_TYPE_RGB:
          mVinList[i] = new AmVinRgb(mIav);
          break;
        case AM_VIN_TYPE_YUV:
          mVinList[i] = new AmVinYuv(mIav);
          break;
        default:break;
      }
    }
    mVoutNumber    = vdevConfig->vout_number;
    mVoutList      = new AmVout* [mVoutNumber];
    mVoutParamList = new VoutParameters* [mVoutNumber];
    mVoutTypeList  = new VoutType [mVoutNumber];
    for (uint32_t i = 0; i < mVoutNumber; ++ i) {
      mVoutList[i] = NULL;
      mVoutParamList[i] = NULL;
      mVoutTypeList[i] = vdevConfig->vout_list[i];
      switch(mVoutTypeList[i]) {
        case AM_VOUT_TYPE_LCD:
          mVoutList[i] = new AmVoutLcd(mIav);
          break;
        case AM_VOUT_TYPE_HDMI:
          mVoutList[i] = new AmVoutHdmi(mIav);
          break;
        case AM_VOUT_TYPE_CVBS:
          mVoutList[i] = new AmVoutCvbs(mIav);
          break;
        default: break;
      }
    }

    if (vdevConfig->stream_number) {
      mStreamNumber = vdevConfig->stream_number;
      mStreamParamList = new StreamParameters*[mStreamNumber];
      memset(mStreamParamList, 0, mStreamNumber * sizeof(StreamParameters*));
    }

    mEncoderParams = new EncoderParameters();
    mPhotoParams   = new PhotoParameters();

    mIsDevCreated = true;
  } else if (!vdevConfig) {
    ERROR("Invlid VideoDevice configurations!");
  } else if (vdevConfig->vin_number == 0) {
    ERROR("No VIN device found!");
  }

  return mIsDevCreated;
}

void AmVideoDevice::set_vin_config(VinParameters *config, uint32_t vinId)
{
  if (vinId < mVinNumber) {
    if (!mVinList[vinId]) {
      ERROR("No such VIN device, VIN id is %u!", vinId + 1);
    } else if (config) {
      if (!mVinParamList[vinId]) {
        mVinParamList[vinId] = new VinParameters();
      }
      memcpy(mVinParamList[vinId], config, sizeof(*config));
      mVinList[vinId]->set_vin_config(mVinParamList[vinId]);
    }
  } else {
    ERROR("VIN device number is %u, but VIN id is %u!", mVinNumber, vinId + 1);
  }
}

void AmVideoDevice::set_vout_config(VoutParameters *config, uint32_t voutId)
{
  if (voutId < mVoutNumber) {
    if (!mVoutList[voutId]) {
      NOTICE("No such VOUT device, VOUT id is %u", voutId + 1);
    } else if (config) {
      if (!mVoutParamList[voutId]) {
        mVoutParamList[voutId] = new VoutParameters();
      }
      memcpy(mVoutParamList[voutId], config, sizeof(*config));
      mVoutList[voutId]->set_vout_config(mVoutParamList[voutId]);
    }
  } else {
    ERROR("VOUT device number is %u, but VOUT id is %u!",
          mVoutNumber, voutId + 1);
  }
}

void AmVideoDevice::set_encoder_config(EncoderParameters *config)
{
  if (config) {
    memcpy(mEncoderParams, config, sizeof(*config));
  }
}

void AmVideoDevice::set_stream_config(StreamParameters *config,
                                      uint32_t streamId)
{
  if (streamId < mStreamNumber) {
    if (config) {
      if (!mStreamParamList[streamId]) {
        mStreamParamList[streamId] = new StreamParameters();
      }
      memcpy(mStreamParamList[streamId], config, sizeof(*config));
    }
  } else {
    ERROR("Stream ID is %d.  The max stream number is %d.\n", streamId,
          mStreamNumber);
  }
}

void AmVideoDevice::set_photo_config(PhotoParameters *config)
{
  if (config) {
    memcpy(mPhotoParams, config, sizeof(*config));
  }
}

bool AmVideoDevice::update_encoder_parameters()
{
  bool ret = false;
#ifdef AM_CAMERA_DEBUG
  iav_system_resource_setup_ex_t &buf =
      mEncoderParams->system_resource_info;
  iav_source_buffer_format_all_ex_t &format =
      mEncoderParams->buffer_format_info;
#endif
  do {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_SYSTEM_SETUP_INFO_EX,
                          &(mEncoderParams->system_setup_info)) < 0)) {
      PERROR("IAV_IOC_SET_SYSTEM_SETUP_INFO_EX");
      break;
    }
    INFO("IAV_IOC_SET_SYSTEM_SETUP_INFO_EX");

#ifdef AM_CAMERA_DEBUG
    DEBUG("1st maxw: %4hu, maxh: %4hu", buf.main_source_buffer_max_width,
          buf.main_source_buffer_max_height);
    DEBUG("2nd maxw: %4hu, maxh: %4hu", buf.second_source_buffer_max_width,
          buf.second_source_buffer_max_height);
    DEBUG("3rd maxw: %4hu, maxh: %4hu", buf.third_source_buffer_max_width,
          buf.third_source_buffer_max_height);
    DEBUG("4th maxw: %4hu, maxh: %4hu", buf.fourth_source_buffer_max_width,
          buf.fourth_source_buffer_max_height);
#endif
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX,
                          &(mEncoderParams->system_resource_info)) < 0)) {
      PERROR("IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX");
      break;
    }
    INFO("IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX");

#ifdef AM_CAMERA_DEBUG
    DEBUG("1st buffer type is %u",
          (uint32_t)mEncoderParams->buffer_type_info.main_buffer_type);
    DEBUG("2nd buffer type is %u",
          (uint32_t)mEncoderParams->buffer_type_info.second_buffer_type);
    DEBUG("3rd buffer type is %u",
          (uint32_t)mEncoderParams->buffer_type_info.third_buffer_type);
    DEBUG("4th buffer type is %u",
          (uint32_t)mEncoderParams->buffer_type_info.fourth_buffer_type);
#endif

    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX,
                          &(mEncoderParams->buffer_type_info)) < 0)) {
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX");
      break;
    }
    INFO("IAV_IOC_SET_SOURCE_BUFFER_TYPE_ALL_EX");

#ifdef AM_CAMERA_DEBUG
    DEBUG("1st w: %4hu, 1st h: %4hu",
          format.main_width, format.main_height);
    DEBUG("2nd w: %4hu, 2nd h: %4hu",
          format.second_width, format.second_height);
    DEBUG("3rd w: %4hu, 3rd h: %4hu",
          format.third_width, format.third_height);
    DEBUG("4th w: %4hu, 4th h: %4hu",
          format.fourth_width, format.fourth_height);
#endif
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX,
                          &(mEncoderParams->buffer_format_info)) < 0)) {
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX");
      break;
    }

    INFO("IAV_IOC_SET_SOURCE_BUFFER_FORMAT_ALL_EX");
    ret = true;
  } while (0);
  return ret;
}

bool AmVideoDevice::update_stream_format(uint32_t streamId)
{
  mStreamParamList[streamId]->encode_params.encode_format.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_ENCODE_FORMAT_EX,
                        &(mStreamParamList[streamId]->encode_params.\
                            encode_format)) < 0)) {
    PERROR("IAV_IOC_SET_ENCODE_FORMAT_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_SET_ENCODE_FORMAT_EX", streamId);
  return true;
}

bool AmVideoDevice::update_stream_h264_config(uint32_t streamId)
{
  mStreamParamList[streamId]->h264_config.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_H264_CONFIG_EX,
                        &(mStreamParamList[streamId]->h264_config)) < 0)) {
    PERROR("IAV_IOC_SET_H264_CONFIG_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_SET_H264_CONFIG_EX", streamId);
  return true;
}

bool AmVideoDevice::update_stream_mjpeg_config(uint32_t streamId)
{
  mStreamParamList[streamId]->mjpeg_config.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_JPEG_CONFIG_EX,
                        &(mStreamParamList[streamId]->mjpeg_config)) < 0)) {
    PERROR("IAV_IOC_SET_JPEG_CONFIG_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_SET_JPEG_CONFIG_EX", streamId);
  return true;
}

bool AmVideoDevice::update_stream_framerate(uint32_t streamId)
{
  mStreamParamList[streamId]->encode_params.encode_framerate.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX,
                        &(mStreamParamList[streamId]->encode_params.\
                            encode_framerate)) < 0)) {
    PERROR("IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_CHANGE_FRAMERATE_FACTOR_EX", streamId);
  return true;
}

bool AmVideoDevice::update_stream_h264_bitrate(uint32_t streamId)
{
  mStreamParamList[streamId]->bitrate_info.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_SET_BITRATE_EX,
                        &(mStreamParamList[streamId]->bitrate_info)) < 0)) {
    PERROR("IAV_IOC_SET_BITRATE_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_SET_BITRATE_EX", streamId);
  return true;
}

bool AmVideoDevice::update_stream_overlay(uint32_t streamId)
{
  OverlayAreaBuffer *pBuffer = NULL;
  mStreamParamList[streamId]->overlay.insert.id = 1 << streamId;

  if (map_overlay()) {
    for (uint32_t i = 0; i < MAX_OVERLAY_AREA_NUM; ++i) {
      if (mStreamParamList[streamId]->overlay.insert.area[i].enable) {
        pBuffer = &mStreamParamList[streamId]->overlay.buffer[i];
        mStreamParamList[streamId]->overlay.insert.enable = 1;
        mStreamParamList[streamId]->overlay.insert.area[i].data =
            pBuffer->data_addr[pBuffer->next_id];
        DEBUG("Stream%u Area%u: clut id %u, data 0x%x (Buffer %u)", streamId, i,
              mStreamParamList[streamId]->overlay.insert.area[i].clut_id,
              (uint32_t)mStreamParamList[streamId]->overlay.insert.area[i].data,
              pBuffer->next_id);
      }
      mStreamParamList[streamId]->overlay.insert.enable |=
          mStreamParamList[streamId]->overlay.insert.area[i].enable;
}

  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_OVERLAY_INSERT_EX,
          &(mStreamParamList[streamId]->overlay.insert)) < 0)) {
    PERROR("IAV_IOC_OVERLAY_INSERT_EX");
    return false;
  }
  INFO("Stream%u: IAV_IOC_OVERLAY_INSERT_EX", streamId);
    for (uint32_t i = 0; i < MAX_OVERLAY_AREA_NUM; ++i) {
      if (mStreamParamList[streamId]->overlay.insert.area[i].enable) {
        pBuffer = &mStreamParamList[streamId]->overlay.buffer[i];
        pBuffer->active_id = pBuffer->next_id;
      }
    }
  }
  return true;
}
#ifdef CONFIG_ARCH_S2

bool AmVideoDevice::check_system_resource()
{
  bool ret = true;
  Resolution vinRes;
  iav_system_resource_setup_ex_t &sysResource =
        mEncoderParams->system_resource_info;

  /* Fixme: Currently only 1 VIN is supported on A5s */
  if (mVinList[0] && mVinList[0]->get_current_vin_size(vinRes)) {
    if (sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width > vinRes.width) {
      WARN("Main buffer max width %hu is greater than VIN width %hu, "
          "reset main buffer max width to %hu",
          sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width,
          vinRes.width, vinRes.width);
      sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width  = vinRes.width;
    }
    if (sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height > vinRes.height) {
      WARN("Main buffer max height %hu is greater than VIN height %hu, "
          "reset main buffer max height to %hu",
          sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height,
          vinRes.height, vinRes.height);
      sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height = vinRes.height;
    }
  } else {
    ERROR("Failed to get current VIN size!");
    ret = false;
  }

  return ret;
}

#else

bool AmVideoDevice::check_system_resource()
{
  bool ret = true;
  Resolution vinRes;
  iav_system_resource_setup_ex_t &sysResource =
        mEncoderParams->system_resource_info;

  /* Fixme: Currently only 1 VIN is supported on A5s */
  if (mVinList[0] && mVinList[0]->get_current_vin_size(vinRes)) {
    if (sysResource.main_source_buffer_max_width > vinRes.width) {
      WARN("Main buffer max width %hu is greater than VIN width %hu, "
          "reset main buffer max width to %hu",
          sysResource.main_source_buffer_max_width,
          vinRes.width, vinRes.width);
      sysResource.main_source_buffer_max_width  = vinRes.width;
    }
    if (sysResource.main_source_buffer_max_height > vinRes.height) {
      WARN("Main buffer max height %hu is greater than VIN height %hu, "
          "reset main buffer max height to %hu",
          sysResource.main_source_buffer_max_height,
          vinRes.height, vinRes.height);
      sysResource.main_source_buffer_max_height = vinRes.height;
    }
  } else {
    ERROR("Failed to get current VIN size!");
    ret = false;
  }

  return ret;
}
#endif
bool AmVideoDevice::check_encoder_parameters()
{
  /* ToDo: check encoder parameter if necessary */
  return true;
}

bool AmVideoDevice::is_stream_encoding(uint32_t streamId)
{
  bool ret = false;
  iav_encode_stream_info_ex_t info;
  info.id = (1 << streamId);
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
          &info) < 0)) {
    ERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX: streams [%d] %s", info.id,
        strerror(errno));
  } else {
    if (info.state == IAV_STREAM_STATE_ENCODING) {
      ret = true;
    }
  }
  return ret;
}

bool AmVideoDevice::check_stream_id(uint32_t streamId)
{
  if (streamId >= mStreamNumber) {
    ERROR("Only support [%d] streams.", mStreamNumber);
    return false;
  }
  return true;
}

bool AmVideoDevice::check_stream_format(uint32_t streamId)
{
  iav_source_buffer_format_all_ex_t &bufferFormat =
      mEncoderParams->buffer_format_info;
  uint16_t &w =
      mStreamParamList[streamId]->encode_params.encode_format.encode_width;
  uint16_t &h =
      mStreamParamList[streamId]->encode_params.encode_format.encode_height;
  uint16_t &offX =
      mStreamParamList[streamId]->encode_params.encode_format.encode_x;
  uint16_t &offY =
      mStreamParamList[streamId]->encode_params.encode_format.encode_y;
  uint8_t &source =
      mStreamParamList[streamId]->encode_params.encode_format.source;

  switch (source) {
    case 0:
      if (w > bufferFormat.main_width || h > bufferFormat.main_height) {
        ERROR("Stream%u: size [%hux%hu] exceeds [%hux%hu].", streamId, w, h,
              bufferFormat.main_width, bufferFormat.main_height);
        return false;
      }
      if ((w + offX) > bufferFormat.main_width) {
        WARN("Stream%u: encoding X offset: %u, width boundary %u has "
            "exceeded %u, reset to %u", streamId, offX, w + offX,
            bufferFormat.main_width, bufferFormat.main_width - offX);
        w = bufferFormat.main_width - offX;
        mStreamParamList[streamId]->config_changed = 1;
      }
      if ((h + offY) > bufferFormat.main_height) {
        WARN("Stream%u: encoding Y offset: %u, height boundary %u has "
            "exceeds %u, reset to %u", streamId, offY, h + offY,
            bufferFormat.main_height, bufferFormat.main_height - offY);
        h = bufferFormat.main_height - offY;
        mStreamParamList[streamId]->config_changed = 1;
      }
      break;

    case 1:
      if (w > bufferFormat.second_width || h > bufferFormat.second_height) {
        ERROR("Stream%u: size [%dx%d] excees [%dx%d].", w, h,
              bufferFormat.second_width, bufferFormat.second_height);
        return false;
      }
      if ((w + offX) > bufferFormat.second_width) {
        WARN("Stream%u: encoding X offset: %u, width boundary %u has "
            "exceeded %u, reset to %u", streamId, offX, w + offX,
            bufferFormat.second_width, bufferFormat.second_width - offX);
        w = bufferFormat.second_width - offX;
        mStreamParamList[streamId]->config_changed = 1;
      }
      if ((h + offY) > bufferFormat.second_height) {
        WARN("Stream%u: encoding Y offset: %u, height boundary %u has "
            "exceeds %u, reset to %u", streamId, offY, h + offY,
            bufferFormat.second_height, bufferFormat.second_height - offY);
        h = bufferFormat.second_height - offY;
        mStreamParamList[streamId]->config_changed = 1;
      }
      break;

    case 2:
      if (w > bufferFormat.third_width || h > bufferFormat.third_height) {
        ERROR("Stream%u: size [%dx%d] excees [%dx%d].", w, h,
              bufferFormat.third_width, bufferFormat.third_height);
        return false;
      }
      if ((w + offX) > bufferFormat.third_width) {
        WARN("Stream%u: encoding X offset: %u, width boundary %u has "
            "exceeded %u, reset to %u", streamId, offX, w + offX,
            bufferFormat.third_width, bufferFormat.third_width - offX);
        w = bufferFormat.third_width - offX;
        mStreamParamList[streamId]->config_changed = 1;
      }
      if ((h + offY) > bufferFormat.third_height) {
        WARN("Stream%u: encoding Y offset: %u, height boundary %u has "
            "exceeds %u, reset to %u", streamId, offY, h + offY,
            bufferFormat.third_height, bufferFormat.third_height - offY);
        h = bufferFormat.third_height - offY;
        mStreamParamList[streamId]->config_changed = 1;
      }
      break;

    case 3:
      if (w > bufferFormat.fourth_width || h > bufferFormat.fourth_height) {
        ERROR("Stream%u: size [%dx%d] excees [%dx%d].", w, h,
              bufferFormat.fourth_width, bufferFormat.fourth_height);
        return false;
      }
      if ((w + offX) > bufferFormat.fourth_width) {
        WARN("Stream%u: encoding X offset: %u, width boundary %u has "
            "exceeded %u, reset to %u", streamId, offX, w + offX,
            bufferFormat.fourth_width, bufferFormat.fourth_width - offX);
        w = bufferFormat.fourth_width - offX;
        mStreamParamList[streamId]->config_changed = 1;
      }
      if ((h + offY) > bufferFormat.fourth_height) {
        WARN("Stream%u: encoding Y offset: %u, height boundary %u has "
            "exceeds %u, reset to %u", streamId, offY, h + offY,
            bufferFormat.fourth_height, bufferFormat.fourth_height - offY);
        h = bufferFormat.fourth_height - offY;
        mStreamParamList[streamId]->config_changed = 1;
      }
      break;
    default:
      ERROR("No such buffer: %hhu!", source);
      return false;
  }

  return true;
}

bool AmVideoDevice::check_stream_h264_config(uint32_t streamId)
{
  iav_system_resource_setup_ex_t &system_resource_info =
      mEncoderParams->system_resource_info;
  uint16_t &m = mStreamParamList[streamId]->h264_config.M;
  uint16_t &n = mStreamParamList[streamId]->h264_config.N;
  uint8_t &idr = mStreamParamList[streamId]->h264_config.idr_interval;

  if (m > system_resource_info.stream_max_GOP_M[streamId]) {
    WARN("Stream%u: M [%d] exceeds the encoder limit [%d]. Reset to the limit.",
         m, system_resource_info.stream_max_GOP_M[streamId]);
    mStreamParamList[streamId]->config_changed = 1;
  }
  if (n > system_resource_info.stream_max_GOP_N[streamId]) {
    WARN("Stream%u: GOP length N[%d] exceeds the encoder limit [%d]. "
        "Reset to 30.", n, system_resource_info.stream_max_GOP_N[streamId]);
    n = 30;
    mStreamParamList[streamId]->config_changed = 1;
  }

  if (!idr) {
    WARN("Stream%u: IDR interval cannot be zero. Reset to 1.");
    idr = 1;
    mStreamParamList[streamId]->config_changed = 1;
  }
  return true;
}

bool AmVideoDevice::apply_encoder_parameters()
{
  return (check_system_resource() && assign_buffer_to_stream() &&
          check_encoder_parameters() && update_encoder_parameters());
}

bool AmVideoDevice::apply_stream_parameters()
{
  if (!check_system_performance())
    return false;

  for (uint32_t sId = 0; sId < mStreamNumber; ++sId) {
    if (mStreamParamList[sId]->encode_params.encode_format.encode_type
        != IAV_ENCODE_NONE) {
      if (!check_stream_format(sId) || !update_stream_format(sId)
          || !update_stream_framerate(sId)
          || !update_stream_overlay(sId))
        return false;

      if (mStreamParamList[sId]->encode_params.encode_format.encode_type ==
          IAV_ENCODE_H264) {
        if (!check_stream_h264_config(sId) || !update_stream_h264_config(sId)
            || !update_stream_h264_bitrate(sId))
          return false;
      } else {
        if (!update_stream_mjpeg_config(sId))
          return false;
      }
    }
  }
  return true;
}

IavState AmVideoDevice::get_iav_status()
{
  iav_state_info_t status;
  IavState iavStatus = AM_IAV_INIT;
  if (mIav >= 0) {
    if (ioctl(mIav, IAV_IOC_GET_STATE_INFO, &status) < 0) {
      PERROR("IAV_IOC_GET_STATE_INFO");
    } else {
      iavStatus = (IavState)((status.state == IAV_STATE_INIT) ?
          AM_IAV_INIT : status.state);
    }
  }

  return iavStatus;
}

bool AmVideoDevice::idle()
{
  bool ret = true;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_ENTER_IDLE, 0) < 0)) {
    PERROR("IAV_IOC_ENTER_IDLE");
    ret = false;
  }
  if (ret) {
    DEBUG("Enter IDLE state!");
  }

  return ret;
}

bool AmVideoDevice::preview()
{
  bool ret = true;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_ENABLE_PREVIEW, 0) < 0)) {
    PERROR("IAV_IOC_ENABLE_PREVIEW");
    ret = false;
  }
  if (ret) {
    DEBUG("Enter PREVIEW state!");
  }

  return ret;
}

bool AmVideoDevice::encode(bool start)
{
  bool ret = false;
  uint16_t disabledStream = 0;
  if (start) {
    if (apply_stream_parameters()) {
      iav_encode_stream_info_ex_t info;
      for (uint32_t i = 0; i < mStreamNumber; ++ i) {
        if (mStreamParamList[i]->encode_params.\
            encode_format.encode_type > 0) { /* Stream not disabled */
          info.id = 1 << i;
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
                                &info) < 0)) {
            PERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
            break;
          }
          if (info.state == IAV_STREAM_STATE_ENCODING) {
            ret = true;
            INFO("Stream%u: Encoding already started!", i);
          } else {
            if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_START_ENCODE_EX,
                                  info.id) < 0)) {
              ERROR("Stream%u: IAV_IOC_START_ENCODE_EX: %s",
                    i, strerror(errno));
              break;
            }
            INFO("Start encoding for Stream%u successfully!", i);
          }
          ret = true;
        } else {
          disabledStream |= 1 << i;
        }
      }
      if (ret) {
        DEBUG("Enter ENCODING state!");
      } else if (disabledStream == ((1<<MAX_ENCODE_STREAM_NUM) - 1)) {
        WARN("All streams are disabled!");
      }
    }
  } else {
    iav_encode_stream_info_ex_t sInfo;
    iav_stream_id_t sId = 0;

    for (uint32_t i = 0; i < mStreamNumber; ++ i) {
      sInfo.id = (1 << i);
      if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_STREAM_INFO_EX,
                            &sInfo) < 0)) {
        ERROR("Failed getting encode stream info for stream %u", i + 1);
        PERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
      } else if (sInfo.state == IAV_STREAM_STATE_ENCODING) {
        sId |= sInfo.id;
        INFO("Stream %u needs to be stopped!", i);
      }
    }
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_STOP_ENCODE_EX, sId) < 0)) {
      PERROR("IAV_IOC_STOP_ENCODE_EX");
    } else {
      ret = true;
    }
    if (ret) {
      DEBUG("Leave ENCODING state!");
    }
  }

  return ret;
}

bool AmVideoDevice::decode(bool start, int mode)
{
  bool ret = true;
  if (start) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_START_DECODE, 0) < 0)) {
      PERROR("IAV_IOC_START_DECODE");
      ret = false;
    }
    if (ret) {
      DEBUG("Enter DECODING state!");
    }
  } else {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_STOP_DECODE, !!mode) < 0)) {
      PERROR("IAV_IOC_STOP_DECODE");
      ret = false;
    }
    if (ret) {
      DEBUG("Leave DECODING  state!");
    }
  }

  return ret;
}

bool AmVideoDevice::map_dsp()
{
  if (!mIsDspMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_MAP_DSP, &mDspMapInfo) < 0)) {
      PERROR("IAV_IOC_MAP_DSP");
    } else {
      mIsDspMapped = true;
    }
  }

  return mIsDspMapped;
}

bool AmVideoDevice::map_bsb()
{
  if (!mIsBsbMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_MAP_BSB, &mBsbMapInfo) < 0)) {
      PERROR("IAV_IOC_MAP_BSB");
    } else {
      mIsBsbMapped = true;
    }
  }

  return mIsBsbMapped;
}

bool AmVideoDevice::map_overlay()
{
  if (!mIsOverlayMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_MAP_OVERLAY, &mOverlayMapInfo) < 0)) {
      PERROR("IAV_IOC_MAP_OVERLAY");
    } else {
      mIsOverlayMapped = true;
      DEBUG("IAV_IOC_MAP_OVERLAY: start = 0x%x, len = 0x%x.", (
          uint32_t)mOverlayMapInfo.addr, mOverlayMapInfo.length);

      OverlayAreaBuffer *pBuffer = NULL;
      uint32_t clutSize = MAX_OVERLAY_CLUT_SIZE * mStreamNumber *
          MAX_OVERLAY_AREA_NUM;
      uint32_t streamMaxSize =
          (mOverlayMapInfo.length - clutSize) / mStreamNumber;
      uint32_t areaMaxSize = streamMaxSize / MAX_OVERLAY_AREA_NUM;
      uint8_t *clutAddr = mOverlayMapInfo.addr;
      uint8_t *dataAddr = clutAddr + clutSize;
      uint32_t totalAreaNum = 0;
      for (uint32_t sId = 0; sId < mStreamNumber; ++sId) {
        for (uint32_t areaId = 0; areaId < MAX_OVERLAY_AREA_NUM; ++areaId) {
          ++ totalAreaNum;
          pBuffer = &mStreamParamList[sId]->overlay.buffer[areaId];
          pBuffer->num = AM_OVERLAY_BUFFER_NUM_SINGLE;
          pBuffer->active_id = pBuffer->next_id = 0;
          mStreamParamList[sId]->overlay.insert.area[areaId].clut_id =
              totalAreaNum - 1;
          pBuffer->clut_addr = clutAddr
              + MAX_OVERLAY_CLUT_SIZE * (totalAreaNum - 1);
#if 0
          DEBUG("Stream%u Area%u Clut%u: start = 0x%x", sId, areaId,
                mStreamParamList[sId]->overlay.insert.area[areaId].clut_id,
                (uint32_t)pBuffer->clut_addr);
#endif
          pBuffer->max_size = areaMaxSize / pBuffer->num;
          for (uint32_t bufId = 0; bufId < pBuffer->num; ++bufId) {
            pBuffer->data_addr[bufId] = dataAddr + sId * streamMaxSize
                + areaId * areaMaxSize + bufId * pBuffer->max_size;
#if 0
            DEBUG("Stream%u Area%u Buffer%u: start = 0x%x, size = 0x%x.",
                  sId, areaId, bufId, (uint32_t)pBuffer->data_addr[bufId],
                  pBuffer->max_size);
#endif
          }
        }
      }
    }
  }

  return mIsOverlayMapped;
}

bool AmVideoDevice::unmap_dsp()
{
  if (mIsDspMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_UNMAP_DSP, &mDspMapInfo) < 0)) {
      PERROR("IAV_IOC_UNMAP_DSP");
    } else {
      mIsDspMapped = false;
    }
  }

  return !mIsDspMapped;
}

bool AmVideoDevice::unmap_bsb()
{
  if (mIsBsbMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_UNMAP_BSB, &mBsbMapInfo) < 0)) {
      PERROR("IAV_IOC_UNMAP_BSB");
    } else {
      mIsBsbMapped = false;
    }
  }

  return !mIsBsbMapped;
}

bool AmVideoDevice::unmap_overlay()
{
  if (mIsOverlayMapped) {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_UNMAP_OVERLAY, &mOverlayMapInfo) < 0)) {
      PERROR("IAV_IOC_UNMAP_OVERLAY");
    } else {
      mIsOverlayMapped = false;
    }
  }

  return !mIsOverlayMapped;
}

const char* AmVideoDevice::iav_state_to_str(IavState state)
{
  if ((state >= AM_IAV_IDLE) && (state < AM_IAV_INVALID)) {
    return iavStatus[(int)state];
  }

  return iavStatus[sizeof(iavStatus)/sizeof(char *) - 1];
}

bool AmVideoDevice::check_system_performance()
{
  bool ret = false;
  if (mIsDevCreated) {
    /* TODO: A5s has only 1 VIN device, but A7, S2 and above will have 2 VIN
     * devices, the relationship between VIN devices and encoding streams are
     * undefined, in this case, only VIN0's FPS can affect Stream's encoding
     * frame-rate.
     */
    do {
      uint32_t vinFpsQ9 = mVinList[0]->get_vin_fps(), vinFps = 0;
      uint32_t countMacroBlocks = 0, uncountMacroBlocks = 0;
      uint32_t maxMacroBlocks = get_system_max_performance();
      uint32_t macroBlocks[MAX_ENCODE_STREAM_NUM];
      switch (vinFpsQ9) {
        case AMBA_VIDEO_FPS_29_97:
          vinFps = 30;
          break;
        case AMBA_VIDEO_FPS_59_94:
          vinFps = 60;
          break;
        case AMBA_VIDEO_FPS_23_976:
          vinFps = 24;
          break;
        default:
          vinFps = round_div(512000000, vinFpsQ9);
          break;
      }
      for (uint32_t i = 0; i < mStreamNumber; ++ i) {
        if (mStreamParamList[i]->encode_params.\
            encode_format.encode_type > 0) {/*Stream not disabled*/
          uint16_t &w = mStreamParamList[i]->encode_params.encode_format.\
              encode_width;
          uint16_t &h = mStreamParamList[i]->encode_params.encode_format.\
              encode_height;
          uint8_t &numerator = mStreamParamList[i]->encode_params.\
              encode_framerate.ratio_numerator;
          uint8_t &denominator = mStreamParamList[i]->encode_params.\
              encode_framerate.ratio_denominator;
          macroBlocks[i] = (round_up(w, 16)/16) * (round_up(h, 16)/16);
          if (numerator && denominator) {
            countMacroBlocks += macroBlocks[i] * vinFps * numerator / denominator;
            DEBUG("Stream%u: %ux%up%u", i, w, h, vinFps * numerator / denominator);
          } else {
            uncountMacroBlocks += macroBlocks[i];
          }
        }
      }

      if ((maxMacroBlocks != 0) && (countMacroBlocks <= maxMacroBlocks)) {
        if (uncountMacroBlocks) {
          uint32_t remainMacroBlocks = maxMacroBlocks - countMacroBlocks;
          uint32_t fps = remainMacroBlocks / uncountMacroBlocks;
          if (AM_UNLIKELY(fps > vinFps)) {
            fps = vinFps;
          }
          for (uint32_t i = 0; i < mStreamNumber; ++ i) {
            if (AM_UNLIKELY(fps == 0)) {
              mStreamParamList[i]->encode_params.encode_format.encode_type =
                  IAV_ENCODE_NONE;
            }
            if ((mStreamParamList[i]->encode_params.encode_format.encode_type > 0)
                && (mStreamParamList[i]->encode_params.encode_framerate.\
                    ratio_numerator == 0)) { // frame rate not configure yet
              mStreamParamList[i]->encode_params.encode_framerate.\
                  ratio_numerator = fps;
              mStreamParamList[i]->encode_params.encode_framerate.\
                  ratio_denominator = vinFps;
              DEBUG("Stream%u: %ux%u, auto framerate %ufps", i,
                    mStreamParamList[i]->encode_params.encode_format.encode_width,
                    mStreamParamList[i]->encode_params.encode_format.encode_height,
                    fps);
              countMacroBlocks += fps * macroBlocks[i];
            }
          }
        }
        DEBUG("Total macroblocks: %u; maximum macroblocks: %u",
              countMacroBlocks, maxMacroBlocks);
      } else if (maxMacroBlocks == 0) {
        ERROR("Failed getting maximum performance for chip %s!", chip_string());
        break;
      } else {
        char message[1024] = {0};
        char *head = message;
        for (uint32_t i = 0; i < mStreamNumber; ++ i) {
          if (mStreamParamList[i]->encode_params.\
              encode_format.encode_type > 0) {/*Stream not disabled*/
            uint16_t &w = mStreamParamList[i]->\
                encode_params.encode_format.encode_width;
            uint16_t &h = mStreamParamList[i]->\
                encode_params.encode_format.encode_height;
            uint8_t &numerator = mStreamParamList[i]->\
                encode_params.encode_framerate.ratio_numerator;
            uint8_t &denominator = mStreamParamList[i]->\
                encode_params.encode_framerate.ratio_denominator;
            sprintf(head, "\nStream%u: %ux%up%u", i, w, h,
                    (vinFps*numerator/denominator));
            head = message + strlen(message);
          }
        }
        sprintf(head, " %u > %u \nExceeds the maximum performance of chip %s!",
                countMacroBlocks, maxMacroBlocks, chip_string());
        ERROR("%s", message);
        break;
      }
      ret = true;
    } while(0);
  }

  return ret;
}

#ifdef CONFIG_ARCH_S2

bool AmVideoDevice::assign_buffer_to_stream()
{
  uint32_t resOrder[MAX_ENCODE_STREAM_NUM] = {0};
  bool isBufferUsed[MAX_ENCODE_BUFFER_NUM] = {false};
  iav_source_buffer_format_all_ex_t &bufferFormat =
      mEncoderParams->buffer_format_info;
  iav_source_buffer_type_all_ex_t &bufferType =
      mEncoderParams->buffer_type_info;
  iav_system_resource_setup_ex_t &sysResource =
      mEncoderParams->system_resource_info;
  Resolution &yuvData = mEncoderParams->yuv_buffer_size;
  StreamParameters *streamList[MAX_ENCODE_STREAM_NUM] = {0};
  Resolution vinRes;

  /* Fixme: Only 1 VIN device is supported currently */
  if (!(mVinList[0] && mVinList[0]->get_current_vin_size(vinRes))) {
    ERROR("Failed to get VIN resolution!");
    return false;
  }

  /* Exclude third/fourth buffer from assignment if they are for preview */
  if (bufferType.third_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
    isBufferUsed[2] = true;
    NOTICE("Third buffer is used for Preivew!");
  }
  if (bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
    isBufferUsed[3] = true;
    NOTICE("Fourth buffer is used for Preivew!");
  }

  mEncoderParams->yuv_buffer_id = MAX_ENCODE_BUFFER_NUM;
  /* Assign buffer to YUV if YUV requires independent buffer */
  if ((mEncoderParams->buffer_assign_method == AM_BUFFER_TO_STREAM_YUV) &&
      yuvData.width && yuvData.height) {
    bool assignedBuffer = false;

    /* Candidate: second/third buffer */
    for (uint32_t bufId = 1; bufId <= 3; ++ bufId) {
      switch(bufId) {
        case 1: {
          if (!isBufferUsed[bufId] &&
              (yuvData.width <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width) &&
              (yuvData.height <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height)) {
            isBufferUsed[bufId] = true;
            assignedBuffer = true;
            mEncoderParams->yuv_buffer_id = bufId;
            bufferFormat.second_width = yuvData.width;
            bufferFormat.second_height = yuvData.height;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to YUV %ux%u",
                  bufId, yuvData.width, yuvData.height);
          }
        }break;
        case 2: {
          if ((yuvData.width <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width) &&
              (yuvData.height <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height)) {
            assignedBuffer = true;
            mEncoderParams->yuv_buffer_id = bufId;
            bufferFormat.third_width = yuvData.width;
            bufferFormat.third_height = yuvData.height;
            if (!isBufferUsed[bufId]) {
              bufferType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            }
            isBufferUsed[bufId] = true;
            DEBUG("Buffer %u assigned to YUV %ux%u",
                  bufId, yuvData.width, yuvData.height);
          }
        }break;
        case 3: {
          if ((yuvData.width <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width) &&
              (yuvData.height <= sysResource.\
               buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height)) {
            assignedBuffer = true;
            mEncoderParams->yuv_buffer_id = bufId;
            bufferFormat.fourth_width = yuvData.width;
            bufferFormat.fourth_height = yuvData.height;
            if (!isBufferUsed[bufId]) {
              bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            }
            isBufferUsed[bufId] = true;
            DEBUG("Buffer %u assigned to YUV %ux%u",
                  bufId, yuvData.width, yuvData.height);
          }
        }break;
        default:break;
      }
      if (assignedBuffer) {
        break;
      }
    }
    if (!assignedBuffer) {
      ERROR("Cannot find source buffer for YUV data %ux%u.", yuvData.width,
            yuvData.height);
    }
  }

  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[i]->encode_params.encode_format;
    if (format.encode_type != IAV_ENCODE_NONE) {
      uint16_t &mainMaxW =
          sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width;
      uint16_t &mainMaxH =
          sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height;
      if ((format.encode_width > mainMaxW) ||
          (format.encode_height > mainMaxH)) {
        if (((format.encode_width > mainMaxW) &&
            (format.encode_width <= vinRes.width))) {
          WARN("Stream%u width %hu is greater than max width %hu but smaller "
               "than VIN width %hu!",
               i, format.encode_width, mainMaxW, vinRes.width);
        } else if (format.encode_width > vinRes.width) {
          ERROR("Stream%u width exceeds the max width %hu!", i, vinRes.width);
          return false;
        }
        if ((format.encode_height > mainMaxH) &&
            (format.encode_height <= vinRes.height)) {
          WARN("Stream%u height %hu is greater than max height %hu but smaller "
               "than VIN height %hu!",
               i, format.encode_height, mainMaxH, vinRes.height);
        } else if (format.encode_height > vinRes.height) {
          ERROR("Stream%u width exceeds the max width %hu!", i, vinRes.height);
          return false;
        }
        WARN("Reset main buffer max size to VIN size [%hux%hu]",
             vinRes.width, vinRes.height);
        mainMaxW = vinRes.width;
        mainMaxH = vinRes.height;
      }
      streamList[i] = mStreamParamList[i];
    }
    resOrder[i] = i;
  }

  /*Re-order stream by stream's width to descending order*/
  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    for (uint32_t j = i + 1; j < mStreamNumber; ++ j) {
      if (streamList[i] && streamList[j]) {
        if (streamList[i]->encode_params.encode_format.encode_width <
            streamList[j]->encode_params.encode_format.encode_width) {
          StreamParameters *ptemp = streamList[i];
          streamList[i] = streamList[j];
          streamList[j] = ptemp;
          uint32_t itemp = resOrder[i];
          resOrder[i] = resOrder[j];
          resOrder[j] = itemp;
        } else if (streamList[i]->encode_params.encode_format.encode_width ==
                   streamList[j]->encode_params.encode_format.encode_width) {
          if (streamList[i]->encode_params.encode_format.encode_height <
              streamList[j]->encode_params.encode_format.encode_height) {
            StreamParameters *ptemp = streamList[i];
            streamList[i] = streamList[j];
            streamList[j] = ptemp;
            uint32_t itemp = resOrder[i];
            resOrder[i] = resOrder[j];
            resOrder[j] = itemp;
          }
        }
      } else if ((NULL == streamList[i]) && streamList[j]) {
        streamList[i] = streamList[j];
        streamList[j] = NULL;
        uint32_t itemp = resOrder[i];
        resOrder[i] = resOrder[j];
        resOrder[j] = itemp;
      }
    }
  }

#ifdef AM_CAMERA_DEBUG
  DEBUG("Stream sequence is: ");
  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    DEBUG("%u", resOrder[i]);
  }
#endif

  /* Assign buffer to stream */
  for (uint32_t i = 0; i < mStreamNumber; ++i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[resOrder[i]]->encode_params.encode_format;

    bool assignedBuffer = false;
    if (format.encode_type == 0) {
      format.source = 0;
      continue;
    }
    mStreamParamList[i]->config_changed = 1;
    for (uint32_t bufId = 0; bufId < MAX_ENCODE_BUFFER_NUM; ++bufId) {
      if (false == isBufferUsed[bufId]) {
        if (bufId == 0) { //BufferMain is available
          isBufferUsed[bufId] = true;
          format.source = bufId;
          bufferFormat.main_width = format.encode_width;
          bufferFormat.main_height = format.encode_height;
          assignedBuffer = true;
          bufferType.main_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
          DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
          break;
        } else if (bufId == 1) { //BufferSub1 is available
          if ((format.encode_width <=
               sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width) &&
              (format.encode_height <=
               sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.second_width = format.encode_width;
            bufferFormat.second_height = format.encode_height;
            assignedBuffer = true;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 2) { //BufferSub2 is available
          if ((format.encode_width <= sysResource.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].width)
              && (format.encode_height
                  <= sysResource.buffer_max_size[IAV_ENCODE_SOURCE_THIRD_BUFFER].height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.third_width = format.encode_width;
            bufferFormat.third_height = format.encode_height;
            assignedBuffer = true;
            bufferType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 3) { //BufferSub3 is available
          if ((format.encode_width <=
               sysResource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].width) &&
              (format.encode_height <=
                  sysResource.buffer_max_size[IAV_ENCODE_SOURCE_FOURTH_BUFFER].height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.fourth_width = format.encode_width;
            bufferFormat.fourth_height = format.encode_height;
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        }
      } else if (mEncoderParams->buffer_assign_method ==
          AM_BUFFER_TO_RESOLUTION) {
        // Assign the stream with the same size to one source buffer
        if (bufId == 0) {
          if ((format.encode_width == bufferFormat.main_width)
              && (format.encode_height == bufferFormat.main_height)) {
            format.source = 0;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 1) {
          if ((format.encode_width == bufferFormat.second_width)
              && (format.encode_height == bufferFormat.second_height)) {
            format.source = 1;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 2) {
          if ((format.encode_width == bufferFormat.third_width) &&
              (format.encode_height == bufferFormat.third_height) &&
              (bufferType.third_buffer_type == IAV_SOURCE_BUFFER_TYPE_ENCODE)) {
            format.source = 2;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 3) {
          if ((format.encode_width == bufferFormat.fourth_width) &&
              (format.encode_height == bufferFormat.fourth_height) &&
              (bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_ENCODE))
          {
            format.source = 3;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        }
      }
    }
    if (!assignedBuffer) {
      WARN("No suitable buffer found for stream%u, assigned main buffer to it",
           resOrder[i]);
      format.source = 0;
    }

    /* Set stream max size according to rotate option */
    if (format.encode_type == IAV_ENCODE_H264) {
      sysResource.stream_max_size[resOrder[i]].width = (
          format.rotate_clockwise ? format.encode_height :
              format.encode_width);
      sysResource.stream_max_size[resOrder[i]].height = (
          format.rotate_clockwise ? format.encode_width :
              format.encode_height);
    } else if (format.encode_type == IAV_ENCODE_MJPEG) {
      sysResource.stream_max_size[resOrder[i]].width = (
          format.rotate_clockwise ? format.encode_height :
              format.encode_width);
      sysResource.stream_max_size[resOrder[i]].height = (
          format.rotate_clockwise ? format.encode_width :
              format.encode_height);
    }

  }

  /* Reset main buffer size if the sub buffer is larger than main */
  /* Set bsize to 0 if the buffer is unused */
  for (uint32_t bufId = 1; bufId < MAX_ENCODE_BUFFER_NUM; ++bufId) {
    uint16_t *pBufWidth = NULL, *pBufHeight = NULL;
    switch (bufId) {
      case 1:
        pBufWidth = &bufferFormat.second_width;
        pBufHeight = &bufferFormat.second_height;
        break;
      case 2:
        pBufWidth = &bufferFormat.third_width;
        pBufHeight = &bufferFormat.third_height;
        break;
      case 3:
        pBufWidth = &bufferFormat.fourth_width;
        pBufHeight = &bufferFormat.fourth_height;
        break;
      default:
        pBufWidth = &bufferFormat.main_width;
        pBufHeight = &bufferFormat.main_height;
        break;
    }
    if (isBufferUsed[bufId]) {
      if (*pBufWidth > bufferFormat.main_width) {
        bufferFormat.main_width = *pBufWidth;
        DEBUG("Reset main buffer width to %u\n", bufferFormat.main_width);
      }
      if (*pBufHeight > bufferFormat.main_height) {
        bufferFormat.main_height = *pBufHeight;
        DEBUG("Reset main buffer height to %u\n", bufferFormat.main_height);
      }
    } else {
      *pBufWidth = 0;
      *pBufHeight = 0;
    }
  }

  /* Reset main buffer size according to the size of streams
   * which are assigned to main buffer */
  for (uint32_t i = 0; i < mStreamNumber; ++i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[resOrder[i]]->encode_params.encode_format;
    if ((format.source == 0) && (format.encode_type != IAV_ENCODE_NONE)) {
      if ((bufferFormat.main_width < format.encode_width) ||
          (bufferFormat.main_height < format.encode_height)) {
        if (bufferFormat.main_width < format.encode_width) {
          WARN("Stream%u width %hu is greater than main buffer width %hu!",
               i, format.encode_width, bufferFormat.main_width);
        }
        if (bufferFormat.main_height < format.encode_height) {
          WARN("Stream%u height %hu is greater than main buffer height %hu!",
               i, format.encode_height, bufferFormat.main_height);
        }
        WARN("Reset main buffer size to VIN size[%hux%hu]",
             vinRes.width, vinRes.height);
        bufferFormat.main_width = vinRes.width;
        bufferFormat.main_height = vinRes.height;
      }
    }
  }

  /* Set main buffer max size to the same size of main buffer
   * -X -bmaxsize -bsize
   */
  sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width =
      bufferFormat.main_width;
  sysResource.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height =
      bufferFormat.main_height;

  /* Set buffer default input window */
  bufferFormat.second_input_width  = bufferFormat.main_width;
  bufferFormat.second_input_height = bufferFormat.main_height;
  bufferFormat.third_input_width   = bufferFormat.main_width;
  bufferFormat.third_input_height  = bufferFormat.main_height;
  bufferFormat.fourth_input_width  = bufferFormat.main_width;
  bufferFormat.fourth_input_height = bufferFormat.main_height;

  /* Set Encode offset */
  for (uint32_t streamId = 0; streamId < mStreamNumber; ++ streamId) {
    iav_encode_format_ex_t &format =
        mStreamParamList[streamId]->encode_params.encode_format;
    uint16_t *pBufW      = NULL;
    uint16_t *pBufH      = NULL;
    uint16_t *pInputBufW = NULL;
    uint16_t *pInputBufH = NULL;
    if (format.encode_type != IAV_ENCODE_NONE) {
      switch (format.source) {
        case 1:
          pBufW = &bufferFormat.second_width;
          pBufH = &bufferFormat.second_height;
          pInputBufW = &bufferFormat.second_input_width;
          pInputBufH = &bufferFormat.second_input_height;
          break;
        case 2:
          pBufW = &bufferFormat.third_width;
          pBufH = &bufferFormat.third_height;
          pInputBufW = &bufferFormat.third_input_width;
          pInputBufH = &bufferFormat.third_input_height;
          break;
        case 3:
          pBufW = &bufferFormat.fourth_width;
          pBufH = &bufferFormat.fourth_height;
          pInputBufW = &bufferFormat.fourth_input_width;
          pInputBufH = &bufferFormat.fourth_input_height;
          break;
        case 0:
        default:
          pBufW = &bufferFormat.main_width;
          pBufH = &bufferFormat.main_height;
          break;
      }
      format.encode_x = round_down((*pBufW - format.encode_width) / 2, 2);
      format.encode_y = round_down((*pBufH - format.encode_height) / 2, 8);
      if (AM_LIKELY(pInputBufW && pInputBufH && pBufW && pBufH)) {
        uint16_t &mainW = bufferFormat.main_width;
        uint16_t &mainH = bufferFormat.main_height;
        if (((*pBufH) * mainW) > ((*pBufW) * mainH)) {
          /* Need to re-calculate input window width */
          *pInputBufW = round_up(((*pBufW) * mainH) / (*pBufH), 2);
        } else if (((*pBufH) * mainW) < ((*pBufW) * mainH)) {
          /* Need to re-calculate input window height */
          *pInputBufH = round_down(((*pBufH) * mainW) / (*pBufW), 4);
        }
      }
      DEBUG("Stream%u: size %ux%u, offset %ux%u (Buffer%u %ux%u)", streamId,
            format.encode_width, format.encode_height, format.encode_x,
            format.encode_y, format.source, *pBufW, *pBufH);
    }
  }
  /* Share buffer to YUV when YUV do not require independent source */
  if (yuvData.width && yuvData.height) {
    switch(mEncoderParams->buffer_assign_method) {
      case AM_BUFFER_TO_RESOLUTION:
      case AM_BUFFER_TO_STREAM: {
        mEncoderParams->yuv_buffer_id = 1;
        bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
        if (!bufferFormat.second_width) {
          bufferFormat.second_width =
              yuvData.width <= sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width ?
                  yuvData.width : sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].width;
          bufferFormat.second_width =
              bufferFormat.main_width <= bufferFormat.second_width ?
                  bufferFormat.main_width : bufferFormat.second_width;
        }
        if (!bufferFormat.second_height) {
          bufferFormat.second_height =
              yuvData.height <= sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height ?
                  yuvData.height : sysResource.buffer_max_size[IAV_ENCODE_SOURCE_SECOND_BUFFER].height;
          bufferFormat.second_height =
              bufferFormat.main_height <= bufferFormat.second_height  ?
                  bufferFormat.main_height : bufferFormat.second_height;
        }
        NOTICE("Second buffer provides YUV %ux%u.",
               bufferFormat.second_width, bufferFormat.second_height);
      }break;
      case AM_BUFFER_TO_STREAM_YUV: {
        if (mEncoderParams->yuv_buffer_id < MAX_ENCODE_BUFFER_NUM) {
          uint16_t *pBufW      = NULL;
          uint16_t *pBufH      = NULL;
          uint16_t *pInputBufW = NULL;
          uint16_t *pInputBufH = NULL;
          switch(mEncoderParams->yuv_buffer_id) {
            case 1: {
              pBufW      = &bufferFormat.second_width;
              pBufH      = &bufferFormat.second_height;
              pInputBufW = &bufferFormat.second_input_width;
              pInputBufH = &bufferFormat.second_input_height;
            }break;
            case 2: {
              pBufW      = &bufferFormat.third_width;
              pBufH      = &bufferFormat.third_height;
              pInputBufW = &bufferFormat.third_input_width;
              pInputBufH = &bufferFormat.third_input_height;
            }break;
            default:{
              ERROR("Impossible!");
            }break;
          }
          if (AM_LIKELY(pInputBufW && pInputBufH && pBufW && pBufH)) {
            uint16_t &mainW = bufferFormat.main_width;
            uint16_t &mainH = bufferFormat.main_height;
            if (((*pBufH) * mainW) > ((*pBufW) * mainH)) {
              /* Need to re-calculate input window width */
              *pInputBufW = round_up(((*pBufW) * mainH) / (*pBufH), 2);
            } else if (((*pBufH) * mainW) < ((*pBufW) * mainH)) {
              /* Need to re-calculate input window height */
              *pInputBufH = round_down(((*pBufH) * mainW) / (*pBufW), 4);
            }
            NOTICE("YUV buffer input windows format: %hux%hu",
                   *pInputBufW, *pInputBufH);
          }
        }
      }break;
    }
  }

  mEncoderParams->config_changed = 1;
  return true;
}

#else

bool AmVideoDevice::assign_buffer_to_stream()
{
  uint32_t resOrder[MAX_ENCODE_STREAM_NUM] = {0};
  bool isBufferUsed[MAX_ENCODE_BUFFER_NUM] = {false};
  iav_source_buffer_format_all_ex_t &bufferFormat =
      mEncoderParams->buffer_format_info;
  iav_source_buffer_type_all_ex_t &bufferType =
      mEncoderParams->buffer_type_info;
  iav_system_resource_setup_ex_t &sysResource =
      mEncoderParams->system_resource_info;
  Resolution &yuvData = mEncoderParams->yuv_buffer_size;
  StreamParameters *streamList[MAX_ENCODE_STREAM_NUM] = {0};
  Resolution vinRes;

  /* Fixme: Only 1 VIN device is supported currently */
  if (!(mVinList[0] && mVinList[0]->get_current_vin_size(vinRes))) {
    ERROR("Failed to get VIN resolution!");
    return false;
  }

  /* Exclude third/fourth buffer from assignment if they are for preview */
  if (bufferType.third_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
    isBufferUsed[2] = true;
    NOTICE("Third buffer is used for Preivew!");
  }
  if (bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_PREVIEW) {
    isBufferUsed[3] = true;
    NOTICE("Fourth buffer is used for Preivew!");
  }

  mEncoderParams->yuv_buffer_id = MAX_ENCODE_BUFFER_NUM;
  /* Assign buffer to YUV if YUV requires independent buffer */
  if ((mEncoderParams->buffer_assign_method == AM_BUFFER_TO_STREAM_YUV) &&
      yuvData.width && yuvData.height) {
    bool assignedBuffer = false;

    for (uint32_t bufId = 1; bufId <= 3; ++bufId) {
      switch(bufId) {
        case 1: {
          if (!isBufferUsed[bufId] &&
              (yuvData.width <= sysResource.second_source_buffer_max_width) &&
              (yuvData.height <= sysResource.second_source_buffer_max_height)) {
            isBufferUsed[bufId] = true;
            assignedBuffer = true;
            mEncoderParams->yuv_buffer_id = bufId;
            bufferFormat.second_width = yuvData.width;
            bufferFormat.second_height = yuvData.height;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to YUV %ux%u",
                  bufId, yuvData.width, yuvData.height);
          }
        }break;
        case 3: {
          if ((yuvData.width <= sysResource.fourth_source_buffer_max_width) &&
              (yuvData.height <= sysResource.fourth_source_buffer_max_height)) {
            isBufferUsed[bufId] = true;
            assignedBuffer = true;
            mEncoderParams->yuv_buffer_id = bufId;
            bufferFormat.fourth_width = yuvData.width;
            bufferFormat.fourth_height = yuvData.height;
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_PREVIEW;
            DEBUG("Buffer %u assigned to YUV %ux%u",
                  bufId, yuvData.width, yuvData.height);
          }
        }break;
        default:break;
      }
      if (assignedBuffer) {
        break;
      }
    }
    if (!assignedBuffer) {
      ERROR("Cannot find source buffer for YUV data %ux%u.", yuvData.width,
            yuvData.height);
    }
  }

  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[i]->encode_params.encode_format;
    if (format.encode_type != IAV_ENCODE_NONE) {
      uint16_t &mainMaxW = sysResource.main_source_buffer_max_width;
      uint16_t &mainMaxH = sysResource.main_source_buffer_max_height;
      if ((format.encode_width > mainMaxW) ||
          (format.encode_height > mainMaxH)) {
        if ((format.encode_width > mainMaxW) &&
            (format.encode_width <= vinRes.width)) {
          WARN("Stream%u width %hu is greater than max width %hu but smaller "
               "than VIN width %hu!",
               i, format.encode_width, mainMaxW);
        } else if (format.encode_width > vinRes.width) {
          ERROR("Stream%u width exceeds the max width %hu!", i, vinRes.width);
        }
        if ((format.encode_height > mainMaxH) &&
            (format.encode_height <= vinRes.height)) {
          WARN("Stream%u height %hu is greater than max height %hu but smaller "
               "than VIN height %hu!",
               i, format.encode_height, mainMaxH, vinRes.height);
        } else if (format.encode_height > vinRes.height) {
          ERROR("Stream%u width exceeds the max width %hu!", i, vinRes.height);
          return false;
        }
        WARN("Reset main buffer max size to VIN size [%hux%hu]",
             vinRes.width, vinRes.height);
        mainMaxW = vinRes.width;
        mainMaxH = vinRes.height;
      }
      streamList[i] = mStreamParamList[i];
    }
    resOrder[i] = i;
  }

  /*Re-order stream by stream's width to descending order*/
  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    for (uint32_t j = i + 1; j < mStreamNumber; ++ j) {
      if (streamList[i] && streamList[j]) {
        if (streamList[i]->encode_params.encode_format.encode_width <
            streamList[j]->encode_params.encode_format.encode_width) {
          StreamParameters *ptemp = streamList[i];
          streamList[i] = streamList[j];
          streamList[j] = ptemp;
          uint32_t itemp = resOrder[i];
          resOrder[i] = resOrder[j];
          resOrder[j] = itemp;
        } else if (streamList[i]->encode_params.encode_format.encode_width ==
                   streamList[j]->encode_params.encode_format.encode_width) {
          if (streamList[i]->encode_params.encode_format.encode_height <
              streamList[j]->encode_params.encode_format.encode_height) {
            StreamParameters *ptemp = streamList[i];
            streamList[i] = streamList[j];
            streamList[j] = ptemp;
            uint32_t itemp = resOrder[i];
            resOrder[i] = resOrder[j];
            resOrder[j] = itemp;
          }
        }
      } else if ((NULL == streamList[i]) && streamList[j]) {
        streamList[i] = streamList[j];
        streamList[j] = NULL;
        uint32_t itemp = resOrder[i];
        resOrder[i] = resOrder[j];
        resOrder[j] = itemp;
      }
    }
  }

#ifdef AM_CAMERA_DEBUG
  DEBUG("Stream sequence is: ");
  for (uint32_t i = 0; i < mStreamNumber; ++ i) {
    DEBUG("%u", resOrder[i]);
  }
#endif

  /* Assign buffer to stream */
  for (uint32_t i = 0; i < mStreamNumber; ++i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[resOrder[i]]->encode_params.encode_format;
    iav_h264_config_ex_t &h264Config =
        mStreamParamList[resOrder[i]]->h264_config;
    iav_jpeg_config_ex_t &mjpegConfig =
        mStreamParamList[resOrder[i]]->mjpeg_config;
    bool assignedBuffer = false;
    if (format.encode_type == 0) {
      format.source = 0;
      continue;
    }
    mStreamParamList[i]->config_changed = 1;
    for (uint32_t bufId = 0; bufId < MAX_ENCODE_BUFFER_NUM; ++ bufId) {
      if (false == isBufferUsed[bufId]) {
        if (bufId == 0) { //BufferMain is available
          isBufferUsed[bufId] = true;
          format.source = bufId;
          bufferFormat.main_width = format.encode_width;
          bufferFormat.main_height = format.encode_height;
          assignedBuffer = true;
          bufferType.main_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
          DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
          break;
        } else if (bufId == 1) { //BufferSub1 is available
          if ((format.encode_width <=
               sysResource.second_source_buffer_max_width) &&
              (format.encode_height <=
               sysResource.second_source_buffer_max_height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.second_width = format.encode_width;
            bufferFormat.second_height = format.encode_height;
            assignedBuffer = true;
            bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 2) { //BufferSub2 is available
          if ((format.encode_width <= sysResource.third_source_buffer_max_width)
              && (format.encode_height
                  <= sysResource.third_source_buffer_max_height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.third_width = format.encode_width;
            bufferFormat.third_height = format.encode_height;
            assignedBuffer = true;
            bufferType.third_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 3) { //BufferSub3 is available
          if ((format.encode_width <=
               sysResource.fourth_source_buffer_max_width) &&
              (format.encode_height <=
                  sysResource.fourth_source_buffer_max_height)) {
            isBufferUsed[bufId] = true;
            format.source = bufId;
            bufferFormat.fourth_width = format.encode_width;
            bufferFormat.fourth_height = format.encode_height;
            bufferType.fourth_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        }
      } else if (mEncoderParams->buffer_assign_method ==
          AM_BUFFER_TO_RESOLUTION) {
        // Assign the stream with the same size to one source buffer
        if (bufId == 0) {
          if ((format.encode_width == bufferFormat.main_width)
              && (format.encode_height == bufferFormat.main_height)) {
            format.source = 0;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 1) {
          if ((format.encode_width == bufferFormat.second_width)
              && (format.encode_height == bufferFormat.second_height)) {
            format.source = 1;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 2) {
          if ((format.encode_width == bufferFormat.third_width) &&
              (format.encode_height == bufferFormat.third_height) &&
              (bufferType.third_buffer_type == IAV_SOURCE_BUFFER_TYPE_ENCODE)) {
            format.source = 2;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        } else if (bufId == 3) {
          if ((format.encode_width == bufferFormat.fourth_width) &&
              (format.encode_height == bufferFormat.fourth_height) &&
              (bufferType.fourth_buffer_type == IAV_SOURCE_BUFFER_TYPE_ENCODE))
          {
            format.source = 3;
            assignedBuffer = true;
            DEBUG("Buffer %u assigned to stream%u", bufId, resOrder[i]);
            break;
          }
        }
      }
    }
    if (!assignedBuffer) {
      WARN("No suitable buffer found for stream%u, assigned main buffer to it!",
           resOrder[i]);
      format.source = 0;
    }

    /* Set stream max size according to rotate option */
    if (format.encode_type == IAV_ENCODE_H264) {
      sysResource.stream_max_encode_size[resOrder[i]].width = (
          h264Config.rotate_clockwise ? format.encode_height :
              format.encode_width);
      sysResource.stream_max_encode_size[resOrder[i]].height = (
          h264Config.rotate_clockwise ? format.encode_width :
              format.encode_height);
    } else if (format.encode_type == IAV_ENCODE_MJPEG) {
      sysResource.stream_max_encode_size[resOrder[i]].width = (
          mjpegConfig.rotate_clockwise ? format.encode_height :
              format.encode_width);
      sysResource.stream_max_encode_size[resOrder[i]].height = (
          mjpegConfig.rotate_clockwise ? format.encode_width :
              format.encode_height);
    }

  }

  /* Reset main buffer size if the sub buffer is larger than main */
  /* Set bsize to 0 if the buffer is unused */
  for (uint32_t bufId = 1; bufId < MAX_ENCODE_BUFFER_NUM; ++bufId) {
    uint16_t *pBufWidth = NULL, *pBufHeight = NULL;
    switch (bufId) {
      case 1:
        pBufWidth = &bufferFormat.second_width;
        pBufHeight = &bufferFormat.second_height;
        break;
      case 2:
        pBufWidth = &bufferFormat.third_width;
        pBufHeight = &bufferFormat.third_height;
        break;
      case 3:
        pBufWidth = &bufferFormat.fourth_width;
        pBufHeight = &bufferFormat.fourth_height;
        break;
      default:
        pBufWidth = &bufferFormat.main_width;
        pBufHeight = &bufferFormat.main_height;
        break;
    }
    if (isBufferUsed[bufId]) {
      if (*pBufWidth > bufferFormat.main_width) {
        bufferFormat.main_width = *pBufWidth;
        DEBUG("Reset main buffer width to %u\n", bufferFormat.main_width);
      }
      if (*pBufHeight > bufferFormat.main_height) {
        bufferFormat.main_height = *pBufHeight;
        DEBUG("Reset main buffer height to %u\n", bufferFormat.main_height);
      }
    } else {
      *pBufWidth = 0;
      *pBufHeight = 0;
    }
  }

  /* Reset main buffer size according to the size of streams
   * which are assigned to main buffer */
  for (uint32_t i = 0; i < mStreamNumber; ++i) {
    iav_encode_format_ex_t &format =
        mStreamParamList[resOrder[i]]->encode_params.encode_format;
    if ((format.source == 0) && (format.encode_type != 0)) {
      if ((bufferFormat.main_width < format.encode_width) ||
          (bufferFormat.main_height < format.encode_height)) {
        if (bufferFormat.main_width < format.encode_width) {
          WARN("Stream%u width %hu is greater than main buffer width %hu!",
               i, format.encode_width, bufferFormat.main_width);
        }
        if (bufferFormat.main_height < format.encode_height) {
          WARN("Stream%u height %hu is greater than main buffer height %hu!",
               i, format.encode_height, bufferFormat.main_height);
        }
        WARN("Reset main buffer size to VIN size[%hux%hu]",
             vinRes.width, vinRes.height);
        bufferFormat.main_width = vinRes.width;
        bufferFormat.main_height = vinRes.height;
      }
    }
  }

  /* Set main buffer max size to the same size of main buffer
   * -X -bmaxsize -bsize
   */
  sysResource.main_source_buffer_max_width  = bufferFormat.main_width;
  sysResource.main_source_buffer_max_height = bufferFormat.main_height;

  /* Set buffer default input window */
  bufferFormat.second_input_width  = bufferFormat.main_width;
  bufferFormat.second_input_height = bufferFormat.main_height;
  bufferFormat.third_input_width   = bufferFormat.main_width;
  bufferFormat.third_input_height  = bufferFormat.main_height;
  bufferFormat.fourth_input_width  = bufferFormat.main_width;
  bufferFormat.fourth_input_height = bufferFormat.main_height;

  /* Set Encode offset */
  for (uint32_t streamId = 0; streamId < mStreamNumber; ++ streamId) {
    iav_encode_format_ex_t &format =
        mStreamParamList[streamId]->encode_params.encode_format;
    uint16_t *pBufW      = NULL;
    uint16_t *pBufH      = NULL;
    uint16_t *pInputBufW = NULL;
    uint16_t *pInputBufH = NULL;
    if (format.encode_type != IAV_ENCODE_NONE) {
      switch (format.source) {
        case 1:
          pBufW = &bufferFormat.second_width;
          pBufH = &bufferFormat.second_height;
          pInputBufW = &bufferFormat.second_input_width;
          pInputBufH = &bufferFormat.second_input_height;
          break;
        case 2:
          pBufW = &bufferFormat.third_width;
          pBufH = &bufferFormat.third_height;
          pInputBufW = &bufferFormat.third_input_width;
          pInputBufH = &bufferFormat.third_input_height;
          break;
        case 3:
          pBufW = &bufferFormat.fourth_width;
          pBufH = &bufferFormat.fourth_height;
          pInputBufW = &bufferFormat.fourth_input_width;
          pInputBufH = &bufferFormat.fourth_input_height;
          break;
        case 0:
        default:
          pBufW = &bufferFormat.main_width;
          pBufH = &bufferFormat.main_height;
          break;
      }
      format.encode_x = round_down((*pBufW - format.encode_width) / 2, 2);
      format.encode_y = round_down((*pBufH - format.encode_height) / 2, 8);
      if (AM_LIKELY(pInputBufW && pInputBufH && pBufW && pBufH)) {
        uint16_t &mainW = bufferFormat.main_width;
        uint16_t &mainH = bufferFormat.main_height;
        if (((*pBufH) * mainW) > ((*pBufW) * mainH)) {
          /* Need to re-calculate input window width */
          *pInputBufW = round_up(((*pBufW) * mainH) / (*pBufH), 2);
        } else if (((*pBufH) * mainW) < ((*pBufW) * mainH)) {
          /* Need to re-calculate input window height */
          *pInputBufH = round_down(((*pBufH) * mainW) / (*pBufW), 4);
        }
      }
      DEBUG("Stream%u: size %ux%u, offset %ux%u (Buffer%u %ux%u)", streamId,
            format.encode_width, format.encode_height, format.encode_x,
            format.encode_y, format.source, *pBufW, *pBufH);
    }
  }
  /* Share buffer to YUV when YUV do not require independent source */
  if (yuvData.width && yuvData.height) {
    switch(mEncoderParams->buffer_assign_method) {
      case AM_BUFFER_TO_RESOLUTION:
      case AM_BUFFER_TO_STREAM: {
        mEncoderParams->yuv_buffer_id = 1;
        bufferType.second_buffer_type = IAV_SOURCE_BUFFER_TYPE_ENCODE;
        if (!bufferFormat.second_width) {
          bufferFormat.second_width =
              yuvData.width <= sysResource.second_source_buffer_max_width ?
                  yuvData.width : sysResource.second_source_buffer_max_width;
          bufferFormat.second_width =
              bufferFormat.main_width <= bufferFormat.second_width ?
                  bufferFormat.main_width : bufferFormat.second_width;
        }
        if (!bufferFormat.second_height) {
          bufferFormat.second_height =
              yuvData.height <= sysResource.second_source_buffer_max_height ?
                  yuvData.height : sysResource.second_source_buffer_max_height;
          bufferFormat.second_height =
              bufferFormat.main_height <= bufferFormat.second_height  ?
                  bufferFormat.main_height : bufferFormat.second_height;
        }
        NOTICE("Second buffer provides YUV %ux%u.",
               bufferFormat.second_width, bufferFormat.second_height);
      }break;
      case AM_BUFFER_TO_STREAM_YUV: {
        if (mEncoderParams->yuv_buffer_id < MAX_ENCODE_BUFFER_NUM) {
          uint16_t *pBufW      = NULL;
          uint16_t *pBufH      = NULL;
          uint16_t *pInputBufW = NULL;
          uint16_t *pInputBufH = NULL;
          switch(mEncoderParams->yuv_buffer_id) {
            case 1: {
              pBufW      = &bufferFormat.second_width;
              pBufH      = &bufferFormat.second_height;
              pInputBufW = &bufferFormat.second_input_width;
              pInputBufH = &bufferFormat.second_input_height;
            }break;
            case 2: {
              pBufW      = &bufferFormat.third_width;
              pBufH      = &bufferFormat.third_height;
              pInputBufW = &bufferFormat.third_input_width;
              pInputBufH = &bufferFormat.third_input_height;
            }break;
            default:{
              ERROR("Impossible!");
            }break;
          }
          if (AM_LIKELY(pInputBufW && pInputBufH && pBufW && pBufH)) {
            uint16_t &mainW = bufferFormat.main_width;
            uint16_t &mainH = bufferFormat.main_height;
            if (((*pBufH) * mainW) > ((*pBufW) * mainH)) {
              /* Need to re-calculate input window width */
              *pInputBufW = round_up(((*pBufW) * mainH) / (*pBufH), 2);
            } else if (((*pBufH) * mainW) < ((*pBufW) * mainH)) {
              /* Need to re-calculate input window height */
              *pInputBufH = round_down(((*pBufH) * mainW) / (*pBufW), 4);
            }
            NOTICE("YUV buffer input windows format: %hux%hu",
                   *pInputBufW, *pInputBufH);
          }
        }
      }break;
    }
  }

  mEncoderParams->config_changed = 1;
  return true;
}

#endif
bool AmVideoDevice::get_stream_size(uint32_t streamId, EncodeSize *pSize)
{
  uint32_t rotate_clockwise , vflip, hflip;
  if (!check_stream_id(streamId))
    return false;

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }
  uint8_t encode_type = mStreamParamList[streamId]->encode_params.encode_format.encode_type;

  mStreamParamList[streamId]->encode_params.encode_format.id = 1 << streamId;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_ENCODE_FORMAT_EX,
      &mStreamParamList[streamId]->encode_params.encode_format) < 0)) {
    PERROR("IAV_IOC_GET_ENCODE_FORMAT_EX");
    return false;
  }

  mStreamParamList[streamId]->encode_params.encode_format.encode_type = encode_type;

  pSize->width =
      mStreamParamList[streamId]->encode_params.encode_format.encode_width;
  pSize->height =
      mStreamParamList[streamId]->encode_params.encode_format.encode_height;
  if (mStreamParamList[streamId]->encode_params.encode_format.encode_type ==
      IAV_ENCODE_MJPEG) {
    iav_jpeg_config_ex_t &mjpegConfig =
        mStreamParamList[streamId]->mjpeg_config;
    mjpegConfig.id = 1 << streamId;
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_JPEG_CONFIG_EX,
                          &mjpegConfig) < 0)) {
      PERROR("IAV_IOC_GET_JPEG_CONFIG_EX");
      return false;
    }
#ifdef CONFIG_ARCH_S2

    rotate_clockwise = mStreamParamList[streamId]->encode_params.encode_format.rotate_clockwise;
    vflip = mStreamParamList[streamId]->encode_params.encode_format.vflip;
    hflip = mStreamParamList[streamId]->encode_params.encode_format.hflip;

#else
    rotate_clockwise = mjpegConfig.rotate_clockwise;
    vflip = mjpegConfig.vflip;
    hflip = mjpegConfig.hflip;
#endif
  } else {
    iav_h264_config_ex_t &h264Config = mStreamParamList[streamId]->h264_config;
    h264Config.id = 1 << streamId;
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_H264_CONFIG_EX, &h264Config) < 0)) {
      PERROR("IAV_IOC_GET_H264_CONFIG_EX");
      return false;
    }
#ifdef CONFIG_ARCH_S2

    rotate_clockwise = mStreamParamList[streamId]->encode_params.encode_format.rotate_clockwise;
    vflip = mStreamParamList[streamId]->encode_params.encode_format.vflip;
    hflip = mStreamParamList[streamId]->encode_params.encode_format.hflip;

#else

    rotate_clockwise = h264Config.rotate_clockwise;
    vflip = h264Config.vflip;
    hflip = h264Config.hflip;
#endif
  }
  pSize->rotate = (rotate_clockwise ? AM_ROTATE_90 : AM_NO_ROTATE_FLIP)
      | (vflip ? AM_VERTICAL_FLIP : AM_NO_ROTATE_FLIP)
      | (hflip ? AM_HORIZONTAL_FLIP : AM_NO_ROTATE_FLIP);

  return true;
}

#ifdef CONFIG_ARCH_S2

bool AmVideoDevice::set_stream_size_streamid(uint32_t sId, const EncodeSize *pSize)
{
  if (sId > mStreamNumber) {
    WARN("Only support [%d] streams. Set [%d] stream size.", mStreamNumber);
    sId = mStreamNumber;
  }

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }

  if ((pSize->width >
         mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width) ||
        (pSize->height >
         mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height)) {
      ERROR("Stream size [%ux%u] exceeds the max size [%ux%u].",
          pSize->width, pSize->height,
          mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width,
          mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height);
      return false;
    }
  mStreamParamList[sId]->encode_params.encode_format.encode_width =
        pSize->width;
    mStreamParamList[sId]->encode_params.encode_format.encode_height =
        pSize->height;

    mStreamParamList[sId]->encode_params.encode_format.rotate_clockwise =
            ((pSize->rotate & AM_ROTATE_90) ? 1 : 0);
    mStreamParamList[sId]->encode_params.encode_format.vflip =
            ((pSize->rotate & AM_VERTICAL_FLIP) ? 1 : 0);
    mStreamParamList[sId]->encode_params.encode_format.hflip =
            ((pSize->rotate & AM_HORIZONTAL_FLIP) ? 1 : 0);
    DEBUG("Stream%u: [%ux%u], rotate %u, vflip %u, hflip %u", sId,
          mStreamParamList[sId]->encode_params.encode_format.encode_width,
          mStreamParamList[sId]->encode_params.encode_format.encode_height,
          mStreamParamList[sId]->encode_params.encode_format.rotate_clockwise,
          mStreamParamList[sId]->encode_params.encode_format.vflip,
          mStreamParamList[sId]->encode_params.encode_format.hflip);

    mStreamParamList[sId]->config_changed = 1;

  return true;
}

bool AmVideoDevice::set_stream_size_all(uint32_t num, const EncodeSize *pSize)
{
  if (num > mStreamNumber) {
    WARN("Only support [%d] streams. Set [%d] stream size.", mStreamNumber);
    num = mStreamNumber;
  }

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }

  for (uint32_t sId = 0; sId < num; ++sId) {
    if ((pSize[sId].width >
         mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width) ||
        (pSize[sId].height >
         mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height)) {
      ERROR("Stream size [%ux%u] exceeds the max size [%ux%u].",
          pSize[sId].width, pSize[sId].height,
          mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].width,
          mEncoderParams->system_resource_info.buffer_max_size[IAV_ENCODE_SOURCE_MAIN_BUFFER].height);
      return false;
    }
  }
  for (uint32_t sId = 0; sId < num; ++sId) {
    mStreamParamList[sId]->encode_params.encode_format.encode_width =
        pSize[sId].width;
    mStreamParamList[sId]->encode_params.encode_format.encode_height =
        pSize[sId].height;

    mStreamParamList[sId]->encode_params.encode_format.rotate_clockwise =
            ((pSize[sId].rotate & AM_ROTATE_90) ? 1 : 0);
    mStreamParamList[sId]->encode_params.encode_format.vflip =
            ((pSize[sId].rotate & AM_VERTICAL_FLIP) ? 1 : 0);
    mStreamParamList[sId]->encode_params.encode_format.hflip =
            ((pSize[sId].rotate & AM_HORIZONTAL_FLIP) ? 1 : 0);
    DEBUG("Stream%u: [%ux%u], rotate %u, vflip %u, hflip %u", sId,
          mStreamParamList[sId]->encode_params.encode_format.encode_width,
          mStreamParamList[sId]->encode_params.encode_format.encode_height,
          mStreamParamList[sId]->encode_params.encode_format.rotate_clockwise,
          mStreamParamList[sId]->encode_params.encode_format.vflip,
          mStreamParamList[sId]->encode_params.encode_format.hflip);

    mStreamParamList[sId]->config_changed = 1;

  }
  return true;
}

#else

bool AmVideoDevice::set_stream_size_streamid(uint32_t sId, const EncodeSize *pSize)
{
  if (sId > mStreamNumber) {
    WARN("Only support [%d] streams. Set [%d] stream size.", mStreamNumber);
    sId = mStreamNumber;
  }

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }

  if ((pSize->width >
         mEncoderParams->system_resource_info.main_source_buffer_max_width) ||
        (pSize->height >
         mEncoderParams->system_resource_info.main_source_buffer_max_height)) {
      ERROR("Stream size [%ux%u] exceeds the max size [%ux%u].",
          pSize->width, pSize->height,
          mEncoderParams->system_resource_info.main_source_buffer_max_width,
          mEncoderParams->system_resource_info.main_source_buffer_max_height);
      return false;
    }
  mStreamParamList[sId]->encode_params.encode_format.encode_width =
        pSize->width;
    mStreamParamList[sId]->encode_params.encode_format.encode_height =
        pSize->height;

    mStreamParamList[sId]->h264_config.rotate_clockwise =
        mStreamParamList[sId]->mjpeg_config.rotate_clockwise =
            ((pSize->rotate & AM_ROTATE_90) ? 1 : 0);
    mStreamParamList[sId]->h264_config.vflip =
        mStreamParamList[sId]->mjpeg_config.vflip =
            ((pSize->rotate & AM_VERTICAL_FLIP) ? 1 : 0);
    mStreamParamList[sId]->h264_config.hflip =
        mStreamParamList[sId]->mjpeg_config.hflip =
            ((pSize->rotate & AM_HORIZONTAL_FLIP) ? 1 : 0);
    DEBUG("Stream%u: [%ux%u], rotate %u, vflip %u, hflip %u", sId,
          mStreamParamList[sId]->encode_params.encode_format.encode_width,
          mStreamParamList[sId]->encode_params.encode_format.encode_height,
          mStreamParamList[sId]->h264_config.rotate_clockwise,
          mStreamParamList[sId]->h264_config.vflip,
          mStreamParamList[sId]->h264_config.hflip);

    mStreamParamList[sId]->config_changed = 1;

  return true;
}

bool AmVideoDevice::set_stream_size_all(uint32_t num, const EncodeSize *pSize)
{
  if (num > mStreamNumber) {
    WARN("Only support [%d] streams. Set [%d] stream size.", mStreamNumber);
    num = mStreamNumber;
  }

  if (!pSize) {
    ERROR("Resolution ptr is NULL.");
    return false;
  }

  for (uint32_t sId = 0; sId < num; ++sId) {
    if ((pSize[sId].width >
         mEncoderParams->system_resource_info.main_source_buffer_max_width) ||
        (pSize[sId].height >
         mEncoderParams->system_resource_info.main_source_buffer_max_height)) {
      ERROR("Stream size [%ux%u] exceeds the max size [%ux%u].",
          pSize[sId].width, pSize[sId].height,
          mEncoderParams->system_resource_info.main_source_buffer_max_width,
          mEncoderParams->system_resource_info.main_source_buffer_max_height);
      return false;
    }
  }
  for (uint32_t sId = 0; sId < num; ++sId) {
    mStreamParamList[sId]->encode_params.encode_format.encode_width =
        pSize[sId].width;
    mStreamParamList[sId]->encode_params.encode_format.encode_height =
        pSize[sId].height;
    mStreamParamList[sId]->h264_config.rotate_clockwise =
        mStreamParamList[sId]->mjpeg_config.rotate_clockwise =
            ((pSize[sId].rotate & AM_ROTATE_90) ? 1 : 0);
    mStreamParamList[sId]->h264_config.vflip =
        mStreamParamList[sId]->mjpeg_config.vflip =
            ((pSize[sId].rotate & AM_VERTICAL_FLIP) ? 1 : 0);
    mStreamParamList[sId]->h264_config.hflip =
        mStreamParamList[sId]->mjpeg_config.hflip =
            ((pSize[sId].rotate & AM_HORIZONTAL_FLIP) ? 1 : 0);
    DEBUG("Stream%u: [%ux%u], rotate %u, vflip %u, hflip %u", sId,
          mStreamParamList[sId]->encode_params.encode_format.encode_width,
          mStreamParamList[sId]->encode_params.encode_format.encode_height,
          mStreamParamList[sId]->h264_config.rotate_clockwise,
          mStreamParamList[sId]->h264_config.vflip,
          mStreamParamList[sId]->h264_config.hflip);

    mStreamParamList[sId]->config_changed = 1;

  }
  return true;
}

#endif


uint32_t AmVideoDevice::get_stream_overlay_max_size(uint32_t streamId,
                                                    uint32_t areaId)
{
  uint32_t ret = 0;
  if (!check_stream_id(streamId))
    return ret;

  if (areaId >= MAX_OVERLAY_AREA_NUM) {
    ERROR("Stream%u: Invalid Area ID [%d]. Max area number "
    "of one stream is %d.", streamId, areaId, MAX_OVERLAY_AREA_NUM);
    return ret;
  }
  if (map_overlay()) {
    ret = mStreamParamList[streamId]->overlay.buffer[areaId].max_size;
  } else {
    ERROR("Cannot map overlay.");
  }
  return ret;
}

bool AmVideoDevice::set_stream_overlay(uint32_t streamId, uint32_t areaId,
                                       const Overlay *pOverlay)
{
  bool ret = false;

  if (!check_stream_id(streamId))
    return ret;

  if (areaId >= MAX_OVERLAY_AREA_NUM) {
    ERROR("Stream%u: Invalid area ID [%d]. "
          "Max area number of one stream is %d.",
          streamId, areaId, MAX_OVERLAY_AREA_NUM);
    return ret;
  }

  if (!pOverlay) {
    ERROR("Overlay ptr is NULL.");
    return ret;
  }

  if (map_overlay()) {
    iav_encode_format_ex_t &format =
        mStreamParamList[streamId]->encode_params.encode_format;
    overlay_insert_area_ex_t &area =
        mStreamParamList[streamId]->overlay.insert.area[areaId];
    OverlayAreaBuffer &buffer =
        mStreamParamList[streamId]->overlay.buffer[areaId];

  if (pOverlay->enable) {
      uint32_t data_size = 0;

      if (!pOverlay->width || !pOverlay->height) {
        ERROR("Stream%u Area%u: width [%d], height [%d] canno be 0.",
            streamId, areaId, pOverlay->width, pOverlay->height);
        return ret;
    }

      if (pOverlay->pitch & 0x1F) {
        ERROR("Stream%u Area%u: pitch [%d] should be aligned to 32.",
            streamId, areaId, pOverlay->pitch);
        return ret;
  }

      if ((pOverlay->offset_x + pOverlay->width > format.encode_width)
          || (pOverlay->offset_y + pOverlay->height > format.encode_height)) {
        ERROR("Stream%u Area%u: size [%dx%d] with offset [%dx%d] is "
        "out of stream size [%dx%d].", streamId, areaId, pOverlay->width,
        pOverlay->height, pOverlay->offset_x, pOverlay->offset_y,
        format.encode_width, format.encode_height);
        return ret;
    }

      data_size = (uint32_t) (pOverlay->pitch * pOverlay->height);
      if (data_size > buffer.max_size) {
        ERROR("Stream%u Area%u:  data size [%dx%d] should be "
        "no greater than %d.", streamId, areaId, pOverlay->pitch,
        pOverlay->height, buffer.max_size);
        return ret;
      }

      area.width = (uint16_t) pOverlay->width;
      area.height = (uint16_t) pOverlay->height;
      area.pitch = (uint16_t) pOverlay->pitch;
      area.start_x = (uint16_t) pOverlay->offset_x;
      area.start_y = (uint16_t) pOverlay->offset_y;
      area.total_size = (uint32_t) data_size;

      if (buffer.num > AM_OVERLAY_BUFFER_NUM_DOUBLE) {
        buffer.next_id = (
            buffer.active_id + 1 >= buffer.num ? 0 : buffer.active_id + 1);
    }

      if (pOverlay->data_addr) {
        DEBUG("Stream%u Area%u: data to Buffer%u (0x%x), size 0x%x.",
            streamId, areaId, buffer.next_id,
            (uint32_t)buffer.data_addr[buffer.next_id], area.total_size);
        memcpy(buffer.data_addr[buffer.next_id], pOverlay->data_addr,
               area.total_size);
      } else {
        WARN("Stream%u Area%u: data address is NULL.", streamId, areaId);
  }

      if (pOverlay->clut_addr) {
        DEBUG("Stream%u Area%u: clut to 0x%x, size 0x%x.", streamId, areaId,
            (uint32_t)buffer.clut_addr, pOverlay->clut_size);
        memcpy(buffer.clut_addr, pOverlay->clut_addr, pOverlay->clut_size);
      } else {
        WARN("Stream%u Area%u: clut address is NULL.", streamId, areaId);
    }
      area.enable = 1;
    } else {
      area.enable = 0;
}

    if (!is_stream_encoding(streamId) || update_stream_overlay(streamId)) {
      ret = true;
  }
  } else {
    ERROR("Cannot map overlay.");
}

  return ret;
}

uint32_t AmVideoDevice::get_system_max_performance()
{
  uint32_t ret = 0;
  do {
#ifdef CONFIG_ARCH_A5S
    iav_chip_id chipId = IAV_CHIP_ID_A5S_UNKNOWN;
#endif
#ifdef CONFIG_ARCH_S2
    iav_chip_s2_id chipId = IAV_CHIP_ID_S2_UNKNOWN;
#endif
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_CHIP_ID_EX, &chipId) < 0)) {
      PERROR("IAV_IOC_GET_CHIP_ID_EX");
      break;
    }
    ret = max_macroblock_number[(int32_t)chipId];
  } while(0);

  return ret;
}



const char *AmVideoDevice::chip_string()
{
  int32_t chipId = -1;
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_GET_CHIP_ID_EX, &chipId) < 0)) {
    PERROR("IAV_IOC_GET_CHIP_ID_EX");
  }
  return chip_str[1 + chipId];
}

bool AmVideoDevice::force_idr_insertion(uint32_t streamId)
{
  if (!check_stream_id(streamId)){
    return false;
  }
  return do_force_idr_insertion(streamId);
}

bool AmVideoDevice::do_force_idr_insertion(uint32_t streamId)
{
  uint32_t streamID = 0;
  streamID |= (1 << streamId);
  if (ioctl(mIav, IAV_IOC_FORCE_IDR_EX, streamID)  < 0) {
    PERROR("IAV_IOC_FORCE_IDR_EX");
    return false;
  } else {
    NOTICE("force idr for stream id 0x%x OK \n", streamId);
    return true;
  }
}
