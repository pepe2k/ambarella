/*******************************************************************************
 * am_vout_lcd.h
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

#ifndef AMVOUTLCD_H_
#define AMVOUTLCD_H_

class AmVoutLcd: public AmVout
{
  public:
    AmVoutLcd(int iavfd = -1): AmVout(AM_VOUT_TYPE_LCD, iavfd){}
    virtual ~AmVoutLcd(){}

  public:
    virtual bool lcd_reset();
    virtual bool lcd_power_on();
    virtual bool lcd_backlight_on();
    virtual bool lcd_pwm_set_brightness(uint32_t brightness);
    virtual bool lcd_pwm_get_max_brightness(int32_t &value);
    virtual bool lcd_pwm_get_current_brightness(int32_t &value);
    virtual const char *lcd_spi_dev_node();

  protected:
    virtual bool init(VoutInitMode initMode, VoutDspMode dspMode, bool force);
    /* LCD setting functions */
    bool lcd_panel_config(VoutParameters       *voutConfig,
                          amba_video_sink_mode &sinkConfig);

  private:
    bool gpio_set_enabled(uint8_t gpioid, bool enabled);
    inline const char* parameter_path(const char *param);
    inline bool get_value(const char *file, int32_t &value);
};


#endif /* AMVOUTLCD_H_ */
