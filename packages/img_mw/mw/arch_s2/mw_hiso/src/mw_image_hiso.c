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

#include "img_abs_filter.h"
#include "img_adv_struct_arch.h"

#include "ambas_vin.h"
#include "AmbaDSP_ImgFilter.h"
#include "img_api_adv_arch.h"

#include "mw_struct_hiso.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"

#include "mw_api_hiso.h"

_mw_global_config G_mw_config = {
	.fd = -1,
	.init_params = {
		.aaa_enable = 0,
		.nl_enable = 0,
	},
	.sensor_params = {
	},
	.ae_params = {
		.ae_enable = 1,
		.ae_exp_level = 100,
		.anti_flicker_mode = MW_ANTI_FLICKER_60HZ,
		.shutter_time_min = SHUTTER_1BY16000_SEC,
		.shutter_time_max = SHUTTER_1BY60_SEC,
		.sensor_gain_max = ISO_102400,
		.slow_shutter_enable = 0,
		.current_vin_fps = AMBA_VIDEO_FPS_60,
		.lens_aperture	= {
			.aperture_min	= APERTURE_AUTO,
			.aperture_max	= APERTURE_AUTO,
		},
	},
	.awb_params = {
		.enable = 1,
		.wb_method = AWB_NORMAL,
		.wb_mode = WB_AUTOMATIC,
	},
	.image_params = {
		.saturation = 64,
		.brightness = 0,
		.hue = 0,
		.contrast = 64,
	},
	.enh_params = {
		.sharpen_strength = 64,
		.mctf_strength = 1,
		.dn_mode = 0,
	},
	.iris_params = {
		.iris_enable = 0,
		.iris_type = 0,
	},
};

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
	MW_ERROR("Not support %s now!\n", __func__);
	return 0;
}

