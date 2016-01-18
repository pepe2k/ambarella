/*
 * amba_video_encoder.cpp
 *
 * History:
 *    20011/4/13 - [Luo Fei] create file
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "amba_video_encoder"
#define AMDROID_DEBUG

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
#include "amba_video_encoder.h"

extern IFilter* CreateAmbaVideoEncoderFilterEx(IEngine *pEngine)
{
	return CAmbaVideoEncoder::Create(pEngine);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoEncoder
//
//-----------------------------------------------------------------------
IFilter* CAmbaVideoEncoder::Create(IEngine *pEngine)
{
	CAmbaVideoEncoder *result = new CAmbaVideoEncoder(pEngine);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaVideoEncoder::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	if ((mpBuf = CSimpleBufferPool::Create("VideoEncodeBuffer", 32)) == NULL) {
		AM_ERROR("Failed to create EncBSB");
		return ME_NO_MEMORY;
	}

	if ((mpOutput = CAmbaVideoEncoderOutput::Create(this)) == NULL ) {
		AM_ERROR("Failed to create Output");
		return ME_NO_MEMORY;
	}
	mpOutput->SetBufferPool(mpBuf);

	if ((mFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
		AM_ERROR("Failed to open /dev/iav");
		return ME_ERROR;
	}

	iav_mmap_info_t info;
	if (::ioctl(mFd, IAV_IOC_MAP_BSB2, &info) < 0) {
		AM_ERROR("Failed to map bsb");
		return ME_ERROR;
	}
	mbMemMapped = true;

	AM_INFO("bsb addr = %p, length = %d", info.addr, info.length);
	return ME_OK;
}

CAmbaVideoEncoder::~CAmbaVideoEncoder()
{
	if (mFd > 0) {
		if (mbMemMapped) ::ioctl(mFd, IAV_IOC_UNMAP_BSB, 0);
		::close(mFd);
	}
	AM_DELETE(mpOutput);
	AM_RELEASE(mpBuf);
}

void CAmbaVideoEncoder::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 1;
	info.pName = "AmbaVidEnc";
}

IPin* CAmbaVideoEncoder::GetOutputPin(AM_UINT index)
{
	if (index == 0) return mpOutput;
	return NULL;
}

void CAmbaVideoEncoder::DoStop(STOP_ACTION action)
{
	AM_INFO("DoStop");
	mbRunFlag = false;
	mAction = action;
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
	AM_INFO("OnRun ...");
	iav_h264_config_t config;
	memset(&config,0,sizeof(config));
	config.stream = 0;
	if (::ioctl(mFd, IAV_IOC_GET_H264_CONFIG, &config) < 0) {
		AM_ERROR("IAV_IOC_GET_H264_CONFIG");
		return CmdAck(ME_ERROR);
	}
	PrintH264Config(&config);

	CBuffer *pBuffer;
	if (!mpOutput->AllocBuffer(pBuffer)) {
		AM_ERROR("Failed to AllocBuffer");
		return CmdAck(ME_ERROR);
	}
	pBuffer->SetType(CBuffer::INFO);
	pBuffer->SetDataPtr((AM_U8*)&config);
	pBuffer->SetDataSize(sizeof(config));
	mpOutput->SendBuffer(pBuffer);
	mpOutput->ResetForEncoding();

	mbRunFlag = true;
	int state = -1;
	while (mbRunFlag) {
		if (::ioctl(mFd, IAV_IOC_GET_STATE, &state) < 0) {
			AM_ERROR("IAV_IOC_GET_STATE");
			return CmdAck(ME_ERROR);
		}
		if (state != IAV_STATE_ENCODING) {
			usleep(10000);
			continue;
		} else {
			CmdAck(ME_OK);
			AM_INFO("OnRun OK ");
			break;
		}
	}

	// read bitstream
	bs_fifo_info_t fifo_info;
	bits_info_t *desc = NULL;
	while (true) {
		if (::ioctl(mFd, IAV_IOC_READ_BITSTREAM, &fifo_info) < 0) {
			if (!mbRunFlag) {
				if (mAction == SA_STOP) {
					if (!mpOutput->AllocBuffer(pBuffer)) {
						AM_ERROR("Failed to AllocBuffer");
						break;
					}
					pBuffer->SetType(CBuffer::EOS);
					//AM_INFO("CAmbaVideoEncoder send EOS\n");
					mpOutput->SendBuffer(pBuffer);
				}
				return;
			}
			//AM_INFO("Should not come here\n");
			usleep(10000);
			continue;
		}

		desc = &fifo_info.desc[0];
		for (AM_UINT i = 0; i < fifo_info.count; i++, desc++) {
			if (!mpOutput->AllocBuffer(pBuffer)) {
				AM_ERROR("Failed to AllocBuffer");
				break;
			}
			pBuffer->SetType(CBuffer::DATA);
			pBuffer->mFlags = 0;
			pBuffer->mPTS = mpOutput->FixPTS(desc->PTS);
			pBuffer->mBlockSize = pBuffer->mDataSize = (desc->pic_size + 31) & ~31;
			pBuffer->mpData = (AM_U8*)desc->start_addr;
			pBuffer->mSeqNum = mpOutput->mnFrames++;
			pBuffer->mFrameType = desc->pic_type;
			//AM_INFO("CAmbaVideoEncoder send DATA\n");
			mpOutput->SendBuffer(pBuffer);
		}
	}

	//AM_INFO("OnRun end, Should not come here, send error\n");

}

//-----------------------------------------------------------------------
//
// CAmbaVideoOutput
//
//-----------------------------------------------------------------------
CAmbaVideoEncoderOutput* CAmbaVideoEncoderOutput::Create(CFilter *pFilter)
{
	CAmbaVideoEncoderOutput *result = new CAmbaVideoEncoderOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

void CAmbaVideoEncoderOutput::ResetForEncoding()
{
	mbEncodingStarted = false;
}

am_pts_t CAmbaVideoEncoderOutput::FixPTS(AM_U32 dsp_pts)
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

	if (dsp_pts >= mLastDspPts) { // PTS increased
		delta = dsp_pts - mLastDspPts;
		if (delta < 0x10000000) pts = mLastPts + delta;
		else { // wrap around
			delta = (mLastDspPts + 0x40000000) - dsp_pts;
			pts = mLastPts - delta;
		}
	} else { // PTS decreased
		delta = mLastDspPts - dsp_pts;
		if (delta < 0x10000000) pts = mLastPts - delta;
		else { // wrap around
			delta = (dsp_pts + 0x40000000) - mLastDspPts;
			pts = mLastPts + delta;
		}
	}

	mLastDspPts = dsp_pts;
	mLastPts = pts;
	return pts;
}

