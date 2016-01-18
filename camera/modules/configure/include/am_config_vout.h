/*******************************************************************************
 * amconfigvout.h
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

#ifndef AMCONFIGVOUT_H_
#define AMCONFIGVOUT_H_
/*******************************************************************************
 * AmConfigVout class provides methods to retrieve and set configs of Vout,
 * it is derived from AmConfig implementing Vout Configuration Retrieving
 * and Setting methods
 ******************************************************************************/

class AmConfigVout: public AmConfigBase
{
  public:
    struct CameraLcdType {
      const char  *lcdName;
      LcdPanelType lcdType;
      CameraLcdType(): lcdName(NULL), lcdType(AM_LCD_PANEL_NONE){}
      CameraLcdType(const char *name, LcdPanelType type)
        : lcdName(NULL), lcdType(type)
      {
        if (name) {
          lcdName = amstrdup(name);
        }
      }
      ~CameraLcdType() {delete[] lcdName;}
    };

  public:
    AmConfigVout(const char *configFileName);
    virtual ~AmConfigVout();

  public:
    VoutParameters* get_vout_config(VoutType type);
    void            set_vout_config(VoutParameters *voutConfig, VoutType type);

  private:
    inline VoutParameters* get_lcd_config();
    inline void set_lcd_config(VoutParameters *lcdConfig);

    inline VoutParameters* get_hdmi_config();
    inline void set_hdmi_config(VoutParameters *hdmiConfig);

    inline VoutParameters* get_cvbs_config();
    inline void set_cvbs_config(VoutParameters *cvbsConfig);

    void init_lcd_type_list();
    LcdPanelType str_to_lcd_type(const char *name);
    const char* lcd_type_to_str(LcdPanelType type);
    amba_vout_rotate_info int_to_rotate_info(int rotate) {
      amba_vout_rotate_info info = AMBA_VOUT_ROTATE_NORMAL;
      if (rotate == 90) {
        info = AMBA_VOUT_ROTATE_90;
      } else if (rotate == 0) {
        info = AMBA_VOUT_ROTATE_NORMAL;
      } else {
        WARN("%d is not an acceptable rotation value, reset to 0", rotate);
      }
      return info;
    }
    char *rotate_info_to_str(amba_vout_rotate_info rotate) {
      char *str = (char *)"0";
      if (rotate == AMBA_VOUT_ROTATE_90) {
        str = (char *)"90";
      } else if (rotate == AMBA_VOUT_ROTATE_NORMAL) {
        str = (char *)"0";
      }
      return str;
    }
    amba_vout_flip_info str_to_flip_info(const char *flip) {
      amba_vout_flip_info info = AMBA_VOUT_FLIP_NORMAL;
      if (is_str_equal(flip, "normal")) {
        info = AMBA_VOUT_FLIP_NORMAL;
      } else if (is_str_equal(flip, "hv")) {
        info = AMBA_VOUT_FLIP_HV;
      } else if (is_str_equal(flip, "horizontal")) {
        info = AMBA_VOUT_FLIP_HORIZONTAL;
      } else if (is_str_equal(flip, "vertical")) {
        info = AMBA_VOUT_FLIP_VERTICAL;
      } else {
        WARN("Unrecognized flip type %s, reset to normal!", flip);
      }
        return info;
    }
    char *flip_info_to_str(amba_vout_flip_info flip) {
      char *str = (char *)"normal";
      if (flip == AMBA_VOUT_FLIP_NORMAL) {
        str = (char *)"Normal";
      } else if (flip == AMBA_VOUT_FLIP_HV) {
        str = (char *)"HV";
      } else if (flip == AMBA_VOUT_FLIP_HORIZONTAL) {
        str = (char *)"Horizontal";
      } else if (flip == AMBA_VOUT_FLIP_VERTICAL) {
        str = (char *)"Vertical";
      }
      return str;
    }

  private:
    VoutParameters *mVoutParamsLcd;
    VoutParameters *mVoutParamsHdmi;
    VoutParameters *mVoutParamsCvbs;
};

#endif /* AMCONFIGVOUT_H_ */
