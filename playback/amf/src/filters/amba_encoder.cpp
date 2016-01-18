
/*
 * amba_encoder.h
 *
 * History:
 *    20010/1/6 - [Oliver Li] create file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_TAG "Amba_encoder"

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
#include "record_if.h"
#include "engine_guids.h"

#include <basetypes.h>
#include "iav_drv.h"
#include "amba_encoder.h"


extern IFilter* CreateAmbaVideoEncoderFilter(IEngine *pEngine, int fd)
{
	return CAmbaVideoEncoder::Create(pEngine, fd);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoEncoder
//
//-----------------------------------------------------------------------
IFilter* CAmbaVideoEncoder::Create(IEngine *pEngine, int fd)
{
	CAmbaVideoEncoder *result = new CAmbaVideoEncoder(pEngine, fd);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoEncoder::Construct()
{
	mpOutput = NULL;
	mpOutput1 = NULL;
	mpBSB = NULL;
	mbDual = false;
	mbMemMapped = false;
	
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	mpBSB = CSimpleBufferPool::Create("EncBSB", 32);
	if (mpBSB == NULL)
		return ME_NO_MEMORY;

	mpOutput = CAmbaVideoOutput::Create(this);
	if (mpOutput == NULL)
		return ME_NO_MEMORY;

	mpOutput->SetBufferPool(mpBSB);

	mbDual = false;

	if(mbDual) {
		mpOutput1 = CAmbaVideoOutput::Create(this);
		if (mpOutput1 == NULL)
			return ME_NO_MEMORY;

		mpOutput1->SetBufferPool(mpBSB);
	}
	iav_mmap_info_t mmap;
	if (::ioctl(mFd, IAV_IOC_MAP_BSB2, &mmap) < 0) {
		AM_PERROR("IAV_IOC_MAP_BSB2");
		return ME_BAD_STATE;
	}

	AM_INFO("mem_base: 0x%p, size = 0x%x\n", mmap.addr, mmap.length);
	mbMemMapped = true;

	return ME_OK;
}

CAmbaVideoEncoder::~CAmbaVideoEncoder()
{
	if (mbMemMapped)
		::ioctl(mFd, IAV_IOC_UNMAP_BSB, 0);

	AM_DELETE(mpOutput);
	AM_DELETE(mpOutput1);
	AM_RELEASE(mpBSB);
}

void CAmbaVideoEncoder::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 0;
	info.pName = "AmbaVEnc";
}

IPin* CAmbaVideoEncoder::GetOutputPin(AM_UINT index)
{
	if (index == 0)
		return mpOutput;
	else if (index == 1)
		return mpOutput1;
	return NULL;
}

void CAmbaVideoEncoder::DoStop(STOP_ACTION action)
{
	if (mbEncoding) {
		mbEncoding = false;
		mAction = action;
		::ioctl(mFd, IAV_IOC_STOP_ENCODE, 0);
	}
}

void CAmbaVideoEncoder::PrintH264Config(iav_h264_config_t *config)
{
	if (config==NULL) return;
	AM_INFO("\t     profile = %s\n", (config->entropy_codec == IAV_ENTROPY_CABAC)? "main":"baseline");
	AM_INFO("\t           M = %d\n", config->M);
	AM_INFO("\t           N = %d\n", config->N);
	AM_INFO("\tidr interval = %d\n", config->idr_interval);
	AM_INFO("\t   gop model = %s\n", (config->gop_model == 0)? "simple":"advanced");
	AM_INFO("\t     bitrate = %d bps\n", config->average_bitrate);
	AM_INFO("\tbitrate ctrl = %s\n", (config->bitrate_control == IAV_BRC_CBR)? "cbr":"vbr");
	if (config->bitrate_control == IAV_BRC_VBR) {
		AM_INFO("\tmin_vbr_rate = %d\n", config->min_vbr_rate_factor);
		AM_INFO("\tmax_vbr_rate = %d\n", config->max_vbr_rate_factor);
	}
	AM_INFO("\t    de-intlc = %s\n", (config->deintlc_for_intlc_vin==0)? "off":"on");
	AM_INFO("\t        ar_x = %d\n", config->pic_info.ar_x);
	AM_INFO("\t        ar_y = %d\n", config->pic_info.ar_y);
	AM_INFO("\t  frame mode = %d\n", config->pic_info.frame_mode);
	AM_INFO("\t        rate = %d\n", config->pic_info.rate);
	AM_INFO("\t       scale = %d\n", config->pic_info.scale);
}

void CAmbaVideoEncoder::OnRun()
{
	// setup encoding format have been moved to Am_Camera.cpp
	int size = sizeof(mH264Config);

	mH264Config.stream = 0;
	if (::ioctl(mFd, IAV_IOC_GET_H264_CONFIG, &mH264Config) < 0) {
		AM_INFO("IAV_IOC_GET_H264_CONFIG");
		return CmdAck(ME_ERROR);
	}

	PrintH264Config(&mH264Config);

       iav_encode_format_t format;

       if (::ioctl(mFd, IAV_IOC_GET_ENCODE_FORMAT, &format) < 0) {
               AM_INFO("IAV_IOC_GET_ENCODE_FORMAT");
               return ;
       }

       AM_INFO(" StopPreview format.main_encode_type  %d, main_width %d, main_height  %d",
               format.encode_type, format.encode_width, format.encode_height );

       format.encode_type = IAV_ENCODE_H264; // for bug 1180

       if (::ioctl(mFd, IAV_IOC_SET_ENCODE_FORMAT, &format) < 0) {
               AM_INFO("IAV_IOC_SET_ENCODE_FORMAT");
               return ;
       }
	// send to downstream
	CBuffer *pBuffer;
	if (!mpOutput->AllocBuffer(pBuffer))
		return CmdAck(ME_ERROR);
	pBuffer->SetType(CBuffer::INFO);
	pBuffer->SetDataPtr((AM_U8*)&mH264Config);
	pBuffer->SetDataSize(sizeof(mH264Config));
	mpOutput->SendBuffer(pBuffer);

	iav_state_info_t info;
	if (::ioctl(mFd, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return CmdAck(ME_ERROR);
	}

	AM_INFO("IAV_IOC_START_ENCODE state = %d  %d  %d\n", info.state, info.dsp_encode_state, info.dsp_encode_mode);
	// start encoding
	if (::ioctl(mFd, IAV_IOC_START_ENCODE, 0) < 0) {
		AM_PERROR("IAV_IOC_START_ENCODE");
		return CmdAck(ME_ERROR);
	}

	// ack OK
	mpOutput->ResetForEncoding();
	if(mbDual)
		mpOutput1->ResetForEncoding();
	mbEncoding = true;
	CmdAck(ME_OK);

	// read bitstream
	while (true) {
		bs_fifo_info_t fifo_info;

		if (::ioctl(mFd, IAV_IOC_READ_BITSTREAM, &fifo_info) < 0) {
			if (!mbEncoding) {
				if (mAction == SA_STOP) {
					CBuffer *pBuffer;
					if (!mpOutput->AllocBuffer(pBuffer))
						return;
					pBuffer->SetType(CBuffer::EOS);
					//AM_INFO("CAmbaVideoEncoder send EOS\n");
					mpOutput->SendBuffer(pBuffer);
					if(mbDual) {
						CBuffer *pBuffer1;
						if (!mpOutput1->AllocBuffer(pBuffer1))
							return;
						pBuffer1->SetType(CBuffer::EOS);
						mpOutput1->SendBuffer(pBuffer1);
					}
				}
				return;
			}
		}

		bits_info_t *desc = &fifo_info.desc[0];
		for (AM_UINT i = 0; i < fifo_info.count; i++, desc++) {
			CBuffer *pBuffer;

			if (!mpOutput->AllocBuffer(pBuffer))
				return;

			pBuffer->SetType(CBuffer::DATA);
			pBuffer->mFlags = 0;
			pBuffer->mPTS = mpOutput->FixPTS(desc->PTS);
			pBuffer->mBlockSize = 
			pBuffer->mDataSize = (desc->pic_size + 31) & ~31;
			pBuffer->mpData = (AM_U8*)desc->start_addr;
			pBuffer->mSeqNum = mpOutput->mnFrames++;
			pBuffer->mFrameType = desc->pic_type;
			//AM_INFO("CAmbaVideoEncoder send DATA\n");
			mpOutput->SendBuffer(pBuffer);
			if(mbDual) {
				CBuffer *pBuffer1;

				if (!mpOutput1->AllocBuffer(pBuffer1))
					return;

				pBuffer1->SetType(CBuffer::DATA);
				pBuffer1->mFlags = 0;
				pBuffer1->mPTS = mpOutput1->FixPTS(desc->PTS);
				pBuffer1->mBlockSize = 
				pBuffer1->mDataSize = (desc->pic_size + 31) & ~31;
				pBuffer1->mpData = (AM_U8*)desc->start_addr;
				pBuffer1->mSeqNum = mpOutput1->mnFrames++;
				pBuffer1->mFrameType = desc->pic_type;

				mpOutput1->SendBuffer(pBuffer1);
			}
		}
	}
}

//-----------------------------------------------------------------------
//
// CAmbaVideoOutput
//
//-----------------------------------------------------------------------
CAmbaVideoOutput* CAmbaVideoOutput::Create(CFilter *pFilter)
{
	CAmbaVideoOutput *result = new CAmbaVideoOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoOutput::Construct()
{
	return ME_OK;
}

void CAmbaVideoOutput::ResetForEncoding()
{
	mbEncodingStarted = false;
}

am_pts_t CAmbaVideoOutput::FixPTS(AM_U32 dsp_pts)
{
	dsp_pts &= 0x3FFFFFFF;

	if (!mbEncodingStarted) {
		mbEncodingStarted = true;
		mLastDspPts = dsp_pts;
		mLastPts = 0;
		mnFrames = 0;
		return 0;
	}

	am_pts_t pts;
	AM_U32 delta;

	if (dsp_pts >= mLastDspPts) {
		// PTS increased
		delta = dsp_pts - mLastDspPts;
		if (delta < 0x10000000)
			pts = mLastPts + delta;
		else {
			// wrap around
			//AM_INFO("pts wrap around 1\n");
			//AM_INFO("mLastDspPts = 0x%x, mLastPts = 0x%x\n",
			//	mLastDspPts, dsp_pts);
			delta = (mLastDspPts + 0x40000000) - dsp_pts;
			pts = mLastPts - delta;
		}
	}
	else {
		// PTS decreased
		delta = mLastDspPts - dsp_pts;
		if (delta < 0x10000000)
			pts = mLastPts - delta;
		else {
			//AM_INFO("pts wrap around 2\n");
			//AM_INFO("mLastDspPts = 0x%x, mLastPts = 0x%x\n",
			//	mLastDspPts, dsp_pts);
			delta = (dsp_pts + 0x40000000) - mLastDspPts;
			pts = mLastPts + delta;
		}
	}

	mLastDspPts = dsp_pts;
	mLastPts = pts;

	return pts;
}

