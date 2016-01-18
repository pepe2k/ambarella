/*******************************************************************************
 * am_config_vout.cpp
 *
 * Histroy:
 *  2012-3-5 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
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
#include <iniparser.h>
#ifdef __cplusplus
}
#endif

#include "am_include.h"
#include "utilities/am_define.h"
#include "utilities/am_log.h"
#include "datastructure/am_structure.h"
#include "am_config_base.h"
#include "am_config_vout.h"

/*******************************************************************************
 * AmConfigVout
 ******************************************************************************/
static AmConfigVout::CameraLcdType GlcdTypeList[] =
  {AmConfigVout::CameraLcdType("None"   , AM_LCD_PANEL_NONE)   ,
   AmConfigVout::CameraLcdType("Digital", AM_LCD_PANEL_DIGITAL),
   AmConfigVout::CameraLcdType("TD043"  , AM_LCD_PANEL_TD043)  ,
   AmConfigVout::CameraLcdType("TPO489" , AM_LCD_PANEL_TPO489) ,
   AmConfigVout::CameraLcdType("1P3828" , AM_LCD_PANEL_1P3828) ,
   AmConfigVout::CameraLcdType("1P3831" , AM_LCD_PANEL_1P3831)
  };
AmConfigVout::AmConfigVout(const char *configFileName)
  : AmConfigBase(configFileName),
    mVoutParamsLcd(NULL),
    mVoutParamsHdmi(NULL),
    mVoutParamsCvbs(NULL)
{
}

AmConfigVout::~AmConfigVout()
{
  delete mVoutParamsLcd;
  delete mVoutParamsHdmi;
  DEBUG("~AmConfigVout");
}

VoutParameters* AmConfigVout::get_vout_config(VoutType type)
{
  VoutParameters* ret = NULL;
  if (init()) {
    switch(type) {
      case AM_VOUT_TYPE_LCD:
        ret = get_lcd_config();
        break;
      case AM_VOUT_TYPE_HDMI:
        ret = get_hdmi_config();
        break;
      case AM_VOUT_TYPE_CVBS:
        ret = get_cvbs_config();
        /* no break */
      default:break;
    }
  }
  return ret;
}

void AmConfigVout::set_vout_config(VoutParameters *voutConfig, VoutType type)
{
  if (init()) {
    switch(type) {
      case AM_VOUT_TYPE_LCD:
        set_lcd_config(voutConfig);
        break;
      case AM_VOUT_TYPE_HDMI:
        set_hdmi_config(voutConfig);
        break;
      case AM_VOUT_TYPE_CVBS:
        set_cvbs_config(voutConfig);
        /* no break */
      default:break;
    }
    voutConfig->config_changed = 0;
    save_config();
  }
}