int mw_get_sharpen_filter(void)
{
	MW_ERROR("Not support %s now!\n", __func__);
	return 0;
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

int mw_start_aaa(void)
{
	if(!G_mw_config.init_params.nl_enable) {
		if (do_prepare_hiso_aaa() < 0) {
			MW_ERROR("do_start_hiso_aaa \n");
			return -1;
		}
		if (do_start_hiso_aaa() < 0) {
			MW_ERROR("do_start_hiso_aaa \n");
			return -1;
		}
	}
	while (1) {
		if (check_state() == 0) {
			break;
		}
		usleep(10000);
	}
	MW_MSG("======= [DONE] mw_start_aaa ======= \n");

	return 0;
}

int mw_stop_aaa(void)
{
	if (G_mw_config.init_params.aaa_enable == 0) {
		MW_INFO("3A is not enabled. Quit silently.\n");
		return 0;
	}

	if (do_stop_hiso_aaa() < 0) {
		MW_ERROR("do_stop_hiso_aaa \n");
		return -1;
	}

	MW_MSG("======= [DONE] mw_stop_aaa ======= \n");
	return 0;
}

int mw_init_aaa(int fd, int mode)
{
	int ret = 0;
	G_mw_config.fd = fd;

	if ((mode == MW_AAA_MODE_LISO) || (mode == MW_AAA_MODE_HISO)) {
		G_mw_config.init_params.mode = mode;
	} else {
		MW_ERROR("Just support set %d [LISO] | %d [HISO] \n",
			MW_AAA_MODE_LISO, MW_AAA_MODE_HISO);
		return -1;
	}

	do {
		iso_cc_reg = malloc(AMBA_DSP_IMG_CC_REG_SIZE);
		if (iso_cc_reg == NULL) {
			ret = -1;
			break;
		}
		iso_cc_matrix = malloc(AMBA_DSP_IMG_CC_3D_SIZE);
		if (iso_cc_matrix == NULL) {
			ret = -1;
			break;
		}
	} while (0);

	if (ret < 0) {
		if (iso_cc_reg) {
			free(iso_cc_reg);
			iso_cc_reg = NULL;
		}
		if (iso_cc_matrix) {
			free(iso_cc_matrix);
			iso_cc_matrix = NULL;
		}
		MW_ERROR("Malloc failed\n");
		return ret;
	}

	init_netlink();
	return ret;
}

int mw_deinit_aaa(void)
{
	deinit_netlink();

	if (iso_cc_reg) {
		free(iso_cc_reg);
		iso_cc_reg = NULL;
	}
	if (iso_cc_matrix) {
		free(iso_cc_matrix);
		iso_cc_matrix = NULL;
	}
	return 0;
}

/* ***************************************************************
	Exposure Control Settings
******************************************************************/

int mw_enable_ae(int enable)
{
	if (enable > 1 && enable < 0) {
		MW_ERROR("The value must be 0 | 1\n");
		return -1;
	}
	img_hiso_enable_ae(enable);
	if (enable) {
		if (load_ae_exp_lines(&(G_mw_config.ae_params)) < 0) {
			MW_ERROR("load ae exp lines error!\n");
			return -1;
		}
	}
	G_mw_config.ae_params.ae_enable = enable;
	return 0;
}

int mw_set_ae_param(mw_ae_param *pAe_param)
{
	static int first_set_ae_parma_flag = 1;
	if (pAe_param == NULL) {
		MW_ERROR("mw_set_ae_param pointer is NULL!\n");
		return -1;
	}

	if ((G_mw_config.sensor_params.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_config.sensor_params.load_files.lens_file, "") != 0)) {
		if (first_set_ae_parma_flag) {
			pAe_param->lens_aperture.aperture_min =
				G_mw_config.ae_params.lens_aperture.FNO_min;
			pAe_param->lens_aperture.aperture_max =
				G_mw_config.ae_params.lens_aperture.FNO_max;
			first_set_ae_parma_flag = 0;
		}
		if ((pAe_param->lens_aperture.aperture_min <
			G_mw_config.ae_params.lens_aperture.FNO_min) ||
			(pAe_param->lens_aperture.aperture_max >
			G_mw_config.ae_params.lens_aperture.FNO_max) ||
			(pAe_param->lens_aperture.aperture_min >
			pAe_param->lens_aperture.aperture_max)) {
			MW_ERROR("The params are not correct, use the default value!\n");
			pAe_param->lens_aperture.aperture_min =
				G_mw_config.ae_params.lens_aperture.FNO_min;
			pAe_param->lens_aperture.aperture_max =
				G_mw_config.ae_params.lens_aperture.FNO_max;

		}

	}
	if (0 == memcmp(&G_mw_config.ae_params, pAe_param,
		sizeof(G_mw_config.ae_params)))
		return 0;

	if (G_mw_config.ae_params.slow_shutter_enable !=
		pAe_param->slow_shutter_enable) {
		G_mw_config.ae_params.slow_shutter_enable =
			pAe_param->slow_shutter_enable;
		if (pAe_param->slow_shutter_enable) {
			create_slowshutter_task();
		} else {
			destroy_slowshutter_task();
		}
	}
	if (G_mw_config.ae_params.current_vin_fps != pAe_param->current_vin_fps) {
		if (pAe_param->slow_shutter_enable) {
			if (pAe_param->current_vin_fps != 0) {
				G_mw_config.sensor_params.current_fps = pAe_param->current_vin_fps;
			} else {
				pAe_param->current_vin_fps = G_mw_config.ae_params.current_vin_fps;
			}
		} else {
			/* If slow shutter task is disabled, sensor frame rate will keep
			 * the "initial" value when 3A process starts. Any attempt to
			 * change sensor frame rate will be restored to "initial" value.
			 */

			if (pAe_param->shutter_time_max >
				G_mw_config.ae_params.current_vin_fps) {
				pAe_param->shutter_time_max =
					G_mw_config.ae_params.current_vin_fps;
			}
			pAe_param->current_vin_fps = G_mw_config.ae_params.current_vin_fps;
		}
	}
	if (pAe_param->shutter_time_min > pAe_param->current_vin_fps) {
		MW_ERROR("Shutter time min [%d] is longer than VIN frametime [%d]."
			" Ignore this change!\n", pAe_param->shutter_time_min,
			pAe_param->current_vin_fps);
		return -1;
	}
	//Disable and re-enable dc iris when loading ae line.
/*	if (G_mw_config.iris_params.iris_enable) {
		switch (G_mw_config.iris_params.iris_type) {
			case DC_IRIS:
				if (enable_dc_iris(0) < 0) {
					MW_ERROR("dc_iris_enable error\n");
					return -1;
				}
				break;
			case P_IRIS:
				if (enable_piris(0) < 0) {
					MW_ERROR("p_iris_enable error\n");
					return -1;
				}
				break;
			default:
				break;
		}
	}
*/
	if (load_ae_exp_lines(pAe_param) < 0) {
		MW_ERROR("load_ae_exp_lines error\n");
		return -1;
	}
	G_mw_config.ae_params = *pAe_param;
/*
	if (G_mw_config.iris_params.iris_enable) {
		usleep(200000);
		switch (G_mw_config.iris_params.iris_type) {
			case DC_IRIS:
				usleep(200000);
				if (enable_dc_iris(1) < 0) {
					MW_ERROR("dc_iris_enable error\n");
					return -1;
				}
				break;
			case P_IRIS:
				if (enable_piris(1) < 0) {
					MW_ERROR("p_iris_enable error\n");
					return -1;
				}
				break;
			default:
				break;
		}
	}
*/
	MW_DEBUG(" === Exposure mode          [%d]\n", pAe_param->anti_flicker_mode);
	MW_DEBUG(" === Shutter time min       [%d]\n", pAe_param->shutter_time_min);
	MW_DEBUG(" === Shutter time max       [%d]\n", pAe_param->shutter_time_max);
	MW_DEBUG(" === Slow shutter enable    [%d]\n", pAe_param->slow_shutter_enable);
	MW_DEBUG(" === Max sensor gain        [%d]\n", pAe_param->sensor_gain_max);

	return 0;
}

