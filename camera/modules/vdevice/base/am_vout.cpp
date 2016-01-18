/*******************************************************************************
 * am_vout.cpp
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
#include <linux/fb.h>
#include <sys/mman.h>
#include <wchar.h>
#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_vout.h"
static const char* vout_type_to_str[] = {
  "None", "Lcd", "Hdmi", "Cvbs"
};
/* Constructor && Destructor */
AmVout::AmVout(VoutType type, int iavfd)
  : mVoutType(type),
    mVoutIav(iavfd),
    mVoutParams(NULL)
{
  mNeedCloseIav = (mVoutIav < 0);
  if (mNeedCloseIav && ((mVoutIav = open("/dev/iav", O_RDWR)) < 0)) {
    PERROR("Failed to open IAV device");
  }
  DEBUG("VOUT IAV fd is %d", mVoutIav);
}

AmVout::~AmVout()
{
  if (mNeedCloseIav && (mVoutIav >= 0)) {
    close(mVoutIav);
  }
  DEBUG("AmVout deleted!");
}

/* Start && Stop */
bool AmVout::start(VoutInitMode mode, bool force)
{
  return init(mode, AM_VOUT_DSP_INDIRECT, force);
}

bool AmVout::stop()
{
  bool ret = true;
  if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_HALT,
                        vout_type_to_source_id(mVoutType)) < 0)) {
    PERROR("IAV_IOC_VOUT_HALT");
    ret = false;
  } else {
    if (is_vout_running()) {
      NOTICE("VOUT%d has been stopped successfully, "
             "but its state is still RUNNING, strange!",
             vout_type_to_source_id(mVoutType));
    }
  }

  return ret;
}

bool AmVout::restart(VoutInitMode mode)
{
  return init(mode, AM_VOUT_DSP_DIRECT, true);
}

bool AmVout::color_conversion_switch(bool onoff)
{
  iav_vout_enable_csc_t csc;
  csc.vout_id = vout_type_to_source_id(mVoutType);
  csc.csc_en  = (onoff ? 1 : 0);
  if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_ENABLE_CSC, &csc))) {
    PERROR("IAV_IOC_VOUT_ENABLE_CSC");
    return false;
  }

  return true;
}

bool AmVout::video_layer_switch(bool onoff)
{
  bool ret = false;
  iav_vout_enable_video_t video;
  video.vout_id = vout_type_to_source_id(mVoutType);
  video.video_en = (onoff ? 1 : 0);
  do {
    if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_ENABLE_VIDEO, &video) < 0)) {
      PERROR("IAV_IOC_VOUT_ENABLE_VIDEO");
      break;
    }
    ret = true;
  }while(0);

  return ret;
}

bool AmVout::video_rotation_switch(bool onoff)
{
  bool ret = true;
  if (mVoutType == AM_VOUT_TYPE_LCD) {
    iav_vout_rotate_video_t rotate;
    rotate.vout_id = vout_type_to_source_id(mVoutType);
    rotate.rotate  = (onoff ? 1 : 0);
    if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_ROTATE_VIDEO, &rotate))) {
      PERROR("IAV_IOC_VOUT_ROTATE_VIDEO");
      ret = false;
    }
  }

  return ret;
}

bool AmVout::video_flip(int32_t flipInfo)
{
  bool ret = true;
  iav_vout_flip_video_t flip;
  flip.vout_id = vout_type_to_source_id(mVoutType);
  flip.flip = flipInfo;
  if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_FLIP_VIDEO, &flip))) {
    PERROR("IAV_IOC_VOUT_FLIP_VIDEO");
    ret = false;
  }

  return ret;
}

bool AmVout::change_video_size(uint32_t width, uint32_t height)
{
  bool ret = true;
  iav_vout_change_video_size_t vsize;
  vsize.vout_id = vout_type_to_source_id(mVoutType);
  vsize.width   = width;
  vsize.height  = height;
  if (ioctl(mVoutIav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &vsize)) {
    PERROR("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE");
    ret = false;
  }

  return ret;
}

