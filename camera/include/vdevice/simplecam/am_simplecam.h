/*******************************************************************************
 * am_simplecam.h
 *
 * Histroy:
 *  2012-3-9 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMSIMPLECAM_H_
#define AMSIMPLECAM_H_

/*******************************************************************************
 * AmSimpleCam is a simple video device, which just enable preview and
 * start/stop encoding functions
 ******************************************************************************/

#define MAX_YUV_BUFFER_SIZE 1920*1080

class AmSimpleCam: public AmVideoDevice
{
  public:
    AmSimpleCam(VDeviceParameters *vDeviceConfig);
    virtual ~AmSimpleCam();

  public:
    virtual bool goto_idle();
    virtual bool enter_preview();
    virtual bool start_encode();
    virtual bool stop_encode();
    virtual bool enter_decode_mode();
    virtual bool leave_decode_mode();
    virtual bool enter_photo_mode();
    virtual bool leave_photo_mode();

    virtual bool apply_encoder_parameters(IavState targetIavState);
    virtual bool apply_stream_parameters();

  public:
    bool ready_for_encode();
    bool reset_device();
    bool get_y_data(YBufferFormat *yBufferFormat);

  private:
    bool save_y_data(uint8_t *yBuffer, iav_yuv_buffer_info_ex_t *yuvInfo);
    bool init_device(int voutInitMode, bool force = false);
    virtual bool change_iav_state(IavState target);

  private:
    uint8_t *mYdataBuffer;
};

#endif /* AMSIMPLECAM_H_ */