int mw_get_ae_param(mw_ae_param *pAe_param)
{
/*	if ((G_mw_config.sensor_params.lens_id != LENS_CMOUNT_ID) &&
		(strcmp(G_mw_config.sensor_params.load_files.lens_file, "") != 0)) {
		G_mw_config.ae_params.lens_aperture.FNO_min = G_lens_aperture.FNO_min;
		G_mw_config.ae_params.lens_aperture.FNO_max = G_lens_aperture.FNO_max;
	}
*/
	*pAe_param = G_mw_config.ae_params;
	return 0;
}

int mw_set_exposure_level(int *exposure_level)
{
	u16 target_ratio[4];
	mw_ae_param *ae_param = &G_mw_config.ae_params;

	if (exposure_level == NULL) {
		MW_ERROR("mw_set_exposure_level pointer is NULL\n");
		return -1;
	}
	if (exposure_level[0] < 25 || exposure_level[0] > 400) {
		MW_ERROR("The value must be [25~400]\n");
		return -1;
	}
	target_ratio[0] = (exposure_level[0] << 10) / 100;

	if (target_ratio[0] == ae_param->ae_exp_level) {
		MW_MSG("Already set\n");
		return 0;
	}

	if (img_hiso_set_ae_target_ratio(target_ratio) < 0) {
		MW_ERROR("img_hiso_set_ae_target_ratio\n");
		return -1;
	}
	ae_param->ae_exp_level = target_ratio[0];
	return 0;
}

int mw_get_exposure_level(int *pExposure_level)
{
	u16 target_ratio[4];
	mw_ae_param *ae_param = &G_mw_config.ae_params;

	if (pExposure_level == NULL) {
		MW_ERROR("mw_get_exposure_level pointer is NULL\n");
		return -1;
	}
	img_hiso_get_ae_target_ratio(target_ratio);
	*pExposure_level = (target_ratio[0] * 100) >> 10;
	ae_param->ae_exp_level = *pExposure_level;

	return 0;
}

int mw_get_shutter_time(int *pShutter_time)
{
//	int shutter_row;

	if (pShutter_time == NULL) {
		MW_ERROR("mw_get_shutter_time pointer is NULL!\n");
		return -1;
	}

	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_SHUTTER_TIME,pShutter_time) < 0) {
		perror("IAV_IOC_VIN_GET_SHUTTER");
		return -1;
	}
//	shutter_row= img_hiso_get_sensor_shutter();
//	MW_DEBUG("---%s: shutter_row = %d, shutter_time = %d!\n", __func__, shutter_row, *pShutter_time);

	return 0;
}

