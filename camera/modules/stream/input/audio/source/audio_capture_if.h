/*******************************************************************************
 * audio_capture_if.h
 *
 * Histroy:
 *   2012-9-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_CAPTURE_IF_H_
#define AUDIO_CAPTURE_IF_H_

extern const AM_IID IID_IAudioSource;

class IAudioSource: public IInterface
{
  public:
    DECLARE_INTERFACE(IAudioSource, IID_IAudioSource);
    virtual AM_ERR Start() = 0;
    virtual AM_ERR Stop()  = 0;
    virtual void SetEnabledAudioId(AM_UINT streams) = 0;
    virtual void SetAvSyncMap(AM_UINT avSyncMap) = 0;
    virtual void SetAudioParameters(AM_UINT samplerate,
                                    AM_UINT channel,
                                    AM_UINT codecType) = 0;
};


#endif /* AUDIO_CAPTURE_IF_H_ */
