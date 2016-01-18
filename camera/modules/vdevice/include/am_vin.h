/*******************************************************************************
 * am_vin.h
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

#ifndef AMVIN_H_
#define AMVIN_H_

/*******************************************************************************
 * AmVin is the base class of all VIN devices, DO NOT use this class
 * directly, use the inherited classes instead
 ******************************************************************************/
class AmVin
{
  public:
    AmVin(VinType type, int iav = -1);
    virtual ~AmVin();

  public:
    void set_vin_config(VinParameters *config);
    bool get_current_vin_mode(amba_video_mode &mode);
    bool get_current_vin_fps(uint32_t *fps);
    bool get_current_vin_size(Resolution& size);

  public:
    bool           start (bool force = false);

  public: /* Configurations */
    uint32_t       get_vin_fps();
    bool           get_vin_size(Resolution& size);
    bool           check_vin_parameters();
    const char*    video_fps_string(uint32_t fps);
    const char*    video_mode_string(int32_t fps);

  protected:
    VinType        mVinType;
    int            mVinIav;
    VinParameters *mVinParams;

  private:
    bool         mNeedCloseIav;
    bool         mIsVinStarted;
};

class AmVinRgb: public AmVin
{
  public:
    AmVinRgb(int iav = -1): AmVin(AM_VIN_TYPE_RGB, iav){}
    virtual ~AmVinRgb(){}
};

class AmVinYuv: public AmVin
{
  public:
    AmVinYuv(int iav = -1): AmVin(AM_VIN_TYPE_YUV, iav){}
    virtual ~AmVinYuv(){}
};

#endif /* AMVIN_H_ */
