/*******************************************************************************
 * am_vout_lcd.cpp
 *
 * Histroy:
 *  2012-3-7 2012 - [ypchang] created file
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
#include "am_vout_lcd.h"

bool AmVoutLcd::lcd_reset()
{
  int32_t gpioValue  = -1;
  int32_t levelValue = -1;
  int32_t delayValue = -1;
  bool ret = false;
  if (get_value((const char *)"board_lcd_reset_gpio_id", gpioValue) &&
      get_value((const char *)"board_lcd_reset_active_level", levelValue) &&
      get_value((const char *)"board_lcd_reset_active_delay", delayValue)) {
    if ((gpioValue >= 0) && (levelValue >= 0) && (delayValue >= 0)) {
      gpio_set_enabled((uint8_t)gpioValue, (levelValue ? true : false));
      ::usleep(delayValue << 10);
      gpio_set_enabled((uint8_t)gpioValue, (levelValue ? false : true));
      ::usleep(delayValue << 10);
      ret = true;
    }
  }

  return ret;
}

bool AmVoutLcd::lcd_power_on()
{
  int32_t gpioValue  = -1;
  int32_t levelValue = -1;
  int32_t delayValue = -1;
  bool ret = false;
  if (get_value((const char *)"board_lcd_power_gpio_id", gpioValue) &&
      get_value((const char *)"board_lcd_power_active_level", levelValue) &&
      get_value((const char *)"board_lcd_power_active_delay", delayValue)) {
    if ((gpioValue >= 0) && (levelValue >= 0) && (delayValue >= 0)) {
      gpio_set_enabled((uint8_t)gpioValue, (levelValue ? true : false));
      ::usleep(delayValue << 10);
      ret = true;
    }
  }

  return ret;
}

bool AmVoutLcd::lcd_backlight_on()
{
  int32_t gpioValue  = -1;
  int32_t levelValue = -1;
  int32_t delayValue = -1;
  bool ret = false;
  if (get_value((const char *)"board_lcd_backlight_gpio_id", gpioValue) &&
      get_value((const char *)"board_lcd_backlight_active_level", levelValue) &&
      get_value((const char *)"board_lcd_backlight_active_delay", delayValue)) {
    if ((gpioValue >= 0) && (levelValue >= 0) && (delayValue >= 0)) {
      gpio_set_enabled((uint8_t)gpioValue, (levelValue ? true : false));
      ::usleep(delayValue << 10);
      ret = true;
    }
  }

  return ret;
}

bool AmVoutLcd::lcd_pwm_set_brightness(uint32_t brightness)
{
  bool ret = false;
  AmFile bn((const char*)"/sys/class/backlight/pwm-backlight.0/brightness");
  if (bn.open(AmFile::AM_FILE_WRITEONLY)) {
    char buf[128] = {0};
    int size = sprintf(buf, "%u", brightness);
    buf[size] = '\0';
    int retval = bn.write(buf, size);
    bn.close();
    ret = (retval == size);
  }

  return ret;
}

bool AmVoutLcd::lcd_pwm_get_max_brightness(int32_t &value)
{
  bool ret = false;
  AmFile maxBright((const char*)
                   "/sys/class/backlight/pwm-backlight.0/max_brightness");
  if (maxBright.open(AmFile::AM_FILE_READONLY)) {
    char buf[64] = {0};
    int32_t size = maxBright.read(buf, sizeof(buf));
    maxBright.close();
    if (size > 0) {
      buf[size] = '\0';
      value = atoi(buf);
      ret = true;
    }
  }

  return ret;
}

bool AmVoutLcd::lcd_pwm_get_current_brightness(int32_t &value)
{
  bool ret = false;
  AmFile curBright((const char*)
                   "/sys/class/backlight/pwm-backlight.0/actual_brightness");
  if (curBright.open(AmFile::AM_FILE_READONLY)) {
    char buf[64] = {0};
    int32_t size = curBright.read(buf, sizeof(buf));
    curBright.close();
    if (size > 0) {
      buf[size] = '\0';
      value = atoi(buf);
      ret = true;
    }
  }

  return ret;
}

const char* AmVoutLcd::lcd_spi_dev_node()
{
  int32_t busid = -1;
  int32_t csid  = -1;
  char   *node  = NULL;
  static char buf[256] = {0};

  if (get_value((const char *)"board_lcd_spi_bus_id", busid) &&
      get_value((const char *)"board_lcd_spi_cs_id", csid)) {
    if ((busid >= 0) && (csid >= 0)) {
      sprintf(buf, "/dev/spidev%d.%d", busid, csid);
      node = buf;
    }
  }

  return node;
}

bool AmVoutLcd::init(VoutInitMode initMode, VoutDspMode dspMode, bool force)
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
        if (lcd_panel_config(mVoutParams, sinkConfig)) {
          sinkConfig.osd_tailor        = mVoutParams->tailored_info;
          sinkConfig.id                = sinkId;
          sinkConfig.frame_rate        = AMBA_VIDEO_FPS_AUTO;
          sinkConfig.csc_en            = mVoutParams->is_video_csc_enabled;
          sinkConfig.hdmi_color_space  = AMBA_VOUT_HDMI_CS_AUTO;
          sinkConfig.hdmi_3d_structure = DDD_RESERVED;
          sinkConfig.hdmi_overscan     = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
          sinkConfig.video_en = ((initMode == AM_VOUT_INIT_DISABLE_VIDEO) ?
              0 : mVoutParams->is_video_enabled);
          sinkConfig.video_size        = mVoutParams->vout_video_size;
          sinkConfig.video_flip        = mVoutParams->video_flip;
          sinkConfig.video_rotate      = mVoutParams->video_rotate;
          sinkConfig.video_offset      = mVoutParams->video_offset;
          sinkConfig.fb_id             = mVoutParams->framebuffer_id;
          sinkConfig.osd_rescale       = mVoutParams->osd_rescale;
          sinkConfig.osd_offset        = mVoutParams->osd_offset;
          sinkConfig.display_input     = AMBA_VOUT_INPUT_FROM_MIXER;
          sinkConfig.direct_to_dsp     = ((dspMode == AM_VOUT_DSP_DIRECT)? 1: 0);
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
              int32_t h = video_mode_width(mVoutParams->video_mode);
              sinkConfig.video_size.video_width =
                  ((w <= sinkConfig.video_size.vout_width)
                      ? w : sinkConfig.video_size.vout_width);
              sinkConfig.video_size.video_height =
                  ((h <= sinkConfig.video_size.vout_height)
                      ? h : sinkConfig.video_size.vout_height);
            }
            if ((sinkConfig.video_size.video_width >
                 sinkConfig.video_size.vout_width) ||
                (sinkConfig.video_size.video_height >
                 sinkConfig.video_size.vout_height)) {
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
      }
    } else {
      ret = true; //Vout is already running, just return true
      NOTICE("LCD is already initialized!");
    }
  } else {
    ERROR("Invalid VOUT configuration for LCD!");
  }

  return ret;
}

bool AmVoutLcd::gpio_set_enabled(uint8_t gpioid, bool enabled)
{
  bool val = false;
  AmFile _export((const char *)"/sys/class/gpio/export");
  AmFile _unexport((const char *)"/sys/class/gpio/unexport");
  AmFile _direction;
  char str[128]  = {0};
  sprintf(str, "/sys/class/gpio/gpio%hhu/direction", gpioid);
  _direction.set_file_name((const char *)str);

  do {
    if (_export.open(AmFile::AM_FILE_WRITEONLY)) {
      int32_t ret = sprintf(str, "%hhu", gpioid);
      str[ret] = '\0';
      int32_t size = _export.write(str, strlen(str));
      _export.close();
      if (size != (int32_t)strlen(str)) {
        break;
      }
      if (_direction.open(AmFile::AM_FILE_WRITEONLY)) {
        char *cmd = (char *)(enabled ? "high" : "low");
        size = _direction.write(cmd, strlen(cmd));
        _direction.close();
        if (size != (int32_t)strlen(cmd)) {
          break;
        }
        if (_unexport.open(AmFile::AM_FILE_WRITEONLY)) {
          size = _unexport.write(str, strlen(str));
          _unexport.close();
          if (size != (int32_t)strlen(str)) {
            break;
          }
          val = true;
        }
      }
    }
  }while(0);

  return val;
}

const char* AmVoutLcd::parameter_path(const char *param)
{
  char * ret = NULL;
  static char pathOne[256] = {0};
  static char pathTwo[256] = {0};
  if (param) {
    int len = sprintf(pathOne, "/sys/module/ambarella_config/parameters/%s",
                      param);
    pathOne[len] = '\0';
    len = sprintf(pathTwo, "/sys/module/board/parameters/%s", param);
    pathTwo[len] = '\0';
    if (AmFile::exists(pathOne)) {
      ret = pathOne;
    } else if (AmFile::exists(pathTwo)) {
      ret = pathTwo;
    }
  }

  return ret;
}

bool AmVoutLcd::get_value(const char *file, int32_t &value)
{
  bool ret = false;
  const char *fPath = parameter_path(file);
  AmFile sysFile(fPath);
  char buf[64] = {0};
  if (sysFile.exists()) {
    if (sysFile.open(AmFile::AM_FILE_READONLY)) {
      int32_t size = sysFile.read(buf, sizeof(buf));
      sysFile.close();
      if (size > 0) {
        buf[size] = '\0';
        value = atoi(buf);
        ret = true;
      }
    }
  } else {
    ERROR("File %s doesn't exist!", fPath);
  }

  return ret;
}
