/**********************************************************************
 *
 * mw_image.c
 *
 * History:
 *	2010/02/28 - [Jian Tang] Created this file
 *	2011/06/20 - [Jian Tang] Modified this file
 *
 * Copyright (C) 2007 - 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include <pthread.h>

#include "ambas_common.h"
#include "iav_drv.h"
#include "iav_drv_ex.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
#include "ambas_imgproc_arch.h"

#include "img_struct_arch.h"
#include "img_api_arch.h"
#include "img_dsp_interface_arch.h"

#include "img_hdr_api_arch.h"
#include "mw_aaa_params.h"
#include "mw_api.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"
#include "mw_dc_iris.h"
#include "mw_piris.h"

int G_aaa_is_enabled = 0;
u32 g_is_rgb_sensor_vin = 0;
int fd_iav_aaa = -1;
img_config_info_t G_img_config_info;
mw_sensor_param G_sensor_param;

_mw_af_info				G_mw_af;

pthread_t nl_thread;
int netlink_exit_flag = 0;

/* ***************************************************************
	Exposure Control Settings
******************************************************************/

mw_ae_param			G_mw_ae_param = {
	.anti_flicker_mode		= MW_ANTI_FLICKER_60HZ,
	.shutter_time_min		= SHUTTER_1BY8000_SEC,
	.shutter_time_max		= SHUTTER_1BY60_SEC,
	.sensor_gain_max		= ISO_6400,
	.slow_shutter_enable	= 0,
	.current_vin_fps		= AMBA_VIDEO_FPS_60,
};

mw_aperture_param	G_lens_aperture = {
		.aperture_min		= APERTURE_AUTO,
		.aperture_max		= APERTURE_AUTO,
};

u32						G_ae_preference = 0;
int						G_exposure_level[MAX_HDR_EXPOSURE_NUM];
u32						G_dc_iris_control_enable = 0;
u32						G_p_iris_control_enable = 0;
u32						G_auto_contrast_enable = 0;
u32						G_auto_wdr_str = 0;
u32						G_iris_type = MW_DC_IRIS;

mw_ae_metering_mode		G_ae_metering_mode = MW_AE_CENTER_METERING;
mw_ae_metering_table		G_ae_metering_table = {
	.metering_weight		= {			//AE_METER_CENTER
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1,
			1, 1, 1, 1, 2, 3, 3, 2, 1, 1, 1, 1,
			1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
			1, 1, 1, 2, 3, 5, 5, 3, 2, 1, 1, 1,
			1, 1, 2, 3, 4, 5, 5, 4, 3, 2, 1, 1,
			1, 2, 3, 4, 4, 4, 4, 4, 4, 3, 2, 1,
			2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 3, 2,
	},
};

/* ***************************************************************
	White Balance Control Settings
******************************************************************/
mw_awb_param			G_mw_awb;
mw_white_balance_method	G_white_balance_method = MW_WB_NORMAL_METHOD;
mw_white_balance_mode	G_white_balance_mode = MW_WB_AUTO;

/* ***************************************************************
	Image Adjustment Settings
******************************************************************/
mw_image_param			G_mw_image = {
	.saturation = 64,
	.brightness = 0,
	.hue = 0,
	.contrast = 64,
	.sharpness = 6,
};

/* ***************************************************************
	Image Enhancement Settings
******************************************************************/
u32						G_mctf_strength = 1;
u32						G_auto_local_exposure_mode = 0;
u32						G_backlight_compensation_enable = 0;
u32						G_day_night_mode_enable = 0;
u32						G_chroma_noise_filter_strength = 64;


/*************************************************
 *
 *		Public Functions, for external use
 *
 *************************************************/

int mw_load_calibration_file(mw_cali_file * pFile)
{
	if (pFile == NULL) {
		MW_ERROR("mw_load_calibration_file pointer is NULL!\n");
		return -1;
	}
	memset(&G_mw_cali, 0, sizeof(G_mw_cali));
	if (strlen(pFile->bad_pixel) > 0) {
		strncpy(G_mw_cali.bad_pixel, pFile->bad_pixel,
			sizeof(G_mw_cali.bad_pixel));
	}
	if (strlen(pFile->wb) > 0) {
		strncpy(G_mw_cali.wb, pFile->wb, sizeof(G_mw_cali.wb));
	}
	if (strlen(pFile->shading) > 0) {
		strncpy(G_mw_cali.shading, pFile->shading, sizeof(G_mw_cali.shading));
	}
	return 0;
}

