/*******************************************************************************
 * video_recorder_if.h
 *
 * Histroy:
 *   2012-10-3 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef VIDEO_RECORDER_IF_H_
#define VIDEO_RECORDER_IF_H_

extern const AM_IID IID_IVideoSource;

class IVideoSource: public IInterface
{
  public:
    DECLARE_INTERFACE(IVideoSource, IID_IVideoSource);
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Start(AM_UINT streamid) = 0;
    virtual AM_ERR Stop()  = 0;
    virtual AM_ERR Stop(AM_UINT streamid)  = 0;
    virtual void SetEnabledVideoId(AM_UINT streams) = 0;
    virtual void SetAvSyncMap(AM_UINT avSyncMap) = 0;
    virtual AM_UINT GetMaxEncodeNumber() = 0;
#if defined(CONFIG_ARCH_A5S) && defined(ENABLE_FRAME_STATISTICS)
    virtual void SetFrameStatisticCallback(void (*callback)(void *)) = 0;
#endif
};

#endif /* VIDEO_RECORDER_IF_H_ */