VoutParameters* AmConfigVout::get_lcd_config()
{
  if (!mVoutParamsLcd) {
    mVoutParamsLcd = new VoutParameters;
  }
  if (mVoutParamsLcd) {
    memset(mVoutParamsLcd, 0, sizeof(*mVoutParamsLcd));
    mVoutParamsLcd->is_video_csc_enabled =
        get_boolean("COMMON:VideoColorConversion", true);
    mVoutParamsLcd->tailored_info =
        (amba_vout_tailored_info)(get_boolean("COMMON:QtSupport", true) ?
            (AMBA_VOUT_OSD_NO_CSC | AMBA_VOUT_OSD_AUTO_COPY) : 0);
    mVoutParamsLcd->lcd_type =
        str_to_lcd_type(get_string("LCD:Type", "None"));
    if (mVoutParamsLcd->lcd_type != AM_LCD_PANEL_NONE) {
      char lcdres[128] = {0};
      sprintf(lcdres, "LCD:%s/Resolution",
              lcd_type_to_str(mVoutParamsLcd->lcd_type));
      mVoutParamsLcd->video_mode =
          str_to_video_mode(get_string(lcdres, "auto"));
    } else {
      mVoutParamsLcd->video_mode = str_to_video_mode("auto");
    }
    mVoutParamsLcd->vout_video_size.specified =
        (uint32_t)get_int("LCD:VoutSize", 0);
    mVoutParamsLcd->vout_video_size.vout_width =
        (uint16_t)get_int("LCD:VoutSize/VoutWidth", 0);
    mVoutParamsLcd->vout_video_size.vout_height =
        (uint16_t)get_int("LCD:VoutSize/VoutHeight", 0);
    mVoutParamsLcd->vout_video_size.video_width =
        (uint16_t)get_int("LCD:VoutSize/VideoWidth", 0);
    mVoutParamsLcd->vout_video_size.video_height =
        (uint16_t)get_int("LCD:VoutSize/VideoHeight", 0);
    mVoutParamsLcd->video_rotate =
        int_to_rotate_info(get_int("LCD:LcdRotateVideo", 0));
    mVoutParamsLcd->is_video_enabled =
        get_int("LCD:EnableVideo", 1);
    mVoutParamsLcd->video_flip =
        str_to_flip_info(get_string("LCD:LcdFlipVideo", "normal"));
    mVoutParamsLcd->framebuffer_id = get_int("Lcd:FramebufferID", -1);
    mVoutParamsLcd->video_offset.specified =
        (uint32_t)get_int("LCD:VideoOffset", 0);
    mVoutParamsLcd->video_offset.offset_x =
        (int16_t)get_int("LCD:VideoOffset/OffsetX", 0);
    mVoutParamsLcd->video_offset.offset_y =
        (int16_t)get_int("LCD:VideoOffset/OffsetY", 0);
    mVoutParamsLcd->osd_rescale.enable =
        (uint32_t)get_int("LCD:OsdRescale", 0);
    mVoutParamsLcd->osd_rescale.width =
        (uint16_t)get_int("LCD:OsdRescale/Width", 0);
    mVoutParamsLcd->osd_rescale.height =
        (uint16_t)get_int("LCD:OsdRescale/Height", 0);
    mVoutParamsLcd->osd_offset.specified =
        (uint32_t)get_int("LCD:OsdOffset", 0);
    mVoutParamsLcd->osd_offset.offset_x =
        (int16_t)get_int("LCD:OsdOffset/OffsetX", 0);
    mVoutParamsLcd->osd_offset.offset_y =
        (int16_t)get_int("LCD:SsdOffset/OffsetY", 0);
  }

  return mVoutParamsLcd;
}

void AmConfigVout::set_lcd_config(VoutParameters *lcdConfig)
{
  if (lcdConfig) {
    set_value("COMMON:VideoColorConversion",
              (lcdConfig->is_video_csc_enabled ? "1" : "0"));
    set_value("COMMON:QtSupport",
              (lcdConfig->tailored_info == 0 ? "0" : "1"));
    set_value("LCD:Type",
              lcd_type_to_str(lcdConfig->lcd_type));
    set_value("LCD:VoutSize",
              (lcdConfig->vout_video_size.specified ? "1" : "0"));
    set_value("LCD:VoutSize/VoutWidth",
              lcdConfig->vout_video_size.vout_width);
    set_value("LCD:VoutSize/VoutHeight",
              lcdConfig->vout_video_size.vout_height);
    set_value("LCD:VoutSize/VideoWidth",
              lcdConfig->vout_video_size.video_width);
    set_value("LCD:VoutSize/VideoHeight",
              lcdConfig->vout_video_size.video_height);
    set_value("LCD:LcdRotateVideo",
              rotate_info_to_str(lcdConfig->video_rotate));
    set_value("LCD:EnableVideo", (lcdConfig->is_video_enabled ? "1" : "0"));
    set_value("LCD:LcdFlipVideo", flip_info_to_str(lcdConfig->video_flip));
    set_value("LCD:FramebufferID", lcdConfig->framebuffer_id);
    set_value("LCD:VideoOffset",
              (lcdConfig->video_offset.specified ? "1" : "0"));
    set_value("LCD:VideoOffset/OffsetX", lcdConfig->video_offset.offset_x);
    set_value("LCD:VideoOffset/OffsetY", lcdConfig->video_offset.offset_y);
    set_value("LCD:OsdRescale", (lcdConfig->osd_rescale.enable ? "1" : "0"));
    set_value("LCD:OsdRescale/Width", lcdConfig->osd_rescale.width);
    set_value("LCD:OsdRescale/Height", lcdConfig->osd_rescale.height);
    set_value("LCD:OsdOffset",
              (lcdConfig->osd_offset.specified ? "1" : "0"));
    set_value("LCD:OsdOffset/OffsetX", lcdConfig->osd_offset.offset_x);
    set_value("LCD:OsdOffset/OffsetY", lcdConfig->osd_offset.offset_y);
  }
}