int mw_set_sharpen_filter(int sharpen_b)
{
	iav_system_resource_setup_ex_t resource;

	if (sharpen_b > 1 || sharpen_b < 0) {
		MW_ERROR("sharpen_b must be [0|1] \n");
		return -1;
	}
	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(fd_iav_aaa, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0) {
		MW_ERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX error");
		return -1;
	}
	resource.sharpen_b_enable = sharpen_b;

	if (ioctl(fd_iav_aaa, IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0) {
		MW_ERROR("IAV_IOC_SET_SYSTEM_RESOURCE_LIMIT_EX error");
		return -1;
	}
	MW_INFO("=== sharpen B [%d].\n", sharpen_b);
	return 0;
}

int mw_get_sharpen_filter(void)
{
	iav_system_resource_setup_ex_t resource;
	memset(&resource, 0, sizeof(resource));
	resource.encode_mode = IAV_ENCODE_CURRENT_MODE;
	if (ioctl(fd_iav_aaa, IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX, &resource) < 0) {
		MW_ERROR("IAV_IOC_GET_SYSTEM_RESOURCE_LIMIT_EX error");
		return -1;
	}
	MW_INFO("=== sharpen B [%d].\n", resource.sharpen_b_enable);
	return resource.sharpen_b_enable;
}

int mw_get_iav_state(int fd_iav, int *pState)
{
	iav_state_info_t info;

	if(ioctl(fd_iav, IAV_IOC_GET_STATE_INFO, &info) < 0) {
		perror("IAV_IOC_GET_STATE_INFO");
		return -1;
	}

	*pState = info.state;
	return 0;
}

int mw_get_sensor_param_for_3A(mw_sensor_param *pHdr_param)
{
	if (pHdr_param == NULL) {
		MW_ERROR("The point is null\n\n");
		return -1;
	}
	pHdr_param->hdr_expo_num = G_img_config_info.expo_num;
	if (pHdr_param->hdr_expo_num > MIN_HDR_EXPOSURE_NUM) {
		pHdr_param->hdr_shutter_mode = 1;
	} else {
		pHdr_param->hdr_shutter_mode = 0;
	}

	pHdr_param->step = G_sensor_param.step;
	pHdr_param->max_g_db = G_sensor_param.max_g_db;
	pHdr_param->max_ag_db = G_sensor_param.max_ag_db;
	pHdr_param->max_dg_db = G_sensor_param.max_dg_db;

	return 0;
}

int mw_start_aaa(int fd_iav)
{
	if (G_aaa_is_enabled == 1) {
		MW_MSG("Already start!\n");
		return -1;
	}

	if (check_aaa_state(fd_iav) < 0) {
		MW_MSG("check_aaa_state error\n");
		return -1;
	}

	init_netlink();
	pthread_create(&nl_thread, NULL, (void *)netlink_loop, (void *)NULL);

	if (start_aaa_task() < 0) {
		MW_ERROR("mw_start_aaa_internal \n");
		return -1;
	}

	G_aaa_is_enabled = 1;

	return 0;
}

int mw_stop_aaa(void)
{
	if (G_aaa_is_enabled == 0) {
		MW_INFO("3A is not enabled. Quit silently.\n");
		return 0;
	}

	if (send_image_msg_stop_aaa() < 0) {
		MW_ERROR("stop_aaa_task\n");
		return -1;
	}

	netlink_exit_flag  = 1;
	pthread_join(nl_thread, NULL);
	nl_thread = -1;

	G_aaa_is_enabled = 0;

	return 0;
}

int mw_init_mctf(int fd_iav, mw_mctf_info_t *mctf_info)
{
	if (mctf_info == NULL) {
		MW_ERROR("The point is NULL!\n");
		return -1;
	}

	if (G_aaa_is_enabled == 1) {
		MW_MSG("Already start!\n");
		return -1;
	}

	if (check_aaa_state(fd_iav) < 0) {
		MW_MSG("check_aaa_state error\n");
		return -1;
	}

	if (img_lib_init(0, mw_get_sharpen_filter()) < 0) {
		MW_ERROR("/dev/iav device!\n");
		return -1;
	}

	if (load_mctf_bin() < 0) {
		MW_ERROR("load_mctf_bin error!\n");
		return -1;
	}

	if (img_dsp_set_video_mctf_enable(2) < 0) {
		MW_ERROR("Failed to img_dsp_set_video_mctf_enable!\n");
		return -1;
	}

	if (img_dsp_set_video_mctf_compression_enable(1) < 0) {
		MW_ERROR("Failed to img_dsp_set_video_mctf_enable!\n");
		return -1;
	}
	if (img_dsp_init_video_mctf(fd_iav_aaa) < 0) {
		MW_ERROR("Failed to img_dsp_init_video_mctf!\n");
		return -1;
	}

	if (img_dsp_set_video_mctf(fd_iav_aaa, (video_mctf_info_t *)mctf_info) < 0) {
		MW_ERROR("Failed to img_dsp_set_video_mctf!\n");
		return -1;
	}

	if (img_lib_deinit() < 0) {
		MW_ERROR("img_lib_deinit error!\n");
		return -1;
	}

	MW_MSG("======= [DONE] init mctf ======= \n");

	return 0;
}

int mw_get_iq_preference(int * pPrefer)
{
	if (pPrefer == NULL) {
		MW_ERROR("mw_get_iq_preference pointer is NULL!\n");
		return -1;
	}
	*pPrefer = G_ae_preference;
	return 0;
}

int mw_set_iq_preference(int prefer)
{
	G_ae_preference = prefer;
	return 0;
}

int mw_get_af_param(mw_af_param * pAF)
{
	if (pAF == NULL) {
		MW_MSG("mw_get_af_param pointer is NULL!\n");
		return -1;
	}
	pAF->lens_type = G_mw_af.lens_type;
	pAF->af_mode = G_mw_af.af_mode;
	pAF->af_tile_mode = G_mw_af.af_tile_mode;
	pAF->zm_dist = G_mw_af.zm_idx;
	if (G_mw_af.fs_idx == 0)
		G_mw_af.fs_idx = 1;
	pAF->fs_dist = FOCUS_INFINITY / G_mw_af.fs_idx;
	if (G_mw_af.fs_near == 0)
		G_mw_af.fs_near = 1;
	pAF->fs_near = FOCUS_INFINITY / G_mw_af.fs_near;
	if (G_mw_af.fs_far == 0)
		G_mw_af.fs_far = 1;
	pAF->fs_far = FOCUS_INFINITY / G_mw_af.fs_far;
	return 0;
}

int mw_set_af_param(mw_af_param * pAF)
{
	static af_range_t af_range;
	int focus_idx = 0;

	if (pAF == NULL) {
		MW_MSG("mw_set_af_param pointer is NULL!\n");
		return -1;
	}
	if (g_is_rgb_sensor_vin == 0)
		return 0;

	// fixed lens ID, disable all af function
	G_mw_af.lens_type = pAF->lens_type;
	if (G_mw_af.lens_type == LENS_CMOUNT_ID) {
		return 0;
	} else {
		if (img_config_lens_info(G_mw_af.lens_type) < 0) {
			MW_ERROR("img_config_lens_info error!\n");
			return -1;
		}
	}

	if (check_af_params(pAF) < 0) {
		MW_ERROR("check_af_params error\n");
		return -1;
	}

	// set af mode (CAF, SAF, MF, CALIB)
	if (G_mw_af.af_mode != pAF->af_mode) {
		if (img_af_set_mode(pAF->af_mode) < 0) {
			MW_ERROR("img_af_set_mode error\n");
			return -1;
		}
		G_mw_af.af_mode = pAF->af_mode;
	}

	// set af tile mode
	if (G_mw_af.af_tile_mode != pAF->af_tile_mode) {
		G_mw_af.af_tile_mode = pAF->af_tile_mode;
	}

	// set zoom index
	if (G_mw_af.zm_idx != pAF->zm_dist) {
		if (img_af_set_zoom_idx(pAF->zm_dist) < 0) {
			MW_ERROR("img_af_set_zoom_idx error\n");
			return -1;
		}
		G_mw_af.zm_idx = pAF->zm_dist;
	}

	// set focus index
	if (pAF->af_mode == MANUAL) {
		focus_idx = FOCUS_INFINITY / pAF->fs_dist;
		if (G_mw_af.fs_idx != focus_idx) {
			if (img_af_set_focus_idx(focus_idx) < 0) {
				MW_ERROR("img_af_set_focus_idx error\n");
				return -1;
			}
			G_mw_af.fs_idx = focus_idx;
		}
	} else {
		af_range.near_bd = FOCUS_INFINITY / pAF->fs_near;
		af_range.far_bd = FOCUS_INFINITY / pAF->fs_far;
		if ((G_mw_af.fs_near != af_range.near_bd) ||
			(G_mw_af.fs_far != af_range.far_bd)) {
			if (img_af_set_range(&af_range) < 0) {
				MW_ERROR("img_af_set_range error\n");
				return -1;
			}
			G_mw_af.fs_near = af_range.near_bd;
			G_mw_af.fs_far = af_range.far_bd;
		}
	}

	MW_MSG("[DONE] mw_set_af_param");
	return 0;
}

int mw_get_image_statistics(mw_image_stat_info * pInfo)
{
	if (pInfo == NULL) {
		MW_MSG("mw_get_image_statistics pointer is NULL!\n");
		return -1;
	}

	int agc;
	u32 shutter;

	if (g_is_rgb_sensor_vin == 0) {
		agc = 0;
		shutter = Q9_BASE;
	} else {
		agc = img_ae_get_system_gain();
		get_shutter_time(&shutter);
	}
	pInfo->agc = agc * 1024 * 6 / 128;		//expressed in db x 1024
	pInfo->shutter = shutter;

	return 0;
}

/* ***************************************************************
	Exposure Control Settings
******************************************************************/

int mw_enable_ae(int enable)
{
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_enable_ae(enable);
	} else {
		img_enable_ae(enable);
		if (enable) {
			if (load_ae_exp_lines(&G_mw_ae_param) < 0) {
				MW_ERROR("load ae exp lines error!\n");
				return -1;
			}
		}
	}

	return 0;
}