int mw_set_shutter_time(int *shutter_time)
{
	aaa_cntl_t aaa_cntl;
	u32 shutter_row;
	int row_ref = 0, shutter_ref  = 0;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.ae_enable != 0) {
		MW_ERROR("[mw_set_shutter_time] cannot work when AE is enabled!\n");
		return -1;
	}

// just verify with mn34220pl
	row_ref = img_hiso_get_sensor_shutter();
	if (ioctl(G_mw_config.fd, IAV_IOC_VIN_SRC_GET_SHUTTER_TIME,&shutter_ref) < 0) {
		perror("IAV_IOC_VIN_SRC_GET_SHUTTER_TIME");
		return -1;
	}

	if (!shutter_ref) {
		perror("IAV_IOC_VIN_SRC_GET_SHUTTER_TIME");
		return -1;
	}
	shutter_row = (u32)((u64)row_ref * (*shutter_time) / shutter_ref + 1);
	if(img_hiso_set_sensor_shutter(G_mw_config.fd, shutter_row) < 0) {
		perror("IAV_IOC_VIN_SRC_SET_SHUTTER_TIME");
		return -1;
	}

	return 0;
}

int mw_get_sensor_gain(int *pGain_db)
{
	int agc_index;

	if (pGain_db == NULL) {
		MW_ERROR("mw_get_shutter_time pointer is NULL!\n");
		return -1;
	}
	agc_index = img_hiso_get_sensor_agc();
	*pGain_db = (int)(((u64)agc_index * G_mw_config.sensor_params.db_step) >> 24);
	return 0;
}

int mw_set_sensor_gain(int *gain_db)
{
	aaa_cntl_t aaa_cntl;
	u32 gain_idx;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.ae_enable != 0) {
		MW_ERROR("[mw_set_sensor_gain] cannot work when AE is enabled!\n");
		return -1;
	}
	gain_idx = ((*gain_db) << 24) / G_mw_config.sensor_params.db_step;

	img_hiso_set_sensor_agc(G_mw_config.fd, gain_idx);
	return 0;
}

int mw_get_digital_gain(int *dgain)
{
	aaa_cntl_t aaa_cntl;
//	AMBA_DSP_IMG_MODE_CFG_s ik_mode;
//	AMBA_DSP_IMG_WB_GAIN_s wb_gain;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.ae_enable != 0) {
		MW_ERROR("[mw_set_sensor_gain] cannot work when AE is enabled!\n");
		return -1;
	}

//	AmbaDSP_ImgGetWbGain(&ik_mode ,&wb_gain);
//	*dgain = wb_gain.AeGain;

	return 0;
}

int mw_set_digital_gain(int *dgain)
{
	aaa_cntl_t aaa_cntl;
//	AMBA_DSP_IMG_MODE_CFG_s ik_mode;
//	AMBA_DSP_IMG_WB_GAIN_s wb_gain;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.ae_enable != 0) {
		MW_ERROR("[mw_set_sensor_gain] cannot work when AE is enabled!\n");
		return -1;
	}

//	AmbaDSP_ImgSetWbGain(G_mw_config.fd, &ik_mode, &wb_gain);
	return 0;
}

int mw_get_ae_luma_value(int * pLuma)
{
	u16 rgb_luma, cfa_luma;

	if (pLuma == NULL) {
		MW_ERROR("mw_get_ae_luma_value pointer is NULL!\n");
		return -1;
	}
	img_hiso_get_ae_rgb_luma(&rgb_luma);
	img_hiso_get_ae_cfa_luma(&cfa_luma);

	MW_DEBUG("rgb luma = %d, cfa luma = %d!\n", rgb_luma, cfa_luma);

	*pLuma = (int)cfa_luma;
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

/* ***************************************************************
	White Balance Control Settings
******************************************************************/

int mw_enable_awb(int enable)
{
	if (enable > 1 && enable < 0) {
		MW_ERROR("The value must be 0 | 1\n");
		return -1;
	}
	img_hiso_enable_awb(enable);
	G_mw_config.awb_params.enable = enable;
	return 0;
}

int mw_get_awb_method(awb_work_method_t * pwb_method)
{
	if (pwb_method == NULL) {
		MW_ERROR("[mw_get_awb_method] NULL pointer!\n");
		return -1;
	}

	if ((img_hiso_get_awb_method(pwb_method)) < 0) {
		MW_ERROR("[img_awb_get_method] NULL pointer!\n");
		return -1;
	}
	G_mw_config.awb_params.wb_method = *pwb_method;
	return 0;
}

int mw_set_awb_method(awb_work_method_t wb_method)
{
	aaa_cntl_t aaa_cntl;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_awb_method] cannot work when AWB is disabled!\n");
		return -1;
	}

	if (G_mw_config.awb_params.wb_method == wb_method)
		return 0;
	if (wb_method < 0 || wb_method >= AWB_METHOD_NUMBER) {
		MW_ERROR("Invalid AWB method [%d].\n", wb_method);
		return -1;
	}

	if (img_hiso_set_awb_method(&wb_method) < 0)
		return -1;

	G_mw_config.awb_params.wb_method = wb_method;
	return 0;
}

