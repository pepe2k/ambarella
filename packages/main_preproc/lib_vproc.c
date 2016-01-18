/*******************************************************************************
 * lib_vproc.c
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "iav_encode_drv.h"

#include "lib_vproc.h"
#include "priv.h"

extern version_t G_version;
vproc_param_t vproc;

static void init_vp_param(void)
{
	static int inited = 0;
	int i;
	if (unlikely(!inited)) {
		memset(&vproc, 0, sizeof(vproc));

		vproc.pm = &vproc.ioctl_pm.arg.pm;
		vproc.cawarp = &vproc.ioctl_cawarp.arg.cawarp;

		for (i = 0; i < IAV_ENCODE_SOURCE_TOTAL_NUM; i++) {
			vproc.dptz[i] = &vproc.ioctl_dptz[i].arg.dptz;
		}

		for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; i++) {
			vproc.streampm[i] = &vproc.ioctl_streampm[i].arg.stream_pm;
			vproc.streamofs[i] = &vproc.ioctl_streamofs[i].arg.stream_offset;
			vproc.overlay[i] = &vproc.ioctl_overlay[i].arg.stream_overlay;
		}
		inited = 1;
		DEBUG("done\n");
	}
}

int vproc_pm(pm_param_t* mask_in_main)
{

	init_vp_param();

	int ret = -1;
	OP op = mask_in_main->enable ? OP_ADD : OP_REMOVE;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = operate_mbmain_pm(op, mask_in_main->id, &mask_in_main->rect);
			break;
		case PM_PIXEL:
			ret = operate_pixel_pm(op, mask_in_main->id, &mask_in_main->rect);
			break;
		case PM_PIXELRAW:
			ret = operate_pixelraw_pm(op, mask_in_main->id, &mask_in_main->rect);
			break;
		case PM_MB_STREAM:
			ret = operate_mbstream_pmoverlay(op, mask_in_main->id,
			    &mask_in_main->rect);
			break;
		default:
			ERROR("Privacy Mask is not supported.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_pm_color(yuv_color_t* mask_color)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = operate_mbmain_color(mask_color);
			break;
		case PM_MB_STREAM:
			ret = operate_mbstream_color(mask_color);
			break;
		default:
			ERROR("Privacy Mask color is not supported in current encode "
				"mode.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_mctf(mctf_param_t* mctf)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = operate_mbmain_mctf(mctf);
			break;
		case PM_MB_STREAM:
		case PM_PIXEL:
		case PM_PIXELRAW:
			ERROR("MCTF strength is not supported in current encode "
				"mode.\n");
			break;
		default:
			ERROR("MCTF strength is not supported.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_dptz(iav_digital_zoom_ex_t* dptz)
{
	init_vp_param();

	int ret = -1;

	switch(get_pm_type()) {
		case PM_MB_MAIN:
			if (dptz->source == IAV_ENCODE_SOURCE_MAIN_BUFFER) {
				ret = (operate_dptz(dptz)
					|| operate_mbmain_pm(OP_UPDATE, -1, NULL)
					|| operate_cawarp(NULL));
			} else {
				ret = operate_dptz(dptz);
			}
			break;
		case PM_PIXEL:
		case PM_PIXELRAW:
			ret = (operate_dptz(dptz)
			    || (dptz->source == IAV_ENCODE_SOURCE_MAIN_BUFFER
			        && operate_cawarp(NULL )));
			break;
		case PM_MB_STREAM:
			if (dptz->source == IAV_ENCODE_SOURCE_MAIN_BUFFER) {
				ret = (operate_dptz(dptz)
					|| operate_cawarp(NULL)
					|| operate_mbstream_pmoverlay(OP_UPDATE, -1, NULL));
			} else {
				ret = (operate_dptz(dptz)
					|| operate_mbstream_pmoverlay(OP_UPDATE, -1, NULL));
			}
			break;
		default:
			ERROR("DPTZ is not supported.\n");
			break;
	}
	return ret ? -1 : ioctl_apply();
}

int vproc_cawarp(cawarp_param_t* cawarp)
{
	init_vp_param();

	int ret = operate_cawarp(cawarp);
	return ret ? -1 : ioctl_apply();
}

int vproc_stream_overlay(overlay_param_t* overlay)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_STREAM:
			ret = operate_mbstream_overlaypm(
			    overlay->enable ? OP_ADD : OP_REMOVE, overlay->stream_id,
			    overlay);
			break;
		case PM_MB_MAIN:
		case PM_PIXEL:
		case PM_PIXELRAW:
			ret = operate_overlay(overlay);
			break;
		default:
			ERROR("Stream overlay is not supported.\n");
			break;
	}
	return ret ? -1 : ioctl_apply();
}

int vproc_stream_offset(offset_param_t* offset)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_STREAM:
			ret = (operate_encofs(offset)
			    || operate_mbstream_overlaypm(OP_UPDATE, offset->stream_id,
			        NULL ));
			break;
		case PM_MB_MAIN:
			case PM_PIXEL:
			case PM_PIXELRAW:
			ret = operate_encofs(offset);
			break;
		default:
			ERROR("Stream offset is not supported.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_stream_prepare(int stream_id)
{
	init_vp_param();

	int ret = -1;

	switch(get_pm_type()) {
		case PM_MB_STREAM:
			ret = operate_mbstream_overlaypm(OP_UPDATE, stream_id, NULL);
			break;
		case PM_MB_MAIN:
		case PM_PIXEL:
		case PM_PIXELRAW:
			ret = 0;
			break;
		default:
			ERROR("Stream preparation is not supported.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_exit(void)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_STREAM:
			ret = disable_stream_pm();
			break;
		case PM_MB_MAIN:
		case PM_PIXEL:
		case PM_PIXELRAW:
			ret = disable_pm();
			break;
		default:
			ERROR("Unknown type\n");
			break;
	}
	ret |= disable_overlay();
	if (!ret) {
		ret = (ioctl_apply() || deinit_mbmain() || deinit_mbstream()
		    || deinit_pixel() || deinit_pixelraw() || deinit_cawarp()
		    || deinit_overlay() || deinit_dptz() || deinit_encofs());
	}
	return ret ? -1 : 0;
}

int vproc_version(version_t* version)
{
	init_vp_param();

	memcpy(version, &G_version, sizeof(version_t));
	return 0;
}
