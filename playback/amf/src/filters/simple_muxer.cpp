
/**
 * simple_muxer.cpp
 *
 * History:
 *    2010/1/8 - [Oliver Li] create file
 *
 * Copyright (C) 2007-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "am_record_if.h"
#include "record_if.h"
#include "engine_guids.h"
#include "simple_muxer.h"

extern IFilter* CreateSimpleMuxer(IEngine *pEngine)
{
	return CSimpleMuxer::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CSimpleMuxer
//
//-----------------------------------------------------------------------
IFilter* CSimpleMuxer::Create(IEngine *pEngine)
{
	CSimpleMuxer *result = new CSimpleMuxer(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleMuxer::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	mpInputPin = CSimpleMuxerInput::Create(this);
	if (mpInputPin == NULL)
		return ME_ERROR;

	mpVideoFile = CFileWriter::Create();
	if (mpVideoFile == NULL)
		return ME_ERROR;

	mpInfoFile = CFileWriter::Create();
	if (mpInfoFile == NULL)
		return ME_ERROR;

	return ME_OK;
}

CSimpleMuxer::~CSimpleMuxer()
{
	AM_DELETE(mpInfoFile);
	AM_DELETE(mpVideoFile);
	AM_DELETE(mpInputPin);
}

void *CSimpleMuxer::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_IMuxer)
		return (IMuxer*)this;
	return inherited::GetInterface(refiid);
}

AM_ERR CSimpleMuxer::Run()
{
	if (!mbFileCreated) {
		AM_ERROR("Call SetOutputFile() first\n");
		return ME_BAD_STATE;
	}

	mpInputPin->mbEOS = false;
	return ME_OK;
}

AM_ERR CSimpleMuxer::Stop()
{
	return ME_OK;
}

void CSimpleMuxer::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 0;
	info.pName = "SimpleMuxer";
}

IPin* CSimpleMuxer::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInputPin;
	return NULL;
}

AM_ERR CSimpleMuxer::SetOutputFile(const char *pFileName)
{
	CloseFiles();

	char filename[260];
	AM_ERR err;

	sprintf(filename, "%s.264", pFileName);
	err = mpVideoFile->CreateFile(filename);
	if (err != ME_OK)
		return err;
	AM_INFO("file created: %s\n", filename);

	sprintf(filename, "%s.264.info", pFileName);
	err = mpInfoFile->CreateFile(filename);
	if (err != ME_OK)
		return err;
	AM_INFO("file created: %s\n", filename);

	mVideoFileSize = 0;
	mInfoFileSize = 0;
	mbError = false;
	mbFileCreated = true;

	return ME_OK;
}

//chuchen,2012_5_18,
AM_ERR CSimpleMuxer::SetThumbNailFile(const char *pThumbNailFileName)
{
      return ME_OK;
}

void CSimpleMuxer::CloseFiles()
{
	mpInfoFile->CloseFile();
	mpVideoFile->CloseFile();
	mbFileCreated = false;
}

void CSimpleMuxer::ErrorStop()
{
	mbError = true;
	PostEngineMsg(IEngine::MSG_ERROR);
}

void CSimpleMuxer::OnEOS()
{
	if (mpInputPin->mbEOS) {
		CloseFiles();
		PostEngineMsg(IEngine::MSG_EOS);
		AM_INFO("CSimpleMuxer::OnEOS\n");
	}
}

void CSimpleMuxer::OnVideoInfo(CBuffer *pBuffer)
{
	// version, size, config
	int version = 0x00000004;
	int size = pBuffer->GetDataSize();
	if (mpInfoFile->WriteFile(&version, sizeof(version)) != ME_OK ||
		mpInfoFile->WriteFile(&size, sizeof(size)) != ME_OK ||
		mpInfoFile->WriteFile(pBuffer->GetDataPtr(), size) != ME_OK)
		return ErrorStop();
	AM_INFO("Write header info\n");
}

void CSimpleMuxer::OnVideoBuffer(CBuffer *pBuffer)
{
	if (mbError || !mbFileCreated)
		return;

	if (pBuffer->mSeqNum % 15 == 0)
		AM_INFO("seq = %d, total: %lld\n", pBuffer->mSeqNum, mVideoFileSize);

	AM_ERR err = mpVideoFile->WriteFile(
		pBuffer->GetDataPtr(), pBuffer->GetDataSize());
	if (err != ME_OK)
		return ErrorStop();

	mVideoFileSize += pBuffer->GetDataSize();

	video_frame_t frame;
	frame.size = pBuffer->GetDataSize();
	frame.pts = (AM_U32)pBuffer->GetPTS();
	frame.flags = pBuffer->mFrameType;
	frame.seq = pBuffer->mFrameType;

	if (mpInfoFile->WriteFile(&frame, sizeof(frame)) != ME_OK)
		return ErrorStop();
}

//-----------------------------------------------------------------------
//
// CSimpleMuxerInput
//
//-----------------------------------------------------------------------
CSimpleMuxerInput* CSimpleMuxerInput::Create(CFilter *pFilter)
{
	CSimpleMuxerInput* result = new CSimpleMuxerInput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleMuxerInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CSimpleMuxerInput::~CSimpleMuxerInput()
{
}

void CSimpleMuxerInput::Receive(CBuffer *pBuffer)
{
	switch (pBuffer->GetType()) {
	case CBuffer::EOS:
		mbEOS = true;
		((CSimpleMuxer*)mpFilter)->OnEOS();
		break;

	case CBuffer::DATA:
		((CSimpleMuxer*)mpFilter)->OnVideoBuffer(pBuffer);
		break;

	case CBuffer::INFO:
		((CSimpleMuxer*)mpFilter)->OnVideoInfo(pBuffer);
		break;

	default:
		break;
	}

	pBuffer->Release();
}

AM_ERR CSimpleMuxerInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ME_OK;
}

