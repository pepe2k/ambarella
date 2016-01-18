/*******************************************************************************
 * audio_decoder_if.h
 *
 * History:
 *   2013-3-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AUDIO_DECODER_IF_H_
#define AUDIO_DECODER_IF_H_

extern const AM_IID IID_IAudioDecoder;

class IAudioDecoder: public IInterface
{
  public:
    DECLARE_INTERFACE(IAudioDecoder, IID_IAudioDecoder);
    virtual AM_ERR SetAudioCodecType(AM_UINT type) = 0;
};


#endif /* AUDIO_DECODER_IF_H_ */
