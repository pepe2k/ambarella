/**
 * filesave_filter.cpp
 *
 * History:
 *    2010/6/1 - [QXiong Zheng] created file
 *     
 *
 * Copyright (C) 2010-2012, Ambarella, Inc.
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
#include <math.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

#include "pbif.h"	//CFFMEPGMEDIAFORMAT , 
//#include "video_renderer.h"//CFrameBufferPool
//#include "audio_renderer.h" //CFixedBP

#include "record_if.h" //FileWrite
#include "engine_guids.h"
#include "filter_list.h"

extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "filesave_filter.h"

//-----------------------------------------------------------------------
//
// CVideoBufferPool
//
//-----------------------------------------------------------------------
CVideoBufferPool* CVideoBufferPool::Create(AM_UINT size, AM_UINT count)
{
	CVideoBufferPool *result = new CVideoBufferPool();
	if (result != NULL && result->Construct(size, count) != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CVideoBufferPool::Construct(AM_UINT size, AM_UINT count)
{
	AM_ERR err;

	if ((err = inherited::Construct(count)) != ME_OK)
		return err;
	
	if ((mpBufferVB = new CVideoBuffer[count]) == NULL)
		return ME_ERROR;
	
	size = ROUND_UP(size, 4);
	if ((mpLumaMemory= new AM_U8[count*size ]) == NULL)
		return ME_ERROR;
	if ((mpChroMemory= new AM_U8[count*size ]) == NULL)
		return ME_ERROR;
	
	CVideoBuffer* pVideoBuffer = mpBufferVB;
	AM_U8* pLumaMemory = mpLumaMemory;
	AM_U8* pChroMemory = mpChroMemory;
	for (AM_UINT i = 0; i < count; i++)
	{
		pVideoBuffer->pLumaAddr = pLumaMemory;
		pVideoBuffer->pChromaAddr = pChroMemory;
		pVideoBuffer->tag = i;
		CBuffer* pBuffer = (CBuffer*)pVideoBuffer;
		err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
		AM_ASSERT_OK(err);
		pLumaMemory = pLumaMemory+size;
		pChroMemory = pChroMemory+size;
		pVideoBuffer ++;		
	}
	return ME_OK;
}

CVideoBufferPool::~CVideoBufferPool()
{
	//delete[] mpBufferVB;
	delete[]	mpLumaMemory;
	delete[] mpChroMemory;
	delete[] mpBufferVB;
}

//TODO
AM_ERR CVideoBufferPool::CreateFrameBP(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight)
{
	return ME_NOT_EXIST;
}

AM_ERR CVideoBufferPool::ReleaseFrameBP()
{
	return ME_NOT_EXIST;
}

bool CVideoBufferPool::AllocBuffer(CBuffer* & pBuffer, AM_UINT size)
{	
	if (!inherited::AllocBuffer(pBuffer, size)) 
		return false;
	pBuffer->SetType(CBuffer::DATA);
	pBuffer->SetDataSize(0);
	pBuffer->SetDataPtr(NULL);
	
	CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
	pVideoBuffer->picWidth = mPicWidth;
	pVideoBuffer->picHeight = mPicHeight;
	pVideoBuffer->fbWidth = mFbWidth;
	pVideoBuffer->fbHeight = mFbHeight;
	pVideoBuffer->flags = 0;
	return true;
}

AM_ERR CVideoBufferPool::OpenDevice(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight)
{
	mPicWidth = picWidth;
	mPicHeight = picHeight;
	mFbWidth = width;
	mFbHeight = height;
	return ME_OK;
}


void CVideoBufferPool::OnReleaseBuffer(CBuffer * pBuffer)
{
	//inherited::OnReleaseBuffer(pBuffer);
}
/*---------------------
//-----------------------------------------------------------------------
//
// CVideoBufferPool2
//
//-----------------------------------------------------------------------
CVideoBufferPool* CVideoBufferPool::Create(int iavFd)
{
	CVideoBufferPool *result = new CVideoBufferPool(iavFd);
	if (result != NULL && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CVideoBufferPool::Construct()
{
	AM_ERR err;

	if ((err = inherited::Construct()) != ME_OK)
		return err;
	return ME_OK;
}

CVideoBufferPool::~CVideoBufferPool()
{
}

AM_ERR CVideoBufferPool::CreateFrameBP(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight)
{
	return ME_NOT_EXIST;
}

AM_ERR CVideoBufferPool::ReleaseFrameBP()
{
	return ME_NOT_EXIST;
}

bool CVideoBufferPool::AllocBuffer(CBuffer* & pBuffer, AM_UINT size)
{	
	return inherited::AllocBuffer(pBuffer, size);
}

AM_ERR CVideoBufferPool::OpenDevice(AM_UINT width, AM_UINT height, AM_UINT picWidth, AM_UINT picHeight)
{
	return inherited::OpenDevice(width, height, picWidth, picHeight);
}

void CVideoBufferPool::CloseDevice()
{
	inherited::CloseDevice();
}

void CVideoBufferPool::OnReleaseBuffer(CBuffer * pBuffer)
{
	inherited::OnReleaseBuffer(pBuffer);
}
----------------*/

