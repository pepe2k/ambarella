/*******************************************************************************
 * am_vout_cvbs.cpp
 *
 * Histroy:
 *  2012-8-13 2012 - [ypchang] created file
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
#include "am_vout.h"
#include "am_vout_cvbs.h"

bool AmVoutCvbs::init(VoutInitMode initMode, VoutDspMode dspMode, bool force)
{
  bool ret = false;
  if (mVoutParams) {
    if (!is_vout_running() || is_vout_mode_changed(mVoutParams->video_mode) ||
        force) {
      int32_t sinkId = get_vout_sink_id();
      if (AM_UNLIKELY(ioctl(mVoutIav, IAV_IOC_VOUT_SELECT_DEV, sinkId) < 0)) {
        ERROR("IAV_IOC_VOUT_SELECT_DEV: %s", strerror(errno));
      } else {
        amba_video_sink_mode sinkConfig;
        memset(&sinkConfig, 0, sizeof(amba_video_sink_mode));
        sinkConfig.mode  = mVoutParams->video_mode;
        sinkConfig.ratio = AMBA_VIDEO_RATIO_AUTO;
        sinkConfig.bits  = AMBA_VIDEO_BITS_AUTO;
        sinkConfig.type  = AMBA_VOUT_SINK_TYPE_CVBS;
        if ((mVoutParams->video_mode == AMBA_VIDEO_MODE_480I) ||
            (mVoutParams->video_mode == AMBA_VIDEO_MODE_576I) ||
            (mVoutParams->video_mode == AMBA_VIDEO_MODE_1080I)||
            (mVoutParams->video_mode == AMBA_VIDEO_MODE_1080I_PAL)) {
          sinkConfig.format = AMBA_VIDEO_FORMAT_INTERLACE;
        } else {
          sinkConfig.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
        }
        sinkConfig.sink_type         = AMBA_VOUT_SINK_TYPE_CVBS;
        sinkConfig.bg_color.y        = 0x10;
        sinkConfig.bg_color.cb       = 0x80;
        sinkConfig.bg_color.cr       = 0x80;
        sinkConfig.lcd_cfg.mode      = AMBA_VOUT_LCD_MODE_DISABLE;
        sinkConfig.osd_tailor        = mVoutParams->tailored_info;
        sinkConfig.id                = sinkId;
        sinkConfig.frame_rate        = AMBA_VIDEO_FPS_AUTO;
        sinkConfig.csc_en            = mVoutParams->is_video_csc_enabled;
        sinkConfig.hdmi_color_space  = AMBA_VOUT_HDMI_CS_AUTO;
        sinkConfig.hdmi_3d_structure = DDD_RESERVED;
        sinkConfig.hdmi_overscan     = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
        sinkConfig.video_en   = ((initMode == AM_VOUT_INIT_DISABLE_VIDEO) ?
            0 : mVoutParams->is_video_enabled);
        sinkConfig.video_size    = mVoutParams->vout_video_size;
        sinkConfig.video_flip    = mVoutParams->video_flip;
        sinkConfig.video_rotate  = mVoutParams->video_rotate;
        sinkConfig.video_offset  = mVoutParams->video_offset;
        sinkConfig.fb_id         = mVoutParams->framebuffer_id;
        sinkConfig.osd_rescale   = mVoutParams->osd_rescale;
        sinkConfig.osd_offset    = mVoutParams->osd_offset;
        sinkConfig.display_input = AMBA_VOUT_INPUT_FROM_MIXER;
        sinkConfig.direct_to_dsp = ((dspMode == AM_VOUT_DSP_DIRECT) ? 1 : 0);
        if (mVoutParams->vout_video_size.specified) {
          if ((mVoutParams->vout_video_size.vout_width == 0) ||
              (mVoutParams->vout_video_size.vout_height == 0)) {
            sinkConfig.video_size.vout_width =
                video_mode_width(mVoutParams->video_mode);
            sinkConfig.video_size.vout_height =
                video_mode_height(mVoutParams->video_mode);
          }
          if ((mVoutParams->vout_video_size.video_width == 0) ||
              (mVoutParams->vout_video_size.video_height == 0)) {
            int32_t w = video_mode_width(mVoutParams->video_mode);
            int32_t h = video_mode_height(mVoutParams->video_mode);
            sinkConfig.video_size.video_width =
                ((w <= sinkConfig.video_size.vout_width)
                    ? w : sinkConfig.video_size.vout_width);
            sinkConfig.video_size.video_height =
                ((h <= sinkConfig.video_size.vout_height)
                    ? h : sinkConfig.video_size.vout_height);
          }
          if ( (sinkConfig.video_size.video_width >
               sinkConfig.video_size.vout_width) ||
              (sinkConfig.video_size.video_height >
               sinkConfig.video_size.vout_height) ) {
            sinkConfig.video_size.video_width =
                sinkConfig.video_size.vout_width;
            sinkConfig.video_size.video_height=
                sinkConfig.video_size.vout_height;
            WARN("Video size is larger than VOUT size, "
                "reset video size to VOUT size!");
          }
        } else {
          sinkConfig.video_size.specified  = 1;
          sinkConfig.video_size.vout_width =
              video_mode_width(mVoutParams->video_mode);
          sinkConfig.video_size.vout_height =
              video_mode_height(mVoutParams->video_mode);
          sinkConfig.video_size.video_width =
              sinkConfig.video_size.vout_width;
          sinkConfig.video_size.video_height =
              sinkConfig.video_size.vout_height;
        }
        if (mVoutParams->framebuffer_id >= 0) {
          clear_framebuffer(mVoutParams);
        }
        if (AM_UNLIKELY(ioctl(mVoutIav,
                              IAV_IOC_VOUT_CONFIGURE_SINK,
                              &sinkConfig) < 0)) {
          ERROR("IAV_IOC_VOUT_CONFIGURE_SINK: %s", strerror(errno));
        } else {
          ret = true;
        }
      }
    } else {
      ret = true; //Vout is already running, just return true.
      NOTICE("CVBS is already initialized!");
    }
  } else {
    ERROR("Invalid VOUT configuration for CVBS!");
  }

  return ret;
}
