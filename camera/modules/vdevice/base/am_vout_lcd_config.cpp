/*******************************************************************************
 * am_vout_lcd_config.cpp
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
#include <linux/types.h>
#include <linux/spi/spidev.h>

static void res_800_400(amba_video_sink_mode &sinkConfig)
{
  sinkConfig.mode      = AMBA_VIDEO_MODE_WVGA;
  sinkConfig.ratio     = AMBA_VIDEO_RATIO_16_9;
  sinkConfig.bits      = AMBA_VIDEO_BITS_16;
  sinkConfig.type      = AMBA_VIDEO_TYPE_RGB_601;
  sinkConfig.format    = AMBA_VIDEO_FORMAT_PROGRESSIVE;
  sinkConfig.sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

  sinkConfig.bg_color.y  = 0x10;
  sinkConfig.bg_color.cb = 0x80;
  sinkConfig.bg_color.cr = 0x80;

  sinkConfig.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_RGB888;
  sinkConfig.lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
  sinkConfig.lcd_cfg.dclk_freq_hz = 27000000;
}

static void res_480_800(amba_video_sink_mode &sinkConfig)
{
  sinkConfig.mode      = AMBA_VIDEO_MODE_480_800;
  sinkConfig.ratio     = AMBA_VIDEO_RATIO_16_9;
  sinkConfig.bits      = AMBA_VIDEO_BITS_16;
  sinkConfig.type      = AMBA_VIDEO_TYPE_RGB_601;
  sinkConfig.format    = AMBA_VIDEO_FORMAT_PROGRESSIVE;
  sinkConfig.sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

  sinkConfig.bg_color.y  = 0x10;
  sinkConfig.bg_color.cb = 0x80;
  sinkConfig.bg_color.cr = 0x80;

  sinkConfig.lcd_cfg.mode         = AMBA_VOUT_LCD_MODE_RGB888;
  sinkConfig.lcd_cfg.seqt         = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.seqb         = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.dclk_edge    = AMBA_VOUT_LCD_CLK_RISING_EDGE;
  sinkConfig.lcd_cfg.dclk_freq_hz = 27000000;
}

static void res_320_480(amba_video_sink_mode &sinkConfig)
{
  sinkConfig.mode      = AMBA_VIDEO_MODE_HVGA;
  sinkConfig.ratio     = AMBA_VIDEO_RATIO_4_3;
  sinkConfig.bits      = AMBA_VIDEO_BITS_16;
  sinkConfig.type      = AMBA_VIDEO_TYPE_RGB_601;
  sinkConfig.format    = AMBA_VIDEO_FORMAT_PROGRESSIVE;
  sinkConfig.sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;

  sinkConfig.bg_color.y  = 0x10;
  sinkConfig.bg_color.cb = 0x80;
  sinkConfig.bg_color.cr = 0x80;

  sinkConfig.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_RGB565;
  sinkConfig.lcd_cfg.seqt = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.seqb = AMBA_VOUT_LCD_SEQ_R0_G1_B2;
  sinkConfig.lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
  sinkConfig.lcd_cfg.dclk_freq_hz = 27000000;
}

static bool td043_config(AmVoutLcd *lcd)
{
  int32_t maxLight = 0;
  if (lcd->lcd_pwm_get_max_brightness(maxLight)) {
    lcd->lcd_power_on();
    return lcd->lcd_pwm_set_brightness(maxLight*4/5);
  }

  return false;
}

static bool tpo489_config(AmVoutLcd *lcd)
{
  bool ret = false;
  const char *SpiNode = lcd->lcd_spi_dev_node();
  if (SpiNode) {
    AmFile node(SpiNode);
    int spiFd = -1;
    if (node.open(AmFile::AM_FILE_READWRITE)) {
      spiFd = node.handle();
      if (spiFd >= 0) {
        uint8_t mode = 0;
        uint8_t bits = 16;
        uint32_t speed = 500000;
        uint16_t cmd[] =
        {
          0x0801, /* Software Reset */
          0x0800,
          0x0200, /* Sync Polarity */
          0x0304, /* Vactive Start */
          0x040e, /* Hactive Start */
          0x0903, /* Interface to CPU */
          0x0b08, /* R32-R59 Access */
          0x0c53, /* Gama */
          0x0d01, /* Vactive Size [9:8] */
          0x0ee0, /* Vactive Size [7:0] */
          0x0f01, /* Hactive Size [10:8] */
          0x10f4, /* Hactive Size [7:0] */
          0x201e, /* R32-R59 */
          0x210a,
          0x220a,
          0x231e,
          0x2532,
          0x2600,
          0x27ac,
          0x2904,
          0x2ae4,
          0x2b45,
          0x2c45,
          0x2d15,
          0x2e5a,
          0x2fff,
          0x306b,
          0x312e,
          0x325e,
          0x338e,
          0x34c8,
          0x35f5,
          0x361e,
          0x3797,
          0x3804,
          0x395c,
          0x3aa8,
          0x3bff,
          0x07c9  /* Auto Power On */
        };
        lcd->lcd_power_on();
        lcd->lcd_reset();
        lcd->lcd_backlight_on();

        do {
          if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0) {
            ERROR("SPI_IOC_WR_MODE: %s", strerror(errno));
            break;
          }

          if (ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
            ERROR("SPI_IOC_WR_BITS_PER_WORD: %s", strerror(errno));
            break;
          }

          if (ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
            ERROR("SPI_IOC_WR_MAX_SPEED_HZ: %s", strerror(errno));
            break;
          }
          node.write(&cmd[0], 2);
          ::usleep(10000);
          for (int i = 1; i < (int)(sizeof(cmd)/sizeof(uint16_t)); ++ i) {
            node.write(&cmd[i], 2);
          }
          ret = true;
        }while(0);
      }
      node.close();
    }
  }

  return ret;
}

