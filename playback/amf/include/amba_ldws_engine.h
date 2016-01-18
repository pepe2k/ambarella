/**
 * amba_ldws_engine.h
 *
 * History:
 *    2012/12/19- [Sky Chen] created file
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AMBA_LDWS_ENGINE_H_
#define _AMBA_LDWS_ENGINE_H_

typedef struct
{
     bool b_useROI;
     ivLDWSPoint s_roiPoint[4];//p1 p2 p3 p4
     AM_INT i_horizon;
     AM_INT i_hood;
     AM_INT i_detectionSensitivity;
     AM_INT i_departureSensitivity;
}Sldws_engine_config;

class AmbaLdwsEngine
{
       typedef AM_ERR (*funcptr)(eLDWSOutputEvent);
public:
	AmbaLdwsEngine(void* callback = NULL);
	~AmbaLdwsEngine();
	AM_ERR InitLDWS(AM_INT width, AM_INT height, AM_INT pitch, eLDWSColorSpace type);
	AM_ERR ConfigEngine(Sldws_engine_config* ldwsconfig);
	AM_ERR GetEngineConfig(Sldws_engine_config* ldwsconfig);
	AM_ERR GetLane(bool* leftlanetracked, bool* rightlanetracked, ivLDWSPoint* leftPoints, ivLDWSPoint* rightPoints);

	AM_ERR StartEngine(void* captureYUV_handler);
	AM_ERR StopEngine();

	AM_ERR ProcessFrame(SYUVData yuv, eLDWSOutputEvent* event);

public:
	eLDWSOutputEvent mEvent;
	funcptr CallbackFunc;
	bool mbRun;
	IRecordControl2* mpRecordControl;

private:
	int mImageWidth;
	int mImageHeight;
	int mYUVType;
	void* mpIVEngine;
	CThread* mpProcessThread;
	bool mbUseROI;
	ivLDWSEngineParams mSensitivityParams;

	char* mpYData;
	char* mpUVData;
	ivLDWSVideoData mYUVData;
};
#endif