VoutParameters* AmConfigVout::get_hdmi_config()
{
  if (!mVoutParamsHdmi) {
    mVoutParamsHdmi = new VoutParameters;
  }
  if (mVoutParamsHdmi) {
    memset(mVoutParamsHdmi, 0, sizeof(*mVoutParamsHdmi));
    mVoutParamsHdmi->is_video_csc_enabled =
        get_boolean("COMMON:VideoColorConversion", true);
    mVoutParamsHdmi->tailored_info =
        (amba_vout_tailored_info)(get_boolean("COMMON:QtSupport", true)
            ? (AMBA_VOUT_OSD_NO_CSC | AMBA_VOUT_OSD_AUTO_COPY) : 0);
    mVoutParamsHdmi->video_mode =
        str_to_video_mode(get_string("HDMI:Resolution", "480p"));
    mVoutParamsHdmi->vout_video_size.specified =
        (uint32_t)get_int("HDMI:VoutSize", 0);
    mVoutParamsHdmi->vout_video_size.vout_width =
        (uint16_t)get_int("HDMI:VoutSize/VoutWidth", 0);
    mVoutParamsHdmi->vout_video_size.vout_height =
        (uint16_t)get_int("HDMI:VoutSize/VoutHeight", 0);
    mVoutParamsHdmi->vout_video_size.video_width =
        (uint16_t)get_int("HDMI:VoutSize/VideoWidth", 0);
    mVoutParamsHdmi->vout_video_size.video_height =
        (uint16_t)get_int("HDMI:VoutSize/VideoHeight", 0);
    mVoutParamsHdmi->video_rotate =
        int_to_rotate_info(get_int("HDMI:HdmiRotateVideo", 0));
    mVoutParamsHdmi->is_video_enabled = get_int("HDMI:EnableVideo", 1);
    mVoutParamsHdmi->video_flip =
        str_to_flip_info(get_string("HDMI:HdmiFlipVideo", "normal"));
    mVoutParamsHdmi->framebuffer_id = get_int("HDMI:FramebufferID", -1);
    mVoutParamsHdmi->video_offset.specified =
        (uint32_t)get_int("HDMI:VideoOffset", 0);
    mVoutParamsHdmi->video_offset.offset_x =
        (int16_t)get_int("HDMI:VideoOffset/OffsetX", 0);
    mVoutParamsHdmi->video_offset.offset_y =
        (int16_t)get_int("HDMI:VideoOffset/OffsetY", 0);
    mVoutParamsHdmi->osd_rescale.enable =
        (uint32_t)get_int("HDMI:OsdRescale", 0);
    mVoutParamsHdmi->osd_rescale.width =
        (uint16_t)get_int("HDMI:OsdRescale/Width", 0);
    mVoutParamsHdmi->osd_rescale.height =
        (uint16_t)get_int("HDMI:OsdRescale/Height", 0);
    mVoutParamsHdmi->osd_offset.specified =
        (uint32_t)get_int("HDMI:OsdOffset", 0);
    mVoutParamsHdmi->osd_offset.offset_x =
        (int16_t)get_int("HDMI:OsdOffset/OffsetX", 0);
    mVoutParamsHdmi->osd_offset.offset_y =
        (int16_t)get_int("HDMI:OsdOffset/OffsetY", 0);
  }
  return mVoutParamsHdmi;
}

