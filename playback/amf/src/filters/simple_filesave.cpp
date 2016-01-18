/**
 * simple_filesave.cpp
 *
 * History:
 *    2010/8/20 - [QXiong Zheng] created file
 *     create this filter for saving demuxer's packet
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
#include "simple_filesave.h"

//-----------------------------------------------------------------------
//
// CSimpleFileSave
//
//-----------------------------------------------------------------------
IFilter* CSimpleFileSave::Create(IEngine *pEngine, const char* pFile)
{
	CSimpleFileSave* result = new CSimpleFileSave(pEngine, pFile);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleFileSave::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;

	mpWriter= CFileWriter::Create();
	if (mpWriter == NULL)
		return ME_ERROR;

	mpInput = CSimpleFileSaveInput::Create(this, "CSimpleFileSave");
	if (mpInput == NULL)
		return ME_ERROR;

	return ME_OK;
}

CSimpleFileSave::~CSimpleFileSave()
{
	AM_DELETE(mpWriter);
	AM_DELETE(mpInput);	
}

void* CSimpleFileSave::GetInterface(AM_REFIID refiid)
{
	return inherited::GetInterface(refiid);
}

void CSimpleFileSave::GetInfo(INFO& info)
{
	info.nInput = 1;
	info.nOutput = 0;
	info.pName = "CSimpleFileSave";
}

IPin* CSimpleFileSave::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return mpInput;
	return NULL;     
}

AM_ERR CSimpleFileSave::Run()
{	
	if(mbIsAudio == false)
	{
		//AM_INFO("Video=>buf Width, Heigh: %d, %d=====pic Width, Heigh: %d, %d\n",mpFormat->bufWidth,
			//	mpFormat->bufHeight, mpFormat->picWidth, mpFormat->picHeight);
	}else{
		//AM_INFO("Audio=>samplerate: %d===channels: %d===\n",mpFormat->auSamplerate,
				//mpFormat->auChannels);
	}

	return mpInput->Run();
}


AM_ERR CSimpleFileSave::Stop()
{
	return mpInput->Stop();
}

AM_ERR CSimpleFileSave::Start()
{
	//return mpInput->Start();
	return ME_OK;
}

#define show 30

AM_ERR CSimpleFileSave::SaveBuffer(CBuffer *pBuffer)
{
	static int num = 1;
	if(!(num % show))
		AM_INFO("Process  Buffer:   %d====\n", num);
	num++;	
	AM_ERR err;
	AVPacket* pPacket = (AVPacket*)((AM_U8*)pBuffer + sizeof(CBuffer));

	AM_UINT len = pBuffer->GetDataSize();
	//AM_INFO("Save a Buffer with len : %d====\n", len);
	err =mpWriter->WriteFile(pPacket->data, len);
	if (err != ME_OK) 
	{
		AM_PRINTF("Error in writing buffer size:%d\n", len);
		//exit(1);
		//return ME_ERROR;
	}
	return ME_OK;
}

AM_ERR CSimpleFileSave::ProcessBuffer(CBuffer *pBuffer)
{
	AM_ERR err;

	if (pBuffer->IsEOS()) 
	{
		AM_PRINTF("CSimpleFileSaveInput receives EOS\n");
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
		err = SaveBuffer(pBuffer);
		if(err != ME_OK)
			return err;
		
		pBuffer->Release();
		return ME_OK;
	}
}

AM_ERR CSimpleFileSave::SetInputFormat(CFFMpegMediaFormat* pFormat)
{
	if (*(pFormat->pMediaType) == GUID_Decoded_Video)
	{
		mbIsAudio = false;
	}
		
	if(*(pFormat->pMediaType) == GUID_Decoded_Audio)
	{
		mbIsAudio = true;
	}
	AM_ERR err = SetOutputFile();
	if(err != ME_OK)	return ME_NOT_EXIST;	
	return ME_OK;
}

AM_ERR CSimpleFileSave::SetOutputFile()
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

	return ME_OK;
}

void CSimpleFileSave::CloseFiles()
{
	mpWriter->CloseFile();
}

void CSimpleFileSave::WriteEOS()
{
	//TODO;
}

//-----------------------------------------------------------------------
//
// CSimpleFileSaveInput
//
//-----------------------------------------------------------------------
CSimpleFileSaveInput* CSimpleFileSaveInput::Create(CFilter *pFilter, const char *pName)
{
	CSimpleFileSaveInput* result = new CSimpleFileSaveInput(pFilter,pName);
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CSimpleFileSaveInput::Construct()
{
	AM_ERR err = inherited::Construct();
	if (err != ME_OK)
		return err;
	return ME_OK;
}

CSimpleFileSaveInput::~CSimpleFileSaveInput()
{
	//AM_DELETE(mpBufferPool);
}

void* CSimpleFileSaveInput::GetInterface(AM_REFIID refiid)
{
	return inherited::GetInterface(refiid);
}

AM_ERR CSimpleFileSaveInput::CheckMediaFormat(CMediaFormat *pFormat)
{
	return ((CSimpleFileSave*)mpFilter)->SetInputFormat((CFFMpegMediaFormat*)pFormat);
}


void CSimpleFileSaveInput::OnRun()
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
				AM_PRINTF("Close CSimpleFileSaveInput!\n");
				CmdAck(ME_OK);
				break;
			}
		}
		else {
			if (PeekBuffer(pBuffer)) {
				AM_ERR err = ((CSimpleFileSave*)mpFilter)->ProcessBuffer(pBuffer);
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

AM_ERR CSimpleFileSaveInput::ProcessBuffer(CBuffer *pBuffer)
{
	return ME_OK;
}
/*  	TODO
void CVideoRendererInput::Purge()
{
}
*/

void CSimpleFileSaveInput::DoStop()
{
	//((CFileSaveFilter*)mpFilter)->DoStop();
}