//-----------------------------------------------------------------------
//
// CAudioBufferPool
//
//-----------------------------------------------------------------------
CAudioBufferPool* CAudioBufferPool::Create(AM_UINT size, AM_UINT count)
{
	CAudioBufferPool *result = new CAudioBufferPool;
	if (result != NULL && result->Construct(size, count) != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CAudioBufferPool::Construct(AM_UINT size, AM_UINT count)
{
	AM_ERR err;

	if ((err = inherited::Construct(count)) != ME_OK)
		return err;

	if ((mpBuffers = new CBuffer[count]) == NULL)
	{
		return ME_ERROR;
	}

	size = ROUND_UP(size, 4);
	if ((mpMemory = new AM_U8[count * size]) == NULL)
	{
		return ME_ERROR;
	}

	AM_U8 *ptr = mpMemory;
	CBuffer *pBuffer = mpBuffers;
	for (AM_UINT i = 0; i < count; i++, ptr += size, pBuffer++)
	{
		pBuffer->mpData = ptr;
		pBuffer->mBlockSize = size;
		pBuffer->mpPool = this;

		err = mpBufferQ->PostMsg(&pBuffer, sizeof(pBuffer));
		AM_ASSERT_OK(err);
	}

	return ME_OK;
}


CAudioBufferPool::~CAudioBufferPool()
{
	delete[] mpMemory;
	delete[] mpBuffers;
}

//-----------------------------------------------------------------------
//
// CFileSaveFilter
//
//-----------------------------------------------------------------------
IFilter* CFileSaveFilter::Create(IEngine *pEngine, const char* pFile)
{
	CFileSaveFilter* result = new CFileSaveFilter(pEngine, pFile);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFileSaveFilter::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
/*
	if ((mIavFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
		perror("/dev/iav");
		return ME_ERROR;
	}
*/	
	mpWriter= CFileWriter::Create();
	if (mpWriter == NULL)
		return ME_ERROR;

	mpInput = CFileSaveInput::Create(this, "FileSaveFilter");
	if (mpInput == NULL)
		return ME_ERROR;
	
	mpAudioBP = CAudioBufferPool::Create(192000, 64);
	
	//mpVideoBP = CVideoBufferPool::Create(mIavFd);//CVideoBP2

	mpVideoBP = CVideoBufferPool::Create(1024*1024*4,4);
	if(mpVideoBP == NULL)
	{   AM_INFO("Create VideoBP faile\n"); return ME_ERROR;	}

	return ME_OK;
}

CFileSaveFilter::~CFileSaveFilter()
{
	AM_DELETE(mpWriter);
	AM_DELETE(mpInput);	
	if(mbIsAudio)
		AM_DELETE(mpAudioBP);
	else	
		AM_DELETE(mpVideoBP);

//	if (mIavFd >= 0)
//		::close(mIavFd);
}

void* CFileSaveFilter::GetInterface(AM_REFIID refiid)
{
	return inherited::GetInterface(refiid);
}

void CFileSaveFilter::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 0;
	info.pName = "FileSaveFilter";
}

IPin* CFileSaveFilter::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInput;
	return NULL;     
}

AM_ERR CFileSaveFilter::Run()
{	
	if(mbIsAudio == false)
	{
		//AM_ERR err = IavInit();
		//if (err != ME_OK)
		//	return err;
		AM_INFO("Video=>buf Width, Heigh: %d, %d=====pic Width, Heigh: %d, %d\n",mpFormat->bufWidth,
				mpFormat->bufHeight, mpFormat->picWidth, mpFormat->picHeight);
		AM_ERR err = mpVideoBP->OpenDevice(mpFormat->bufWidth, mpFormat->bufHeight,
									mpFormat->picWidth, mpFormat->picHeight);	
		if (err != ME_OK)
			return err;
	}else{
		AM_INFO("Audio=>samplerate: %d===channels: %d===\n",mpFormat->auSamplerate,
				mpFormat->auChannels);
	}

	return mpInput->Run();
}

/*
AM_ERR CFileSaveFilter::IavInit()
{
	::ioctl(mIavFd, IAV_IOC_STOP_DECODE, 0);

	if (::ioctl(mIavFd, IAV_IOC_START_DECODE, 0) < 0) {
		return ME_ERROR;
	}

	return ME_OK;
}

void CFileSaveFilter::IavStop()
{
	if (::ioctl(mIavFd, IAV_IOC_STOP_DECODE, 0) < 0) {
		AM_PRINTF("----IAV_IOC_STOP_DECODE  Fail----\n");	
	}
}*/

