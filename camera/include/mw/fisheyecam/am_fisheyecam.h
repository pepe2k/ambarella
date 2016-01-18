/*******************************************************************************
 * am_fisheyecam.h
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

#ifndef AM_FISHEYECAM_H_
#define AM_FISHEYECAM_H_

#define MAX_FISHEYE_REGION_NUM     6

class AmFisheyeTransform;
class AmOverlayGenerator;
class AmOverlayTimeTask;
class AmCam;

struct RegionToArea {
    TransformMode mode;
    uint32_t      area_num;
    uint32_t      area_map;
};

class AmFisheyeCam : public AmCam {
  public:
    AmFisheyeCam(AmConfig *config);
    virtual ~AmFisheyeCam();

  public:
    virtual bool init(void);

    virtual bool start_encode();
    virtual bool stop_encode();

    virtual bool get_stream_type(uint32_t streamId, EncodeType *pType);
    virtual bool set_stream_type(uint32_t streamId, const EncodeType type);
    virtual bool get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate);
    virtual bool set_stream_framerate(uint32_t streamId, const uint32_t frameRate);
    virtual bool get_stream_idr(uint32_t streamId, uint8_t *idr_interval);
    virtual bool set_stream_idr(uint32_t streamId, uint8_t idr_interval);
    virtual bool get_stream_N(uint32_t streamId, uint16_t *n);
    virtual bool set_stream_N(uint32_t streamId, uint16_t n);
    virtual bool get_stream_profile(uint32_t streamId, uint8_t *profile);
    virtual bool set_stream_profile(uint32_t streamId, uint8_t profile);
    virtual bool get_mjpeg_quality(uint32_t streamId, uint8_t *quality);
    virtual bool set_mjpeg_quality(uint32_t streamId, uint8_t quality);
    virtual bool get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate);
    virtual bool set_cbr_bitrate(uint32_t streamId, uint32_t bitrate);
    virtual bool set_stream_size_all(uint32_t totalNum, const EncodeSize *pSize);
    virtual bool set_privacy_mask(PRIVACY_MASK * pm_in);
    virtual bool get_privacy_mask(uint32_t * pm_in);
    virtual bool set_digital_ptz(DPTZParam	 *dptz_set);
    virtual bool get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get);

    bool start(FisheyeParameters* fisheye_config);

    bool set_transform_mode_all(uint32_t totalNum, TransformMode *mode);
    bool set_transform_region(uint32_t totalNum, TransformParameters *pTrans);

  private:
    bool  is_valid_region_id(uint32_t regionId);

  private:
    uint32_t           mUsedWarpAreaNumber;
    AmWarpDevice       *mWarpDevice;
    AmFisheyeTransform *mTransform;

  private:
    RegionToArea mRegionToArea[MAX_FISHEYE_REGION_NUM];
};


#endif /* AM_FISHEYECAM_H_ */
