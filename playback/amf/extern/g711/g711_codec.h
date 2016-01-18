/*
 * g711_codec.h
 *
 * History:
 *    2013/11/20 - [SkyChen] create file
 *
 * Copyright (C) 2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _G711_CODEC_H_
#define _G711_CODEC_H_
//-----------------------------------------------------------------------
//
// CG711Codec
//
//-----------------------------------------------------------------------
class CG711Codec {

public:
    static CG711Codec* CreateG711Codec();
private:
    CG711Codec();
    //~CG711Codec();

public:
    void ConfigCodec();
    AM_INT Encoder(AM_U8* input, AM_U8* output, AM_UINT length);
private:
    AM_U8 alawEncode(AM_INT pcm);
    AM_INT alawDecode(AM_U8 alaw);

    AM_U8 linear2alaw( AM_INT pcm_val);
    AM_INT alaw2linear( AM_U8 a_val);
    AM_U8 linear2ulaw(AM_INT pcm_val);
    AM_INT ulaw2linear(AM_U8 u_val);

    AM_U8 alaw2ulaw( AM_U8 aval);
    AM_U8 ulaw2alaw( AM_U8 uval);
};
#endif