static bool config_1p3828(AmVoutLcd *lcd)
{
  bool ret = false;
  const char *SpiNode = lcd->lcd_spi_dev_node();
  if (SpiNode) {
    AmFile node(SpiNode);
    int spiFd = -1;
    if (node.open(AmFile::AM_FILE_READWRITE)) {
      int32_t maxLight = 0;
      uint8_t mode   = 0;      /*CPHA = 0, CPOL = 0*/
      uint8_t bits   = 9;      /*bits per word is 9*/
      uint32_t speed = 500000; /*SSI_CLK = 500KHz, can speed up if necessary*/
      uint16_t cmd[] =
      {
       0x00B9, //Set_EXTC
       0x01FF,
       0x0183,
       0x0169,
       0x00B1,  //Set Power
       0x0101,
       0x0100,
       0x0134,
       0x0106,
       0x0100,
       0x010F,
       0x010F,
       0x012A,
       0x0132,
       0x013F,
       0x013F,
       0x0107,
       0x0123,
       0x0101,
       0x01E6,
       0x01E6,
       0x01E6,
       0x01E6,
       0x01E6,
       0x00B2,  // SET Display  480x800
       0x0100,
       0x012b,
       0x010A,
       0x010A,
       0x0170,
       0x0100,
       0x01FF,
       0x0100,
       0x0100,
       0x0100,
       0x0100,
       0x0103,
       0x0103,
       0x0100,
       0x0101,
       0x00B4,  // SET Display  480x800
       0x0100,
       0x0118,
       0x0180,
       0x0110,
       0x0101,
       0x00B6,  // SET VCOM
       0x012C,
       0x012C,
       0x00D5,  //SET GIP
       0x0100,
       0x0105,
       0x0103,
       0x0100,
       0x0101,
       0x0109,
       0x0110,
       0x0180,
       0x0137,
       0x0137,
       0x0120,
       0x0131,
       0x0146,
       0x018A,
       0x0157,
       0x019B,
       0x0120,
       0x0131,
       0x0146,
       0x018A,
       0x0157,
       0x019B,
       0x0107,
       0x010F,
       0x0102,
       0x0100,
       0x00E0,  //SET GAMMA
       0x0100,
       0x0108,
       0x010D,
       0x012D,
       0x0134,
       0x013F,
       0x0119,
       0x0138,
       0x0109,
       0x010E,
       0x010E,
       0x0112,
       0x0114,
       0x0112,
       0x0114,
       0x0113,
       0x0119,
       0x0100,
       0x0108,
       0x010D,
       0x012D,
       0x0134,
       0x013F,
       0x0119,
       0x0138,
       0x0109,
       0x010E,
       0x010E,
       0x0112,
       0x0114,
       0x0112,
       0x0114,
       0x0113,
       0x0119,
       0x00C1, //set DGC
       0x0101, //enable DGC function
       0x0102, //SET R-GAMMA
       0x0108,
       0x0112,
       0x011A,
       0x0122,
       0x012A,
       0x0131,
       0x0136,
       0x013F,
       0x0148,
       0x0151,
       0x0158,
       0x0160,
       0x0168,
       0x0170,
       0x0178,
       0x0180,
       0x0188,
       0x0190,
       0x0198,
       0x01A0,
       0x01A7,
       0x01AF,
       0x01B6,
       0x01BE,
       0x01C7,
       0x01CE,
       0x01D6,
       0x01DE,
       0x01E6,
       0x01EF,
       0x01F5,
       0x01FB,
       0x01FC,
       0x01FE,
       0x018C,
       0x01A4,
       0x0119,
       0x01EC,
       0x011B,
       0x014C,
       0x0140,
       0x0102, //SET G-Gamma
       0x0108,
       0x0112,
       0x011A,
       0x0122,
       0x012A,
       0x0131,
       0x0136,
       0x013F,
       0x0148,
       0x0151,
       0x0158,
       0x0160,
       0x0168,
       0x0170,
       0x0178,
       0x0180,
       0x0188,
       0x0190,
       0x0198,
       0x01A0,
       0x01A7,
       0x01AF,
       0x01B6,
       0x01BE,
       0x01C7,
       0x01CE,
       0x01D6,
       0x01DE,
       0x01E6,
       0x01EF,
       0x01F5,
       0x01FB,
       0x01FC,
       0x01FE,
       0x018C,
       0x01A4,
       0x0119,
       0x01EC,
       0x011B,
       0x014C,
       0x0140,
       0x0102, //SET B-Gamma
       0x0108,
       0x0112,
       0x011A,
       0x0122,
       0x012A,
       0x0131,
       0x0136,
       0x013F,
       0x0148,
       0x0151,
       0x0158,
       0x0160,
       0x0168,
       0x0170,
       0x0178,
       0x0180,
       0x0188,
       0x0190,
       0x0198,
       0x01A0,
       0x01A7,
       0x01AF,
       0x01B6,
       0x01BE,
       0x01C7,
       0x01CE,
       0x01D6,
       0x01DE,
       0x01E6,
       0x01EF,
       0x01F5,
       0x01FB,
       0x01FC,
       0x01FE,
       0x018C,
       0x01A4,
       0x0119,
       0x01EC,
       0x011B,
       0x014C,
       0x0140,
       0x002D,//Look up table
      };
      spiFd = node.handle();

      lcd->lcd_power_on();
      lcd->lcd_reset();
      lcd->lcd_backlight_on();

      do {
        uint16_t data = 0;
        if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0) {
          ERROR("SPI_IOC_WR_MODE: %s", strerror(errno));
          break;
        }

        if (ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
          ERROR("SPI_IOC_WR_BITS_PER_WORD: %s", strerror(errno));
          break;
        }

        if (ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
          ERROR("SPI_IOC_WR_MAX_SPEED_HZ: %s", strerror(errno));
          break;
        }

        for (int i = 0; i < (int)(sizeof(cmd)/sizeof(uint16_t)); ++ i) {
          node.write(&cmd[i], 2);
        }
        for (int i = 0; i < 64; ++ i) {
          data = ((8*i) & 0xffff) | 0x0100;
          node.write(&data, 2);
        }
        for (int i = 0; i < 64; ++ i) {
          data = ((4*i) & 0xffff) | 0x0100;
          node.write(&data, 2);
        }
        for (int i = 0; i < 64; ++ i) {
          data = ((8*i) & 0xffff) | 0x0100;
          node.write(&data, 2);
        }
        data = 0x003A; //Set Color Mode
        node.write(&data, 2);
        data = 0x0177; //RGB888
        node.write(&data, 2);
        data = 0x0011; //Sleep Out
        node.write(&data, 2);
        ::usleep(120000);
        data = 0x0029; //Display On
        node.write(&data, 2);

        if (lcd->lcd_pwm_get_max_brightness(maxLight)) {
          ret = lcd->lcd_pwm_set_brightness(maxLight*4/5);
        } else {
          ret = true;
        }
      } while(0);
      node.close();
    }
  }

  return ret;
}

