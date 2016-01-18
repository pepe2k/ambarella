
/**
 * am_camera.cpp
 *
 * History:
 *    2010/1/6 - [Oliver Li] created file
 *
 * Copyright (C) 2009-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "am_camara"
#define AMDROID_DEBUG

#include <stdio.h>

#include "am_new.h"
#include "am_types.h"
#include "am_if.h"
#include "camera_if.h"
#include "am_camera.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>

#include <basetypes.h>
#include "iav_drv.h"

#include "ambas_common.h"
#include "ambas_vin.h"

extern ICamera* CreateAmbaCamera()
{
	return (ICamera*)CCamera::Create();
}

//-----------------------------------------------------------------------
//
// CCamera
//
//-----------------------------------------------------------------------
ICamera* CCamera::Create()
{
	CCamera *result = new CCamera();
	if (result && result->Construct() != ME_OK) {
		delete result;
		result = NULL;
	}
	return result;
}

AM_ERR CCamera::Construct()
{
	if ((mFd = ::open("/dev/iav", O_RDWR, 0)) < 0) {
		AM_PERROR("/dev/iav");
		return ME_ERROR;
	}

	return ME_OK;
}

CCamera::~CCamera()
{
	if (mFd >= 0)
		::close(mFd);
}

void CCamera::Delete()
{
	delete this;
}

void *CCamera::GetInterface(AM_REFIID refiid)
{
	if (refiid == IID_ICamera)
		return (ICamera*)this;
	if (refiid == IID_IInterface)
		return (IInterface*)this;
	return NULL;
}

int CCamera::GetDevFd()
{
	return mFd;
}

AM_ERR CCamera::StartPreview()
{
	if (::ioctl(mFd, IAV_IOC_ENTER_IDLE, 0) < 0) {
		AM_PERROR("IAV_IOC_ENTER_IDLE");
		return ME_ERROR;
	}

	int source = 0;
	if (::ioctl(mFd, IAV_IOC_VIN_SET_CURRENT_SRC, &source) < 0) {
		AM_PERROR("IAV_IOC_VIN_SET_CURRENT_SRC");
		return ME_ERROR;
	}

	int mode = 0;
	if (::ioctl(mFd, IAV_IOC_VIN_SRC_SET_VIDEO_MODE, mode) < 0) {
		AM_PERROR("IAV_IOC_VIN_SRC_SET_VIDEO_MODE");
		return ME_ERROR;
	}

	{ //set_vin_param,is used to set 3A data, only for sensor that output RGB raw data
		int vin_eshutter_time = 60;	// 1/60 sec
		int vin_agc_db = 6;	// 0dB

		u32 shutter_time_q9;
		s32 agc_db;
		shutter_time_q9 = 512000000/vin_eshutter_time;
		agc_db = vin_agc_db<<24;

		if (::ioctl(mFd, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME, shutter_time_q9) < 0) {
			AM_PERROR("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
			return ME_ERROR;
		}

		if (::ioctl(mFd, IAV_IOC_VIN_SRC_SET_AGC_DB, agc_db) < 0) {
			AM_PERROR("IAV_IOC_VIN_SRC_SET_AGC_DB");
			return ME_ERROR;
		}
	}

	//We have to set encode format before enter preview because of UCode Limitaion
	//temp set encode format magic number here for authortest&rectest, for next, should set it in am_CameraHal for android, TBD 
	iav_encode_format_t format;
	::memset(&format, 0, sizeof(format));
	format.encode_type = IAV_ENCODE_H264;
	format.encode_width = 1280; // this number should be equal to FFMepgMuxer video_enc->width
	format.encode_height = 720;

	if (::ioctl(mFd, IAV_IOC_SET_ENCODE_FORMAT, &format) < 0) {
		AM_INFO("IAV_IOC_SET_ENCODE_FORMAT");
		return ME_ERROR;
	}

	if (::ioctl(mFd, IAV_IOC_GET_ENCODE_FORMAT, &format) < 0) {
		AM_INFO("IAV_IOC_SET_ENCODE_FORMAT");
		return ME_ERROR;
	}

	if (::ioctl(mFd, IAV_IOC_ENABLE_PREVIEW, 0) < 0) {
		AM_PERROR("IAV_IOC_ENABLE_PREVIEW");
		return ME_ERROR;
	}

	return ME_OK;
}

void CCamera::StopPreview()
{
	if (::ioctl(mFd, IAV_IOC_ENTER_IDLE, 0) < 0) {
		AM_PERROR("IAV_IOC_ENTER_IDLE");
	}
	if (::ioctl(mFd, IAV_IOC_START_DECODE, 0) < 0) {
		AM_PERROR("IAV_IOC_START_DECODE");
	}
}

AM_ERR CCamera::StartRecording()
{
	return ME_OK;
}

void CCamera::StopRecording()
{
}

