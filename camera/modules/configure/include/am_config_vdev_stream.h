/*******************************************************************************
 * am_config_stream.h
 *
 * Histroy:
 *  2012-3-20 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMCONFIGSTREAM_H_
#define AMCONFIGSTREAM_H_

class AmConfigStream: public AmConfigBase
{
  public:
    AmConfigStream(const char *configFileName)
      : AmConfigBase(configFileName)
    {
      memset(mStreamParamsList, 0, sizeof(mStreamParamsList));
    }
    virtual ~AmConfigStream()
    {
      for (uint32_t i = 0; i < MAX_ENCODE_STREAM_NUM; ++ i) {
        delete mStreamParamsList[i];
      }
      DEBUG("AmConfigStream deleted!");
    }

  public:
    StreamParameters* get_stream_config(int mIav, uint32_t streamID);
    void set_stream_config(StreamParameters *streamConfig, uint32_t streamID);

  private:
    void get_bitrate_config(iav_bitrate_info_ex_t &config, uint32_t streamID);
    void set_bitrate_config(iav_bitrate_info_ex_t &config, uint32_t streamID);

    void get_encode_format_config(StreamEncodeFormat &config, uint32_t streamID);
    void set_encode_format_config(StreamEncodeFormat &config, uint32_t streamID);

#ifdef CONFIG_ARCH_S2
    void get_warp_window_config(Rect& config, uint32_t);
    void set_warp_window_config(Rect& config, uint32_t);

    void get_h264_config(iav_h264_config_ex_t &config, iav_encode_format_ex_t &encode_format, uint32_t streamID);
    void set_h264_config(iav_h264_config_ex_t &config, iav_encode_format_ex_t &encode_format, uint32_t streamID);

    void get_mjpeg_config(iav_jpeg_config_ex_t &config, iav_encode_format_ex_t &encode_format, uint32_t streamID);
    void set_mjpeg_config(iav_jpeg_config_ex_t &config, iav_encode_format_ex_t &encode_format, uint32_t streamID);


#else
    void get_h264_config(iav_h264_config_ex_t &config, uint32_t streamID);
    void set_h264_config(iav_h264_config_ex_t &config, uint32_t streamID);

    void get_mjpeg_config(iav_jpeg_config_ex_t &config, uint32_t streamID);
    void set_mjpeg_config(iav_jpeg_config_ex_t &config, uint32_t streamID);
#endif

  private:
    StreamParameters *mStreamParamsList[MAX_ENCODE_STREAM_NUM];
};


#endif /* AMCONFIGSTREAM_H_ */
