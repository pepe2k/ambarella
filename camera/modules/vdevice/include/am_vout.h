/*******************************************************************************
 * am_vout.h
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

#ifndef AMVOUT_H_
#define AMVOUT_H_

/*******************************************************************************
 * AmVout is the base class of all VOUT devices, including LCD and HDMI.
 * This class implements basic manipulating functions of VOUT devices.
 * DO NOT use this class directly, use the inherited classes instead
 ******************************************************************************/
class AmVout
{
  public:
    enum VoutInitMode {
      AM_VOUT_INIT_DISABLE_VIDEO,
      AM_VOUT_INIT_ENABLE_VIDEO
    };
    enum VoutDspMode {
      AM_VOUT_DSP_DIRECT,
      AM_VOUT_DSP_INDIRECT
    };
  public: /* Constructor && Destructor */
    AmVout(VoutType type, int iavfd = -1);
    virtual ~AmVout();

  public: /* Configurations */
    void set_vout_config(VoutParameters *voutConfig)
    {
      mVoutParams = voutConfig;
    }
    VoutType get_vout_type() {return mVoutType;}

  public: /* Start && Stop */
    bool start(VoutInitMode mode = AM_VOUT_INIT_ENABLE_VIDEO,
               bool force = false);
    bool stop();
    bool restart(VoutInitMode mode = AM_VOUT_INIT_ENABLE_VIDEO);

  public: /* Feature Manipulating */
    bool color_conversion_switch(bool onoff);
    bool video_layer_switch(bool onoff);
    bool video_rotation_switch(bool onoff);
    bool video_flip(int32_t flipInfo);
    bool change_video_size(uint32_t width, uint32_t height);
    bool change_video_offset(uint32_t offx, uint32_t offy, bool center = false);
    bool set_framebuffer_id(int32_t fb);
    bool osd_flip(int32_t flipInfo);
    bool change_osd_offset(uint32_t offx, uint32_t offy, bool center = false);

  public:
    /* LCD Specific Methods */
    virtual bool lcd_reset(){return false;}
    virtual bool lcd_power_on(){return false;}
    virtual bool lcd_backlight_on(){return false;}
    virtual bool lcd_pwm_set_brightness(uint32_t brightness){return false;}
    virtual bool lcd_pwm_get_max_brightness(int32_t &value){return false;}
    virtual bool lcd_pwm_get_current_brightness(int32_t &value){return false;}
    virtual const char *lcd_spi_dev_node(){return NULL;}
    /* HDMI Specific Methods */
    virtual bool is_hdmi_plugged(){return false;}

  protected:
    int32_t video_mode_width(amba_video_mode mode);
    int32_t video_mode_height(amba_video_mode mode);

  protected: /* Configurations */
    bool set_framebuffer_bg_color(VoutParameters *vConfig,
                                  uint8_t r,
                                  uint8_t g,
                                  uint8_t b);
    bool clear_framebuffer(VoutParameters *voutConfig);
    bool get_vout_sink_info(amba_vout_sink_info &sinkInfo);
    bool is_vout_running();
    bool is_vout_mode_changed(amba_video_mode mode);
    int32_t get_vout_sink_id();
    int32_t vout_type_to_source_id(VoutType type);

  protected:
    /* Vout Init */
    virtual bool init(VoutInitMode mode, VoutDspMode dspMode, bool force) = 0;

  protected:
    VoutType        mVoutType;
    int             mVoutIav;
    VoutParameters *mVoutParams;

  private:
    bool          mNeedCloseIav;
};

#endif /* AMVOUT_H_ */
