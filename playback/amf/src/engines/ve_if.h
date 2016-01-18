
/**
 * ve_if.h
 *
 * History:
 *    2011/08/08 - [GangLiu] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VE_IF_H__
#define __VE_IF_H__

extern const AM_IID IID_IVEEngine;
//extern const AM_IID IID_IAudioSink;
extern const AM_IID IID_IVideoEffecterControl;
extern const AM_IID IID_IVideoTranscoder;
extern const AM_IID IID_IVideoMemEncoder;
//-----------------------------------------------------------------------
//
// IVEEngine
//
//-----------------------------------------------------------------------
class IVEEngine: virtual public IEngine
{
	typedef IEngine inherited;

public:
    enum {
        MSG_VE_ADD_MEDIA_DONE = inherited::MSG_LAST,
        MSG_VE_DEL_MEDIA_DONE,
        MSG_VE_RELOAD_FILE_DONE, //change file in a filter branch when seeking or preview need to change file
        MSG_VE_SAVE_PROJECT_DONE

    };

public:
	DECLARE_INTERFACE(IVEEngine, IID_IVEEngine);
};

//-----------------------------------------------------------------------
//
// IVideoEffecterControl
//
//-----------------------------------------------------------------------
class IVideoEffecterControl: public IInterface
{
public:
    DECLARE_INTERFACE(IVideoEffecterControl, IID_IVideoEffecterControl);
    //virtual AM_ERR AddInputPin(AM_UINT& index, AM_UINT type) = 0;
};

//-----------------------------------------------------------------------
//
// IVideoTranscoder
//
//-----------------------------------------------------------------------
class IVideoTranscoder: public IInterface
{
public:
	DECLARE_INTERFACE(IVideoTranscoder, IID_IVideoTranscoder);
	virtual AM_ERR StopEncoding(AM_INT stopcode) = 0;
	virtual void setTranscFrms(AM_INT frames) = 0;
	virtual AM_ERR SetEncParamaters(AM_UINT en_w, AM_UINT en_h, AM_UINT fpsBase, AM_UINT bitrate, bool hasBfrm) = 0;
	virtual AM_ERR SetTranscSettings(bool RenderHDMI, bool RenderLCD, bool PreviewEnc, bool ctrlfps) = 0;
};

//-----------------------------------------------------------------------
//
// IVideoMemEncoder
//
//-----------------------------------------------------------------------
class IVideoMemEncoder: public IInterface
{
public:
	typedef enum {
		SA_NONE,
		SA_STOP,
	} STOP_ACTION;

public:
	DECLARE_INTERFACE(IVideoMemEncoder, IID_IVideoMemEncoder);
	//virtual AM_ERR StopEncoding() = 0;
};

extern IFilter* CreateGeneralVideoDecodeFilter(IEngine *pEngine);
extern IFilter* CreateVideoRenderFilter(IEngine *pEngine);
extern IFilter* CreateVideoEffecterPreviewFilter(IEngine *pEngine);
extern IFilter* CreateVideoEffecterFinalizeFilter(IEngine *pEngine);
extern IFilter* CreateVideoTranscoderFilter(IEngine *pEngine);
extern IFilter* CreateVideoMemEncoderFilter(IEngine *pEngine);

#endif

