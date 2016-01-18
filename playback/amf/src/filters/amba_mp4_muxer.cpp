/*
 * amba_mp4_muxer.cpp
 *
 * History:
 *	2009/5/26 - [Jacky Zhang] created file
 *	2011/2/23 - [ Yi Zhu] modified
 *  2011/5/16 - [Hanbo Xiao] modified
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 
//#define LOG_NDEBUG 0
//#define LOG_TAG "amba_mp4_muxer"
//#define AMDROID_DEBUG

#include <sys/ioctl.h> //for ioctl
#include <fcntl.h>     //for open O_* flags
#include <unistd.h>    //for read/write/lseek
#include <stdlib.h>    //for malloc/free
#include <string.h>    //for strlen/memset
#include <stdio.h>     //for printf
#include <sys/socket.h>
#include <arpa/inet.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_record_if.h"
#include "record_if.h"
#include "amba_mp4_muxer.h"

#include <basetypes.h>
#include "iav_drv.h"
#include "amba_create_mp4.h"



//-----------------------------------------------------------------------
//
// CMp4Muxer
//
//-----------------------------------------------------------------------
extern IFilter* CreateAmbaMp4Muxer(IEngine *pEngine)
{
	return CMp4Muxer::Create(pEngine);
}

CMp4Muxer *CMp4Muxer::Create(IEngine *pEngine)
{
	CMp4Muxer *result = new CMp4Muxer(pEngine);
	if (result != NULL && result->Construct() != ME_OK)
	{
		delete result;
		result = NULL;
	}
	return result;
}

CMp4Muxer::CMp4Muxer(IEngine *pEngine):inherited(pEngine, "CMp4Muxer")
{
	_pVideo = NULL;
	_pCreateMp4 = NULL;

	_pAudio = NULL;
	_Config.dest_name = new char[256];
	_Config.audio_info = NULL;
	_Config.h264_info = NULL;
	_Config.record_info = NULL;
	mbRunFlag = false;
}

AM_ERR CMp4Muxer::Construct()
{
	AM_ERR err;
	if ((err = inherited::Construct()) != ME_OK) {
		return err;
	}

	if ((_pVideo = CMp4MuxerInput::Create(this)) == NULL) {
		return ME_NO_MEMORY;
	}
	
	if((_pAudio = CMp4MuxerInput::Create(this)) == NULL) {
		return ME_NO_MEMORY;
	}

	return ME_OK;
}

CMp4Muxer::~CMp4Muxer()
{
	AM_DELETE(_pVideo);
	AM_DELETE(_pCreateMp4);

	AM_DELETE(_pAudio);
	delete _Config.dest_name;
	if (_Config.h264_info) {
		delete _Config.h264_info;
	}
	if (_Config.audio_info) {
		delete _Config.audio_info;
	}
	if (_Config.record_info) {
		delete _Config.record_info;
	}
	CCreateMp4::destory_h264_config();
}

void CMp4Muxer::GetInfo(IFilter::INFO& info)
{
	info.nInput =  2;
	info.nOutput = 0;
	info.pName = "Mp4 Muxer";
}

void *CMp4Muxer::GetInterface(AM_REFIID ifID)
{
	if (ifID == IID_IMp4Muxer)
		return static_cast<IMp4Muxer*>(this);
	return inherited::GetInterface(ifID);
}


void CMp4Muxer::Delete()
{
	delete this;
}

IPin* CMp4Muxer::GetInputPin(AM_UINT index)
{
	if (index == 0)
		return _pVideo;
	else if (index == 1)
		return _pAudio;
	else {
		AM_ERROR("no such pin %d\n", index);
		return NULL;
	}
}

void CMp4Muxer::DoStop()
{
	mbRunFlag = false;
}

AM_ERR CMp4Muxer::Config(CMP4MUX_CONFIG* pConfig){
	if (pConfig->dest_name == NULL) {
		return ME_ERROR;
	}
	strcpy(_Config.dest_name,pConfig->dest_name);

	if (pConfig->h264_info) {
		_Config.h264_info = new CMP4MUX_H264_INFO;
		memcpy(_Config.h264_info,pConfig->h264_info, sizeof(CMP4MUX_H264_INFO));
	}

	if (pConfig->audio_info) {
		_Config.audio_info = new CMP4MUX_AUDIO_INFO;
		memcpy(_Config.audio_info,pConfig->audio_info, sizeof(CMP4MUX_AUDIO_INFO));
	}

	if (pConfig->record_info) {
		_Config.record_info = new CMP4MUX_RECORD_INFO;
		memcpy(_Config.record_info,pConfig->record_info, sizeof(CMP4MUX_RECORD_INFO));
	}
	return ME_OK;
}

AM_ERR
CMp4Muxer::SetOutputFile (const char *pFileName) 
{
	if (pFileName == NULL)
		return ME_ERROR;

	strcpy (_Config.dest_name, pFileName);
	AM_INFO("_Config.dest_name = %s", _Config.dest_name);
	return ME_OK;
}

void 
CMp4Muxer::OnRun()
{
	AM_ERR ret = ME_OK;
	AM_ERR put_data_ret = ME_OK;

	CBuffer *pBuffer;
	CQueueInputPin *pInputPin;
	bool bVideoEOS, bAudioEOS;

	mbRunFlag = true;
	bVideoEOS = bAudioEOS = false;
	CmdAck(ME_OK);

	AM_INFO("CMp4Muxer begin to run.\n");
	
	while (true)
	{

		if(!WaitInputBuffer(pInputPin, pBuffer)) {
			AM_ERROR("WaitInputBuffer failed when get MSG.\n");
			break;
		}

		if (!mbRunFlag) {
			pBuffer->Release();
			break;
		}

		if (pBuffer->GetType () == CBuffer::INFO) {
			if (pInputPin == _pVideo) {
				AM_INFO("Video INFO buffer received in mp4 muxer.\n");
				iav_h264_config_t *_p_config = (iav_h264_config_t *)pBuffer->mpData;

				AM_INFO("Video's width : %d", _p_config->pic_info.width);
				AM_INFO("Video's height: %d", _p_config->pic_info.height);
				AM_INFO("Video's rate  : %d", _p_config->pic_info.rate);
				AM_INFO("Video's scale : %d", _p_config->pic_info.scale);

				if ((ret = CCreateMp4::set_h264_config (_p_config)) != ME_OK) {
					AM_INFO("Memory is not enough.\n");
					pBuffer->Release();
					break;
				}
				
			}			
		} else if (pBuffer->GetType() == CBuffer::DATA) {
			if (pInputPin == _pVideo) { //video iput
				if (_pCreateMp4 == NULL) { //&&
					//pBuffer->IsSyncPoint()) {
					//AM_INFO(">>Record stream to %s_MMDDHHmmSS.mp4\n", _Config.dest_name);
					_pCreateMp4 = new CCreateMp4(_Config.dest_name, 0);
					ret = _pCreateMp4->Init(_Config.h264_info,_Config.audio_info, _Config.record_info);
					if (ret != ME_OK) {
						AM_PERROR("Mp4 muxer Init failed.\n");
						pBuffer->Release();
						break;
					}
				}
				if (_pCreateMp4) {
					AM_INFO("Video: frame_num = %d, Size is %d\n", 
							pBuffer->mSeqNum, pBuffer->mDataSize);				
					put_data_ret = _pCreateMp4->put_VideoData(pBuffer->mpData,
									pBuffer->mDataSize, pBuffer->mPTS);
					if (put_data_ret == ME_TOO_MANY) {
						_pCreateMp4->Finish();
						//delete _pCreateMp4[i];
						_pCreateMp4 = NULL;
						AM_PRINTF("Reach file limit\n");
					} else if (put_data_ret != ME_OK) {
						delete _pCreateMp4;
						_pCreateMp4 = NULL;
						pBuffer->Release();
						AM_PERROR("put_VideoData failed.\n");
						ret = put_data_ret;
						break;
					}
				}
			} else { //audio input
				//for (int j = 0; j<MAX_ENCODE_STREAM_NUM; j++) {
				if (_pCreateMp4) {
					AM_INFO("Audio: size is %d, mDataSize = %d\n", pBuffer->GetDataSize(), pBuffer->mDataSize);	
					put_data_ret = _pCreateMp4->put_AudioData(pBuffer->mpData, pBuffer->mDataSize);
					if (put_data_ret == ME_TOO_MANY) {
						_pCreateMp4->Finish();
						//delete _pCreateMp4[j];
						_pCreateMp4 = NULL;
						AM_PRINTF("Reach file limit\n");
					} else if ( put_data_ret != ME_OK) {
						delete _pCreateMp4;
						_pCreateMp4 = NULL;
						pBuffer->Release();
						AM_PERROR("put_AudioData\n");
						ret = put_data_ret;
						break;
					}
				}
			}
		}else if (pBuffer->GetType() == CBuffer::EOS) {
			if (pInputPin == _pVideo) { 
				bVideoEOS = true;
			} else if (pInputPin == _pAudio) {
				bAudioEOS = true;
			}

			if (bVideoEOS && bAudioEOS) {
				AM_INFO("EOSes for video and audio have been received.\n");
				if (_pCreateMp4) {
					_pCreateMp4->Finish();
					PostEngineMsg(IEngine::MSG_EOS);
				}
				pBuffer->Release();
				break;
			}
		}
		pBuffer->Release();
	}
}

