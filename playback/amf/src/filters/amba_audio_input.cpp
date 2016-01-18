/*
 * amba_audio_input.cpp
 *
 * History:
 *    2011/4/13 - [Luo Fei] created file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#define LOG_NDEBUG 0
#define LOG_TAG "amba_audio_input"
#define AMDROID_DEBUG

#include <media/AudioRecord.h>
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "record_if.h"
#include "am_record_if.h"
#include "engine_guids.h"
extern "C" {
#define INT64_C(a) (a ## LL)
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#include "amba_audio_input.h"

#define MAX_FRAME_SIZE 6144
//#define RECORD_AUD_FILE

using namespace android;

extern IFilter* CreateAmbaAudioInput(IEngine * pEngine)
{
	return CAmbaAudioInput::Create(pEngine);
}

IFilter* CAmbaAudioInput::Create(IEngine * pEngine)
{
	CAmbaAudioInput * result = new CAmbaAudioInput(pEngine);

	if(result && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAmbaAudioInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if(err != ME_OK)
		return err;

	if ((mpBuf = CSimpleBufferPool::Create("AudioInputBuffer", 128, sizeof(CBuffer) + MAX_FRAME_SIZE) ) == NULL)
		return ME_NO_MEMORY;

	if ((mpOutput = CAmbaAudioInputOutput::Create(this)) == NULL)
		return ME_NO_MEMORY;

	mpOutput->SetBufferPool(mpBuf);

#ifdef RECORD_AUD_FILE
        mpAFile = CFileWriter::Create();
        if (mpAFile == NULL)
                return ME_ERROR;
#endif

	return ME_OK;
}


CAmbaAudioInput::~CAmbaAudioInput()
{
	AM_DELETE(mpOutput);
	AM_DELETE(mpBuf);

#ifdef RECORD_AUD_FILE
        AM_DELETE(mpAFile);
#endif

}

void CAmbaAudioInput::GetInfo(INFO& info)
{
	info.nInput = 0;
	info.nOutput = 1;
	info.pName = "AudioInput";
}

IPin* CAmbaAudioInput::GetOutputPin(AM_UINT index)
{
	if (index == 0) return mpOutput;
	return NULL;
}


void CAmbaAudioInput::DoStop(STOP_ACTION action)
{
	AM_INFO("DoStop");
	mAction = action;
	mbRunFlag = false;
}

AM_ERR CAmbaAudioInput::GetParametersFromEngine()
{
	IAmbaAuthorControl * p = NULL;
	if ((p = IAmbaAuthorControl::GetInterfaceFrom(mpEngine)) == NULL) {
		mSamplingRate = 48000;
		mNumberOfChannels = 2;
		mSampleFmt = SAMPLE_FMT_S16;
		mCodecId = CODEC_ID_AAC;
	}
	else {
		int fmt,codecId;
		p->GetAudioParam(&mSamplingRate,&mNumberOfChannels,NULL,&fmt,&codecId);
		mSampleFmt = (SampleFormat)fmt;
		mCodecId = (CodecID)codecId;
	}

	AM_INFO("mSamplingRate=%d, mNumberOfChannels=%d,mSampleFmt=%d,mCodecId=%d",
		mSamplingRate,mNumberOfChannels,mSampleFmt,mCodecId);

	mSampleSize = 0;
	if (mSampleFmt == SAMPLE_FMT_U8) mSampleSize = 1;
	else if (mSampleFmt == SAMPLE_FMT_S16) mSampleSize = 2;
	else return ME_ERROR;
	int frameSize = 0;
	if (mCodecId == CODEC_ID_AAC) frameSize =1024;
	else if (mCodecId == CODEC_ID_AC3) frameSize =1536;
	else return ME_ERROR;
	mFrameSizeInByte = frameSize * mNumberOfChannels * mSampleSize;

	return ME_OK;
}

void CAmbaAudioInput::OnRun()
{
	AM_INFO("OnRun ...");
	FILE* fIn = NULL;
	CBuffer *pBuffer;
	AM_U8 * buf = NULL;
	int outSize = 0;
	long iTimeStamp = 0;
	long dataDuration = 0;
	struct timeval tv;
	long timeNow = 0;
	long timeNext = 0;
	int frameNum =0;

	if (GetParametersFromEngine() < 0 ) {
		AM_ERROR("Failed to get parameters");
		return CmdAck(ME_ERROR);
	}

	if (mFrameSizeInByte > MAX_FRAME_SIZE) {
		AM_ERROR("frame size error");
		return CmdAck(ME_ERROR);
	}

	//open file
	char PCMFileName[DAMF_MAX_FILENAME_LEN + DAMF_MAX_FILENAME_EXTENTION_LEN] = {0};
	snprintf(PCMFileName, sizeof(PCMFileName), "%s/test.pcm", AM_GetPath(AM_PathSdcard));
	if ((fIn = ::fopen(PCMFileName,"r")) == NULL) {
		AM_ERROR("Failed to open %s", PCMFileName);
		return CmdAck(ME_ERROR);
	}

	AM_INFO("OnRun OK");
	CmdAck(ME_OK);

	mbRunFlag = true;
	while(true) {
		if (!mpOutput->AllocBuffer(pBuffer)) {
			AM_ERROR("Failed to AllocBuffer");
			break;
		}

		if (!mbRunFlag) {
			if (mAction == SA_STOP) {
				AM_INFO("CAudioInput send EOS\n");
				pBuffer->SetType(CBuffer::EOS);
				mpOutput->SendBuffer(pBuffer);
			}
			break;
		}

		buf = (AM_U8*)((AM_U8*)pBuffer+sizeof(CBuffer));
		outSize = ::fread(buf, 1, mFrameSizeInByte, fIn);
		if (outSize < 0) {
			AM_ERROR("Failed to read");
			break;
		}
		else if (outSize == 0) {
			AM_INFO("Read again");
			fseek(fIn,0,SEEK_SET);
			continue;
		}
		else {
			dataDuration = (outSize/mSampleSize/mNumberOfChannels) * 1000 /mSamplingRate; //ms
			pBuffer->SetType(CBuffer::DATA);
			pBuffer->mpData = buf;
			pBuffer->mFlags = 0;
			pBuffer->mDataSize = outSize;
			pBuffer->SetPTS(iTimeStamp);
			/* gettimeofday(&tv,NULL);
			timeNow = tv.tv_sec*1000000 + tv.tv_usec;
			AM_INFO("1 timeNow: %ld, timeNext:%ld\n",timeNow,timeNext);
			while (timeNow < timeNext) {
				AM_INFO("2 sleep (%d)\n",(int)(timeNext - timeNow));
				usleep((int)(timeNext - timeNow));
				gettimeofday(&tv,NULL);
				timeNow = tv.tv_sec*1000000 + tv.tv_usec;
				AM_INFO("3 timeNow: %ld, timeNext:%ld\n",timeNow,timeNext);
			}
			timeNext = timeNow + dataDuration*1000; // us */

			/*
			 * Because the audio data is simulated through reading from
			 * a specific file and the audio data's format is AAC, audio
			 * input filter need to go to sleep for 21 ms when it read
			 * certain simulated data.
			 */
			usleep (21000);

			AM_INFO("CAudioInput send data, %ld, %ld, %d, frameNum %d\n",timeNow,timeNext,outSize,frameNum++);
			mpOutput->SendBuffer(pBuffer);
			iTimeStamp += dataDuration;
		}

	}

	::fclose(fIn);
	AM_INFO("OnRun end");
}

