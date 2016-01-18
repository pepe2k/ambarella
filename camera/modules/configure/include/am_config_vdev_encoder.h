/*******************************************************************************
 * am_config_encoder.h
 *
 * Histroy:
 *  2012-3-19 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMCONFIGENCODER_H_
#define AMCONFIGENCODER_H_

/*******************************************************************************
 * AmConfigEncoder class provides methods to retrieve and set configs of encoder
 * it is derived from AmConfig implementing encoder Configuration
 * Retrieving and Setting methods
 ******************************************************************************/
#define BUFFER_SUB1_MAX_W 720
#define BUFFER_SUB1_MAX_H 576

#define BUFFER_SUB2_MAX_W 1920
#define BUFFER_SUB2_MAX_H 1080

#define BUFFER_SUB3_MAX_W 1280
#define BUFFER_SUB3_MAX_H 720

class AmConfigEncoder: public AmConfigBase
{
  public:
    AmConfigEncoder(const char *configFileName)
      : AmConfigBase(configFileName),
        mEncoderParams(NULL){}
    virtual ~AmConfigEncoder()
    {
      delete mEncoderParams;
      DEBUG("AmConfigEncoder deleted!");
    }

  public:
    EncoderParameters* get_encoder_config(int mIav);
    void set_encoder_config(EncoderParameters *config);

  private:
    void get_sys_setup_config(iav_system_setup_info_ex_t &config);
    void set_sys_setup_config(iav_system_setup_info_ex_t &config);

    void get_sys_limit_config(iav_system_resource_setup_ex_t &config, int iav);
    void set_sys_limit_config(iav_system_resource_setup_ex_t &config);

    void get_buffer_format_config(iav_source_buffer_format_all_ex_t &config);
    void set_buffer_format_config(iav_source_buffer_format_all_ex_t &config);

#ifdef CONFIG_ARCH_S2
    void get_warp_config(WarpParameters &config);
    void set_warp_config(WarpParameters &config);
#endif

  private:
    EncoderParameters *mEncoderParams;
};

#endif /* AMCONFIGENCODER_H_ */
