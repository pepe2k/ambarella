/*******************************************************************************
 * audio_encoder_if.h
 *
 * Histroy:
 *   2012-10-8 - [ypchang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_ENCODER_IF_H_
#define AUDIO_ENCODER_IF_H_

extern const AM_IID IID_IAudioEncoder;

class IAudioEncoder: public IInterface
{
  public:
    DECLARE_INTERFACE(IAudioEncoder, IID_IAudioEncoder);
    virtual AM_ERR Stop() = 0;
    virtual AM_ERR SetAudioCodecType(AM_UINT type, void *codecInfo) = 0;
};


#endif /* AUDIO_ENCODER_IF_H_ */
