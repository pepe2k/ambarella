/*******************************************************************************
 * AmMdCam.cpp
 *
 * Histroy:
 *  2014-2-11  [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "AmMdCam.h"

AmMdCam::AmMdCam(VDeviceParameters *vDeviceConfig)
  : AmSimpleCam(vDeviceConfig)
{
}

AmMdCam::~AmMdCam()
{
}

bool AmMdCam::get_yuv_data_init(void)
{
  return map_dsp();
}

bool AmMdCam::get_yuv_data_deinit(void)
{
  return unmap_dsp();
}

bool AmMdCam::get_y_raw_data(YRawFormat *yRawFormat, IavBufType bufType)
{
  bool ret = false;

  if (yRawFormat && bufType < AM_IAV_BUF_MAX &&
      mEncoderParams->yuv_buffer_id < MAX_ENCODE_BUFFER_NUM) {
    IavState iavStatus = get_iav_status();
    if (AM_LIKELY((iavStatus == AM_IAV_PREVIEW) ||
                  (iavStatus == AM_IAV_ENCODING))) {
      iav_yuv_buffer_info_ex_t yuvInfo    = {0};
      iav_me1_buffer_info_ex_t yuvMe1Info = {0};
      uint32_t count = 0;
      do {
        switch (bufType) {
        case AM_IAV_BUF_SECOND:
          yuvInfo.source = mEncoderParams->yuv_buffer_id;
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_YUV_BUFFER_INFO_EX,
                                &yuvInfo) < 0)) {
            if ((errno == EAGAIN) && (++ count < 10)) {
              usleep(100000);
              continue;
            }
            PERROR("IAV_IOC_READ_YUV_BUFFER_INFO_EX");
          }
          yRawFormat->y_addr  = yuvInfo.y_addr;
          yRawFormat->width   = yuvInfo.width;
          yRawFormat->height  = yuvInfo.height;
          yRawFormat->pitch   = yuvInfo.pitch;
          ret = true;
          break;
#if defined(CONFIG_ARCH_S2) || defined(CONFIG_ARCH_A9)
        case AM_IAV_BUF_ME1_MAIN:
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_ME1_BUFFER_INFO_EX,
                                &yuvMe1Info) < 0)) {
            if ((errno == EAGAIN) && (++ count < 10)) {
              usleep(100000);
              continue;
            }
            PERROR("IAV_IOC_READ_ME1_BUFFER_INFO_EX");
          }
          yRawFormat->y_addr  = yuvMe1Info.addr;
          yRawFormat->width   = yuvMe1Info.width;
          yRawFormat->height  = yuvMe1Info.height;
          yRawFormat->pitch   = yuvMe1Info.pitch;
          ret = true;
          break;
#else
        case AM_IAV_BUF_ME1_MAIN:
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_ME1_BUFFER_INFO_EX,
                                &yuvMe1Info) < 0)) {
            if ((errno == EAGAIN) && (++ count < 10)) {
              usleep(100000);
              continue;
            }
            PERROR("IAV_IOC_READ_ME1_BUFFER_INFO_EX");
          }
          yRawFormat->y_addr  = yuvMe1Info.main_addr;
          yRawFormat->width   = yuvMe1Info.main_width;
          yRawFormat->height  = yuvMe1Info.main_height;
          yRawFormat->pitch   = yuvMe1Info.main_pitch;
          ret = true;
          break;
        case AM_IAV_BUF_ME1_SECOND:
          if (AM_UNLIKELY(ioctl(mIav, IAV_IOC_READ_ME1_BUFFER_INFO_EX,
                                &yuvMe1Info) < 0)) {
            if ((errno == EAGAIN) && (++ count < 10)) {
              usleep(100000);
              continue;
            }
            PERROR("IAV_IOC_READ_ME1_BUFFER_INFO_EX");
          }
          yRawFormat->y_addr  = yuvMe1Info.second_addr;
          yRawFormat->width   = yuvMe1Info.second_width;
          yRawFormat->height  = yuvMe1Info.second_height;
          yRawFormat->pitch   = yuvMe1Info.second_pitch;
          ret = true;
          break;
#endif
        default:
          ERROR("Invalid iav buffer type!");
          count = 10;
          break;
        }
      } while (!ret && (count < 10) &&
               ((errno == EINTR) || (errno == EAGAIN)));
    } else {
      ERROR("To get YUV data, IAV must be in encoding or preview state!");
    }
  } else {
    if (!yRawFormat) {
      ERROR("Invalid YRawFormat parameter!");
    }
    if (bufType >= AM_IAV_BUF_MAX) {
      ERROR("Invalid IavBufType parameter!");
    }
    if (mEncoderParams->yuv_buffer_id >= MAX_ENCODE_BUFFER_NUM) {
      ERROR("No source assigned to YUV data.");
    }
  }

  return ret;
}
