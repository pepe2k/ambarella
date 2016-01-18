/*******************************************************************************
 * am_webcam.h
 *
 * History:
 *  Dec 18, 2012 2012 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_WEBCAM_H_
#define AM_WEBCAM_H_

class AmWebCam : public AmCam
{
  public:
    AmWebCam(AmConfig *config);
    virtual ~AmWebCam();

  public:
    virtual bool init(void);

    virtual bool start_encode();
    virtual bool stop_encode();

    virtual void get_streamMaxSize(Resolution *pMaxSize);

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

    virtual bool set_privacy_mask(PRIVACY_MASK * pm_in);
    virtual bool set_digital_ptz(DPTZParam	 *dptz_set);
    virtual bool get_digital_ptz(uint32_t stream_id, DPTZParam *dptz_get);
#ifdef CONFIG_ARCH_S2
    virtual bool get_privacy_mask(uint32_t * pm_in);
    int reset_dptz_pm();
#endif

  protected:
    AmEncodeDevice  *mEncodeDevice;
};


#endif /* AM_WEBCAM_H_ */
