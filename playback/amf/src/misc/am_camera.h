
/**
 * am_camera.h
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

//-----------------------------------------------------------------------
//
// CCamera
//
//-----------------------------------------------------------------------
class CCamera: public ICamera
{
public:
	static ICamera* Create();

private:
	CCamera(): mFd(-1) {}
	AM_ERR Construct();
	virtual ~CCamera();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete();

	// ICamera
	virtual int GetDevFd();
	virtual AM_ERR StartPreview();
	virtual void StopPreview();

	virtual AM_ERR StartRecording();
	virtual void StopRecording();

private:
	int mFd;
};