int mw_set_ae_param(mw_ae_param *pAe_param)
{
	static int first_set_ae_parma_flag = 1;
	if (pAe_param == NULL) {
		MW_ERROR("mw_set_ae_param pointer is NULL!\n");
		return -1;
	}
	if (g_is_rgb_sensor_vin == 0)
		return 0;
	if ((G_mw_sensor_model.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_sensor_model.load_files.lens_file, "") != 0)) {
		if (first_set_ae_parma_flag) {
			G_mw_ae_param.lens_aperture.FNO_min = G_lens_aperture.FNO_min;
			G_mw_ae_param.lens_aperture.FNO_max = G_lens_aperture.FNO_max;
			pAe_param->lens_aperture.aperture_min = G_lens_aperture.FNO_min;
			pAe_param->lens_aperture.aperture_max = G_lens_aperture.FNO_max;
			first_set_ae_parma_flag = 0;
		}
		if ((pAe_param->lens_aperture.aperture_min <
			G_mw_ae_param.lens_aperture.FNO_min) ||
			(pAe_param->lens_aperture.aperture_max >
			G_mw_ae_param.lens_aperture.FNO_max) ||
			(pAe_param->lens_aperture.aperture_min >
			pAe_param->lens_aperture.aperture_max)) {
			MW_ERROR("The params are not correct, use the default value!\n");
			pAe_param->lens_aperture.aperture_min = G_lens_aperture.FNO_min;
			pAe_param->lens_aperture.aperture_max = G_lens_aperture.FNO_max;

		}

	}
	if (0 == memcmp(&G_mw_ae_param, pAe_param, sizeof(G_mw_ae_param)))
		return 0;

	if (G_mw_ae_param.slow_shutter_enable != pAe_param->slow_shutter_enable) {
		if (pAe_param->slow_shutter_enable) {
			create_slowshutter_task();
		} else {
			destroy_slowshutter_task();
		}
	}
	if (G_mw_ae_param.current_vin_fps != pAe_param->current_vin_fps) {
		if (pAe_param->slow_shutter_enable) {
			if (pAe_param->current_vin_fps != 0) {
				G_mw_sensor_model.current_fps = pAe_param->current_vin_fps;
			} else {
				pAe_param->current_vin_fps = G_mw_ae_param.current_vin_fps;
			}
		} else {
			/* If slow shutter task is disabled, sensor frame rate will keep
			 * the "initial" value when 3A process starts. Any attempt to
			 * change sensor frame rate will be restored to "initial" value.
			 */

			if (pAe_param->shutter_time_max > G_mw_ae_param.current_vin_fps) {
				pAe_param->shutter_time_max = G_mw_ae_param.current_vin_fps;
			}
			pAe_param->current_vin_fps = G_mw_ae_param.current_vin_fps;
		}
	}
	if (pAe_param->shutter_time_min > pAe_param->current_vin_fps) {
		MW_ERROR("Shutter time min [%d] is longer than VIN frametime [%d]."
			" Ignore this change!\n", pAe_param->shutter_time_min,
			pAe_param->current_vin_fps);
		return -1;
	}
	//Disable and re-enable dc iris when loading ae line.
	if (G_dc_iris_control_enable) {
		if (enable_dc_iris(0) < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	}

	if (G_p_iris_control_enable) {
		if (enable_piris(0) < 0) {
			MW_ERROR("p_iris_enable error\n");
			return -1;
		}
	}

	if (load_ae_exp_lines(pAe_param) < 0) {
		MW_ERROR("load_ae_exp_lines error\n");
		return -1;
	}
	G_mw_ae_param = *pAe_param;

	if (G_dc_iris_control_enable) {
		usleep(200000);
		if (enable_dc_iris(1) < 0) {
			MW_ERROR("dc_iris_enable error\n");
			return -1;
		}
	}

	if (G_p_iris_control_enable) {
		if (enable_piris(1) < 0) {
			MW_ERROR("p_iris_enable error\n");
			return -1;
		}
	}

	MW_DEBUG(" === Exposure mode          [%d]\n", pAe_param->anti_flicker_mode);
	MW_DEBUG(" === Shutter time min       [%d]\n", pAe_param->shutter_time_min);
	MW_DEBUG(" === Shutter time max       [%d]\n", pAe_param->shutter_time_max);
	MW_DEBUG(" === Slow shutter enable    [%d]\n", pAe_param->slow_shutter_enable);
	MW_DEBUG(" === Max sensor gain        [%d]\n", pAe_param->sensor_gain_max);

	return 0;
}

int mw_get_ae_param(mw_ae_param *pAe_param)
{
	if ((G_mw_sensor_model.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_sensor_model.load_files.lens_file, "") != 0)) {
		G_mw_ae_param.lens_aperture.FNO_min = G_lens_aperture.FNO_min;
		G_mw_ae_param.lens_aperture.FNO_max = G_lens_aperture.FNO_max;
	}
	*pAe_param = G_mw_ae_param;
	return 0;
}

