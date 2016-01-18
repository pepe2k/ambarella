
/**
 * am_iav.h
 *
 * History:
 *    2010/6/10 - [Wangf] created file
 *
 * Open IAV, close, config
 * (Camera device)    
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_IAV__
#define __AM_IAV__

#include <camera/CameraParameters.h>
#include <basetypes.h>
#include "iav_drv.h"

using namespace android;
//-----------------------------------------------------------------------
//
// AM_IAV
//
//-----------------------------------------------------------------------
class AM_IAV
{
public:
	static AM_IAV *getInstance();
	
	virtual ~AM_IAV();
	
protected:
	AM_IAV();

	AM_ERR Init();
	//AM_ERR  InitVout(int vout_id);
public:
	virtual void Close();
	virtual int GetDevFd();
	//below to follow android orginial API
	virtual AM_ERR SetVideoEncoder(AM_UINT enc_type) 	;
	virtual AM_ERR SetVideoSize(AM_UINT width, AM_UINT height) ;
	virtual AM_ERR SetVideoFrameRate(AM_UINT framerate) ;
	virtual AM_ERR SetPreviewPosition(AM_UINT vout_id, AM_UINT x, AM_UINT y, AM_UINT width, AM_UINT height);
	virtual AM_ERR StartPreview();
	virtual void StopPreview();

	virtual bool PreviewEnabled();
	virtual bool RecordingEnabled();

	virtual void SetParameters(CameraParameters &params);

	//Ambarella IPCam special API
	virtual AM_ERR SetVideoQuality(AM_UINT quality) ;
	virtual AM_ERR TakeEffectAll();
	virtual AM_ERR StartAAA();
	virtual AM_ERR StopAAA();
	//Ambarella DV special API
	//TBD

private:
	virtual int GetState();
	virtual void PrintH264Config(iav_h264_config_t *config);
	virtual AM_ERR TakeEffectVideoSize();
	virtual AM_ERR TakeEffectFrameRate();
	virtual AM_ERR TakeEffectVideoQuality();

	int mFd;
	int mMainEncType ;
	int mSecdEncType ;
	int mThidEncType;
	int mMainWidth;
	int mMainHeight;
	int mMainSizeFlag;
	int mMainFramerate;
	int mMainFramerateFlag;
	int mMainVideoQuality;
	int mMainVideoQualityFlag;
	int mVinResolutionChangedFlag;
	bool mAAAEnabled;
};
#endif