AM_ERR CFileSaveFilter::Stop()
{
	//IavStop();
	return mpInput->Stop();
}

AM_ERR CFileSaveFilter::Start()
{
	//return mpInput->Start();
	return ME_OK;
}

void CFileSaveFilter::SetVideoBuffer(AVPicture& pic, CBuffer* pBuffer)
{
	CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
	pVideoBuffer->picXoff = mpFormat->picXoff;
	pVideoBuffer->picYoff = mpFormat->picYoff;
	
	if (pVideoBuffer->picXoff > 0) {
		pic.data[0] = (uint8_t*)pVideoBuffer->pLumaAddr +
			pVideoBuffer->picYoff * pVideoBuffer->fbWidth +
			pVideoBuffer->picXoff;
		//AM_INFO("============== %d * %d [%d]===\n",
		//	pVideoBuffer->fbWidth,pVideoBuffer->fbHeight, pic.data[0]);
		pic.linesize[0] = pVideoBuffer->fbWidth;

		pic.data[1] = (uint8_t*)pVideoBuffer->pChromaAddr +
			(pVideoBuffer->picYoff / 2) * pVideoBuffer->fbWidth / 2 +
			pVideoBuffer->picXoff / 2;
		pic.linesize[1] = pVideoBuffer->fbWidth / 2;

		pic.data[2] = pic.data[1] + pVideoBuffer->fbWidth * 
			pVideoBuffer->fbHeight / 4;
		pic.linesize[2] = pVideoBuffer->fbWidth / 2;
	}
	else {
		pic.data[0] = (uint8_t*)pVideoBuffer->pLumaAddr;
		pic.linesize[0] = pVideoBuffer->fbWidth;
		pic.data[1] = (uint8_t*)pVideoBuffer->pChromaAddr;
		pic.linesize[1] = pVideoBuffer->fbWidth;
		pic.data[2] = pic.data[1] + 1;
		pic.linesize[2] = pVideoBuffer->fbWidth;
	}
}

#define show 15
AM_ERR CFileSaveFilter::ProcessVideoBuffer(CBuffer * pBuffer)
{
	static int num = 1;
	if(!(num % show))
		AM_INFO("Process Video Buffer:   %d==\n", num);
	num++;
	AM_ERR err;
	AVPicture pic;
	SetVideoBuffer(pic, pBuffer);

	CVideoBuffer *pVideoBuffer = (CVideoBuffer*)pBuffer;
	int height = pVideoBuffer->picHeight;
	int width = pVideoBuffer->picWidth;
	/*
	int i;
	for(i=0; i<height; i++)
	mpWriter->WriteFile(pic.data[0]+ i * pic.linesize[0], width);
	for(i=0; i<height/2; i++)
	mpWriter->WriteFile(pic.data[1]+ i * pic.linesize[1], width/2);
	for(i=0; i<height/2; i++)
	mpWriter->WriteFile(pic.data[2]+ i * pic.linesize[2], width/2);
	*/
	
	/*	int i,k,shift;
  	uint8_t* yuv_ptr;
	for(k=0;k<3;k++)
	{
  		shift = (k==0 ? 0:1);
  		yuv_ptr=pic.data[k];
  		for(i=0; i<(height>>shift);i++)
		{
  			err =mpWriter->WriteFile(yuv_ptr, (width>>shift));
			if (err != ME_OK) 
			{
				AM_PRINTF("Error in writing audio size:%d\n", (width>>shift));
			}
			mVideoBytesWritten = mVideoBytesWritten + (width>>shift);
			yuv_ptr+=pic.linesize[k];
  		}
 	}*/
	//pBuffer->Release();
	int i;
	uint8_t *ptr, *ptr1, *ptr2;
	ptr = pic.data[0];
   	for(i=0;i<height;i++) {
       	 	mpWriter->WriteFile(ptr, width);
        	ptr += pic.linesize[0];
    	}

 
   	 ptr1 = pic.data[2];
   	 ptr2 = pic.data[1];
    	for(i=0;i<height/2;i++) {     /* Cb */
        	mpWriter->WriteFile(ptr1, width/2);
        	ptr1 += pic.linesize[1];
    	}
    	for(i=0;i<height/2;i++) {     /* Cr */
         	mpWriter->WriteFile(ptr2, width/2);
            	ptr2 += pic.linesize[2];
    	}
	return ME_OK;
  }

