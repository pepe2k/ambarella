/**
 * simple_demuxer.cpp
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

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <basetypes.h>
#include "iav_drv.h"

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_pbif.h"
#include "pbif.h"
#include "engine_guids.h"
#include "filter_list.h"

#include "simple_demuxer.h"


filter_entry g_simple_demuxer = {
	"SimpleDemuxer",
	CSimpleDemuxer::Create,
	CSimpleDemuxer::ParseMedia,
	NULL,
};

int CSimpleDemuxer::ParseMedia(parse_file_s *pParseFile, parser_obj_s *pParser)
{
	return 1;
}

//-----------------------------------------------------------------------
//
// CSimpleDemuxer
//
//-----------------------------------------------------------------------
IFilter* CSimpleDemuxer::Create(IEngine *pEngine)
{
	CSimpleDemuxer *result = new CSimpleDemuxer(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleDemuxer::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpVideoOutputPin = CVideoOutput::Create(this, "VideOutput")) == NULL)
		return ME_NO_MEMORY;

	return ME_OK;
}

CSimpleDemuxer::~CSimpleDemuxer()
{
	AM_DELETE(mpVideoOutputPin);
}

void *CSimpleDemuxer::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IDemuxer)
		return (IDemuxer*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CSimpleDemuxer::Run()
{
	return mpVideoOutputPin->Run();
}

AM_ERR CSimpleDemuxer::Stop()
{
	return mpVideoOutputPin->Stop();
}

void CSimpleDemuxer::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 1;
	info.pName = "SimpleDemuxer";
}

IPin* CSimpleDemuxer::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpVideoOutputPin;
	return NULL;
}

AM_ERR CSimpleDemuxer::LoadFile(const char *pFileName, void *pParserObj)
{
	return mpVideoOutputPin->LoadFile(pFileName);
}


AM_ERR CSimpleDemuxer::Seek(AM_U64 &ms)
{
	return ME_NOT_SUPPORTED;
}

AM_ERR CSimpleDemuxer::GetTotalLength(AM_U64& ms)
{
	return ME_NOT_SUPPORTED;
}

AM_ERR CSimpleDemuxer::GetFileType(const char *&pFileType)
{
    return ME_NOT_SUPPORTED;
}
//-----------------------------------------------------------------------
//
// CVideoOutput
//
//-----------------------------------------------------------------------
CVideoOutput* CVideoOutput::Create(CFilter *pFilter, const char *pName)
{
	CVideoOutput *result = new CVideoOutput(pFilter, pName);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

CVideoOutput::CVideoOutput(CFilter *pFilter, const char *pName):
	inherited(pFilter, pName),
	mpVideoFile(NULL),
	mpInfoFile(NULL),
	mDir(IPBControl::DIR_FORWARD),
	mSpeed(IPBControl::SPEED_NORMAL)
{
	mMediaFormat.pMediaType = &GUID_NULL;
	mMediaFormat.pSubType = &GUID_NULL;
	mMediaFormat.pFormatType = &GUID_NULL;
}

AM_ERR CVideoOutput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpVideoFile = CFileReader::Create()) == NULL) {
		return ME_ERROR;
	}

	if ((mpInfoFile = CFileReader::Create()) == NULL) {
		return ME_ERROR;
	}

	return ME_OK;
}

CVideoOutput::~CVideoOutput()
{
	AM_DELETE(mpVideoFile);
	AM_DELETE(mpInfoFile);
}

#define GOP_NALU_SIZE	22

void CVideoOutput::OnRun()
{
	CmdAck(ME_OK);

	while (1) {
		CBuffer *pBuffer;

		if (IsEOS()) {
			if (!AllocBuffer(pBuffer, 0))
				break;

			pBuffer->SetType(CBuffer::EOS);
			SendBuffer(pBuffer);

			AM_PRINTF("SimpleDemuxer reaches EOS\n");
			break;
		}

		video_frame_t *pFrame;
		if ((pFrame = ReadFrameInfo(mCurrFrame, true)) == NULL)
			break;

		AM_U32 size = pFrame->size;
		if (pFrame->flags == IDR_FRAME)
			size += GOP_NALU_SIZE;

		if (!AllocBuffer(pBuffer, size))
			break;

		pBuffer->SetType(CBuffer::DATA);
		pBuffer->mFrameType = pFrame->flags;

		pBuffer->SetDataSize(0);
		if (pFrame->flags == IDR_FRAME) {
			FillGopHeader(pBuffer, pFrame, 0, 0, 0);
			pBuffer->SetDataSize(GOP_NALU_SIZE);
		}

		if (ReadFrameData(pBuffer, pFrame) != ME_OK) {
			pBuffer->Release();
			break;
		}
		pBuffer->SetDataSize(size);

		pBuffer->mFlags = (pFrame->flags == IDR_FRAME) ? 0 : CBuffer::SYNC_POINT;

		SendBuffer(pBuffer);

		if (mDir == IPBControl::DIR_FORWARD)
			mCurrFrame++;
		else
			mCurrFrame--;
	}
}

AM_ERR CVideoOutput::LoadFile(const char *pFileName)
{
	AM_ERR err;

	err = mpVideoFile->OpenFile(pFileName);
	if (err != ME_OK)
		return err;

	char buffer[260];
	sprintf(buffer, "%s.info", pFileName);

	err = mpInfoFile->OpenFile(buffer);
	if (err != ME_OK)
		return err;

	int version;
	int config_size;

	if (mpInfoFile->ReadFile(0, &version, sizeof(int)) != ME_OK ||
		mpInfoFile->ReadFile(sizeof(int), &config_size, sizeof(int)) != ME_OK) {
		AM_ERROR("bad file\n");
		return ME_ERROR;
	}

	if (version != 3 && version != 4) {
		AM_ERROR("Version is not correct\n");
		return ME_ERROR;
	}

	if (mpInfoFile->ReadFile(sizeof(int) * 2, &mH264Config, config_size) != ME_OK) {
		AM_ERROR("bad file\n");
		return ME_ERROR;
	}

	mHeaderSize = sizeof(int) * 2 + config_size;
	mnTotalFrames = (mpInfoFile->GetFileSize() - mHeaderSize) / sizeof(video_frame_t);
	mCurrFrame = 0;
	mVideoFileOffset = 0;

	mMediaFormat.pMediaType = &GUID_Video;
	mMediaFormat.pSubType = &GUID_AmbaVideoAVC;
	mMediaFormat.pFormatType = &GUID_NULL;

	mCacheStartIndex = 0;
	mCacheEndIndex = 0;

	return ME_OK;
}

bool CVideoOutput::IsEOS()
{
	if (mDir == IPBControl::DIR_FORWARD) {
		return mCurrFrame >= mnTotalFrames;
	}
	else {
		return (int)mCurrFrame < 0;
	}
}

CVideoOutput::video_frame_t *
CVideoOutput::ReadFrameInfo(AM_UINT index, bool bForward)
{
	if (index >= mCacheStartIndex && index < mCacheEndIndex)
		return &mFrameCache[index - mCacheStartIndex];

	AM_UINT startIndex;
	AM_UINT count;

	if (bForward) {
		startIndex = index;
		count = mnTotalFrames - index;
		if (count > ARRAY_SIZE(mFrameCache))
			count = ARRAY_SIZE(mFrameCache);
	}
	else {
		if (index >= ARRAY_SIZE(mFrameCache)) {
			count = ARRAY_SIZE(mFrameCache);
			startIndex = index + 1 - count;
		}
		else {
			count = index + 1;
			startIndex = 0;
		}
	}

	if (mpInfoFile->ReadFile(mHeaderSize + startIndex * sizeof(video_frame_t),
			(AM_U8*)&mFrameCache[0], count * sizeof(video_frame_t)) != ME_OK) {
		AM_ERROR("Read index file failed\n");
		PostEngineMsg(IEngine::MSG_ERROR);
		return NULL;
	}

	mCacheStartIndex = startIndex;
	mCacheEndIndex = startIndex + count;

	return &mFrameCache[index - startIndex];
}

void CVideoOutput::FillGopHeader(CBuffer *pBuffer, video_frame_t *pFrame,
	int skip_first_I, int skip_last_I, am_pts_t pts)
{
	u8 *pBuf = pBuffer->GetDataPtr();
	u32 tick_high = (mH264Config.pic_info.rate >> 16) & 0x0000ffff;
	u32 tick_low = mH264Config.pic_info.rate & 0x0000ffff;
	u32 scale_high = (mH264Config.pic_info.scale >> 16) & 0x0000ffff;
	u32 scale_low = mH264Config.pic_info.scale & 0x0000ffff;
	u32 pts_high;
	u32 pts_low;

	pts_high = (pts >> 16) & 0x00003fff;
	pts_low = pts & 0x0000ffff;

	pBuf[0] = 0;     // start code
	pBuf[1] = 0;
	pBuf[2] = 0;
	pBuf[3] = 1;

	pBuf[4] = 0x7a;      // NAL header
	pBuf[5] = 0x01;      // version main
	pBuf[6] = 0x01;      // version sub

	pBuf[7] = (skip_first_I << 7) | (skip_last_I << 6) | (tick_high >> 10);
	pBuf[8] = tick_high >> 2;
	pBuf[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
	pBuf[10] = tick_low >> 3;

	pBuf[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
	pBuf[12] = scale_high >> 4;
	pBuf[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
	pBuf[14] = scale_low >> 5;

	pBuf[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
	pBuf[16] = pts_high >> 6;

	pBuf[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
	pBuf[18] = pts_low >> 7;
	pBuf[19] = (pts_low << 1) | 1;
	pBuf[20] = mH264Config.N;

	pBuf[21] = (mH264Config.M << 4) & 0xf0;
}

AM_ERR CVideoOutput::ReadFrameData(CBuffer *pBuffer, video_frame_t *pFrame)
{
	AM_U8 *pBuf = pBuffer->GetDataPtr() + pBuffer->GetDataSize();
	AM_ERR err = mpVideoFile->ReadFile(mVideoFileOffset, pBuf, pFrame->size);
	if (err != ME_OK) {
		AM_ERROR("Read video file failed\n");
		PostEngineMsg(IEngine::MSG_ERROR);
		return err;
	}

	mVideoFileOffset += pFrame->size;
	return ME_OK;
}
