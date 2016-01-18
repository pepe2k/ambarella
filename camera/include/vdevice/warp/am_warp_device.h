/*******************************************************************************
 * am_warp_device.h
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

#ifndef AM_WARP_DEVICE_H_
#define AM_WARP_DEVICE_H_

#include "am_pm_dptz.h"

#define WARP_MAP_NUM          2

struct Vertex {
    int  x;
    int  y;
    bool is_left;
    bool is_right;
    bool is_upper;
    bool is_lower;
    Vertex(int xx = 0, int yy = 0, bool left = false, bool right = false,
           bool upper = false, bool lower = false):
      x(xx), y(yy), is_left(left), is_right(right), is_upper(upper),
      is_lower(lower) {}
};

class AmWarpDevice: public AmVideoDevice {
  public:
    AmWarpDevice(VDeviceParameters *vDeviceSize);
    virtual ~AmWarpDevice();

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

    virtual bool assign_buffer_to_stream();
    virtual bool assign_dptz_warp();
    virtual bool apply_encoder_parameters();
    virtual bool apply_stream_parameters();
    virtual bool apply_encoder_parameters_in_preview();

    virtual bool get_stream_size(uint32_t streamId, EncodeSize *pSize);
    virtual bool set_stream_size_id(uint32_t streamId, const EncodeSize *pSize);
    virtual bool set_stream_size_all(uint32_t totalNum, const EncodeSize *pSize);

  public:
    void set_warp_config(WarpParameters *config);

  public:
    bool start(bool force = false);

    void get_max_stream_num(uint32_t *pMaxNum);
    void get_max_stream_size(Resolution *pMaxSize);

    bool ready_for_encode();

    bool start_encode_stream(uint32_t streamId);
    bool stop_encode_stream(uint32_t streamId);

    bool get_warp_control(WarpControl *pControl);
    bool set_warp_control(uint32_t totalNum, const WarpControl *pControl);

    bool get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate);
    bool set_stream_framerate(uint32_t streamId, const uint32_t frameRate);

    bool get_stream_type(uint32_t streamId, EncodeType *pType);
    bool set_stream_type(uint32_t streamId, const EncodeType type);

    bool get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate);
    bool set_cbr_bitrate(uint32_t streamId, uint32_t bitrate);

    bool get_stream_idr(uint32_t streamId, uint8_t *idr_interval);
    bool set_stream_idr(uint32_t streamId, uint8_t idr_interval);
    bool get_stream_n(uint32_t streamId, uint16_t *n);
    bool set_stream_n(uint32_t streamId, uint16_t n);
    bool get_stream_profile(uint32_t streamId, uint8_t *profile);
    bool set_stream_profile(uint32_t streamId, uint8_t profile);
    bool get_mjpeg_quality(uint32_t streamId, uint8_t *quality);
    bool set_mjpeg_quality(uint32_t streamId, uint8_t quality);
     /**************** Privacy Mask related ****************/
    bool set_pm_param(PRIVACY_MASK * pm_in);
    bool get_pm_param(uint32_t * pm_in);
    bool disable_pm();
    bool reset_pm();
    /**************** Privacy Mask related ****************/

    //DPTZ
    bool set_dptz_param(DPTZParam	 *dptz_set);

 private:
    bool init_device(int voutInitMode, bool force = false);
    bool eval_qp_mode(uint32_t streamId);
    bool is_vertex_inside_rectangle(Vertex &v, Rect &r);
    uint32_t linear_scale(uint32_t in, uint32_t in_min, uint32_t in_max, uint32_t out_min, uint32_t out_max);
    uint32_t get_grid_spacing(uint32_t exponent);
    uint32_t get_grid_exponent(uint32_t spacing);


    // IAV ioctl
    bool update_stream_h264_qp(uint32_t streamId);
    bool update_encoder_warp();
    bool update_encoder_dptz_warp();

  protected:
    bool create_warp_map();
    bool destroy_warp_map();

    bool encode(uint32_t* streams);

  protected:
    int16_t  *mWarpMap[MAX_WARP_AREA_NUM][WARP_MAP_NUM];

  private:
    bool mIsWarpMapCreated;
    bool mIsDeviceStarted;
  private:

    AmPrivacyMaskDPTZ *instance_pm_dptz;
};

#endif /* AM_WARP_DEVICE_H_ */