int mw_set_exposure_level(int *exposure_level)
{
	u32 ae_target;
	int i = 0;

	if (exposure_level == NULL) {
		MW_ERROR("mw_set_exposure_level pointer is NULL\n");
		return -1;
	}

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		/* set ae target for multi exposure mode  */
		for (i  = 0; i < G_img_config_info.expo_num; i++) {
			if (exposure_level[i] < MW_EXPOSURE_LEVEL_MIN)
				exposure_level[i] = MW_EXPOSURE_LEVEL_MIN;
			if (exposure_level[i] > MW_EXPOSURE_LEVEL_MAX - 1)
				exposure_level[i] = MW_EXPOSURE_LEVEL_MAX - 1;
		}
		for (i  = 0; i < G_img_config_info.expo_num; i++) {
			if (G_exposure_level[i] == exposure_level[i]) {
				continue;
			}
			ae_target = (exposure_level[i] << 10) / 25;
			img_ae_hdr_set_target(ae_target, i);
			G_exposure_level[i] = exposure_level[i];
		}
	} else {
		/* set ae target for single exposure mode  */
		if (G_exposure_level[0] == exposure_level[0]) {
			return 0;
		}
		if (exposure_level[0] < MW_EXPOSURE_LEVEL_MIN)
			exposure_level[0] = MW_EXPOSURE_LEVEL_MIN;
		if (exposure_level[0] > MW_EXPOSURE_LEVEL_MAX)
			exposure_level[0] = MW_EXPOSURE_LEVEL_MAX;

		ae_target = (exposure_level[0] << 10) / 100;
		if (img_ae_set_target_ratio(ae_target) < 0) {
			MW_ERROR("img_ae_set_target_ratio error\n");
			return -1;
		}
		G_exposure_level[0] = exposure_level[0];
	}

	return 0;
}

int mw_get_exposure_level(int *pExposure_level)
{
	int i = 0;

	if (pExposure_level == NULL) {
		MW_ERROR("mw_get_exposure_level pointer is NULL\n");
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		for (i = 0; i < G_img_config_info.expo_num; i++) {
			pExposure_level[i] = (img_ae_hdr_get_target(i) * 25) >> 10;
			G_exposure_level[i] = pExposure_level[i];
		}
	} else {
		G_exposure_level[0] = (img_ae_get_target_ratio() * 100) >> 10;
		*pExposure_level = G_exposure_level[0];
	}

	return 0;
}

int mw_set_ae_metering_mode(mw_ae_metering_mode mode)
{
	ae_metering_mode metering_mode;
	metering_mode = mode;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("Not Support\n");
		return -1;
	} else {
		if (img_ae_set_meter_mode(metering_mode) < 0) {
			MW_ERROR("img_ae_set_meter_mode error\n");
			return -1;
		}
	}
	G_ae_metering_mode = mode;
	return 0 ;
}

int mw_get_ae_metering_mode(mw_ae_metering_mode *pMode)
{
	*pMode = G_ae_metering_mode;
	return 0;
}

int mw_set_ae_metering_table(mw_ae_metering_table *pAe_metering_table)
{
	G_ae_metering_table = *pAe_metering_table;
	img_set_ae_roi(&G_ae_metering_table.metering_weight[0]);
	return 0;
}

int mw_get_ae_metering_table(mw_ae_metering_table *pAe_metering_table)
{
	*pAe_metering_table = G_ae_metering_table;
	return 0;
}

int mw_set_shutter_time(int fd_iav, int *shutter_time)
{
	aaa_cntl_t aaa_cntl;

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_get_aaa_status(&aaa_cntl);
		if (aaa_cntl.ae_enable != 0) {
			MW_ERROR("[mw_set_shutter_time] cannot work when AE is enabled!\n");
			return -1;
		}
		img_ae_hdr_set_shutter_row(shutter_time, G_img_config_info.expo_num);
	} else {
		img_get_3a_cntl_status(&aaa_cntl);
		if (aaa_cntl.ae_enable != 0) {
			MW_ERROR("[mw_set_shutter_time] cannot work when AE is enabled!\n");
			return -1;
		}

		if(ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_SHUTTER_TIME, *shutter_time)<0) {
			perror("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
			return -1;
		}
	}

	return 0;
}

int mw_get_shutter_time(int fd_iav, int *pShutter_time)
{
	int i;
	u8 shutter_mode;
	int *shutter_row = NULL, *shutter_index = NULL;

	if (pShutter_time == NULL) {
		MW_ERROR("mw_get_shutter_time pointer is NULL!\n");
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		shutter_row = malloc(sizeof(int) * G_img_config_info.expo_num);
		shutter_index = malloc(sizeof(int) * G_img_config_info.expo_num);
		shutter_mode = 1;
		img_ae_hdr_get_shutter_row(shutter_row, G_img_config_info.expo_num);
		MW_DEBUG("shutter mode :%s\n", shutter_mode ?  "Row" : "Index");
		for (i = 0; i < G_img_config_info.expo_num; i++) {
			MW_DEBUG("[%d]: Row=%d, Index=%d\n",
			i, shutter_row[i], shutter_index[i]);
		}
		if (shutter_mode) {
			memcpy(pShutter_time, shutter_row,
				sizeof(int) * G_img_config_info.expo_num);
		} else {
			memcpy(pShutter_time, shutter_index,
				sizeof(int) * G_img_config_info.expo_num);
		}
		free(shutter_index);
		free(shutter_row);

	} else {
		if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_SHUTTER_TIME, pShutter_time)<0) {
			perror("IAV_IOC_VIN_SRC_GET_SHUTTER_TIME");
			return -1;
		}
	}
	return 0;
}

int mw_set_sensor_gain(int fd_iav, int *gain_db)
{
	aaa_cntl_t aaa_cntl;

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_get_aaa_status(&aaa_cntl);
		if (aaa_cntl.ae_enable != 0) {
			MW_ERROR("[mw_set_sensor_gain] cannot work when AE is enabled!\n");
			return -1;
		}
		img_ae_hdr_set_agc_index(gain_db, G_img_config_info.expo_num);
	} else {
		img_get_3a_cntl_status(&aaa_cntl);
		if (aaa_cntl.ae_enable != 0) {
			MW_ERROR("[mw_set_sensor_gain] cannot work when AE is enabled!\n");
			return -1;
		}

		if(ioctl(fd_iav, IAV_IOC_VIN_SRC_SET_AGC_DB, *gain_db)<0) {
			perror("IAV_IOC_VIN_SRC_SET_AGC_DB");
			return -1;
		}
	}

	return 0;
}

