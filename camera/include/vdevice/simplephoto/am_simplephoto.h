/*******************************************************************************
 * am_simplephoto.h
 *
 * Histroy:
 *  2012-4-9 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_SIMPLEPHOTO_H_
#define AM_SIMPLEPHOTO_H_

class AmSimplePhoto: public AmVideoDevice
{
  public:
    AmSimplePhoto(VDeviceParameters *vDeviceConfig);
    virtual ~AmSimplePhoto();

  public:
    bool take_photo(PhotoType type,      uint32_t num = 1,
                    uint32_t jpeg_w = 0, uint32_t jpeg_h = 0);
    bool reset_device();
    bool is_camera_started();

  private:
    virtual bool goto_idle();
    virtual bool enter_preview();
    virtual bool start_encode();
    virtual bool stop_encode();
    virtual bool enter_decode_mode();
    virtual bool leave_decode_mode();
    virtual bool enter_photo_mode();
    virtual bool leave_photo_mode();

  private:
    virtual bool apply_encoder_parameters();
    virtual bool change_iav_state(IavState target);
    bool init_device(int voutInitMode, bool force = false);
    bool shoot(PhotoType type, uint32_t jpeg_w = 0, uint32_t jpeg_h = 0);
    bool save_raw(const char *filename);
    bool save_jpeg(const char *filename);
    void generate_file_name(char *name, const char *prefix, const char *ext);

  private:
    uint32_t        mPhotoCount;
};


#endif /* AM_SIMPLEPHOTO_H_ */
