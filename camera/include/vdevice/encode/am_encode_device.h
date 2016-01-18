/*******************************************************************************
 * am_encode_device.h
 *
 * History:
 *  Nov 28, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_ENCODE_DEVICE_H_
#define AM_ENCODE_DEVICE_H_

#include <assert.h>
#include "am_pm_dptz.h"

class AmEncodeDevice: public AmVideoDevice
{
  public:
    AmEncodeDevice(VDeviceParameters *vDeviceConfig);
    virtual ~AmEncodeDevice();

  public:
    virtual bool start_encode();
    virtual bool stop_encode();

    virtual bool goto_idle();
    virtual bool enter_preview();
    virtual bool enter_decode_mode();
    virtual bool leave_decode_mode();
    virtual bool enter_photo_mode();
    virtual bool leave_photo_mode();
    virtual bool change_iav_state(IavState target);

    virtual bool change_stream_state(IavState target, uint32_t streams = 0);

    virtual bool apply_encoder_parameters();
    virtual bool apply_stream_parameters();

  public:
    void get_max_stream_num(uint32_t *pMaxNum);
    void get_max_stream_size(Resolution *pMaxSize);

    bool get_stream_source(uint32_t streamId, u8 *source);

    bool ready_for_encode();
    bool start_encode_stream(uint32_t streamId);
    bool stop_encode_stream(uint32_t streamId);

    bool get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate);
    bool set_stream_framerate(uint32_t streamId, const uint32_t frameRate);

    bool get_stream_type(uint32_t streamId, EncodeType *pType);
    bool set_stream_type(uint32_t streamId, const EncodeType type);

    bool get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate);
    bool set_cbr_bitrate(uint32_t streamId, uint32_t bitrate);

    /**************** Privacy Mask related ****************/
    int set_pm_param(PRIVACY_MASK * pm_in);
    int get_pm_param(uint32_t * pm_in);
#ifdef CONFIG_ARCH_S2
    int disable_pm();
    int reset_pm();
#endif
    /**************** Privacy Mask related ****************/

    //DPTZ
    int set_dptz_param(DPTZParam	 *dptz_set);
    int get_dptz_param(uint32_t stream_id, DPTZParam *dptz_get);
    //DPTZ

    bool get_stream_idr(uint32_t streamId, uint8_t *idr_interval);
    bool set_stream_idr(uint32_t streamId, uint8_t idr_interval);
    bool get_stream_n(uint32_t streamId, uint16_t *n);
    bool set_stream_n(uint32_t streamId, uint16_t n);
    bool get_stream_profile(uint32_t streamId, uint8_t *profile);
    bool set_stream_profile(uint32_t streamId, uint8_t profile);
    bool get_mjpeg_quality(uint32_t streamId, uint8_t *quality);
    bool set_mjpeg_quality(uint32_t streamId, uint8_t quality);

  protected:
    bool encode(uint32_t* streams);

  private:
    bool init_device(int voutInitMode, bool force = false);
    bool eval_qp_mode(uint32_t streamId);
    uint32_t linear_scale(uint32_t in, uint32_t in_min, uint32_t in_max,
                          uint32_t out_min, uint32_t out_max);

    // IAV ioctl
    bool update_stream_h264_qp(uint32_t streamId);
    bool do_force_idr_insertion(uint32_t streamId);

  private:

    AmPrivacyMaskDPTZ *instance_pm_dptz;
};


#endif /* AM_ENCODE_DEVICE_H_ */