void AmConfigVout::set_hdmi_config(VoutParameters *hdmiConfig)
{
  if (hdmiConfig) {
    set_value("COMMON:VideoColorConversion",
              (hdmiConfig->is_video_csc_enabled ? "1" : "0"));
    set_value("COMMON:QtSupport",
              (hdmiConfig->tailored_info == 0 ? "0" : "1"));
    set_value("HDMI:Resolution",
              video_mode_to_str(hdmiConfig->video_mode));
    set_value("HDMI:VoutSize",
              (hdmiConfig->vout_video_size.specified ? "1" : "0"));
    set_value("HDMI:VoutSize/VoutWidth",
              hdmiConfig->vout_video_size.vout_width);
    set_value("HDMI:VoutSize/VoutHeight",
              hdmiConfig->vout_video_size.vout_height);
    set_value("HDMI:VoutSize/VideoWidth",
              hdmiConfig->vout_video_size.video_width);
    set_value("HDMI:VoutSize/VideoHeight",
              hdmiConfig->vout_video_size.video_height);
    set_value("HDMI:HdmiRotateVideo",
              rotate_info_to_str(hdmiConfig->video_rotate));
    set_value("HDMI:EnableVideo",
              (hdmiConfig->is_video_enabled ? "1" : "0"));
    set_value("HDMI:HdmiFlipVideo",
              flip_info_to_str(hdmiConfig->video_flip));
    set_value("HDMI:FramebufferID", hdmiConfig->framebuffer_id);
    set_value("HDMI:VideoOffset",
              (hdmiConfig->video_offset.specified ? "1" : "0"));
    set_value("HDMI:VideoOffset/OffsetX", hdmiConfig->video_offset.offset_x);
    set_value("HDMI:VideoOffset/OffsetY", hdmiConfig->video_offset.offset_y);
    set_value("HDMI:OsdRescale",
              (hdmiConfig->osd_rescale.enable ? "1" : "0"));
    set_value("HDMI:OsdRescale/Width", hdmiConfig->osd_rescale.width);
    set_value("HDMI:OsdRescale/Height", hdmiConfig->osd_rescale.height);
    set_value("HDMI:OsdOffset",
              (hdmiConfig->osd_offset.specified ? "1" : "0"));
    set_value("HDMI:OsdOffset/OffsetX", hdmiConfig->osd_offset.offset_x);
    set_value("HDMI:OsdOffset/OffsetY", hdmiConfig->osd_offset.offset_y);
  }
}

VoutParameters* AmConfigVout::get_cvbs_config()
{
  if (!mVoutParamsCvbs) {
    mVoutParamsCvbs = new VoutParameters;
  }
  if (mVoutParamsCvbs) {
    memset(mVoutParamsCvbs, 0, sizeof(*mVoutParamsCvbs));
    mVoutParamsCvbs->is_video_csc_enabled =
    get_boolean("COMMON:VideoColorConversion", true);
    mVoutParamsCvbs->tailored_info =
    (amba_vout_tailored_info)(get_boolean("COMMON:QtSupport", true)
    ? (AMBA_VOUT_OSD_NO_CSC | AMBA_VOUT_OSD_AUTO_COPY) : 0);
    mVoutParamsCvbs->video_mode =
    str_to_video_mode(get_string("CVBS:Resolution", "576i"));
    mVoutParamsCvbs->vout_video_size.specified =
    (uint32_t)get_int("CVBS:VoutSize", 0);
    mVoutParamsCvbs->vout_video_size.vout_width =
    (uint16_t)get_int("CVBS:VoutSize/VoutWidth", 0);
    mVoutParamsCvbs->vout_video_size.vout_height =
    (uint16_t)get_int("CVBS:VoutSize/VoutHeight", 0);
    mVoutParamsCvbs->vout_video_size.video_width =
    (uint16_t)get_int("CVBS:VoutSize/VideoWidth", 0);
    mVoutParamsCvbs->vout_video_size.video_height =
    (uint16_t)get_int("CVBS:VoutSize/VideoHeight", 0);
    mVoutParamsCvbs->video_rotate =
    int_to_rotate_info(get_int("CVBS:CvbsRotateVideo", 0));
    mVoutParamsCvbs->is_video_enabled = get_int("CVBS:EnableVideo", 1);
    mVoutParamsCvbs->video_flip =
    str_to_flip_info(get_string("CVBS:CvbsFlipVideo", "normal"));
    mVoutParamsCvbs->framebuffer_id = get_int("CVBS:FramebufferID", -1);
    mVoutParamsCvbs->video_offset.specified =
    (uint32_t)get_int("CVBS:VideoOffset", 0);
    mVoutParamsCvbs->video_offset.offset_x =
    (int16_t)get_int("CVBS:VideoOffset/OffsetX", 0);
    mVoutParamsCvbs->video_offset.offset_y =
    (int16_t)get_int("CVBS:VideoOffset/OffsetY", 0);
    mVoutParamsCvbs->osd_rescale.enable =
    (uint32_t)get_int("CVBS:OsdRescale", 0);
    mVoutParamsCvbs->osd_rescale.width =
    (uint16_t)get_int("CVBS:OsdRescale/Width", 0);
    mVoutParamsCvbs->osd_rescale.height =
    (uint16_t)get_int("CVBS:OsdRescale/Height", 0);
    mVoutParamsCvbs->osd_offset.specified =
    (uint32_t)get_int("CVBS:OsdOffset", 0);
    mVoutParamsCvbs->osd_offset.offset_x =
    (int16_t)get_int("CVBS:OsdOffset/OffsetX", 0);
    mVoutParamsCvbs->osd_offset.offset_y =
    (int16_t)get_int("CVBS:OsdOffset/OffsetY", 0);
  }
  return mVoutParamsCvbs;
}

