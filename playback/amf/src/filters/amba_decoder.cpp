
/*
 * amba_decoder.h
 *
 * History:
 *    2009/12/18 - [Oliver Li] create file
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "pbif.h"
#include "engine_guids.h"

#include <basetypes.h>
#include "iav_drv.h"
#include "amba_decoder.h"

#include "filter_list.h"

filter_entry g_amba_vdec = {
	"AmbaVideoDecoder",
	CAmbaVideoDecoder::Create,
	NULL,
	CAmbaVideoDecoder::AcceptMedia,
};

//-----------------------------------------------------------------------
//
// CAmbaBSB
//
//-----------------------------------------------------------------------
CAmbaBSB* CAmbaBSB::Create(int iavFd)
{
	CAmbaBSB* result = new CAmbaBSB(iavFd);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaBSB::Construct()
{
	AM_ERR err = inherited::Construct(32, sizeof(CBuffer));
	if (err != ME_OK)
		return err;

	if ((mpMutex = CMutex::Create(false)) == NULL)
		return ME_NO_MEMORY;

	iav_mmap_info_t info;
	if (::ioctl(mIavFd, IAV_IOC_MAP_DECODE_BSB2, &info) < 0) {
		AM_PERROR("IAV_IOC_MAP_DECODE_BSB2");
		return ME_IO_ERROR;
	}

	mpStartAddr = info.addr;
	mpEndAddr = info.addr + info.length / 2;
	mpCurrAddr = info.addr;
	mSpace = info.length / 2;

	AM_INFO("BSB created, start = 0x%p, length = 0x%x\n", info.addr, info.length);

	return ME_OK;
}

CAmbaBSB::~CAmbaBSB()
{
	if (::ioctl(mIavFd, IAV_IOC_UNMAP_DECODE_BSB, 0) < 0) {
		AM_PERROR("IAV_IOC_UNMAP_DECODE_BSB");
	}
	AM_DELETE(mpMutex);
}

#define EOS_NALU_SIZE       5
static AM_U8 eos_nalu[EOS_NALU_SIZE] = {0, 0, 0, 1, 0x0a};

bool CAmbaBSB::AllocBuffer(CBuffer*& pBuffer, AM_UINT size)
{
	if (!inherited::AllocBuffer(pBuffer, size))
		return false;

	pBuffer->SetDataSize(0);

	if (size == 0)
		size = EOS_NALU_SIZE;

	AM_ERR err = AllocBSB(size);
	if (err != ME_OK) {
		pBuffer->Release();
		return false;
	}

	pBuffer->mpData = mpCurrAddr;
	pBuffer->mBlockSize = size;

	mpCurrAddr += size;
	if (mpCurrAddr >= mpEndAddr) {
		mpCurrAddr = mpStartAddr + (mpCurrAddr - mpEndAddr);
		AM_INFO("bsb wrap\n");
	}

	return true;
}

AM_ERR CAmbaBSB::AllocBSB(AM_UINT size)
{
	__LOCK(mpMutex);

	while (true) {
		if (size <= mSpace) {
			mSpace -= size;
			__UNLOCK(mpMutex);
			return ME_OK;
		}

		iav_wait_decoder_t wait;
		wait.emptiness.room = size + EOS_NALU_SIZE;
		wait.emptiness.start_addr = mpCurrAddr;
		wait.flags = IAV_WAIT_BSB;

		__UNLOCK(mpMutex);
		if (::ioctl(mIavFd, IAV_IOC_WAIT_DECODER, &wait) < 0) {
			AM_PERROR("IAV_IOC_WAIT_DECODER");
			return ME_ERROR;
		}
		__LOCK(mpMutex);

		if ((wait.flags | IAV_WAIT_BSB) == 0) {
			// decoder is stopped
			return ME_ERROR;
		}

		mSpace = wait.emptiness.room;
	}
}

//-----------------------------------------------------------------------
//
// CAmbaVideoDecoder
//
//-----------------------------------------------------------------------
IFilter* CAmbaVideoDecoder::Create(IEngine *pEngine)
{
	CAmbaVideoDecoder *result = new CAmbaVideoDecoder(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

int CAmbaVideoDecoder::AcceptMedia(CMediaFormat& format)
{
	if (*format.pMediaType == GUID_Video && *format.pSubType == GUID_AmbaVideoAVC)
		return 1;
	return 0;
}

AM_ERR CAmbaVideoDecoder::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mIavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
		AM_PERROR("/dev/iav");
		return ME_ERROR;
	}

	if ((mpVideoInputPin = CAmbaVideoInput::Create(this, mIavFd)) == NULL)
		return ME_ERROR;

	if ((mpBSB = CAmbaBSB::Create(mIavFd)) == NULL)
		return ME_ERROR;

	mpVideoInputPin->SetBufferPool(mpBSB);

	return ME_OK;
}

CAmbaVideoDecoder::~CAmbaVideoDecoder()
{
	AM_DELETE(mpVideoInputPin);
	AM_DELETE(mpBSB);
	if (mIavFd >= 0)
		::close(mIavFd);
}

AM_ERR CAmbaVideoDecoder::Run()
{
	if (::ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
		AM_PERROR("IAV_IOC_UNMAP_DECODE_BSB");
		return ME_BAD_STATE;
	}

	mpVideoInputPin->mbConfigDecoder = true;

	AM_ERR err = mpVideoInputPin->Run();
	if (err != ME_OK)
		return err;

	return ME_OK;
}

AM_ERR CAmbaVideoDecoder::Stop()
{
	if (::ioctl(mIavFd, IAV_IOC_STOP_DECODE, 0) < 0) {
		AM_PERROR("IAV_IOC_STOP_DECODE");
		//return ME_BAD_STATE;
	}

	AM_ERR err = mpVideoInputPin->Stop();
	AM_ASSERT_OK(err);

	mpBSB->Reset();

	return ME_OK;
}

void CAmbaVideoDecoder::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 0;
	info.pName = "AmbaVDec";
}

IPin* CAmbaVideoDecoder::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpVideoInputPin;
	return NULL;
}

//-----------------------------------------------------------------------
//
// CAmbaVideoInput
//
//-----------------------------------------------------------------------
CAmbaVideoInput* CAmbaVideoInput::Create(CFilter *pFilter, int iavFd)
{
	CAmbaVideoInput *result = new CAmbaVideoInput(pFilter, iavFd);
	if (result && result->Construct()) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CAmbaVideoInput::~CAmbaVideoInput()
{
}

AM_ERR CAmbaVideoInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	// todo
	return ME_OK;
}

AM_ERR CAmbaVideoInput::ProcessBuffer(CBuffer *pBuffer)
{
	AM_ERR err;

	if (pBuffer->IsEOS()) {
		FillEOS(pBuffer);

		err = DecodeBuffer(pBuffer, 0);
		if (err != ME_OK)
			return err;

		err = WaitEOS(pBuffer->mPTS);
		if (err != ME_OK)
			return ME_OK;

		PostEngineMsg(IEngine::MSG_EOS);
		return ME_CLOSED;	// EOS
	}
	else {
		err = DecodeBuffer(pBuffer, 1);
		if (err != ME_OK)
			return err;
		return ME_OK;
	}
}

/*
void CAmbaVideoInput::OnRun()
{
	if (::ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
		AM_PERROR("IAV_IOC_START_DECODE");
		CmdAck(ME_ERROR);
		return;
	}

	inherited::OnRun();

	
}
*/

