/*******************************************************************************
 * AmMdCam.h
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

#ifndef _AM_MD_CAM_H_
#define _AM_MD_CAM_H_

#include "am_include.h"
#include "am_data.h"
#include "am_utility.h"
#include "am_vdevice.h"
#include "am_vin.h"
#include "am_vout.h"
#include "am_vout_lcd.h"
#include "am_vout_hdmi.h"

enum IavBufType {
  AM_IAV_BUF_SECOND     = 0,
  AM_IAV_BUF_ME1_MAIN   = 1,
  AM_IAV_BUF_ME1_SECOND = 2,

  AM_IAV_BUF_MAX,
};

struct YRawFormat {
    uint8_t *y_addr;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
};

class AmMdCam: public AmSimpleCam
{
public:
  AmMdCam(VDeviceParameters *vDeviceConfig);
  virtual ~AmMdCam();

public:
  bool get_yuv_data_init(void);
  bool get_yuv_data_deinit(void);
  bool get_y_raw_data(YRawFormat *yRawFormat, IavBufType bufType);
};
#endif //_AM_MD_CAM_H_