void AmConfigVout::set_cvbs_config(VoutParameters *cvbsConfig)
{
  if (cvbsConfig) {
    set_value("COMMON:VideoColorConversion",
              (cvbsConfig->is_video_csc_enabled ? "1" : "0"));
    set_value("COMMON:QtSupport",
              (cvbsConfig->tailored_info == 0 ? "0" : "1"));
    set_value("CVBS:Resolution",
              video_mode_to_str(cvbsConfig->video_mode));
    set_value("CVBS:VoutSize",
              (cvbsConfig->vout_video_size.specified ? "1" : "0"));
    set_value("CVBS:VoutSize/VoutWidth",
              cvbsConfig->vout_video_size.vout_width);
    set_value("CVBS:VoutSize/VoutHeight",
              cvbsConfig->vout_video_size.vout_height);
    set_value("CVBS:VoutSize/VideoWidth",
              cvbsConfig->vout_video_size.video_width);
    set_value("CVBS:VoutSize/VideoHeight",
              cvbsConfig->vout_video_size.video_height);
    set_value("CVBS:CvbsRotateVideo",
              rotate_info_to_str(cvbsConfig->video_rotate));
    set_value("CVBS:EnableVideo",
              (cvbsConfig->is_video_enabled ? "1" : "0"));
    set_value("CVBS:CvbsFlipVideo",
              flip_info_to_str(cvbsConfig->video_flip));
    set_value("CVBS:FramebufferID", cvbsConfig->framebuffer_id);
    set_value("CVBS:VideoOffset",
              (cvbsConfig->video_offset.specified ? "1" : "0"));
    set_value("CVBS:VideoOffset/OffsetX", cvbsConfig->video_offset.offset_x);
    set_value("CVBS:VideoOffset/OffsetY", cvbsConfig->video_offset.offset_y);
    set_value("CVBS:OsdRescale",
              (cvbsConfig->osd_rescale.enable ? "1" : "0"));
    set_value("CVBS:OsdRescale/Width", cvbsConfig->osd_rescale.width);
    set_value("CVBS:OsdRescale/Height", cvbsConfig->osd_rescale.height);
    set_value("CVBS:OsdOffset",
              (cvbsConfig->osd_offset.specified ? "1" : "0"));
    set_value("CVBS:OsdOffset/OffsetX", cvbsConfig->osd_offset.offset_x);
    set_value("CVBS:OsdOffset/OffsetY", cvbsConfig->osd_offset.offset_y);
  }
}

LcdPanelType AmConfigVout::str_to_lcd_type(const char *name)
{
  LcdPanelType type = AM_LCD_PANEL_NONE;
  if (AM_LIKELY(name)) {
    for (uint32_t i = 0;
         i < sizeof(GlcdTypeList)/sizeof(CameraLcdType);
         ++ i) {
      if (is_str_equal(name, GlcdTypeList[i].lcdName)) {
        type = GlcdTypeList[i].lcdType;
        break;
      }
    }
  }

  return type;
}

const char* AmConfigVout::lcd_type_to_str(LcdPanelType type)
{
  char * name = NULL;
  for (uint32_t i = 0;
       i < sizeof(GlcdTypeList)/sizeof(CameraLcdType);
       ++ i) {
    if (GlcdTypeList[i].lcdType == type) {
      name = (char *)GlcdTypeList[i].lcdName;
      break;
    }
  }
  return name;
}