AM_ERR CAmbaVideoInput::DecodeBuffer(CBuffer *pBuffer, int numPics)
{
//	static int count;
//	printf("%d decode buffer, start = 0x%x, end = 0x%x\n", 
//		count++, pBuffer->GetDataPtr(), pBuffer->GetDataPtr() + pBuffer->GetDataSize());

	if (mbConfigDecoder) {
		iav_config_decoder_t config;

		config.flags = 0;
		config.decoder_type = IAV_DEFAULT_DECODER;
		config.pic_width = 720;		//
		config.pic_height = 480;	//
		if (::ioctl(mIavFd, IAV_IOC_CONFIG_DECODER, &config) < 0) {
			AM_PERROR("IAV_IOC_CONFIG_DECODER");
			return ME_ERROR;
		}

		mbConfigDecoder = false;
	}

	iav_h264_decode_t decode_info;

	decode_info.start_addr = pBuffer->GetDataPtr();
	decode_info.end_addr = pBuffer->GetDataPtr() + pBuffer->GetDataSize();
	decode_info.first_display_pts = 0;
	decode_info.num_pics = numPics;
	decode_info.next_size = 0;
	decode_info.pts = 0;
	decode_info.pts_high = 0;
//	decode_info.pic_width = 0;
//	decode_info.pic_height = 0;

	if (::ioctl(mIavFd, IAV_IOC_DECODE_H264, &decode_info) < 0) {
		AM_PERROR("IAV_IOC_DECODE_H264");
		pBuffer->Release();
		return ME_ERROR;
	}

	pBuffer->Release();
	return ME_OK;
}

void CAmbaVideoInput::FillEOS(CBuffer *pBuffer)
{
}

AM_ERR CAmbaVideoInput::WaitEOS(am_pts_t pts)
{
	// todo
	return ME_OK;
}


