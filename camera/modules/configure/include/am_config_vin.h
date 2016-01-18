/*******************************************************************************
 * am_config_vin.h
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

#ifndef AMCONFIGVIN_H_
#define AMCONFIGVIN_H_

/*******************************************************************************
 * AmConfigVin class provides methods to retrieve and set configs of Vin,
 * it is derived from AmConfig implementing Vin Configuration Retrieving
 * and Setting methods
 ******************************************************************************/

class AmConfigVin: public AmConfigBase
{
  public:
    AmConfigVin(const char *configFileName);
    virtual ~AmConfigVin();

  public:
    VinParameters* get_vin_config(VinType type);
    void           set_vin_config(VinParameters *vinConfig, VinType type);
    uint32_t       str_to_fps(const char *fps);
    const char*    fps_to_str(uint32_t fps);

  private:
    enum DigitalType {TYPE_NONE, TYPE_INT, TYPE_DOUBLE};

  private:
    inline VinParameters* get_yuv_config();
    inline void set_yuv_config(VinParameters *vinConfig);

    inline VinParameters* get_rgb_config();
    inline void set_rgb_config(VinParameters *vinConfig);

    DigitalType check_digits_type(const char *str);

  private:
    VinParameters *mVinParamsYuv;
    VinParameters *mVinParamsRgb;
};
#endif /* AMCONFIGVIN_H_ */
