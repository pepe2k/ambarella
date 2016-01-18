/*
 * simple_demuxer.h
 *
 * History:
 *    2008/8/8 - [Oliver Li] created file
 *    2009/12/18 - [Oliver Li] rewrite for new AMF
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __SIMPLE_DEMUXER_H__
#define __SIMPLE_DEMUXER_H__


class CSimpleDemuxer;
class CVideoOutput;

//-----------------------------------------------------------------------
//
// CSimpleDemuxer
//
//-----------------------------------------------------------------------
class CSimpleDemuxer: public CFilter, public IDemuxer
{
	typedef CFilter inherited;
	friend class CActiveOutputPin;

public:
	static int ParseMedia(struct parse_file_s *pParseFile, struct parser_obj_s *pParser);

public:
	static IFilter* Create(IEngine *pEngine);

protected:
	CSimpleDemuxer(IEngine *pEngine):
		inherited(pEngine),
		mpVideoOutputPin(NULL) {}
	AM_ERR Construct();
	virtual ~CSimpleDemuxer();

public:
	// IInterface
	virtual void *GetInterface(AM_REFIID refiid);
	virtual void Delete() { inherited::Delete(); }

	// IFilter
	virtual AM_ERR Run();
	virtual AM_ERR Stop();

	virtual void GetInfo(INFO& info);
	virtual IPin* GetOutputPin(AM_UINT index);

	// IDemuxer
	virtual AM_ERR LoadFile(const char *pFileName, void *pParserObj);
	virtual AM_ERR Seek(AM_U64 &ms);
	virtual AM_ERR GetTotalLength(AM_U64& ms);
    virtual AM_ERR GetFileType(const char *&pFileType);
	virtual void EnableAudio(bool enable) {}
	virtual void EnableVideo(bool enable) {}
    virtual void EnableSubtitle(bool enable) {}

protected:
	CVideoOutput *mpVideoOutputPin;
};

//-----------------------------------------------------------------------
//
// CVideoOutput
//
//-----------------------------------------------------------------------
class CVideoOutput: public CActiveOutputPin
{
	typedef CActiveOutputPin inherited;
	friend class CSimpleDemuxer;

private:
	struct video_frame_t
	{
		AM_U32		size;
		AM_U32		pts;
		AM_U32		flags;
		AM_U32		seq;
	};

public:
	static CVideoOutput* Create(CFilter *pFilter, const char *pName);

protected:
	CVideoOutput(CFilter *pFilter, const char *pName);
	AM_ERR Construct();
	virtual ~CVideoOutput();

public:
	virtual AM_ERR GetMediaFormat(CMediaFormat*& pMediaFormat)
	{
		pMediaFormat = &mMediaFormat;
		return ME_OK;
	}

protected:
	// called when stopped, and received CMD_RUN
	virtual void OnRun();

protected:
	AM_ERR LoadFile(const char *pFileName);
	bool IsEOS();

	video_frame_t *ReadFrameInfo(AM_UINT index, bool bForward);
	void FillGopHeader(CBuffer *pBuffer, video_frame_t *pFrame,
		int skip_first_I, int skip_last_I, am_pts_t pts);
	AM_ERR ReadFrameData(CBuffer *pBuffer, video_frame_t *pFrame);

private:
	CMediaFormat mMediaFormat;
	IFileReader *mpVideoFile;
	IFileReader *mpInfoFile;

	iav_h264_config_t mH264Config;
	AM_UINT mHeaderSize;
	AM_UINT mnTotalFrames;
	AM_UINT mCurrFrame;
	am_file_off_t mVideoFileOffset;

	IPBControl::PB_DIR mDir;
	IPBControl::PB_SPEED mSpeed;

	AM_UINT mCacheStartIndex;
	AM_UINT mCacheEndIndex;
	video_frame_t mFrameCache[256];
};

#endif