struct LcdReg {
  uint16_t addr;
  uint8_t  val;
  LcdReg(uint16_t a, uint8_t v) : addr(a), val(v){}
};

static void lcd_1p3831_write_cmd(int spiFd, uint16_t cmd)
{
  uint16_t data;
  /* Address High Byte */
  data = 0x2000 | (cmd >> 8);
  ::write(spiFd, &data, sizeof(data));

  /* Address Low Byte */
  data = 0x0000 | (cmd & 0x00ff);
  ::write(spiFd, &data, sizeof(data));
}

static void lcd_1p3831_write_cmd_data(int spiFd, LcdReg& reg)
{
  uint16_t data;

  lcd_1p3831_write_cmd(spiFd, reg.addr);
  /* Data */
  data = 0x4000 | reg.val;
  ::write(spiFd, &data, sizeof(data));
}

static bool config_1p3831(AmVoutLcd *lcd)
{
  bool ret = false;
  const char *SpiNode = lcd->lcd_spi_dev_node();
  if (SpiNode) {
    AmFile node(SpiNode);
    int spiFd = -1;
    if (node.open(AmFile::AM_FILE_READWRITE)) {
      int32_t  maxLight = 0;
      uint8_t  mode     = 0;      /*CPHA = 0, CPOL = 0*/
      uint8_t  bits     = 16;     /*bits per word is 9*/
      uint32_t speed    = 500000; /*SSI_CLK = 500KHz*/
      LcdReg regs[]     =
      { /* Power Control */
        LcdReg(0xc000, 0x86),
        LcdReg(0xc001, 0x00),
        LcdReg(0xc002, 0x86),
        LcdReg(0xc003, 0x00),
        LcdReg(0xc100, 0x40),
        LcdReg(0xc200, 0x12),
        LcdReg(0xc202, 0x42),

        /* Gamma Control */
        LcdReg(0xe000, 0x0e),
        LcdReg(0xe001, 0x2a),
        LcdReg(0xe002, 0x33),
        LcdReg(0xe003, 0x3b),
        LcdReg(0xe004, 0x1e),
        LcdReg(0xe005, 0x30),
        LcdReg(0xe006, 0x64),
        LcdReg(0xe007, 0x3f),
        LcdReg(0xe008, 0x21),
        LcdReg(0xe009, 0x27),
        LcdReg(0xe00a, 0x88),
        LcdReg(0xe00b, 0x14),
        LcdReg(0xe00c, 0x35),
        LcdReg(0xe00d, 0x56),
        LcdReg(0xe00e, 0x79),
        LcdReg(0xe00f, 0xb8),
        LcdReg(0xe010, 0x55),
        LcdReg(0xe011, 0x57),
        LcdReg(0xe100, 0x0e),
        LcdReg(0xe101, 0x2a),
        LcdReg(0xe102, 0x33),
        LcdReg(0xe103, 0x3b),
        LcdReg(0xe104, 0x1e),
        LcdReg(0xe105, 0x30),
        LcdReg(0xe106, 0x64),
        LcdReg(0xe107, 0x3f),
        LcdReg(0xe108, 0x21),
        LcdReg(0xe109, 0x27),
        LcdReg(0xe10a, 0x88),
        LcdReg(0xe10b, 0x14),
        LcdReg(0xe10c, 0x35),
        LcdReg(0xe10d, 0x56),
        LcdReg(0xe10e, 0x79),
        LcdReg(0xe10f, 0xb8),
        LcdReg(0xe110, 0x55),
        LcdReg(0xe111, 0x57),
        LcdReg(0xe200, 0x0e),
        LcdReg(0xe201, 0x2a),
        LcdReg(0xe202, 0x33),
        LcdReg(0xe203, 0x3b),
        LcdReg(0xe204, 0x1e),
        LcdReg(0xe205, 0x30),
        LcdReg(0xe206, 0x64),
        LcdReg(0xe207, 0x3f),
        LcdReg(0xe208, 0x21),
        LcdReg(0xe209, 0x27),
        LcdReg(0xe20a, 0x88),
        LcdReg(0xe20b, 0x14),
        LcdReg(0xe20c, 0x35),
        LcdReg(0xe20d, 0x56),
        LcdReg(0xe20e, 0x79),
        LcdReg(0xe20f, 0xb8),
        LcdReg(0xe210, 0x55),
        LcdReg(0xe211, 0x57),
        LcdReg(0xe300, 0x0e),
        LcdReg(0xe301, 0x2a),
        LcdReg(0xe302, 0x33),
        LcdReg(0xe303, 0x3b),
        LcdReg(0xe304, 0x1e),
        LcdReg(0xe305, 0x30),
        LcdReg(0xe306, 0x64),
        LcdReg(0xe307, 0x3f),
        LcdReg(0xe308, 0x21),
        LcdReg(0xe309, 0x27),
        LcdReg(0xe30a, 0x88),
        LcdReg(0xe30b, 0x14),
        LcdReg(0xe30c, 0x35),
        LcdReg(0xe30d, 0x56),
        LcdReg(0xe30e, 0x79),
        LcdReg(0xe30f, 0xb8),
        LcdReg(0xe310, 0x55),
        LcdReg(0xe311, 0x57),
        LcdReg(0xe400, 0x0e),
        LcdReg(0xe401, 0x2a),
        LcdReg(0xe402, 0x33),
        LcdReg(0xe403, 0x3b),
        LcdReg(0xe404, 0x1e),
        LcdReg(0xe405, 0x30),
        LcdReg(0xe406, 0x64),
        LcdReg(0xe407, 0x3f),
        LcdReg(0xe408, 0x21),
        LcdReg(0xe409, 0x27),
        LcdReg(0xe40a, 0x88),
        LcdReg(0xe40b, 0x14),
        LcdReg(0xe40c, 0x35),
        LcdReg(0xe40d, 0x56),
        LcdReg(0xe40e, 0x79),
        LcdReg(0xe40f, 0xb8),
        LcdReg(0xe410, 0x55),
        LcdReg(0xe411, 0x57),
        LcdReg(0xe500, 0x0e),
        LcdReg(0xe501, 0x2a),
        LcdReg(0xe502, 0x33),
        LcdReg(0xe503, 0x3b),
        LcdReg(0xe504, 0x1e),
        LcdReg(0xe505, 0x30),
        LcdReg(0xe506, 0x64),
        LcdReg(0xe507, 0x3f),
        LcdReg(0xe508, 0x21),
        LcdReg(0xe509, 0x27),
        LcdReg(0xe50a, 0x88),
        LcdReg(0xe50b, 0x14),
        LcdReg(0xe50c, 0x35),
        LcdReg(0xe50d, 0x56),
        LcdReg(0xe50e, 0x79),
        LcdReg(0xe50f, 0xb8),
        LcdReg(0xe510, 0x55),
        LcdReg(0xe511, 0x57),

        /* RGB Interface Format */
        LcdReg(0x3a00, 0x07),
        LcdReg(0x3b00, 0x03)};
      spiFd = node.handle();

      lcd->lcd_power_on();
      lcd->lcd_reset();
      lcd->lcd_backlight_on();

      do {
        if (ioctl(spiFd, SPI_IOC_WR_MODE, &mode) < 0) {
          ERROR("SPI_IOC_WR_MODE: %s", strerror(errno));
          break;
        }
        if (ioctl(spiFd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
          ERROR("SPI_IOC_WR_MODE: %s", strerror(errno));
          break;
        }
        if (ioctl(spiFd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
          ERROR("SPI_IOC_WR_MAX_SPEED_HZ");
          break;
        }
        lcd_1p3831_write_cmd(spiFd, 0x1100); /* Sleep Out */
        ::usleep(200000);

        for (int i = 0; i < (int)(sizeof(regs)/sizeof(LcdReg)); ++ i) {
          lcd_1p3831_write_cmd_data(spiFd, regs[i]);
        }

        lcd_1p3831_write_cmd(spiFd, 0x2900); /* Display On*/
        ::usleep(2000000);
        if (lcd->lcd_pwm_get_max_brightness(maxLight)) {
          ret = lcd->lcd_pwm_set_brightness(maxLight*4/5);
        } else {
          ret = true;
        }
      } while (0);
      node.close();
    }
  }

  return ret;
}

bool AmVoutLcd::lcd_panel_config(VoutParameters       *voutConfig,
                                 amba_video_sink_mode &sinkConfig)
{
  bool ret = false;
  sinkConfig.mode = voutConfig->video_mode;
  switch(voutConfig->video_mode) {
    case AMBA_VIDEO_MODE_WVGA: {
      res_800_400(sinkConfig);
      DEBUG("AMBA_VIDEO_MODE_WVGA");
    }break;
    case AMBA_VIDEO_MODE_HVGA: {
      res_320_480(sinkConfig);
      DEBUG("AMBA_VIDEO_MODE_HVGA");
    }break;
    case AMBA_VIDEO_MODE_480_800: {
      res_480_800(sinkConfig);
      DEBUG("AMBA_VIDEO_MODE_480_800");
    }break;
    default:
      DEBUG("AMBA_VIDEO_MODE_AUTO");
      break;
  }
  switch(voutConfig->lcd_type) {
    case AM_LCD_PANEL_TD043: {
      sinkConfig.lcd_cfg.model = AMBA_VOUT_LCD_MODEL_TD043;
      DEBUG("LCD TD043");
      ret = td043_config(this);
    }break;
    case AM_LCD_PANEL_TPO489: {
      sinkConfig.lcd_cfg.model = AMBA_VOUT_LCD_MODEL_TPO489;
      DEBUG("LCD TPO489");
      ret = tpo489_config(this);
    }break;
    case AM_LCD_PANEL_1P3828: {
      sinkConfig.lcd_cfg.model = AMBA_VOUT_LCD_MODEL_1P3828;
      DEBUG("LCD 1P3828");
      ret = config_1p3828(this);
    }break;
    case AM_LCD_PANEL_1P3831: {
      sinkConfig.lcd_cfg.model = AMBA_VOUT_LCD_MODEL_1P3831;
      DEBUG("LCD 1P3831");
      ret = config_1p3831(this);
    }break;
    default: break;
  }

  return ret;
}