int mw_set_awb_mode(awb_control_mode_t wb_mode)
{
	aaa_cntl_t aaa_cntl;

	img_hiso_get_3a_cntl_status(&aaa_cntl);
	if (aaa_cntl.awb_enable == 0) {
		MW_ERROR("[mw_set_white_balance_mode] cannot work when AWB is disabled!\n");
		return -1;
	}

	if (G_mw_config.awb_params.wb_method != AWB_NORMAL) {
		MW_ERROR("only set in Normal method.\n");
		return -1;
	}

	if (G_mw_config.awb_params.wb_mode == wb_mode)
		return 0;

	if (wb_mode < 0 || wb_mode >= WB_MODE_NUMBER) {
		MW_ERROR("Invalid WB mode [%d].\n", wb_mode);
		return -1;
	}

	//awb_control_mode = wb_mode;			//the two enums share the definition
	if (img_hiso_set_awb_mode(&wb_mode) < 0)
		return -1;

	G_mw_config.awb_params.wb_mode = wb_mode;
	return 0;
}

int mw_get_awb_mode(awb_control_mode_t *pWb_mode)
{
	if (pWb_mode == NULL) {
		MW_ERROR("[mw_get_white_balance_mode] NULL pointer!\n");
		return -1;
	}

	if (G_mw_config.awb_params.wb_method != AWB_NORMAL) {
		printf("only work on Normal method\n");
		return -1;
	}

	if (img_hiso_get_awb_mode(pWb_mode) < 0)
		return -1;

	G_mw_config.awb_params.wb_mode = *pWb_mode;

	return 0;
}

int mw_get_wb_cal(wb_gain_t * pGain)
{
	if (pGain == NULL) {
		MW_ERROR("[mw_get_wb_cal] NULL pointer!\n");
		return -1;
	}

	img_hiso_get_awb_manual_gain(pGain);
	return 0;
}

int mw_set_custom_gain(wb_gain_t * pGain)
{
	if (pGain == NULL) {
		MW_ERROR("[mw_set_custom_gain] NULL pointer!\n");
		return -1;
	}

	if (img_hiso_set_awb_custom_gain(pGain) < 0) {
		MW_ERROR("img_awb_set_custom_gain!\n");
		return -1;
	}

	return 0;
}

/* ***************************************************************
	Image Adjustment Settings
******************************************************************/

