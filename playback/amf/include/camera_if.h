
/**
 * camera_if.h
 *
 * History:
 *	2010/1/6 - [Oliver Li] rewrite
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

extern const AM_IID IID_ICamera;

//-----------------------------------------------------------------------
//
// ICamera
//
//-----------------------------------------------------------------------
class ICamera: public IInterface
{
public:
	DECLARE_INTERFACE(ICamera, IID_ICamera);


	virtual int GetDevFd() = 0;
	virtual AM_ERR StartPreview() = 0;
	virtual void StopPreview() = 0;

	virtual AM_ERR StartRecording() = 0;
	virtual void StopRecording() = 0;
};


extern ICamera* CreateAmbaCamera();

