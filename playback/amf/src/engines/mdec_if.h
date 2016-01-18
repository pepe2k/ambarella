
/**
 * mdec_if.h
 *
 * History:
 *    2011/12/08 - [GangLiu] created file
 *    2012/3/30- [Qingxiong Z] modify file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MDEC_IF_H__
#define __MDEC_IF_H__

extern const AM_IID IID_IMDECEngine;
//-----------------------------------------------------------------------
//
// IMDECEngine
//
//-----------------------------------------------------------------------
class IMDECEngine: virtual public IEngine
{
	typedef IEngine inherited;
public:

    //Multidec defined cmd/msg
    enum {
        MD_FIRST_CMD = IActiveObject::CMD_LAST,
        MDCMD_EDIT_SOURCE_DONE,
        MDCMD_EDIT_SOURCE,
        MDCMD_BACK_SOURCE,
        MDCMD_DELETE_SOURCE,
        MDCMD_ADD_SOURCE,
        MDCMD_SELECT_AUDIO,
        MDCMD_AUDIO_TALKBACK,
        MDCMD_CONFIG_WIN,
        MDCMD_CONFIG_WINMAP,
        MDCMD_PERFORM_WIN,
        MDCMD_AUTOSWITCH,
        MDCMD_SWITCHSET,

        MDCMD_PREPARE,
        MDCMD_START,
        MDCMD_STOP,
        MDCMD_DISCONNECT,
        MDCMD_RECONNECT,
        MDCMD_PAUSEPLAY,
        MDCMD_RESUMEPLAY,
        MDCMD_SEEKTO,
        MDCMD_STEPPLAY,
        MDCMD_SPEED_FEEDING_RULE,

        MDCMD_RELAYOUT,
        MDCMD_EXCHANGE_WINDOW,

        MDCMD_CONFIG_WINDOW,

        MD_LAST_CMD,
    };

    enum {
        MSG_MD_ADD_MEDIA_DONE = inherited::MSG_LAST,

    };

public:
	DECLARE_INTERFACE(IMDECEngine, IID_IMDECEngine);
};

class CGeneralDemuxer;
class CGeneralDecoder;
class CGeneralRenderer;
class CGeneralTranscoder;
class CGeneralTransAudioSink;
class CGConfig;
extern CGeneralDemuxer* CreateGeneralDemuxerFilter(IEngine* pEngine, CGConfig* config);
extern CGeneralDecoder* CreateGeneralDecoderFilter(IEngine* pEngine, CGConfig* config);
extern CGeneralRenderer* CreateGeneralRendererFilter(IEngine* pEngine, CGConfig* config);
extern CGeneralTranscoder* CreateGeneralTranscoderFilter(IEngine* pEngine, CGConfig* config);
extern CGeneralTransAudioSink* CreateGeneralTransAudioSinkFilter(IEngine* pEngine, CGConfig* config);

#endif

