
/**
 * pbif.h
 *
 * History:
 *    2009/12/7 - [Oliver Li] created file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __PBIF_H__
#define __PBIF_H__


extern const AM_IID IID_IPBEngine;

//-----------------------------------------------------------------------
//
// IPBEngine
//
//-----------------------------------------------------------------------
class IPBEngine: virtual public IEngine
{
	typedef IEngine inherited;

public:
	enum {
		MSG_PARSE_MEDIA_DONE = inherited::MSG_LAST,

	};

public:
	DECLARE_INTERFACE(IPBEngine, IID_IPBEngine);
};

//-----------------------------------------------------------------------
//
// shared data structures
//
//-----------------------------------------------------------------------

struct CFFMpegStreamFormat: public CMediaFormat
{

};

struct CFFMpegMediaFormat: public CMediaFormat
{
	AM_UINT		picWidth;
	AM_UINT		picHeight;
	AM_UINT		bufWidth;
	AM_UINT		bufHeight;
	AM_UINT		picXoff;
	AM_UINT		picYoff;
	AM_UINT		picWidthWithEdge;
	AM_UINT		picHeightWithEdge;

	AM_UINT		auSamplerate;
	AM_UINT		auChannels;
	AM_INT		auSampleFormat;
	AM_U8		isChannelInterleave;
	AM_U8		reserved1;
	AM_U8		reserved2;
	AM_U8		reserved3;
};

struct CAudioMediaFormat: public CMediaFormat
{
    AM_UINT         auSamplerate;
    AM_UINT         auChannels;
    AM_INT           auSampleFormat;
    AM_U8            isChannelInterleave;
    AM_U8            isSampleInterleave;
    AM_U8             reserved2;
    AM_U8             reserved3;
};

// default is enabout both
#define	ENABLE_VIDEO		1
#define	ENABLE_AUDIO		1
#define	ENABLE_SUBTITLE		1
#endif

