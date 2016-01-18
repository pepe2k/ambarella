/*******************************************************************************
 * am_photo.cpp
 *
 * Histroy:
 *  2012-4-9 2012 - [ypchang] created file
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
#include "img_struct_arch.h"
#include "img_api_arch.h"

static const char* vout_type_to_str[] = {
  "None", "Lcd", "Hdmi", "Cvbs"
};

AmSimplePhoto::AmSimplePhoto(VDeviceParameters *vDeviceConfig)
  : AmVideoDevice(),
    mPhotoCount(0)
{
  if (!create_video_device(vDeviceConfig)) {
    ERROR("Failed to create the components of this video device!");
  }
}

AmSimplePhoto::~AmSimplePhoto()
{
  unmap_bsb();
  DEBUG("AmSimplePhoto deleted!");
}

bool AmSimplePhoto::take_photo(PhotoType type,  uint32_t num,
                               uint32_t jpeg_w, uint32_t jpeg_h)
{
  bool ret = false;

  if (enter_photo_mode() && map_bsb() && map_dsp()) {
    for (uint32_t i = 0; i < num; ++ i) {
      ret = shoot(type, jpeg_w, jpeg_h);
      if (!ret) {
        break;
      }
    }
    if (!unmap_bsb() || !unmap_dsp() || !leave_photo_mode()) {
      ERROR("Failed to leave photo mode!");
    }
  }

  return ret;
}

static bool error_message()
{
  ERROR("Not supported in SimplePhoto!"
        "Use SimpleCam instead!");
  return false;
}

bool AmSimplePhoto::goto_idle()
{
  return change_iav_state(AM_IAV_IDLE);
}

bool AmSimplePhoto::enter_preview()
{
  return change_iav_state(AM_IAV_PREVIEW);
}
bool AmSimplePhoto::start_encode()
{
  return error_message();
}

bool AmSimplePhoto::stop_encode()
{
  return error_message();
}

bool AmSimplePhoto::enter_decode_mode()
{
  return error_message();
}

bool AmSimplePhoto::leave_decode_mode()
{
  return error_message();
}

bool AmSimplePhoto::enter_photo_mode()
{
  return change_iav_state(AM_IAV_STILL_CAPTURE);
}

bool AmSimplePhoto::leave_photo_mode()
{
  return change_iav_state(AM_IAV_PREVIEW);
}

bool AmSimplePhoto::reset_device()
{
  return (change_iav_state(AM_IAV_IDLE) &&
          init_device(AmVout::AM_VOUT_INIT_ENABLE_VIDEO, true));
}

bool AmSimplePhoto::is_camera_started()
{
  return (get_iav_status() != AM_IAV_INIT);
}

bool AmSimplePhoto::init_device(int voutInitMode, bool force)
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

bool AmSimplePhoto::apply_encoder_parameters()
{
  if (mEncoderParams->buffer_format_info.main_width > 1920) {
    mEncoderParams->buffer_format_info.main_width = 1920;
  }
  return AmVideoDevice::apply_encoder_parameters();
}

bool AmSimplePhoto::change_iav_state(IavState target)
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
              if (preview() && idle()) {
                /* Turn on video layer of VOUT0 and VOUT1 */
                for (uint32_t i = 0; i < mVoutNumber; ++ i) {
                  if (mVoutTypeList[i] != AM_VOUT_TYPE_NONE) {
                    mVoutList[i]->video_layer_switch(true);
                  }
                }
                if (target == AM_IAV_IDLE) {
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
              if (!decode(true)) {
                ret = false;
                error = true;
              } else if (target == AM_IAV_DECODING) {
                ret = true;
              }
            }break;
            case AM_IAV_PREVIEW:
            case AM_IAV_ENCODING:
            case AM_IAV_STILL_CAPTURE: {
              if (!apply_encoder_parameters() || !preview()) {
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
          case AM_IAV_PREVIEW:
            /* In Photo vDevice, every IAV mode needs to be reset to IDLE,
             * except for AM_IAV_STILL_CAPTURE mode
             */
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
#ifdef CONFIG_ARCH_S2
            ret = false;
            break;
#endif
            if (img_init_still_capture(mIav, mPhotoParams->quality) < 0) {
              ret = false;
              error = true;
              ERROR("Failed to initialize still capture!");
            } else {
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
      case AM_IAV_ENCODING: {
        switch(target) {
          case AM_IAV_ENCODING: {
            ret = true;
          }break;
          case AM_IAV_PREVIEW:
          case AM_IAV_STILL_CAPTURE:
          case AM_IAV_IDLE:
          case AM_IAV_DECODING: {
            if (!encode(false) || !idle()) {
              /* In Photo vDevice, every IAV mode needs to be reset to IDLE,
               * except for AM_IAV_STILL_CAPTURE mode
               */
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
        switch(target) {
          case AM_IAV_STILL_CAPTURE: {
            ret = true;
          }break;
          case AM_IAV_IDLE:
          case AM_IAV_PREVIEW:
          case AM_IAV_ENCODING:
          case AM_IAV_DECODING: {
            if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_LEAVE_STILL_CAPTURE, 0) < 0)) {
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
          }
        }
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

bool AmSimplePhoto::shoot(PhotoType type, uint32_t jpeg_w, uint32_t jpeg_h)
{
  bool ret = false;
  char filename[256] = {0};
  sprintf(filename, "%s/%s", mPhotoParams->file_store_location,
          mPhotoParams->file_name_prefix);
  still_cap_info_t info;
  memset(&info, 0, sizeof(still_cap_info_t));
  info.capture_num  = 1;
  info.jpeg_w       = jpeg_w;
  info.jpeg_h       = jpeg_h;
  info.need_raw     = ((type == AM_PHOTO_RAW) ? 1 : 0);
#ifdef CONFIG_ARCH_A5S
  info.keep_AR_flag = 1;
#endif
  DEBUG("Before img_start_still_capture, IAV state is %s",
        iav_state_to_str(get_iav_status()));
  if (img_start_still_capture(mIav, &info) < 0) {
    ERROR("Failed to start capture!");
  } else {
    switch(type) {
      case AM_PHOTO_JPEG:
        DEBUG("Photo type is JPEG!");
        ret = save_jpeg(filename);
        break;
      case AM_PHOTO_RAW:
        DEBUG("Photo type is RAW!");
        ret = save_raw(filename);
        break;
      default:
        ERROR("File type not supported!");
        ret = false;
        break;
    }
    if (AM_UNLIKELY(img_stop_still_capture(mIav) < 0)) {
      ERROR("Failed to stop capture!");
    } else if (AM_LIKELY(ret)) {
      INFO("Still capture done!");
    } else {
      INFO("Stop still capture failed!");
    }
    DEBUG("After img_stop_still_capture, IAV state is %s",
          iav_state_to_str(get_iav_status()));
  }
  return ret;
}

void AmSimplePhoto::generate_file_name(char *name,
                                 const char *prefix,
                                 const char *ext)
{
  char timeStr[32] = {0};
  time_t current = time(NULL);
  struct tm *utc = localtime(&current);
  if (strftime(timeStr, sizeof(timeStr), "%Y%m%d%H%M%S", utc) == 0) {
    ERROR("Date string format error!");
  }
  sprintf(name, "%s%d_%s.%s", prefix, mPhotoCount ++, timeStr, ext);
}

bool AmSimplePhoto::save_raw(const char *filename)
{
  bool ret       = false;
  iav_raw_info_t rawInfo;

  memset(&rawInfo, 0, sizeof(iav_raw_info_t));
  if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_RAW_INFO, &rawInfo) < 0)) {
    ERROR("Failed to read RAW info!");
  } else {
    char name[256] = {0};
    AmFile *raw    = NULL;

    generate_file_name(name, filename, "raw");
    raw = new AmFile(name);
    if (raw->open(AmFile::AM_FILE_CREATE)) {
      ssize_t size = raw->write(rawInfo.raw_addr,
                                rawInfo.width * rawInfo.height * 2);
      ret = ((uint32_t)size == (uint32_t)(rawInfo.width * rawInfo.height * 2));
      if (!ret) {
        ERROR("Failed to write file %s!", name);
      }
      raw->close();
    } else {
      ERROR("Failed to open file %s!", name);
    }
    delete raw;
  }

  return ret;
}

bool AmSimplePhoto::save_jpeg(const char *filename)
{
  bool ret = false;
  bs_fifo_info_t bsInfo;
  uint8_t *bsbAddr = mBsbMapInfo.addr;
  uint32_t bsbSize = mBsbMapInfo.length;
  memset(&bsInfo, 0, sizeof(bsInfo));

  do {
    if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_BITSTREAM,
                          &bsInfo) < 0)) {
      PERROR("IAV_IOC_READ_BITSTREAM");
      memset(&bsInfo, 0, sizeof(bsInfo));
      break;
    } else if (bsInfo.count == 0) {
      NOTICE("JPEG is not available now, try again!");
    }
  } while (bsInfo.count == 0);
  if (bsInfo.count != 0) {
    char name[256]    = {0};
    AmFile *jpeg      = NULL;
    generate_file_name(name, filename, "jpeg");
    jpeg = new AmFile(name);
    if (jpeg->open(AmFile::AM_FILE_CREATE)) {
      uint32_t bsbEnd    = (uint32_t)bsbAddr + bsbSize;
      uint32_t picSize   = round_up(bsInfo.desc[0].pic_size, 32);
      uint32_t startAddr = bsInfo.desc[0].start_addr;
      if ((startAddr + picSize) <= bsbEnd) {
        ssize_t size = jpeg->write((void *)startAddr, picSize);
        ret = ((uint32_t)size == picSize);
      } else {
        uint32_t remain = picSize - (bsbEnd - startAddr);
        uint32_t size = jpeg->write((void *)startAddr,
                                    (bsbEnd - startAddr));
        ret = (size == (bsbEnd - startAddr));
        if (ret) {
          size = jpeg->write(bsbAddr, remain);
          ret = (size == remain);
        }
      }
      if (!ret) {
        ERROR("Failed to write %s!", name);
      }
      jpeg->close();
    }
    delete jpeg;
  }

  return ret;
}
