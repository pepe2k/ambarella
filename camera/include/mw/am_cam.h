/*******************************************************************************
 * am_cam.h
 *
 * History:
 *  Jul 1, 2013 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CAM_H_
#define AM_CAM_H_

#define MAX_TIME_LENGTH         128

class AmOverlayGenerator;
class AmOverlayTimeTask;
class AmCam {
  public:
    AmCam();
      virtual ~AmCam();

    public:
      virtual bool start_encode() = 0;
      virtual bool stop_encode() = 0;

      virtual bool get_stream_type(uint32_t streamId, EncodeType *pType) = 0;
      virtual bool set_stream_type(uint32_t streamId, const EncodeType type) = 0;
      virtual bool get_stream_framerate(uint32_t streamId, uint32_t *pFrameRate) = 0;
      virtual bool set_stream_framerate(uint32_t streamId, const uint32_t frameRate) = 0;
      virtual bool get_stream_idr(uint32_t streamId, uint8_t *idr_interval) = 0;
      virtual bool set_stream_idr(uint32_t streamId, uint8_t idr_interval) = 0;
      virtual bool get_stream_N(uint32_t streamId, uint16_t *n) = 0;
      virtual bool set_stream_N(uint32_t streamId, uint16_t n) = 0;
      virtual bool get_stream_profile(uint32_t streamId, uint8_t *profile) = 0;
      virtual bool set_stream_profile(uint32_t streamId, uint8_t profile) = 0;
      virtual bool get_mjpeg_quality(uint32_t streamId, uint8_t *quality) = 0;
      virtual bool set_mjpeg_quality(uint32_t streamId, uint8_t quality) = 0;
      virtual bool get_cbr_bitrate(uint32_t streamId, uint32_t *pBitrate) = 0;
      virtual bool set_cbr_bitrate(uint32_t streamId, uint32_t bitrate) = 0;

      virtual uint32_t get_stream_max_num(void);
      virtual bool is_stream_encoding(uint32_t streamId);

      virtual bool get_stream_size(uint32_t streamId, EncodeSize *pSize);
      virtual bool set_stream_size(uint32_t id, const EncodeSize *pSize);
      virtual bool set_stream_size_all(uint32_t num, const EncodeSize *pSize);

      virtual bool add_overlay_bitmap(uint32_t streamId, uint32_t areaId,
                                    Point offset,  const char *pBitmap);
      virtual bool add_overlay_text(uint32_t streamId, uint32_t areaId, Point offset,
                          TextBox *pBox, char *pText);
      virtual bool add_overlay_time(uint32_t streamId, uint32_t areaId, Point offset,
                          TextBox *pBox);
      virtual bool remove_overlay(uint32_t streamId, uint32_t areaId);
      virtual bool set_privacy_mask(PRIVACY_MASK * pm_in);
      virtual bool get_privacy_mask(uint32_t * pm_in);
      virtual bool set_digital_ptz(DPTZParam	 *dptz_set);
      virtual bool get_digital_ptz(uint32_t streamId, DPTZParam *dptz_get);

    protected:
      virtual bool init(void);

      bool  create_overlaytime_task();
      bool  destroy_overlaytime_task();

    protected:
      AmConfig         *mConfig;
      AmVideoDevice    *mVideoDevice;

    protected:
      uint32_t          mStreamMaxNumber;
      AmOverlayTimeTask *mOverlayTimeTask;

    protected:
      bool mIsCamCreated;
      bool mIsCamInited;
};


#endif /* AM_CAM_H_ */
