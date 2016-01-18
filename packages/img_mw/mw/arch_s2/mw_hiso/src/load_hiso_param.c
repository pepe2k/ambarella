/**********************************************************************
 *
 * mw_image.c
 *
 * History:
 *	2014/09/30 - [Jingyang Qiu] Modified this file
 *
 * Copyright (C) 20012 - 2016, Ambarella, Inc.
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
#include <ctype.h>
#include <errno.h>

#include <sys/stat.h>
#include <sys/ioctl.h>

#include "img_abs_filter.h"
#include "img_adv_struct_arch.h"
#include "AmbaDSP_ImgFilter.h"
#include "img_api_adv_arch.h"
#include "ambas_vin.h"

#include "mw_struct_hiso.h"
#include "mw_pri_struct.h"
#include "mw_image_priv.h"

#include "mn34210pl_adj_param_adv.c"
#include "mn34210pl_aeb_param_adv.c"
#include "mn34220pl_adj_param_adv.c"
#include "mn34220pl_aeb_param_adv.c"
#include "imx185_adj_param_adv.c"
#include "imx185_aeb_param_adv.c"
#include "imx224_adj_param_adv.c"
#include "imx224_aeb_param_adv.c"

u8 *iso_cc_reg = NULL;
u8 *iso_cc_matrix = NULL;

int load_iso_cc_bin(char * sensor_name)
{
	int file, count;
	char filename[128];
	u8 *matrix[4];
	u8 i;

	for (i = 0; i < 4; i++) {
		matrix[i] = malloc(AMBA_DSP_IMG_CC_3D_SIZE);
		sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, (i+1));
		if ((file = open(filename, O_RDONLY, 0)) < 0) {
			printf("%s cannot be opened\n", filename);
			return -1;
		}
		if((count = read(file, matrix[i], AMBA_DSP_IMG_CC_3D_SIZE)) != AMBA_DSP_IMG_CC_3D_SIZE) {
			printf("read %s error\n",filename);
			return -1;
		}
		close(file);
	}
	cc_binary_addr_t cc_addr;
	cc_addr.cc_0 = matrix[0];
	cc_addr.cc_1 = matrix[1];
	cc_addr.cc_2 = matrix[2];
	cc_addr.cc_3 = matrix[3];
	img_hiso_adj_load_cc_binary(&cc_addr);

	for(i=0; i<4; i++)
		free(matrix[i]);
	return 0;
}

int enable_iso_cc(char* sensor_name, int mode)
{
	AMBA_DSP_IMG_COLOR_CORRECTION_REG_s cc_reg;
	AMBA_DSP_IMG_COLOR_CORRECTION_s cc_3d;
	IMG_PIPELINE_SEL img_pipe =IMG_PIPELINE_LISO;
	char filename[128];
	int file, count;
	AMBA_DSP_IMG_MODE_CFG_s ik_mode;

	if (iso_cc_reg == NULL || iso_cc_matrix == NULL) {
		printf("The point: iso_cc_reg or iso_cc_reg is NULL \n");
		return -1;
	}
	memset(&ik_mode, 0, sizeof(ik_mode));
	ik_mode.Pipe = AMBA_DSP_IMG_PIPE_VIDEO;
	ik_mode.BatchId = 0xff;
	if(mode == MW_AAA_MODE_HISO) {
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_HISO;
		img_pipe =IMG_PIPELINE_HISO;
	} else if(mode == MW_AAA_MODE_LISO) {
		ik_mode.AlgoMode = AMBA_DSP_IMG_ALGO_MODE_FAST;
		img_pipe =IMG_PIPELINE_LISO;
	}

	sprintf(filename,"%s/reg.bin", IMGPROC_PARAM_PATH);
	if ((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("%s cannot be opened\n", filename);
		return -1;
	}
	if((count = read(file, iso_cc_reg, AMBA_DSP_IMG_CC_REG_SIZE)) != AMBA_DSP_IMG_CC_REG_SIZE) {
		printf("read %s error\n", filename);
		return -1;
	}
	close(file);

	sprintf(filename,"%s/sensors/%s_0%d_3D.bin", IMGPROC_PARAM_PATH, sensor_name, 2);
	if((file = open(filename, O_RDONLY, 0)) < 0) {
		printf("%s cannot be opened\n", filename);
		return -1;
	}
	if((count = read(file, iso_cc_matrix, AMBA_DSP_IMG_CC_3D_SIZE)) != AMBA_DSP_IMG_CC_3D_SIZE) {
		printf("read %s error\n", filename);
		return -1;
	}
	close(file);
	cc_reg.RegSettingAddr =(u32)iso_cc_reg;
	AmbaDSP_ImgSetColorCorrectionReg(&ik_mode, &cc_reg);
	cc_3d.MatrixThreeDTableAddr = (u32)iso_cc_matrix;
	AmbaDSP_ImgSetColorCorrection(G_mw_config.fd, &ik_mode, &cc_3d);

	img_hiso_config_pipeline(img_pipe, 1);// 1:exposure number =1

	return 0;
}

int load_iso_containers(u32 sensor_id)
{
	fc_collection_t fcc;
	memset(&fcc, 0, sizeof(fcc));
	fc_collection_hiso_t fcc_hiso;
	memset(&fcc_hiso, 0, sizeof(fcc_hiso));
	switch(sensor_id){
		case SENSOR_MN34210PL:
			printf("mn34210pl\n");
			fcc.fc_ae_target = &mn34210pl_fc_ae_target;
			fcc.fc_wb_ratio = &mn34210pl_fc_wb_ratio;
			fcc.fc_blc = &mn34210pl_fc_blc;
			fcc.fc_antialiasing = &mn34210pl_fc_antialiasing;
			fcc.fc_grgbmismatch = &mn34210pl_fc_grgbmismatch;
			fcc.fc_dpc = &mn34210pl_fc_dpc;
			fcc.fc_cfanf_low = &mn34210pl_fc_cfanf;
			fcc.fc_cfanf_high = NULL;
			fcc.fc_le = NULL;
			fcc.fc_demosaic = &mn34210pl_fc_demosaic;
			fcc.fc_cc = &mn34210pl_fc_cc;
			fcc.fc_tone = &mn34210pl_fc_tone;
			fcc.fc_rgb2yuv = &mn34210pl_fc_rgb2yuv;
			fcc.fc_chroma_scale = &mn34210pl_fc_chroma_scale;
			fcc.fc_chroma_median = &mn34210pl_fc_chroma_median;
			fcc.fc_chroma_nf = &mn34210pl_fc_chroma_nf;
			fcc.fc_cdnr = &mn34210pl_fc_cdnr;
			fcc.fc_1stmode_sel = &mn34210pl_fc_1stmode_sel;
			fcc.fc_asf = &mn34210pl_fc_asf;
			fcc.fc_1st_shpboth = &mn34210pl_fc_1st_shpboth;
			fcc.fc_1st_shpnoise = &mn34210pl_fc_1st_shpnoise;
			fcc.fc_1st_shpfir = &mn34210pl_fc_1st_shpfir;
			fcc.fc_1st_shpcoring = &mn34210pl_fc_1st_shpcoring;
			fcc.fc_1st_shpcoring_idx_scale = &mn34210pl_fc_1st_shpcoring_idx_scale;
			fcc.fc_1st_shpcoring_min = &mn34210pl_fc_1st_shpcoring_min;
			fcc.fc_1st_shpcoring_scale_coring = &mn34210pl_fc_1st_shpcoring_scale_coring;
			fcc.fc_final_shpboth = &mn34210pl_fc_final_shpboth;
			fcc.fc_final_shpnoise = &mn34210pl_fc_final_shpnoise;
			fcc.fc_final_shpfir = &mn34210pl_fc_final_shpfir;
			fcc.fc_final_shpcoring = &mn34210pl_fc_final_shpcoring;
			fcc.fc_final_shpcoring_idx_scale = &mn34210pl_fc_final_shpcoring_idx_scale;
			fcc.fc_final_shpcoring_min = &mn34210pl_fc_final_shpcoring_min;
			fcc.fc_final_shpcoring_scale_coring = &mn34210pl_fc_final_shpcoring_scale_coring;
			fcc.fc_video_mctf = &mn34210pl_fc_video_mctf;

			fcc_hiso.fc_antialiasing_hiso = &mn34210pl_fc_antialiasing_hiso;
			fcc_hiso.fc_dpc_hiso = &mn34210pl_fc_dpc_hiso;
			fcc_hiso.fc_cfanf_hiso = &mn34210pl_fc_cfanf_hiso;
			fcc_hiso.fc_grgbmismatch_hiso = &mn34210pl_fc_grgbmismatch_hiso;
			fcc_hiso.fc_demosaic_hiso = &mn34210pl_fc_demosaic_hiso;
			fcc_hiso.fc_chroma_median_hiso = &mn34210pl_fc_chroma_median_hiso;
			fcc_hiso.fc_cdnr_hiso = &mn34210pl_fc_cdnr_hiso;
			fcc_hiso.fc_asf_hiso = &mn34210pl_fc_asf_hiso;
			fcc_hiso.fc_asf_hiso_high = &mn34210pl_fc_asf_hiso_high;
			fcc_hiso.fc_asf_hiso_low = &mn34210pl_fc_asf_hiso_low;
			fcc_hiso.fc_asf_hiso_med1 = &mn34210pl_fc_asf_hiso_med1;
			fcc_hiso.fc_asf_hiso_med2 = &mn34210pl_fc_asf_hiso_med2;

			fcc_hiso.fc_shpboth_hihigh = &mn34210pl_fc_shpboth_hihigh;
			fcc_hiso.fc_shpnoise_hihigh = &mn34210pl_fc_shpnoise_hihigh;
			fcc_hiso.fc_shpfir_hihigh = &mn34210pl_fc_shpfir_hihigh;
			fcc_hiso.fc_shpcoring_hihigh = &mn34210pl_fc_shpcoring_hihigh;
			fcc_hiso.fc_shpcoring_idx_scale_hihigh = &mn34210pl_fc_shpcoring_idx_scale_hihigh;
			fcc_hiso.fc_shpcoring_min_hihigh = &mn34210pl_fc_shpcoring_min_hihigh;
			fcc_hiso.fc_shpcoring_scale_coring_hihigh = &mn34210pl_fc_shpcoring_scale_coring_hihigh;

			fcc_hiso.fc_shpboth_himed = &mn34210pl_fc_shpboth_himed;
			fcc_hiso.fc_shpnoise_himed = &mn34210pl_fc_shpnoise_himed;
			fcc_hiso.fc_shpfir_himed = &mn34210pl_fc_shpfir_himed;
			fcc_hiso.fc_shpcoring_himed = &mn34210pl_fc_shpcoring_himed;
			fcc_hiso.fc_shpcoring_idx_scale_himed = &mn34210pl_fc_shpcoring_idx_scale_himed;
			fcc_hiso.fc_shpcoring_min_himed = &mn34210pl_fc_shpcoring_min_himed;
			fcc_hiso.fc_shpcoring_scale_coring_himed = &mn34210pl_fc_shpcoring_scale_coring_himed;

			fcc_hiso.fc_shpboth_hili = &mn34210pl_fc_shpboth_hili;
			fcc_hiso.fc_shpnoise_hili = &mn34210pl_fc_shpnoise_hili;
			fcc_hiso.fc_shpfir_hili = &mn34210pl_fc_shpfir_hili;
			fcc_hiso.fc_shpcoring_hili = &mn34210pl_fc_shpcoring_hili;
			fcc_hiso.fc_shpcoring_idx_scale_hili = &mn34210pl_fc_shpcoring_idx_scale_hili;
			fcc_hiso.fc_shpcoring_min_hili = &mn34210pl_fc_shpcoring_min_hili;
			fcc_hiso.fc_shpcoring_scale_coring_hili = &mn34210pl_fc_shpcoring_scale_coring_hili;

			fcc_hiso.fc_chroma_nf_hihigh = &mn34210pl_fc_chroma_nf_hihigh;
			fcc_hiso.fc_chroma_nf_hilowlow = &mn34210pl_fc_chroma_nf_hilowlow;
			fcc_hiso.fc_chroma_nf_hipre = &mn34210pl_fc_chroma_nf_hipre;
			fcc_hiso.fc_chroma_nf_himed = &mn34210pl_fc_chroma_nf_himed;
			fcc_hiso.fc_chroma_nf_hilow = &mn34210pl_fc_chroma_nf_hilow;
			fcc_hiso.fc_chroma_nf_hiverylow = &mn34210pl_fc_chroma_nf_hiverylow;

			fcc_hiso.fc_chroma_combine_med= &mn34210pl_fc_chroma_combine_med;
			fcc_hiso.fc_chroma_combine_low= &mn34210pl_fc_chroma_combine_low;
			fcc_hiso.fc_chroma_combine_verylow= &mn34210pl_fc_chroma_combine_verylow;
			fcc_hiso.fc_luma_combine_noise= &mn34210pl_fc_luma_combine_noise;
			fcc_hiso.fc_combine_lowasf= &mn34210pl_fc_combine_lowasf;
			fcc_hiso.fc_combine_iso= &mn34210pl_fc_combine_iso;
			fcc_hiso.fc_freq_recover= &mn34210pl_fc_freq_recover;
			fcc_hiso.fc_ta = &mn34210pl_fc_ta;
			break;
		case SENSOR_MN34220PL:
			printf("mn34220pl\n");
			fcc.fc_ae_target = &mn34220pl_fc_ae_target;
			fcc.fc_wb_ratio = &mn34220pl_fc_wb_ratio;
			fcc.fc_blc = &mn34220pl_fc_blc;
			fcc.fc_antialiasing = &mn34220pl_fc_antialiasing;
			fcc.fc_grgbmismatch = &mn34220pl_fc_grgbmismatch;
			fcc.fc_dpc = &mn34220pl_fc_dpc;
			fcc.fc_cfanf_low = &mn34220pl_fc_cfanf;
			fcc.fc_cfanf_high = NULL;
			fcc.fc_le = NULL;
			fcc.fc_demosaic = &mn34220pl_fc_demosaic;
			fcc.fc_cc = &mn34220pl_fc_cc;
			fcc.fc_tone = &mn34220pl_fc_tone;
			fcc.fc_rgb2yuv = &mn34220pl_fc_rgb2yuv;
			fcc.fc_chroma_scale = &mn34220pl_fc_chroma_scale;
			fcc.fc_chroma_median = &mn34220pl_fc_chroma_median;
			fcc.fc_chroma_nf = &mn34220pl_fc_chroma_nf;
			fcc.fc_cdnr = &mn34220pl_fc_cdnr;
			fcc.fc_1stmode_sel = &mn34220pl_fc_1stmode_sel;
			fcc.fc_asf = &mn34220pl_fc_asf;
			fcc.fc_1st_shpboth = &mn34220pl_fc_1st_shpboth;
			fcc.fc_1st_shpnoise = &mn34220pl_fc_1st_shpnoise;
			fcc.fc_1st_shpfir = &mn34220pl_fc_1st_shpfir;
			fcc.fc_1st_shpcoring = &mn34220pl_fc_1st_shpcoring;
			fcc.fc_1st_shpcoring_idx_scale = &mn34220pl_fc_1st_shpcoring_idx_scale;
			fcc.fc_1st_shpcoring_min = &mn34220pl_fc_1st_shpcoring_min;
			fcc.fc_1st_shpcoring_scale_coring = &mn34220pl_fc_1st_shpcoring_scale_coring;
			fcc.fc_final_shpboth = &mn34220pl_fc_final_shpboth;
			fcc.fc_final_shpnoise = &mn34220pl_fc_final_shpnoise;
			fcc.fc_final_shpfir = &mn34220pl_fc_final_shpfir;
			fcc.fc_final_shpcoring = &mn34220pl_fc_final_shpcoring;
			fcc.fc_final_shpcoring_idx_scale = &mn34220pl_fc_final_shpcoring_idx_scale;
			fcc.fc_final_shpcoring_min = &mn34220pl_fc_final_shpcoring_min;
			fcc.fc_final_shpcoring_scale_coring = &mn34220pl_fc_final_shpcoring_scale_coring;
			fcc.fc_video_mctf = &mn34220pl_fc_video_mctf;

			fcc_hiso.fc_antialiasing_hiso = &mn34220pl_fc_antialiasing_hiso;
			fcc_hiso.fc_dpc_hiso = &mn34220pl_fc_dpc_hiso;
			fcc_hiso.fc_cfanf_hiso = &mn34220pl_fc_cfanf_hiso;
			fcc_hiso.fc_grgbmismatch_hiso = &mn34220pl_fc_grgbmismatch_hiso;
			fcc_hiso.fc_demosaic_hiso = &mn34220pl_fc_demosaic_hiso;
			fcc_hiso.fc_chroma_median_hiso = &mn34220pl_fc_chroma_median_hiso;
			fcc_hiso.fc_cdnr_hiso = &mn34220pl_fc_cdnr_hiso;
			fcc_hiso.fc_asf_hiso = &mn34220pl_fc_asf_hiso;
			fcc_hiso.fc_asf_hiso_high = &mn34220pl_fc_asf_hiso_high;
			fcc_hiso.fc_asf_hiso_low = &mn34220pl_fc_asf_hiso_low;
			fcc_hiso.fc_asf_hiso_med1 = &mn34220pl_fc_asf_hiso_med1;
			fcc_hiso.fc_asf_hiso_med2 = &mn34220pl_fc_asf_hiso_med2;

			fcc_hiso.fc_shpboth_hihigh = &mn34220pl_fc_shpboth_hihigh;
			fcc_hiso.fc_shpnoise_hihigh = &mn34220pl_fc_shpnoise_hihigh;
			fcc_hiso.fc_shpfir_hihigh = &mn34220pl_fc_shpfir_hihigh;
			fcc_hiso.fc_shpcoring_hihigh = &mn34220pl_fc_shpcoring_hihigh;
			fcc_hiso.fc_shpcoring_idx_scale_hihigh = &mn34220pl_fc_shpcoring_idx_scale_hihigh;
			fcc_hiso.fc_shpcoring_min_hihigh = &mn34220pl_fc_shpcoring_min_hihigh;
			fcc_hiso.fc_shpcoring_scale_coring_hihigh = &mn34220pl_fc_shpcoring_scale_coring_hihigh;

			fcc_hiso.fc_shpboth_himed = &mn34220pl_fc_shpboth_himed;
			fcc_hiso.fc_shpnoise_himed = &mn34220pl_fc_shpnoise_himed;
			fcc_hiso.fc_shpfir_himed = &mn34220pl_fc_shpfir_himed;
			fcc_hiso.fc_shpcoring_himed = &mn34220pl_fc_shpcoring_himed;
			fcc_hiso.fc_shpcoring_idx_scale_himed = &mn34220pl_fc_shpcoring_idx_scale_himed;
			fcc_hiso.fc_shpcoring_min_himed = &mn34220pl_fc_shpcoring_min_himed;
			fcc_hiso.fc_shpcoring_scale_coring_himed = &mn34220pl_fc_shpcoring_scale_coring_himed;

			fcc_hiso.fc_shpboth_hili = &mn34220pl_fc_shpboth_hili;
			fcc_hiso.fc_shpnoise_hili = &mn34220pl_fc_shpnoise_hili;
			fcc_hiso.fc_shpfir_hili = &mn34220pl_fc_shpfir_hili;
			fcc_hiso.fc_shpcoring_hili = &mn34220pl_fc_shpcoring_hili;
			fcc_hiso.fc_shpcoring_idx_scale_hili = &mn34220pl_fc_shpcoring_idx_scale_hili;
			fcc_hiso.fc_shpcoring_min_hili = &mn34220pl_fc_shpcoring_min_hili;
			fcc_hiso.fc_shpcoring_scale_coring_hili = &mn34220pl_fc_shpcoring_scale_coring_hili;

			fcc_hiso.fc_chroma_nf_hihigh = &mn34220pl_fc_chroma_nf_hihigh;
			fcc_hiso.fc_chroma_nf_hilowlow = &mn34220pl_fc_chroma_nf_hilowlow;
			fcc_hiso.fc_chroma_nf_hipre = &mn34220pl_fc_chroma_nf_hipre;
			fcc_hiso.fc_chroma_nf_himed = &mn34220pl_fc_chroma_nf_himed;
			fcc_hiso.fc_chroma_nf_hilow = &mn34220pl_fc_chroma_nf_hilow;
			fcc_hiso.fc_chroma_nf_hiverylow = &mn34220pl_fc_chroma_nf_hiverylow;

			fcc_hiso.fc_chroma_combine_med= &mn34220pl_fc_chroma_combine_med;
			fcc_hiso.fc_chroma_combine_low= &mn34220pl_fc_chroma_combine_low;
			fcc_hiso.fc_chroma_combine_verylow= &mn34220pl_fc_chroma_combine_verylow;
			fcc_hiso.fc_luma_combine_noise= &mn34220pl_fc_luma_combine_noise;
			fcc_hiso.fc_combine_lowasf= &mn34220pl_fc_combine_lowasf;
			fcc_hiso.fc_combine_iso= &mn34220pl_fc_combine_iso;
			fcc_hiso.fc_freq_recover= &mn34220pl_fc_freq_recover;
			fcc_hiso.fc_ta = &mn34220pl_fc_ta;
			break;
		case SENSOR_IMX185:
			printf("imx185\n");
			fcc.fc_ae_target = &imx185_fc_ae_target;
			fcc.fc_wb_ratio = &imx185_fc_wb_ratio;
			fcc.fc_blc = &imx185_fc_blc;
			fcc.fc_antialiasing = &imx185_fc_antialiasing;
			fcc.fc_grgbmismatch = &imx185_fc_grgbmismatch;
			fcc.fc_dpc = &imx185_fc_dpc;
			fcc.fc_cfanf_low = &imx185_fc_cfanf;
			fcc.fc_cfanf_high = NULL;
			fcc.fc_le = NULL;
			fcc.fc_demosaic = &imx185_fc_demosaic;
			fcc.fc_cc = &imx185_fc_cc;
			fcc.fc_tone = &imx185_fc_tone;
			fcc.fc_rgb2yuv = &imx185_fc_rgb2yuv;
			fcc.fc_chroma_scale = &imx185_fc_chroma_scale;
			fcc.fc_chroma_median = &imx185_fc_chroma_median;
			fcc.fc_chroma_nf = &imx185_fc_chroma_nf;
			fcc.fc_cdnr = &imx185_fc_cdnr;
			fcc.fc_1stmode_sel = &imx185_fc_1stmode_sel;
			fcc.fc_asf = &imx185_fc_asf;
			fcc.fc_1st_shpboth = &imx185_fc_1st_shpboth;
			fcc.fc_1st_shpnoise = &imx185_fc_1st_shpnoise;
			fcc.fc_1st_shpfir = &imx185_fc_1st_shpfir;
			fcc.fc_1st_shpcoring = &imx185_fc_1st_shpcoring;
			fcc.fc_1st_shpcoring_idx_scale = &imx185_fc_1st_shpcoring_idx_scale;
			fcc.fc_1st_shpcoring_min = &imx185_fc_1st_shpcoring_min;
			fcc.fc_1st_shpcoring_scale_coring = &imx185_fc_1st_shpcoring_scale_coring;
			fcc.fc_final_shpboth = &imx185_fc_final_shpboth;
			fcc.fc_final_shpnoise = &imx185_fc_final_shpnoise;
			fcc.fc_final_shpfir = &imx185_fc_final_shpfir;
			fcc.fc_final_shpcoring = &imx185_fc_final_shpcoring;
			fcc.fc_final_shpcoring_idx_scale = &imx185_fc_final_shpcoring_idx_scale;
			fcc.fc_final_shpcoring_min = &imx185_fc_final_shpcoring_min;
			fcc.fc_final_shpcoring_scale_coring = &imx185_fc_final_shpcoring_scale_coring;
			fcc.fc_video_mctf = &imx185_fc_video_mctf;

			fcc_hiso.fc_antialiasing_hiso = &imx185_fc_antialiasing_hiso;
			fcc_hiso.fc_dpc_hiso = &imx185_fc_dpc_hiso;
			fcc_hiso.fc_cfanf_hiso = &imx185_fc_cfanf_hiso;
			fcc_hiso.fc_grgbmismatch_hiso = &imx185_fc_grgbmismatch_hiso;
			fcc_hiso.fc_demosaic_hiso = &imx185_fc_demosaic_hiso;
			fcc_hiso.fc_chroma_median_hiso = &imx185_fc_chroma_median_hiso;
			fcc_hiso.fc_cdnr_hiso = &imx185_fc_cdnr_hiso;
			fcc_hiso.fc_asf_hiso = &imx185_fc_asf_hiso;
			fcc_hiso.fc_asf_hiso_high = &imx185_fc_asf_hiso_high;
			fcc_hiso.fc_asf_hiso_low = &imx185_fc_asf_hiso_low;
			fcc_hiso.fc_asf_hiso_med1 = &imx185_fc_asf_hiso_med1;
			fcc_hiso.fc_asf_hiso_med2 = &imx185_fc_asf_hiso_med2;

			fcc_hiso.fc_shpboth_hihigh = &imx185_fc_shpboth_hihigh;
			fcc_hiso.fc_shpnoise_hihigh = &imx185_fc_shpnoise_hihigh;
			fcc_hiso.fc_shpfir_hihigh = &imx185_fc_shpfir_hihigh;
			fcc_hiso.fc_shpcoring_hihigh = &imx185_fc_shpcoring_hihigh;
			fcc_hiso.fc_shpcoring_idx_scale_hihigh = &imx185_fc_shpcoring_idx_scale_hihigh;
			fcc_hiso.fc_shpcoring_min_hihigh = &imx185_fc_shpcoring_min_hihigh;
			fcc_hiso.fc_shpcoring_scale_coring_hihigh = &imx185_fc_shpcoring_scale_coring_hihigh;

			fcc_hiso.fc_shpboth_himed = &imx185_fc_shpboth_himed;
			fcc_hiso.fc_shpnoise_himed = &imx185_fc_shpnoise_himed;
			fcc_hiso.fc_shpfir_himed = &imx185_fc_shpfir_himed;
			fcc_hiso.fc_shpcoring_himed = &imx185_fc_shpcoring_himed;
			fcc_hiso.fc_shpcoring_idx_scale_himed = &imx185_fc_shpcoring_idx_scale_himed;
			fcc_hiso.fc_shpcoring_min_himed = &imx185_fc_shpcoring_min_himed;
			fcc_hiso.fc_shpcoring_scale_coring_himed = &imx185_fc_shpcoring_scale_coring_himed;

			fcc_hiso.fc_shpboth_hili = &imx185_fc_shpboth_hili;
			fcc_hiso.fc_shpnoise_hili = &imx185_fc_shpnoise_hili;
			fcc_hiso.fc_shpfir_hili = &imx185_fc_shpfir_hili;
			fcc_hiso.fc_shpcoring_hili = &imx185_fc_shpcoring_hili;
			fcc_hiso.fc_shpcoring_idx_scale_hili = &imx185_fc_shpcoring_idx_scale_hili;
			fcc_hiso.fc_shpcoring_min_hili = &imx185_fc_shpcoring_min_hili;
			fcc_hiso.fc_shpcoring_scale_coring_hili = &imx185_fc_shpcoring_scale_coring_hili;

			fcc_hiso.fc_chroma_nf_hihigh = &imx185_fc_chroma_nf_hihigh;
			fcc_hiso.fc_chroma_nf_hilowlow = &imx185_fc_chroma_nf_hilowlow;
			fcc_hiso.fc_chroma_nf_hipre = &imx185_fc_chroma_nf_hipre;
			fcc_hiso.fc_chroma_nf_himed = &imx185_fc_chroma_nf_himed;
			fcc_hiso.fc_chroma_nf_hilow = &imx185_fc_chroma_nf_hilow;
			fcc_hiso.fc_chroma_nf_hiverylow = &imx185_fc_chroma_nf_hiverylow;

			fcc_hiso.fc_chroma_combine_med= &imx185_fc_chroma_combine_med;
			fcc_hiso.fc_chroma_combine_low= &imx185_fc_chroma_combine_low;
			fcc_hiso.fc_chroma_combine_verylow= &imx185_fc_chroma_combine_verylow;
			fcc_hiso.fc_luma_combine_noise= &imx185_fc_luma_combine_noise;
			fcc_hiso.fc_combine_lowasf= &imx185_fc_combine_lowasf;
			fcc_hiso.fc_combine_iso= &imx185_fc_combine_iso;
			fcc_hiso.fc_freq_recover= &imx185_fc_freq_recover;
			fcc_hiso.fc_ta = &imx185_fc_ta;
			break;
		case SENSOR_IMX224:
			printf("imx224\n");
			fcc.fc_ae_target = &imx224_fc_ae_target;
			fcc.fc_wb_ratio = &imx224_fc_wb_ratio;
			fcc.fc_blc = &imx224_fc_blc;
			fcc.fc_antialiasing = &imx224_fc_antialiasing;
			fcc.fc_grgbmismatch = &imx224_fc_grgbmismatch;
			fcc.fc_dpc = &imx224_fc_dpc;
			fcc.fc_cfanf_low = &imx224_fc_cfanf;
			fcc.fc_cfanf_high = NULL;
			fcc.fc_le = NULL;
			fcc.fc_demosaic = &imx224_fc_demosaic;
			fcc.fc_cc = &imx224_fc_cc;
			fcc.fc_tone = &imx224_fc_tone;
			fcc.fc_rgb2yuv = &imx224_fc_rgb2yuv;
			fcc.fc_chroma_scale = &imx224_fc_chroma_scale;
			fcc.fc_chroma_median = &imx224_fc_chroma_median;
			fcc.fc_chroma_nf = &imx224_fc_chroma_nf;
			fcc.fc_cdnr = &imx224_fc_cdnr;
			fcc.fc_1stmode_sel = &imx224_fc_1stmode_sel;
			fcc.fc_asf = &imx224_fc_asf;
			fcc.fc_1st_shpboth = &imx224_fc_1st_shpboth;
			fcc.fc_1st_shpnoise = &imx224_fc_1st_shpnoise;
			fcc.fc_1st_shpfir = &imx224_fc_1st_shpfir;
			fcc.fc_1st_shpcoring = &imx224_fc_1st_shpcoring;
			fcc.fc_1st_shpcoring_idx_scale = &imx224_fc_1st_shpcoring_idx_scale;
			fcc.fc_1st_shpcoring_min = &imx224_fc_1st_shpcoring_min;
			fcc.fc_1st_shpcoring_scale_coring = &imx224_fc_1st_shpcoring_scale_coring;
			fcc.fc_final_shpboth = &imx224_fc_final_shpboth;
			fcc.fc_final_shpnoise = &imx224_fc_final_shpnoise;
			fcc.fc_final_shpfir = &imx224_fc_final_shpfir;
			fcc.fc_final_shpcoring = &imx224_fc_final_shpcoring;
			fcc.fc_final_shpcoring_idx_scale = &imx224_fc_final_shpcoring_idx_scale;
			fcc.fc_final_shpcoring_min = &imx224_fc_final_shpcoring_min;
			fcc.fc_final_shpcoring_scale_coring = &imx224_fc_final_shpcoring_scale_coring;
			fcc.fc_video_mctf = &imx224_fc_video_mctf;

			fcc_hiso.fc_antialiasing_hiso = &imx224_fc_antialiasing_hiso;
			fcc_hiso.fc_dpc_hiso = &imx224_fc_dpc_hiso;
			fcc_hiso.fc_cfanf_hiso = &imx224_fc_cfanf_hiso;
			fcc_hiso.fc_grgbmismatch_hiso = &imx224_fc_grgbmismatch_hiso;
			fcc_hiso.fc_demosaic_hiso = &imx224_fc_demosaic_hiso;
			fcc_hiso.fc_chroma_median_hiso = &imx224_fc_chroma_median_hiso;
			fcc_hiso.fc_cdnr_hiso = &imx224_fc_cdnr_hiso;
			fcc_hiso.fc_asf_hiso = &imx224_fc_asf_hiso;
			fcc_hiso.fc_asf_hiso_high = &imx224_fc_asf_hiso_high;
			fcc_hiso.fc_asf_hiso_low = &imx224_fc_asf_hiso_low;
			fcc_hiso.fc_asf_hiso_med1 = &imx224_fc_asf_hiso_med1;
			fcc_hiso.fc_asf_hiso_med2 = &imx224_fc_asf_hiso_med2;

			fcc_hiso.fc_shpboth_hihigh = &imx224_fc_shpboth_hihigh;
			fcc_hiso.fc_shpnoise_hihigh = &imx224_fc_shpnoise_hihigh;
			fcc_hiso.fc_shpfir_hihigh = &imx224_fc_shpfir_hihigh;
			fcc_hiso.fc_shpcoring_hihigh = &imx224_fc_shpcoring_hihigh;
			fcc_hiso.fc_shpcoring_idx_scale_hihigh = &imx224_fc_shpcoring_idx_scale_hihigh;
			fcc_hiso.fc_shpcoring_min_hihigh = &imx224_fc_shpcoring_min_hihigh;
			fcc_hiso.fc_shpcoring_scale_coring_hihigh = &imx224_fc_shpcoring_scale_coring_hihigh;

			fcc_hiso.fc_shpboth_himed = &imx224_fc_shpboth_himed;
			fcc_hiso.fc_shpnoise_himed = &imx224_fc_shpnoise_himed;
			fcc_hiso.fc_shpfir_himed = &imx224_fc_shpfir_himed;
			fcc_hiso.fc_shpcoring_himed = &imx224_fc_shpcoring_himed;
			fcc_hiso.fc_shpcoring_idx_scale_himed = &imx224_fc_shpcoring_idx_scale_himed;
			fcc_hiso.fc_shpcoring_min_himed = &imx224_fc_shpcoring_min_himed;
			fcc_hiso.fc_shpcoring_scale_coring_himed = &imx224_fc_shpcoring_scale_coring_himed;

			fcc_hiso.fc_shpboth_hili = &imx224_fc_shpboth_hili;
			fcc_hiso.fc_shpnoise_hili = &imx224_fc_shpnoise_hili;
			fcc_hiso.fc_shpfir_hili = &imx224_fc_shpfir_hili;
			fcc_hiso.fc_shpcoring_hili = &imx224_fc_shpcoring_hili;
			fcc_hiso.fc_shpcoring_idx_scale_hili = &imx224_fc_shpcoring_idx_scale_hili;
			fcc_hiso.fc_shpcoring_min_hili = &imx224_fc_shpcoring_min_hili;
			fcc_hiso.fc_shpcoring_scale_coring_hili = &imx224_fc_shpcoring_scale_coring_hili;

			fcc_hiso.fc_chroma_nf_hihigh = &imx224_fc_chroma_nf_hihigh;
			fcc_hiso.fc_chroma_nf_hilowlow = &imx224_fc_chroma_nf_hilowlow;
			fcc_hiso.fc_chroma_nf_hipre = &imx224_fc_chroma_nf_hipre;
			fcc_hiso.fc_chroma_nf_himed = &imx224_fc_chroma_nf_himed;
			fcc_hiso.fc_chroma_nf_hilow = &imx224_fc_chroma_nf_hilow;
			fcc_hiso.fc_chroma_nf_hiverylow = &imx224_fc_chroma_nf_hiverylow;

			fcc_hiso.fc_chroma_combine_med= &imx224_fc_chroma_combine_med;
			fcc_hiso.fc_chroma_combine_low= &imx224_fc_chroma_combine_low;
			fcc_hiso.fc_chroma_combine_verylow= &imx224_fc_chroma_combine_verylow;
			fcc_hiso.fc_luma_combine_noise= &imx224_fc_luma_combine_noise;
			fcc_hiso.fc_combine_lowasf= &imx224_fc_combine_lowasf;
			fcc_hiso.fc_combine_iso= &imx224_fc_combine_iso;
			fcc_hiso.fc_freq_recover= &imx224_fc_freq_recover;
			fcc_hiso.fc_ta = &imx224_fc_ta;
			break;
		default:
			printf("undefined sensor id\n");
			return -1;
		}
	img_hiso_adj_init_containers_hiso(&fcc_hiso);
	img_hiso_adj_init_containers_liso(&fcc);

	return 0;
}

int load_img_iso_aaa_config(u32 sensor_id)
{
	int i =0;
	ae_cfg_tbl_t ae_tbl[4];
	u32 db_step = G_mw_config.sensor_params.db_step;
	switch(sensor_id){
		case SENSOR_MN34210PL:
			img_hiso_config_sensor_info(&mn34210pl_sensor_config);
			img_hiso_config_stat_tiles(&mn34210pl_tile_config);
			for(i=0; i<4; i++) {
				ae_tbl[i].p_expo_lines = mn34210pl_50hz_lines;
				ae_tbl[i].line_num = 4;
				ae_tbl[i].belt = 4;
				ae_tbl[i].db_step = db_step;
				ae_tbl[i].p_gain_tbl = mn34210pl_gain_table;
				ae_tbl[i].gain_tbl_size = sizeof(mn34210pl_gain_table)/4;
				ae_tbl[i].p_sht_nl_tbl = mn34210pl_sht_nl_table;
				ae_tbl[i].sht_nl_tbl_size = sizeof(mn34210pl_sht_nl_table)/4;
			}
			img_hiso_config_ae_tables(ae_tbl,1);
			img_hiso_config_awb_param(&mn34210pl_awb_param);
			break;
		case SENSOR_MN34220PL:
			img_hiso_config_sensor_info(&mn34220pl_sensor_config);
			img_hiso_config_stat_tiles(&mn34220pl_tile_config);
			for(i=0; i<4; i++) {
				ae_tbl[i].p_expo_lines = mn34220pl_50hz_lines;
				ae_tbl[i].line_num = 4;
				ae_tbl[i].belt = 4;
				ae_tbl[i].db_step = db_step;
				ae_tbl[i].p_gain_tbl = mn34220pl_gain_table;
				ae_tbl[i].gain_tbl_size = sizeof(mn34220pl_gain_table)/4;
				ae_tbl[i].p_sht_nl_tbl = mn34220pl_sht_nl_table;
				ae_tbl[i].sht_nl_tbl_size = sizeof(mn34220pl_sht_nl_table)/4;
			}
			img_hiso_config_ae_tables(ae_tbl,1);
			img_hiso_config_awb_param(&mn34220pl_awb_param);
			break;
		case SENSOR_IMX185:
			img_hiso_config_sensor_info(&imx185_sensor_config);
			img_hiso_config_stat_tiles(&imx185_tile_config);
			for(i=0; i<4; i++) {
				ae_tbl[i].p_expo_lines = imx185_50hz_lines;
				ae_tbl[i].line_num = 4;
				ae_tbl[i].belt = 4;
				ae_tbl[i].db_step = db_step;
				ae_tbl[i].p_gain_tbl = imx185_gain_table;
				ae_tbl[i].gain_tbl_size = sizeof(imx185_gain_table)/4;
				ae_tbl[i].p_sht_nl_tbl = imx185_sht_nl_table;
				ae_tbl[i].sht_nl_tbl_size = sizeof(imx185_sht_nl_table)/4;
			}
			img_hiso_config_ae_tables(ae_tbl,1);
			img_hiso_config_awb_param(&imx185_awb_param);
			break;
		case SENSOR_IMX224:
			img_hiso_config_sensor_info(&imx224_sensor_config);
			img_hiso_config_stat_tiles(&imx224_tile_config);
			for(i=0; i<4; i++) {
				ae_tbl[i].p_expo_lines = imx224_50hz_lines;
				ae_tbl[i].line_num = 4;
				ae_tbl[i].belt = 4;
				ae_tbl[i].db_step = db_step;
				ae_tbl[i].p_gain_tbl = imx224_gain_table;
				ae_tbl[i].gain_tbl_size = sizeof(imx224_gain_table)/4;
				ae_tbl[i].p_sht_nl_tbl = imx224_sht_nl_table;
				ae_tbl[i].sht_nl_tbl_size = sizeof(imx224_sht_nl_table)/4;
			}
			img_hiso_config_ae_tables(ae_tbl,1);
			img_hiso_config_awb_param(&imx224_awb_param);
			break;

		default:
			printf("undefined sensor id\n");
			return -1;
	}
	return 0;
}