int mw_get_sensor_gain(int fd_iav, int *pGain_db)
{
	int i;
	int *agc_index = NULL;
	u32 *dgain = NULL;

	if (pGain_db == NULL) {
		MW_ERROR("mw_get_shutter_time pointer is NULL!\n");
		return -1;
	}

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		agc_index = malloc(sizeof(int) * G_img_config_info.expo_num);
		dgain = malloc(sizeof(u32) * G_img_config_info.expo_num);
		img_ae_hdr_get_agc_index(agc_index, G_img_config_info.expo_num);
		img_ae_hdr_get_dgain(dgain, G_img_config_info.expo_num);
		for (i = 0; i < G_img_config_info.expo_num; i++) {
			MW_DEBUG("Gain:[%d], Agc:%d, Dgain: %d\n", i,
			agc_index[i], dgain[i]);
		}
		memcpy(pGain_db, agc_index,
			sizeof(int) * G_img_config_info.expo_num);
		free(agc_index);
		free(dgain);
	} else {
		if(ioctl(fd_iav, IAV_IOC_VIN_SRC_GET_AGC_DB, pGain_db)<0) {
			perror("IAV_IOC_VIN_SRC_GET_AGC_DB");
			return -1;
		}
	}

	return 0;
}

int mw_set_iris_type(int type)
{
	if (type < MW_LENS_TYPE_FIRST || type >= MW_LENS_TYPE_LAST) {
		MW_ERROR("Lens type error\n");
		return -1;
	}
	G_iris_type = type;
	return 0;
}

int mw_get_iris_type(void)
{
	return G_iris_type;
}

int mw_enable_dc_iris_control(int enable)
{
	if (MW_DC_IRIS != G_iris_type) {
		MW_ERROR("Lens type is not dc-iris\n");
		return -1;
	}
	if (enable_dc_iris(enable) < 0) {
		MW_ERROR("dc_iris_enable error\n");
		return -1;
	}
	G_dc_iris_control_enable = enable;
	return 0;
}


int mw_set_piris_exit_step(int step)
{
	int ret = 0;
	ret = set_piris_exit_step(step);
	return ret;
}

int mw_get_piris_exit_step(void)
{
	return get_piris_exit_step();
}

int mw_enable_p_iris_control(int enable)
{
	if (MW_P_IRIS != G_iris_type) {
		MW_ERROR("Lens type is not p-iris\n");
		return -1;
	}
	if (enable_piris(enable) < 0) {
		MW_ERROR("p_iris_enable error\n");
		return -1;
	}
	G_p_iris_control_enable = enable;
	return 0;
}


int mw_set_dc_iris_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	if (dc_iris_set_pid_coef(pPid_coef) < 0) {
		MW_ERROR("dc_iris_set_pid_coef error\n");
		return -1;
	}
	return 0;
}

int mw_get_dc_iris_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	if (dc_iris_get_pid_coef(pPid_coef) < 0) {
		MW_ERROR("dc_iris_get_pid_coef error\n");
		return -1;
	}
	return 0;
}

int mw_get_ae_luma_value(int * pLuma)
{
	if (pLuma == NULL) {
		MW_ERROR("mw_get_ae_luma_value pointer is NULL!\n");
		return -1;
	}
	*pLuma = img_get_ae_luma_value();
	return 0;
}

int mw_get_ae_lines(mw_ae_line * lines, u32 line_num)
{
	if (lines == NULL) {
		MW_ERROR("mw_get_ae_lines pointer is NULL!\n");
		return -1;
	}
	if (get_ae_exposure_lines(lines, line_num) < 0) {
		MW_ERROR("get_ae_exposure_lines error\n");
		return -1;
	}
	return 0;
}

int mw_set_ae_lines(mw_ae_line * lines, u32 line_num)
{
	if (lines == NULL) {
		MW_ERROR("mw_set_ae_lines pointer is NULL!\n");
		return -1;
	}
	if (set_ae_exposure_lines(lines, line_num) < 0) {
		MW_ERROR("set_ae_exposure_lines error\n");
		return -1;
	}
	return 0;
}

int mw_get_ae_points(mw_ae_point * point_arr, u32 point_num)
{
	if (point_arr == NULL) {
		MW_ERROR("mw_get_ae_points pointer is NULL!\n");
		return -1;
	}
	return 0;
}

int mw_set_ae_points(mw_ae_point * point_arr, u32 point_num)
{
	if (point_arr == NULL) {
		MW_ERROR("mw_set_ae_points pointer is NULL!\n");
		return -1;
	}
	if (set_ae_switch_points(point_arr, point_num) < 0) {
		MW_ERROR("set_ae_switch_points error\n");
		return -1;
	}
	return 0;
}

/* ***************************************************************
	White Balance Control Settings
******************************************************************/

int mw_enable_awb(int enable)
{
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_enable_awb(enable);
	} else {
		img_enable_awb(enable);
	}
	return 0;
}

int mw_get_awb_param(mw_awb_param * pAWB)
{
	if (pAWB == NULL) {
		MW_ERROR("mw_get_awb_param pointer is NULL!\n");
		return -1;
	}
	pAWB->wb_mode = G_mw_awb.wb_mode;

	MW_DEBUG(" === Get AWB Param ===\n");
	MW_DEBUG(" === wb_mode [%d]\n", pAWB->wb_mode);

	return 0;
}

int mw_set_awb_param(mw_awb_param * pAWB)
{
	if (pAWB == NULL) {
		MW_ERROR("mw_set_awb_param pointer is NULL");
		return -1;
	}
	if (g_is_rgb_sensor_vin == 0)
		return 0;

	if (pAWB->wb_mode == MW_WB_MODE_HOLD) {
		img_enable_awb(0);
		goto awb_exit;
	}

	if (pAWB->wb_mode >= WB_MODE_NUMBER) {
		MW_MSG("Wrong WB mode [%d].\n", pAWB->wb_mode);
		return -1;
	}

	if (pAWB->wb_mode != G_mw_awb.wb_mode) {
		if (img_enable_awb(1) < 0)
			return -1;
		if (img_awb_set_mode(pAWB->wb_mode) < 0)
			return -1;
		G_mw_awb.wb_mode = pAWB->wb_mode;
	}

awb_exit:

	MW_DEBUG(" === Set AWB Param ===\n");
	MW_DEBUG(" === wb_mode [%d]\n", pAWB->wb_mode);

	return 0;
}

int mw_get_awb_method(mw_white_balance_method * wb_method)
{
	if (wb_method == NULL) {
		MW_ERROR("[mw_get_awb_method] NULL pointer!\n");
		return -1;
	}

	if ((G_white_balance_method = img_awb_get_method()) < 0) {
		MW_ERROR("[img_awb_get_method] NULL pointer!\n");
		return -1;
	}
	*wb_method = G_white_balance_method;
	return 0;
}