bool AmVout::change_video_offset(uint32_t offx, uint32_t offy, bool center)
{
  bool ret = true;
  iav_vout_change_video_offset_t voffset;
  voffset.vout_id = vout_type_to_source_id(mVoutType);
  voffset.offset_x = offx;
  voffset.offset_y = offy;
  voffset.specified = (center ? 0 : 1);

  if (ioctl(mVoutIav, IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET, &voffset)) {
    PERROR("IAV_IOC_VOUT_CHANGE_VIDEO_OFFSET");
    ret = false;
  }

  return ret;
}

bool AmVout::set_framebuffer_id(int32_t fb)
{
  bool ret = true;
  iav_vout_fb_sel_t fbSelect;
  fbSelect.vout_id = vout_type_to_source_id(mVoutType);
  fbSelect.fb_id   = fb;

  if (ioctl(mVoutIav, IAV_IOC_VOUT_SELECT_FB, &fbSelect)) {
    PERROR("IAV_IOC_VOUT_SELECT_FB");
    ret = false;
  }

  return ret;
}

bool AmVout::osd_flip(int32_t flipInfo)
{
  bool ret = true;
  iav_vout_flip_osd_t flip;
  flip.vout_id = vout_type_to_source_id(mVoutType);
  flip.flip = flipInfo;
  if (ioctl(mVoutIav, IAV_IOC_VOUT_FLIP_OSD, &flip)) {
    PERROR("IAV_IOC_VOUT_FLIP_OSD");
    ret = false;
  }
  return ret;
}

bool AmVout::change_osd_offset(uint32_t offx, uint32_t offy, bool center)
{
  bool ret = true;
  iav_vout_change_osd_offset_t offset;
  offset.vout_id = vout_type_to_source_id(mVoutType);
  offset.offset_x = offx;
  offset.offset_y = offy;
  offset.specified = (center ? 0 : 1);

  if (ioctl(mVoutIav, IAV_IOC_VOUT_CHANGE_OSD_OFFSET, &offset)) {
    PERROR("IAV_IOC_VOUT_CHANGE_OSD_OFFSET");
    ret = false;
  }

  return ret;
}

int32_t AmVout::video_mode_width(amba_video_mode mode)
{
  int32_t w = gVideoModeList[0].width;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      w = gVideoModeList[i].width;
      break;
    }
  }

  return w;
}

int32_t AmVout::video_mode_height(amba_video_mode mode)
{
  int32_t h = gVideoModeList[0].height;
  for (uint32_t i = 0;
      i < (sizeof(gVideoModeList) / sizeof(CamVideoMode));
      ++ i) {
    if (gVideoModeList[i].ambaMode == mode) {
      h = gVideoModeList[i].height;
      break;
    }
  }

  return h;
}

/* Protected Configurations */
inline bool AmVout::set_framebuffer_bg_color(VoutParameters *voutConfig,
                                             uint8_t r,
                                             uint8_t g,
                                             uint8_t b)
{
  bool ret = false;
  if (voutConfig->framebuffer_id >= 0) {
    char fbdev[32] = {0};
    int fd = -1;
    sprintf(fbdev, "/dev/fb%d", voutConfig->framebuffer_id);
    if ((fd = open(fbdev, O_RDWR)) < 0) {
      ERROR("Failed to open framebuffer device %s: %s",
            fbdev, strerror(errno));
    } else {
      fb_var_screeninfo var;
      fb_fix_screeninfo fix;
      if ((ioctl(fd, FBIOGET_VSCREENINFO, &var) >= 0) &&
          (ioctl(fd, FBIOGET_FSCREENINFO, &fix) >= 0)) {
        wchar_t *buf = (wchar_t *)mmap(NULL,
                                       fix.smem_len,
                                       PROT_WRITE,
                                       MAP_SHARED,
                                       fd,
                                       0);
        switch (var.bits_per_pixel / 8) {
          case 2: {
            uint16_t fakeRgb = 0;
            uint8_t y = 0, u = 0, v = 0;
            if (AM_LIKELY(voutConfig->is_video_csc_enabled)) {
              y = (( 66*r + 129*g +  25*b + 128) >> 8) +  16;
              u = ((-38*r -  74*g + 112*b + 128) >> 8) + 128;
              v = ((112*r -  94*g +  18*b + 128) >> 8) + 128;
            } else {
              u = r; y = g; v = b;
            }
            fakeRgb = (((u>>3) << 11) | ((y>>2) << 5) | (v>>3));
            wmemset(buf, ((fakeRgb << 16) | fakeRgb), fix.smem_len/4);
          }break;
          default: {
            WARN("%d bits format framebuffer is not supported currently!",
                 var.bits_per_pixel);
          }break;
        }
        munmap(buf, fd);
        close(fd);
        ret = true;
      }
    }
  } else {
    WARN("Invalid framebuffer ID %d, "
         "probably framebuffer device is not enabled on %s",
         voutConfig->framebuffer_id, vout_type_to_str[mVoutType]);
  }

  return ret;
}

