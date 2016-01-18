/*******************************************************************************
 * do_ioctl.c
 *
 * History:
 *  Mar 21, 2014 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <assert.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#include "lib_vproc.h"
#include "priv.h"

static const char* VIDEO_PROC_STR[IAV_VIDEO_PROC_NUM] = {
	"DPTZ", "PM", "CAWARP", "STREAM_PM", "STREAM_OFFSET",
	"STREAM_OVERLAY"
};

static int G_iav_fd = -1;

static void open_iav(void)
{
	if (unlikely(G_iav_fd < 0)) {
		if ((G_iav_fd = open("/dev/iav", O_RDWR, 0)) < 0) {
			perror("/dev/iav");
			exit(-1);
		}
		TRACE("Open /dev/iav\n");
	}
}

int ioctl_map_pm(void)
{
	static int mapped = 0;
	open_iav();
	if (unlikely(!mapped)) {
		if (ioctl(G_iav_fd, IAV_IOC_MAP_PRIVACY_MASK_EX, &vproc.pm_mem) < 0) {
			perror("IAV_IOC_MAP_PRIVACY_MASK_EX");
			return -1;
		}
		mapped = 1;
		TRACE("IAV_IOC_MAP_PRIVACY_MASK_EX\n");
	}
	return 0;
}

int ioctl_map_overlay(void)
{
	static int mapped = 0;
	open_iav();
	if (unlikely(!mapped)) {
		if (ioctl(G_iav_fd, IAV_IOC_MAP_OVERLAY, &vproc.overlay_mem) < 0) {
			perror("IAV_IOC_MAP_OVERLAY");
			return -1;
		}
		mapped = 1;
		TRACE("IAV_IOC_MAP_OVERLAY\n");
	}
	return 0;
}

int ioctl_get_pm_info(void)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_PRIVACY_MASK_INFO_EX, &vproc.pm_info) < 0) {
		perror("IAV_IOC_GET_PRIVACY_MASK_INFO_EX");
		return -1;
	}
	TRACE("IAV_IOC_GET_PRIVACY_MASK_INFO_EX\n");
	return 0;
}

int ioctl_get_srcbuf_format(int source_id)
{
	open_iav();
	vproc.srcbuf[source_id].source = source_id;
	if (ioctl(G_iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX,
		&vproc.srcbuf[source_id]) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX");
		return -1;
	}
	TRACE("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_EX\n");
	return 0;
}

int ioctl_get_srcbuf_setup(void)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &vproc.srcbuf_setup) < 0) {
		perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
		return -1;
	}
	TRACE("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX\n");
	return 0;
}

int ioctl_get_vin_info(void)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vproc.vin) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
		return -1;
	}
	TRACE("IAV_IOC_VIN_SRC_GET_VIDEO_INFO\n");
	return 0;
}

int ioctl_get_stream_format(int stream_id)
{
	open_iav();
	vproc.stream[stream_id].id = (1 << stream_id);
	if (ioctl(G_iav_fd, IAV_IOC_GET_ENCODE_FORMAT_EX,
		&vproc.stream[stream_id]) < 0) {
		perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
		return -1;
	}
	TRACE("IAV_IOC_GET_ENCODE_FORMAT_EX\n");
	return 0;
}

int ioctl_gdma_copy(iav_gdma_copy_ex_t* gdma)
{
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GDMA_COPY_EX, gdma) < 0) {
		perror("IAV_IOC_GDMA_COPY_EX");
		return -1;
	}
	TRACE("IAV_IOC_GDMA_COPY_EX\n");
	return 0;
}

int ioctl_get_system_resource(void)
{
	open_iav();
	vproc.resource.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(G_iav_fd, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &vproc.resource) < 0) {
		perror("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX");
		return -1;
	}
	TRACE("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX\n");
	return 0;
}

int ioctl_get_pm(void)
{
	iav_video_proc_t* vp = &vproc.ioctl_pm;
	vp->cid = IAV_PM;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s\n", VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_get_dptz(int source_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_dptz[source_id];
	vp->cid = IAV_DPTZ;
	vp->arg.dptz.source = source_id;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], source_id);
	return 0;
}

int ioctl_get_stream_pm(int stream_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_streampm[stream_id];
	vp->cid = IAV_STREAM_PM;
	vp->arg.stream_pm.id = 1 << stream_id;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], stream_id);
	return 0;
}

int ioctl_get_stream_offset(int stream_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_streamofs[stream_id];
	vp->cid = IAV_STREAM_OFFSET;
	vp->arg.stream_offset.id = 1 << stream_id;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s %d\n",VIDEO_PROC_STR[vp->cid],  stream_id);
	return 0;
}

int ioctl_get_stream_overlay(int stream_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_overlay[stream_id];
	vp->cid = IAV_STREAM_OVERLAY;
	vp->arg.stream_overlay.id = 1 << stream_id;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_GET_VIDEO_PROC, vp) < 0) {
		perror("IAV_IOC_GET_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_GET_VIDEO_PROC: %s %d\n",VIDEO_PROC_STR[vp->cid],  stream_id);
	return 0;
}

int ioctl_cfg_pm(void)
{
	iav_video_proc_t* vp = &vproc.ioctl_pm;
	vp->cid = IAV_PM;
	vproc.apply_flag[IAV_PM].apply = 1;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_PM].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s\n",VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_cfg_cawarp(void)
{
	iav_video_proc_t* vp = &vproc.ioctl_cawarp;
	vp->cid = IAV_CAWARP;
	vproc.apply_flag[IAV_CAWARP].apply = 1;
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_CAWARP].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s\n", VIDEO_PROC_STR[vp->cid]);
	return 0;
}

int ioctl_cfg_dptz(int source_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_dptz[source_id];
	vp->cid = IAV_DPTZ;
	vp->arg.dptz.source = source_id;
	vproc.apply_flag[IAV_DPTZ].apply = 1;
	vproc.apply_flag[IAV_DPTZ].param |= (1 << source_id);
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_DPTZ].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], source_id);
	return 0;
}

int ioctl_cfg_stream_pm(u32 streams)
{
	open_iav();

	int i;
	iav_video_proc_t* vp;

	for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; i++) {
		if (streams & (1 << i)) {
			vp = &vproc.ioctl_streampm[i];
			vp->cid = IAV_STREAM_PM;
			vp->arg.stream_pm.id = 1 << i;
			vproc.apply_flag[IAV_STREAM_PM].apply = 1;
			vproc.apply_flag[IAV_STREAM_PM].param |= (1 << i);
			if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
				vproc.apply_flag[IAV_STREAM_PM].apply = 0;
				perror("IAV_IOC_CFG_VIDEO_PROC");
				return -1;
			}
			TRACE("IAV_IOC_CFG_VIDEO_PROC: %s %d \n", VIDEO_PROC_STR[vp->cid],
			    i);
		}
	}

	return 0;
}

int ioctl_cfg_stream_offset(int stream_id)
{
	iav_video_proc_t* vp = &vproc.ioctl_streamofs[stream_id];
	vp->cid = IAV_STREAM_OFFSET;
	vp->arg.stream_offset.id = 1 << stream_id;
	vproc.apply_flag[IAV_STREAM_OFFSET].apply = 1;
	vproc.apply_flag[IAV_STREAM_OFFSET].param |= (1 << stream_id);
	open_iav();
	if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
		vproc.apply_flag[IAV_STREAM_OFFSET].apply = 0;
		perror("IAV_IOC_CFG_VIDEO_PROC");
		return -1;
	}
	TRACE("IAV_IOC_CFG_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid], stream_id);
	return 0;
}

int ioctl_cfg_stream_overlay(u32 streams)
{
	open_iav();

	int i, j;
	iav_video_proc_t* vp;

	for (i = 0; i < OVERLAY_MAX_STREAM_NUM; i++) {
		if (streams & (1 << i)) {
			vp = &vproc.ioctl_overlay[i];
			vp->arg.stream_overlay.enable = 0;
			for (j = 0; j < MAX_NUM_OVERLAY_AREA; j++) {
				if (vp->arg.stream_overlay.area[j].enable) {
					vp->arg.stream_overlay.enable = 1;
					break;
				}
			}

			vp->cid = IAV_STREAM_OVERLAY;
			vp->arg.stream_overlay.id = 1 << i;
			vproc.apply_flag[IAV_STREAM_OVERLAY].apply = 1;
			vproc.apply_flag[IAV_STREAM_OVERLAY].param |= (1 << i);

			if (ioctl(G_iav_fd, IAV_IOC_CFG_VIDEO_PROC, vp) < 0) {
				vproc.apply_flag[IAV_STREAM_OVERLAY].apply = 0;
				perror("IAV_IOC_CFG_VIDEO_PROC");
				return -1;
			}
			TRACE("IAV_IOC_CFG_VIDEO_PROC: %s %d\n", VIDEO_PROC_STR[vp->cid],
			    i);
		}
	}

	return 0;
}

int ioctl_apply(void)
{
	int i;
	int do_apply = 0;
	for (i = 0; i < IAV_VIDEO_PROC_NUM; i++) {
		if (vproc.apply_flag[i].apply) {
			do_apply = 1;
		}
		DEBUG("%s:\tapply %d, params 0x%x\n", VIDEO_PROC_STR[i],
		    vproc.apply_flag[i].apply, vproc.apply_flag[i].param);
	}
	if (do_apply) {
		open_iav();
		if (ioctl(G_iav_fd, IAV_IOC_APPLY_VIDEO_PROC, vproc.apply_flag) < 0) {
				perror("IAV_IOC_APPLY_VIDEO_PROC");
				return -1;
			}
			TRACE("IAV_IOC_APPLY_VIDEO_PROC\n");
	}
	// Clear all apply flags
	memset(vproc.apply_flag, 0, sizeof(vproc.apply_flag));
	return 0;
}
