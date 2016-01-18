/*******************************************************************************
 * am_video_device.h
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

#ifndef AM_VIDEO_DEVICE_H_
#define AM_VIDEO_DEVICE_H_

/*******************************************************************************
 * AmVideoDevice is the base class of all the video devices,
 * DO NOT use this class directly, use the inherited classes instead
 ******************************************************************************/
class AmVin;
class AmVout;

class AmVideoDevice
{
  public:
    AmVideoDevice();
    virtual ~AmVideoDevice();

  public:
    bool create_video_device(VDeviceParameters *vdevConfig);
    void set_vin_config(VinParameters *config, uint32_t vinId);
    void set_vout_config(VoutParameters *config, uint32_t voutId);
    void set_encoder_config(EncoderParameters *config);
    void set_stream_config(StreamParameters *config, uint32_t streamId);
    void set_photo_config(PhotoParameters *config);
    bool is_stream_encoding(uint32_t streamId);
    bool force_idr_insertion(uint32_t streamId);

  public:
    /* The child classes should implement the following functions
     * regarding to the actual needs and requirements
     */
    virtual bool goto_idle()         = 0;
    virtual bool enter_preview()     = 0;
    virtual bool start_encode()      = 0;
    virtual bool stop_encode()       = 0;
    virtual bool enter_decode_mode() = 0;
    virtual bool leave_decode_mode() = 0;
    virtual bool enter_photo_mode()  = 0;
    virtual bool leave_photo_mode()  = 0;

    virtual bool get_stream_size(uint32_t streamId, EncodeSize *pSize);
    virtual bool set_stream_size_streamid(uint32_t id, const EncodeSize *pSize);
    virtual bool set_stream_size_all(uint32_t num, const EncodeSize *pSize);

    virtual uint32_t get_stream_overlay_max_size(uint32_t streamId, uint32_t areaId);
    virtual bool     set_stream_overlay(uint32_t streamId, uint32_t areaId,
                                        const Overlay *pOverlay);

  protected:
    virtual bool change_iav_state(IavState target) = 0;

  protected:
    /* This function should be implemented to apply encoder settings */
    virtual bool apply_encoder_parameters();

    /* This function should be implemented to apply stream settings */
    virtual bool apply_stream_parameters();

    virtual bool check_system_performance();
    virtual bool assign_buffer_to_stream();

  protected:
    IavState get_iav_status();
    bool idle();
    bool preview();
    bool encode(bool start);
    bool decode(bool start, int mode = 0);
    bool map_dsp();
    bool map_bsb();
    bool unmap_dsp();
    bool unmap_bsb();
    bool map_overlay();
    bool unmap_overlay();

    bool check_system_resource();
    bool check_encoder_parameters();
    bool check_stream_format(uint32_t streamId);
    bool check_stream_h264_config(uint32_t streamId);
    bool check_stream_id(uint32_t streamId);

    /* These functions are used to update the parameters in IAV driver */
    bool update_encoder_parameters();
    bool update_stream_format(uint32_t streamId);
    bool update_stream_framerate(uint32_t streamId);
    bool update_stream_h264_config(uint32_t streamId);
    bool update_stream_h264_bitrate(uint32_t streamId);
    bool update_stream_mjpeg_config(uint32_t streamId);
    bool update_stream_overlay(uint32_t streamId);

    const char* iav_state_to_str(IavState state);

  private:
    uint32_t get_system_max_performance();
    const char * chip_string();
    bool do_force_idr_insertion(uint32_t streamId);

  protected:
    AmVin            **mVinList;
    AmVout           **mVoutList;
    VinParameters    **mVinParamList;
    VinType           *mVinTypeList;
    VoutParameters   **mVoutParamList;
    VoutType          *mVoutTypeList;
    StreamParameters **mStreamParamList;
    EncoderParameters *mEncoderParams;
    PhotoParameters   *mPhotoParams;

  protected:
    uint32_t        mVinNumber;
    uint32_t        mVoutNumber;
    uint32_t        mStreamNumber;
    int             mIav;
    iav_mmap_info_t mDspMapInfo;
    iav_mmap_info_t mBsbMapInfo;
    iav_mmap_info_t mOverlayMapInfo;

  private:
    bool mIsDevCreated;
    bool mIsDspMapped;
    bool mIsBsbMapped;
    bool mIsOverlayMapped;

};

#endif /* AM_VIDEO_DEVICE_H_ */