int mw_set_awb_method(mw_white_balance_method wb_method)
{
	aaa_cntl_t aaa_cntl;

	img_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_awb_method] cannot work when AWB is disabled!\n");
		return -1;
	}

	if (G_white_balance_method == wb_method)
		return 0;
	if (wb_method < 0 || wb_method >= MW_WB_METHOD_NUMBER) {
		MW_ERROR("Invalid AWB method [%d].\n", wb_method);
		return -1;
	}

	if (img_awb_set_method(wb_method) < 0)
		return -1;

	G_white_balance_method = wb_method;
	return 0;
}

int mw_set_white_balance_mode(mw_white_balance_mode wb_mode)
{
	aaa_cntl_t aaa_cntl;

	img_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_white_balance_mode] cannot work when AWB is disabled!\n");
		return -1;
	}

	if (G_white_balance_method != MW_WB_NORMAL_METHOD) {
		MW_ERROR("only set in Normal method.\n");
		return -1;
	}

	if (G_white_balance_mode == wb_mode)
		return 0;

	if (wb_mode < 0 || wb_mode >= MW_WB_MODE_NUMBER) {
		MW_ERROR("Invalid WB mode [%d].\n", wb_mode);
		return -1;
	}

	//awb_control_mode = wb_mode;			//the two enums share the definition
	if (img_awb_set_mode(wb_mode) < 0)
		return -1;

	G_white_balance_mode = wb_mode;
	return 0;
}

int mw_get_white_balance_mode(mw_white_balance_mode *pWb_mode)
{
	if (pWb_mode == NULL) {
		MW_ERROR("[mw_get_white_balance_mode] NULL pointer!\n");
		return -1;
	}

	if (G_white_balance_method != MW_WB_NORMAL_METHOD) {
		printf("only work on Normal method\n");
		return -1;
	}

	if ((*pWb_mode = img_awb_get_mode()) < 0)
		return -1;

	G_white_balance_mode = *pWb_mode;

	return 0;
}

int mw_get_rgb_gain(mw_wb_gain * pGain)
{
	wb_gain_t wb_gain;
	u32 d_gain;

	if (pGain == NULL) {
		MW_ERROR("[mw_get_rgb_gain] NULL pointer!\n");
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("[mw_get_rgb_gain] Not support!\n");
		return -1;
	}
	if (img_dsp_get_rgb_gain(&wb_gain, &d_gain) < 0) {
		MW_ERROR("img_dsp_get_rgb_gain error!\n");
		return -1;
	}
	pGain->r_gain = wb_gain.r_gain;
	pGain->g_gain = wb_gain.g_gain;
	pGain->b_gain = wb_gain.b_gain;
	pGain->d_gain = d_gain;

	return 0;
}

int mw_set_rgb_gain(int fd_iav, mw_wb_gain * pGain)
{
	aaa_cntl_t aaa_cntl;
	wb_gain_t wb_gain;

	if ((pGain == NULL) || (fd_iav < 0)) {
		MW_ERROR("[mw_set_rgb_gain] NULL pointer or invalid iav fd!\n");
		return -1;
	}

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("[mw_set_rgb_gain] Not support!\n");
		return -1;
	} else {
		img_get_3a_cntl_status(&aaa_cntl);
	}

	if (aaa_cntl.awb_enable != 0) {
		MW_ERROR("[mw_set_rgb_gain] cannot manually set rgb gain when AWB is enabled!\n");
		return -1;
	}

	wb_gain.r_gain = pGain->r_gain;
	wb_gain.g_gain = pGain->g_gain;
	wb_gain.b_gain = pGain->b_gain;

	if (img_dsp_set_rgb_gain(fd_iav, &wb_gain, pGain->d_gain) < 0) {
		MW_ERROR("img_dsp_set_rgb_gain error!\n");
		return -1;
	}
	return 0;
}


/* ***************************************************************
	Image Adjustment Settings
******************************************************************/

int mw_get_image_param(mw_image_param * pImage)
{
	if (pImage == NULL) {
		MW_ERROR("mw_get_image_param pointer is NULL!\n");
		return -1;
	}
	pImage->saturation = G_mw_image.saturation;
	pImage->brightness = G_mw_image.brightness;
	pImage->hue = G_mw_image.hue;
	pImage->contrast = G_mw_image.contrast;
	pImage->sharpness = G_mw_image.sharpness;

	MW_DEBUG(" === Get Image Param ===\n");
	MW_DEBUG(" === Saturation [%d]\n", pImage->saturation);
	MW_DEBUG(" === Brightness [%d]\n", pImage->brightness);
	MW_DEBUG(" === Hue        [%d]\n", pImage->hue);
	MW_DEBUG(" === Contrast   [%d]\n", pImage->contrast);
	MW_DEBUG(" === Sharpness  [%d]\n", pImage->sharpness);

	return 0;
}

int mw_set_image_param(mw_image_param * pImage)
{
	if (pImage == NULL) {
		MW_ERROR("mw_set_image_param pointer is NULL!\n");
		return -1;
	}
	if (g_is_rgb_sensor_vin == 0)
		return 0;

	if (pImage->saturation != G_mw_image.saturation) {
		if (img_set_color_saturation(pImage->saturation) < 0)
			return -1;
		G_mw_image.saturation = pImage->saturation;
	}
	if (pImage->brightness != G_mw_image.brightness) {
		if (img_set_color_brightness(pImage->brightness) < 0)
			return -1;
		G_mw_image.brightness = pImage->brightness;
	}
	if (pImage->hue != G_mw_image.hue) {
		if (img_set_color_hue(pImage->hue) < 0)
			return -1;
		G_mw_image.hue = pImage->hue;
	}
	if (pImage->contrast != G_mw_image.contrast) {
		if (img_set_color_contrast(pImage->contrast) < 0)
			return -1;
		G_mw_image.contrast = pImage->contrast;
	}
	if (pImage->sharpness != G_mw_image.sharpness) {
		if (img_set_sharpness(pImage->sharpness) < 0)
			return -1;
		G_mw_image.sharpness = pImage->sharpness;
	}

	MW_DEBUG(" === Set Image Param ===\n");
	MW_DEBUG(" === Saturation [%d]\n", pImage->saturation);
	MW_DEBUG(" === Brightness [%d]\n", pImage->brightness);
	MW_DEBUG(" === Hue        [%d]\n", pImage->hue);
	MW_DEBUG(" === Contrast   [%d]\n", pImage->contrast);
	MW_DEBUG(" === Sharpness  [%d]\n", pImage->sharpness);

	return 0;
}

int mw_get_saturation(int *pSaturation)
{
	*pSaturation = G_mw_image.saturation;
	return 0;
}