int mw_get_saturation(int *pSaturation)
{
	if (pSaturation == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pSaturation = G_mw_config.image_params.saturation;
	return 0;
}

int mw_set_saturation(int saturation)
{
	if (G_mw_config.image_params.saturation == saturation)
		return 0;

	if (saturation < MW_SATURATION_MIN || saturation > MW_SATURATION_MAX) {
		MW_ERROR("The value must be in [%d ~ %d]!\n",
			MW_SATURATION_MIN, MW_SATURATION_MAX);
		return -1;
	}
	if (img_hiso_set_color_saturation(saturation) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	G_mw_config.image_params.saturation = saturation;
	return 0;
}

int mw_get_brightness(int *pBrightness)
{
	if (pBrightness == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pBrightness = G_mw_config.image_params.brightness;
	return 0;
}

int mw_set_brightness(int brightness)
{
	if (G_mw_config.image_params.brightness == brightness)
		return 0;

	if (brightness < MW_BRIGHTNESS_MIN || brightness > MW_BRIGHTNESS_MAX) {
		MW_ERROR("The value must be in [%d ~ %d]!\n",
			MW_BRIGHTNESS_MIN, MW_BRIGHTNESS_MAX);
		return -1;
	}
	if (img_hiso_set_color_brightness(brightness) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	G_mw_config.image_params.brightness = brightness;
	return 0;
}

int mw_get_contrast(int *pContrast)
{
	if (pContrast == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pContrast = G_mw_config.image_params.contrast;
	return 0;
}

int mw_set_contrast(int contrast)
{
	if (G_mw_config.image_params.contrast == contrast)
		return 0;

	if (contrast < MW_CONTRAST_MIN || contrast > MW_CONTRAST_MAX) {
		MW_ERROR("The value must be in [%d ~ %d]!\n",
			MW_CONTRAST_MIN, MW_CONTRAST_MAX);
		return -1;
	}
	if (img_hiso_set_color_contrast(contrast) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	G_mw_config.image_params.contrast = contrast;
	return 0;
}


/* ***************************************************************
	Image Enhancement Settings
******************************************************************/

int mw_enable_adj(int enable)
{
	if (enable > 1 && enable < 0) {
		MW_ERROR("The value must be 0 | 1\n");
		return -1;
	}
	img_hiso_enable_adj(enable);

	return 0;
}

int mw_get_sharpening_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pStrength = G_mw_config.enh_params.sharpen_strength;
	return 0;
}

int mw_set_sharpening_strength(int strength)
{
	if (G_mw_config.enh_params.sharpen_strength == strength)
		return 0;

	if (strength < MW_SHARPENING_STR_MIN ||
		strength > MW_SHARPENING_STR_MAX) {
		MW_ERROR("The value must be in [%d ~ %d]!\n",
			MW_SHARPENING_STR_MIN, MW_SHARPENING_STR_MAX);
		return -1;
	}
	if (img_hiso_set_sharpening_strength(strength) < 0) {
		MW_ERROR("img_set_color_saturation error!\n");
		return -1;
	}

	G_mw_config.enh_params.sharpen_strength = strength;
	return 0;
}

int mw_get_mctf_strength(int *pStrength)
{
	if (pStrength == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pStrength = G_mw_config.enh_params.mctf_strength;
	return 0;
}

int mw_set_mctf_strength(int strength)
{
	if (G_mw_config.enh_params.mctf_strength == strength)
		return 0;

	if (strength < MW_MCTF_STRENGTH_MIN || strength > MW_MCTF_STRENGTH_MAX) {
		MW_ERROR("The value must be in [%d ~ %d]!\n",
			MW_MCTF_STRENGTH_MIN, MW_MCTF_STRENGTH_MAX);
			return -1;
	}
	if (img_hiso_set_mctf_strength((u8)strength) < 0) {
		MW_ERROR("img_hiso_set_mctf_strength error!\n");
		return -1;
	}
	G_mw_config.enh_params.mctf_strength = strength;
	return 0;
}

int mw_get_night_mode(int *pDn_mode)
{
	if (pDn_mode == NULL) {
		MW_ERROR("[%s] NULL pointer!\n", __func__);
		return -1;
	}
	*pDn_mode = G_mw_config.enh_params.dn_mode;
	return 0;
}

int mw_set_night_mode(int dn_mode)
{
	if (dn_mode < 0 || dn_mode > 1) {
		MW_ERROR("The value must be 0 | 1\n");
		return -1;
	}
	if (img_hiso_set_bw_mode((u8)dn_mode) < 0) {
		MW_ERROR("img_hiso_set_bw_mode error.\n");
		return -1;
	}
	G_mw_config.enh_params.dn_mode = dn_mode;

	return 0;
}

/* ***************************************************************
	Version Gettings
******************************************************************/
int  mw_get_lib_version(mw_aaa_lib_version* pAlgo_lib_version,
	mw_aaa_lib_version* pDsp_lib_version)
{
	if ((pAlgo_lib_version == NULL) || (pDsp_lib_version == NULL)) {
		MW_ERROR("mw_get_lib_version error: algo_lib_version or dsp_lib_version is null\n");
		return -1;
	}

	img_hiso_algo_get_lib_version((img_lib_version_t *)pAlgo_lib_version);
	return 0;
}

#define __END_OF_FILE__