#if 0
void CAudioInput::OnRun()
{

	/*if (GetFrameSize() !=ME_OK) {
		AM_ERROR("Get FrameSize failed\n");
		return CmdAck(ME_ERROR);
	}*/

	//TODO: get buffer size from AudioFlinger
	const int kBufferSize = 2048;
	int iTimeStamp = 0;
	int dataDuration = 0;
	CBuffer *pBuffer;

	AM_UINT flags = AudioRecord::RECORD_AGC_ENABLE | AudioRecord::RECORD_NS_ENABLE | AudioRecord::RECORD_IIR_ENABLE;

	AudioRecord * record = new AudioRecord(
			mAudioSource, mAudioSamplingRate,
			AudioSystem::PCM_16_BIT,
			(mAudioNumberOfChannels > 1) ? AudioSystem::CHANNEL_IN_STEREO : AudioSystem::CHANNEL_IN_MONO,
			32*kBufferSize/mAudioNumberOfChannels/sizeof(AM_U16), flags);
	status_t res = record->initCheck();
	if (res == NO_ERROR)
		res = record->start();

	if (res == NO_ERROR) {

#ifdef RECORD_AUD_FILE
	AM_ERR er = mpAFile->CreateFile("/data/audio_input.pcm");
	if (er != ME_OK) {
		AM_ERROR("CreateFile failed");
		return ;
	}
	AM_INFO("CreateFile /data/audio_input.pcm");
#endif

		mbInputting = true;
		CmdAck(ME_OK);

		while (1) {

			AM_U8*  data = NULL;
			int numOfBytes = 0;

			if (!mpOutput->AllocBuffer(pBuffer)) {
				AM_ERROR("AllocBuffer failed %s", __FUNCTION__);
				break;
			}

			if (!mbInputting) {
				if (mAction == SA_STOP) {
					//AM_INFO("CAudioInput send EOS\n");
					pBuffer->SetType(CBuffer::EOS);
					mpOutput->SendBuffer(pBuffer);
				}
				break;
			}

			data = (AM_U8*)((AM_U8*)pBuffer+sizeof(CBuffer));

			numOfBytes = record->read(data, mFrameSizeInByte);
			if (numOfBytes <= 0) {
				AM_ERROR("CAudioInput::OnRun() read bytes <= 0\n");
				return;
			}
			dataDuration = (numOfBytes/sizeof(AM_U16)/mAudioNumberOfChannels) * 1000 /mAudioSamplingRate; //ms
			pBuffer->SetType(CBuffer::DATA);
			pBuffer->mpData = data;
			pBuffer->mFlags = 0;
			pBuffer->mDataSize = numOfBytes;
			pBuffer->SetPTS(iTimeStamp);
			//AM_INFO("CAudioInput send data\n");
			mpOutput->SendBuffer(pBuffer);

			iTimeStamp += dataDuration;

#ifdef RECORD_AUD_FILE
			AM_INFO("WriteFile bytes:%d\n",numOfBytes);
			mpAFile->WriteFile(data, numOfBytes);
#endif

		}
		record->stop();

#ifdef RECORD_AUD_FILE
	AM_INFO("CloseFile\n");
	mpAFile->CloseFile();
#endif

	}
	else {
			AM_ERROR("AudioRecord Error!\n");
			CmdAck(ME_ERROR);
	}
	delete record;

}
#endif
//-----------------------------------------------------------------------
//
// CAmbaAudioInputOutput
//
//-----------------------------------------------------------------------
CAmbaAudioInputOutput* CAmbaAudioInputOutput::Create(CFilter *pFilter)
{
	CAmbaAudioInputOutput *result = new CAmbaAudioInputOutput(pFilter);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}