AM_ERR CFileSaveFilter::ProcessAudioBuffer(CBuffer *pBuffer)
{
	static int num = 1;
	if(!(num % show))
		AM_INFO("Process Audio Buffer:   %d==\n", num);
	num++;	
	AM_ERR err;
	AM_UINT len = pBuffer->GetDataSize();
	err =mpWriter->WriteFile(pBuffer->GetDataPtr(), len);
	if (err != ME_OK) 
	{
		AM_PRINTF("Error in writing audio size:%d\n", len);
		//exit(1);
		//return ME_ERROR;
	}
	mAudioBytesWritten = mAudioBytesWritten + len;	
	return ME_OK;
}

AM_ERR CFileSaveFilter::ProcessBuffer(CBuffer *pBuffer)
{
	AM_ERR err;

	if (pBuffer->IsEOS()) 
	{
		AM_PRINTF("CFileSaveFilter receives EOS\n");
		pBuffer->Release();
		WriteEOS();
		PostEngineMsg(IEngine::MSG_EOS);
		return ME_CLOSED;
	}else {
		// send MSG_READY
		if (mbFirstBuffer) 
		{
			PostEngineMsg(IEngine::MSG_READY);
			AM_PRINTF("Starting save to file\n");
			mbFirstBuffer = false;
		}
		if(mbIsAudio)
		{
			err = ProcessAudioBuffer(pBuffer);
			if(err != ME_OK)
				return err;
		}else{
			err = ProcessVideoBuffer(pBuffer);
			if(err != ME_OK)
				return err;
		}		
		pBuffer->Release();
		return ME_OK;
	}
}

AM_ERR CFileSaveFilter::SetInputFormat(CFFMpegMediaFormat* pFormat)
{
	if (*(pFormat->pMediaType) == GUID_Decoded_Video)
	{
		mbIsAudio = false;
		AM_DELETE(mpAudioBP);
		mpAudioBP = NULL;
		mpInput->SetBufferPool(mpVideoBP);
	}
		
	if(*(pFormat->pMediaType) == GUID_Decoded_Audio)
	{
		mbIsAudio = true;
		AM_DELETE(mpVideoBP);
		mpVideoBP = NULL;
		mpInput->SetBufferPool(mpAudioBP);
	}
	AM_ERR err = SetOutputFile();
	if(err != ME_OK)	return ME_NOT_EXIST;	
	mpFormat = pFormat;
	return ME_OK;
}

AM_ERR CFileSaveFilter::SetOutputFile()
{
	CloseFiles();
	char filename[260];
	AM_ERR err;

	//if(mbIsAudio)
	sprintf(filename, "%s", mpFileName);
	err = mpWriter->CreateFile(filename);
	if (err != ME_OK)
		return err;
	AM_INFO("Save File created: %s\n", filename);

	mAudioBytesWritten = 0;
	mVideoBytesWritten = 0;
	return ME_OK;
}

void CFileSaveFilter::CloseFiles()
{
	mpWriter->CloseFile();
}

void CFileSaveFilter::WriteEOS()
{
	//TODO;
}

//-----------------------------------------------------------------------
//
// CFileSaveInput
//
//-----------------------------------------------------------------------
CFileSaveInput* CFileSaveInput::Create(CFilter *pFilter, const char *pName)
{
	CFileSaveInput* result = new CFileSaveInput(pFilter,pName);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CFileSaveInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CFileSaveInput::~CFileSaveInput()
{
	//AM_DELETE(mpBufferPool);
}

void* CFileSaveInput::GetInterface(AM_REFIID refiid)
{
	return inherited::GetInterface(refiid);
}

AM_ERR CFileSaveInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ((CFileSaveFilter*)mpFilter)->SetInputFormat((CFFMpegMediaFormat*)pFormat);
}


void CFileSaveInput::OnRun()
{
	CMD cmd;
	CBuffer *pBuffer;
	CQueue::WaitResult result;
	CmdAck(ME_OK);

	while (1) 
	{
		CQueue::QType type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);

		if (type == CQueue::Q_MSG) 
		{
			if (cmd.code == AO::CMD_STOP) {
				// TO DO Something?
				DoStop();
				AM_PRINTF("Close FileSaveFilter!\n");
				CmdAck(ME_OK);
				break;
			}
		}
		else {
			if (PeekBuffer(pBuffer)) {
				AM_ERR err = ((CFileSaveFilter*)mpFilter)->ProcessBuffer(pBuffer);
				if (err != ME_OK) {
					if (err != ME_CLOSED)
						PostEngineErrorMsg(err);
					//AM_ERROR("CFileSaveInput --err != ME_OK\n");
					break;
				}
			}
		}
	}
}

AM_ERR CFileSaveInput::ProcessBuffer(CBuffer *pBuffer)
{
	return ME_OK;
}
/*  	TODO
void CVideoRendererInput::Purge()
{
}
*/

void CFileSaveInput::DoStop()
{
	//((CFileSaveFilter*)mpFilter)->DoStop();
}