bool AmVout::clear_framebuffer(VoutParameters *voutConfig)
{
  return set_framebuffer_bg_color(voutConfig, 0, 0, 0);
}

bool AmVout::get_vout_sink_info(amba_vout_sink_info &sinkInfo)
{
  bool ret = false;
  int32_t num = 0;
  int32_t chan = vout_type_to_source_id(mVoutType);
  int32_t sinkType;
  memset(&sinkInfo, 0, sizeof(sinkInfo));

  switch(mVoutType) {
    case AM_VOUT_TYPE_LCD: sinkType = AMBA_VOUT_SINK_TYPE_DIGITAL; break;
    case AM_VOUT_TYPE_HDMI: sinkType = AMBA_VOUT_SINK_TYPE_HDMI;   break;
    case AM_VOUT_TYPE_CVBS: sinkType = AMBA_VOUT_SINK_TYPE_CVBS;   break;
    case AM_VOUT_TYPE_NONE:
    default: sinkType = AMBA_VOUT_SINK_TYPE_AUTO; break;
  }

  if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0)) {
    PERROR("IAV_IOC_GET_SINK_NUM");
  } else {
    for (int32_t i = 0; i < num; ++ i) {
      sinkInfo.id = i;
      if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_GET_SINK_INFO,
                            &sinkInfo) < 0)) {
        PERROR("IAV_IOC_VOUT_GET_SINK_INFO");
        break;
      } else if ((sinkInfo.sink_type == sinkType) &&
                 (sinkInfo.source_id == chan)) {
        ret = true;
        break;
      }
    }
  }

  return ret;
}

bool AmVout::is_vout_running()
{
  bool ret = false;
  amba_vout_sink_info sinkInfo;

  if (get_vout_sink_info(sinkInfo)) {
    ret = (sinkInfo.state == AMBA_VOUT_SINK_STATE_RUNNING);
    NOTICE("VOUT%d:%s state is %d", vout_type_to_source_id(mVoutType),
           sinkInfo.name, (int32_t)sinkInfo.state);
  }

  return ret;
}

bool AmVout::is_vout_mode_changed(amba_video_mode mode)
{
  bool ret = true;
  amba_vout_sink_info sinkInfo;
  if (get_vout_sink_info(sinkInfo)) {
    ret = (sinkInfo.sink_mode.mode != (uint32_t)mode);
    if (ret) {
      NOTICE("%s's video mode has chanegd!", vout_type_to_str[mVoutType]);
    }
  } else {
    WARN("Failed to get VOUT sink info, assume VOUT video mode has changed.");
  }

  return ret;
}

int32_t AmVout::get_vout_sink_id()
{
  int32_t ret = -1;
  amba_vout_sink_info sinkInfo;

  if (get_vout_sink_info(sinkInfo)) {
    ret = sinkInfo.id;
  }

  return ret;
}

int32_t AmVout::vout_type_to_source_id(VoutType type)
{
  int32_t ret = -1;
  switch(type) {
    case AM_VOUT_TYPE_LCD:  ret = 0; break;
    case AM_VOUT_TYPE_HDMI: ret = 1; break;
    case AM_VOUT_TYPE_CVBS: ret = 1; break;
    case AM_VOUT_TYPE_NONE:
    default: ret = -1; break;
  }

  return ret;
}