int mw_set_saturation(int saturation)
{
	if (G_mw_image.saturation == saturation)
		return 0;

	if (saturation < MW_SATURATION_MIN || saturation > MW_SATURATION_MAX)
		return -1;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_hdr_set_color_saturation(saturation) < 0)
			return -1;
	} else {
		if (img_set_color_saturation(saturation) < 0)
			return -1;
	}
	G_mw_image.saturation = saturation;
	return 0;
}

int mw_get_brightness(int *pBrightness)
{
	*pBrightness = G_mw_image.brightness;
	return 0;
}

int mw_set_brightness(int brightness)
{
	if (G_mw_image.brightness == brightness)
		return 0;

	if (brightness < MW_BRIGHTNESS_MIN || brightness > MW_BRIGHTNESS_MAX)
		return -1;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_hdr_set_color_brightness(brightness) < 0)
			return -1;
	} else {
		if (img_set_color_brightness(brightness) < 0)
			return -1;
	}
	G_mw_image.brightness = brightness;
	return 0;
}

int mw_get_contrast(int *pContrast)
{
	*pContrast = G_mw_image.contrast;
	return 0;
}

int mw_set_contrast(int contrast)
{
	if (G_mw_image.contrast == contrast)
		return 0;

	if (contrast < MW_CONTRAST_MIN || contrast > MW_CONTRAST_MAX)
		return -1;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_hdr_set_yuv_contrast_strength(contrast) < 0)
			return -1;
	} else {
		if (img_set_color_contrast(contrast) < 0)
			return -1;
	}
	G_mw_image.contrast = contrast;
	return 0;
}

int mw_get_sharpness(int *pSharpness)
{
	*pSharpness = G_mw_image.sharpness;
	return 0;
}

int mw_set_sharpness(int sharpness)
{
	if (G_mw_image.sharpness == sharpness)
		return 0;

	if (sharpness < MW_SHARPNESS_MIN || sharpness > MW_SHARPNESS_MAX)
		return -1;

	if (img_set_sharpness(sharpness) < 0)
		return -1;

	G_mw_image.sharpness = sharpness;
	return 0;
}


/* ***************************************************************
	Image Enhancement Settings
******************************************************************/

int mw_set_mctf_strength(u32 strength)
{
	if (G_mctf_strength == strength)
		return 0;

	if (strength < MW_MCTF_STRENGTH_MIN || strength > MW_MCTF_STRENGTH_MAX)
		return -1;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_hdr_set_mctf_strength(strength) < 0)
			return -1;
	} else {
		if (img_set_mctf_strength(strength) < 0)
			return -1;
	}
	G_mctf_strength = strength;
	return 0;
}

int mw_get_mctf_strength(u32 *pStrength)
{
	*pStrength = G_mctf_strength;
	return 0;
}

int mw_set_auto_local_exposure_mode(u32 mode)
{
	if (img_set_auto_local_exposure(mode) < 0) {
		MW_ERROR("img_set_auto_local_exposure error.\n");
		return -1;
	}
	MW_INFO("Local exposure mode : [%d].\n", mode);
	G_auto_local_exposure_mode = mode;
	return 0;
}

int mw_get_auto_local_exposure_mode(u32 * pMode)
{
	if (pMode == NULL) {
		MW_ERROR("mw_get_auto_local_exposure_mode pointer is NULL!\n");
		return -1;
	}
	*pMode = G_auto_local_exposure_mode;
	return 0;
}

int mw_set_local_exposure_curve(int fd_iav, mw_local_exposure_curve *pLocal_exposure)
{
	local_exposure_t  local_exposure;
	local_exposure.enable = 1;
	local_exposure.radius = 0;
	local_exposure.luma_weight_red = 16;
	local_exposure.luma_weight_green = 16;
	local_exposure.luma_weight_blue = 16;
	local_exposure.luma_weight_shift = 0;
	memcpy(local_exposure.gain_curve_table, pLocal_exposure->gain_curve_table,
		sizeof(local_exposure.gain_curve_table));

	return img_dsp_set_local_exposure(fd_iav, &local_exposure);
}

int mw_enable_backlight_compensation(int enable)
{
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("Not support\n");
		return -1;
	} else {
		if (img_ae_set_backlight(enable) < 0) {
			MW_ERROR("img_ae_set_backlight error\n");
			return -1;
		}
	}

	G_backlight_compensation_enable = enable;
	return 0;
}

int mw_enable_day_night_mode(int enable)
{
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		MW_ERROR("Not Support\n");
		return -1;
	} else {
		if (set_day_night_mode(enable) < 0) {
			MW_ERROR("set_day_night_mode error\n");
			return -1;
		}
	}

	G_day_night_mode_enable = enable;
	return 0;
}

int mw_get_adj_status(int *pEnable)
{
	if (pEnable == NULL) {
		MW_ERROR("%s error: pEnable is null\n", __func__);
		return -1;
	}
	aaa_cntl_t aaa_status;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_get_aaa_status(&aaa_status);
	} else {
		img_get_3a_cntl_status(&aaa_status);
	}
	*pEnable = aaa_status.adj_enable;
	return 0;
}

int mw_enable_adj(int enable)
{
	if (enable > 1 || enable < 0) {
		MW_ERROR("%s error: the value must be 0 | 1\n", __func__);
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_enable_adj(enable);
	} else {
		img_enable_adj(enable);
	}
	return 0;
}

int mw_get_chroma_noise_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("%s error: pStrength is null\n", __func__);
		return -1;
	}
	*pStrength = G_chroma_noise_filter_strength;
	return 0;
}

int mw_set_chroma_noise_strength(int strength)
{
	if (strength > 256 || strength < 0) {
		MW_ERROR("%s error: the value must be [0~256]\n", __func__);
		return -1;
	}
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		if (img_hdr_set_chroma_noise_filter_strength(strength) < 0) {
			MW_ERROR("img_set_chroma_noise_filter_strength error\n");
			return -1;
		}
	} else {
		if (img_set_chroma_noise_filter_strength(strength) < 0) {
			MW_ERROR("img_set_chroma_noise_filter_strength error\n");
			return -1;
		}
	}

	G_chroma_noise_filter_strength = strength;

	return 0;
}

int  mw_get_lib_version(mw_aaa_lib_version* pAlgo_lib_version,
	mw_aaa_lib_version* pDsp_lib_version)
{
	if ((pAlgo_lib_version == NULL) || (pDsp_lib_version == NULL)) {
		MW_ERROR("mw_get_lib_version error: algo_lib_version or dsp_lib_version is null\n");
		return -1;
	}

	img_algo_get_lib_version((img_lib_version_t *)pAlgo_lib_version);
	img_dsp_get_lib_version((img_lib_version_t *)pDsp_lib_version);
	return 0;
}

