/*******************************************************************************
 * am_config_fisheye.h
 *
 * History:
 *  Mar 20, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CONFIG_FISHEYE_H_
#define AM_CONFIG_FISHEYE_H_

class AmConfigFisheye: public AmConfigBase
{
  public:
    AmConfigFisheye(const char *configFileName) :
      AmConfigBase(configFileName),
      mFisheyeParameters(NULL){}
    virtual ~AmConfigFisheye()
    {
      delete mFisheyeParameters;
    }

  public:
    FisheyeParameters *get_fisheye_config();
    void set_fisheye_config(FisheyeParameters *config);

  private:
    void get_warp_config(WarpParameters &config);
    void set_warp_config(WarpParameters &config);

  private:
    FisheyeParameters *mFisheyeParameters;
};


#endif /* AM_CONFIG_FISHEYE_H_ */