int mw_set_auto_color_contrast(u32 enable)
{
	if (enable > 1 || enable < 0) {
		MW_ERROR("The value must be 0 or 1\n");
		return -1;
	}
	if (img_set_auto_color_contrast(enable) <  0) {
		MW_ERROR("img_set_auto_color_contrast error\n");
		return -1;
	}

	G_auto_contrast_enable = enable;
	return 0;
}

int mw_get_auto_color_contrast(u32 *pEnable)
{
	if (pEnable == NULL) {
		MW_ERROR("%s error: pEnable is null\n", __func__);
		return -1;
	}
	*pEnable = G_auto_contrast_enable;
	return 0;
}

int mw_set_auto_color_contrast_strength (u32 value)
{
	if (!G_auto_contrast_enable) {
		MW_ERROR("\nPlease enable auto color contrast mode first\n");
		return -1;
	}
	if (value > 128) {
		MW_ERROR("\nThe value must be 0~128\n");
		return -1;
	}
	if (img_set_auto_color_contrast_strength(value) < 0) {
		MW_ERROR("\nimg_set_auto_color_contrast_strength error\n");
		return -1;
	}
	return 0;
}

#define	IMAGE_AREA_WIDTH		4096
#define	IMAGE_AREA_HEIGHT		4096

int mw_set_ae_area(int enable)
{
	iav_source_buffer_format_all_ex_t buf_format;
	statistics_config_t config;

	memset(&buf_format, 0, sizeof(iav_source_buffer_format_all_ex_t));

	if (ioctl(fd_iav_aaa, IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX, &buf_format) < 0) {
		MW_ERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT_ALL_EX\n");
		return -1;
	}

	if (img_dsp_get_config_statistics_info(&config) < 0) {
		MW_ERROR("img_dsp_get_config_statistics_info error\n");
		return -1;
	}
	if (enable) {
		config.ae_tile_col_start = buf_format.pre_main_input.x *
			IMAGE_AREA_WIDTH / buf_format.main_width;
		config.ae_tile_row_start = buf_format.pre_main_input.y *
			IMAGE_AREA_HEIGHT / buf_format.main_height;
		config.ae_tile_width = (buf_format.pre_main_width / config.ae_tile_num_col) *
			IMAGE_AREA_WIDTH / buf_format.main_width;
		config.ae_tile_height = (buf_format.pre_main_height / config.ae_tile_num_row) *
			IMAGE_AREA_HEIGHT / buf_format.main_height;
	} else {
		config.ae_tile_col_start = 0;
		config.ae_tile_row_start = 0;
		config.ae_tile_width = IMAGE_AREA_WIDTH / config.ae_tile_num_col;
		config.ae_tile_height = IMAGE_AREA_HEIGHT / config.ae_tile_num_row;
	}

	if (img_dsp_config_statistics_info(fd_iav_aaa, &config) < 0) {
		return -1;
	}
	return 0;
}

int mw_get_wb_gain(mw_wb_gain *wb_gain)
{
	int i;
	wb_gain_t img_wb_gain[MAX_HDR_EXPOSURE_NUM];

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_get_wb_gain(img_wb_gain, G_img_config_info.expo_num);
	}

	for (i = 0; i < G_img_config_info.expo_num; i++) {
		wb_gain[i].r_gain = img_wb_gain[i].r_gain;
		wb_gain[i].g_gain = img_wb_gain[i].g_gain;
		wb_gain[i].b_gain = img_wb_gain[i].b_gain;
	}
	return 0;

}
int mw_set_wb_gain(mw_wb_gain *wb_gain)
{
	int i;
	wb_gain_t img_wb_gain[MAX_HDR_EXPOSURE_NUM];

	for (i = 0; i < G_img_config_info.expo_num; i++) {
		img_wb_gain[i].r_gain = wb_gain[i].r_gain;
		img_wb_gain[i].g_gain = wb_gain[i].g_gain;
		img_wb_gain[i].b_gain = wb_gain[i].b_gain;
	}

	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_set_wb_gain(img_wb_gain, G_img_config_info.expo_num);
	}

	return 0;
}

int mw_get_auto_wdr_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("%s error: pStrength is null\n", __func__);
		return -1;
	}
	*pStrength = G_auto_wdr_str;
	return 0;
}
int mw_set_auto_wdr_strength(int strength)
{
	if (strength > 128 || strength < 0) {
		MW_ERROR("%s error:The value must be in 0~128\n", __func__);
		return -1;
	}

	if (G_auto_wdr_str == strength) {
		MW_MSG("The strength is %d already\n", strength);
		return 0;
	}

	if (strength == 0) {
		img_set_wdr_enable(strength);
		img_set_wdr_strength(strength);
	} else {
		img_set_wdr_enable(1);
		img_set_wdr_strength(strength);
	}

	G_auto_wdr_str = strength;

	return 0;
}

int mw_load_aaa_param_file(char *pFile_name, int type)
{
	if (pFile_name == NULL) {
		MW_ERROR("%s error: pFile_name is null\n", __func__);
		return -1;
	}

	if (strlen(pFile_name) > (FILE_NAME_LENGTH - 1)) {
		MW_ERROR("%s error: the file name length must be < %d\n",
			__func__, FILE_NAME_LENGTH);
		return -1;

	}

	switch (type) {
	case FILE_TYPE_ADJ:
		memcpy(G_mw_sensor_model.load_files.adj_file, pFile_name,
			sizeof(G_mw_sensor_model.load_files.adj_file));
		break;
	case FILE_TYPE_AEB:
		memcpy(G_mw_sensor_model.load_files.aeb_file, pFile_name,
			sizeof(G_mw_sensor_model.load_files.aeb_file));
		break;
	case FILE_TYPE_PIRIS:
		memcpy(G_mw_sensor_model.load_files.lens_file, pFile_name,
			sizeof(G_mw_sensor_model.load_files.lens_file));
		break;
	default:
		MW_ERROR("No the type\n");
		break;
	}
	return 0;
}

int mw_set_lens_id(int lens_id)
{
	G_mw_sensor_model.lens_id = lens_id;
	return 0;
}

int mw_set_video_freeze(int enable)
{
	enable = !!enable;
	if (G_img_config_info.expo_num > MIN_HDR_EXPOSURE_NUM) {
		img_hdr_enable_adj(!enable);
	} else {
		img_enable_adj(!enable);
	}
	return img_dsp_set_video_freeze(fd_iav_aaa, enable);
}

#define __END_OF_FILE__

