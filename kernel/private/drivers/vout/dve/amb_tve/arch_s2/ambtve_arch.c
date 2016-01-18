/*
 * kernel/private/drivers/ambarella/vout/dve/amb_tve/arch_a7/ambtve_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
struct ambtve_info {
	struct __amba_vout_video_sink			video_sink;
	u32						*format_list;

	struct amba_video_source_scale_analog_info	ntsc_analog_info;
	struct amba_video_source_scale_analog_info	pal_analog_info;
};

/* ========================================================================== */
static const char ambtve_cvbs_name[] = "CVBS";
static struct ambtve_info *pambtve_cvbs_info;

/* ========================================================================== */
static int ambtve_probe_arch(void)
{
	int				errorCode = 0;
	struct __amba_vout_video_sink	*psink;

	pambtve_cvbs_info = kzalloc(sizeof(struct ambtve_info), GFP_KERNEL);
	if (!pambtve_cvbs_info) {
		errorCode = -ENOMEM;
		goto ambtve_probe_arch_exit;
	}

	pambtve_cvbs_info->format_list = cvbs_format_list;
	pambtve_cvbs_info->ntsc_analog_info.y_coeff = 0x000000b8;
	pambtve_cvbs_info->ntsc_analog_info.pb_coeff = 0x000004e0;
	pambtve_cvbs_info->ntsc_analog_info.pr_coeff = 0x00000438;
	pambtve_cvbs_info->pal_analog_info.y_coeff = 0x000000b4;
	pambtve_cvbs_info->pal_analog_info.pb_coeff = 0x000004d8;
	pambtve_cvbs_info->pal_analog_info.pr_coeff = 0x00000428;

	psink = &pambtve_cvbs_info->video_sink;
	psink->source_id = 1;
	psink->sink_type = AMBA_VOUT_SINK_TYPE_CVBS;
	strlcpy(psink->name, ambtve_cvbs_name, sizeof(psink->name));
	psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	psink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
	psink->hdmi_native_mode = AMBA_VIDEO_MODE_AUTO;
	psink->owner = THIS_MODULE;
	psink->pinfo = pambtve_cvbs_info;
	psink->docmd = ambtve_docmd;
	errorCode = amba_vout_add_video_sink(psink);
	if (errorCode)
		goto ambtve_free_cvbs;
	vout_notice("%s:%d@%d probed!\n", psink->name,
		psink->id, psink->source_id);

	goto ambtve_probe_arch_exit;

ambtve_free_cvbs:
	kfree(pambtve_cvbs_info);
	pambtve_cvbs_info = NULL;

	AMBA_VOUT_FUNC_EXIT(ambtve_probe_arch)
}

static int ambtve_remove_arch(void)
{
	int				errorCode = 0;

	if (pambtve_cvbs_info) {
		errorCode = amba_vout_del_video_sink(
			&pambtve_cvbs_info->video_sink);

		vout_notice("%s removed!\n",
			pambtve_cvbs_info->video_sink.name);
		kfree(pambtve_cvbs_info);
	}

	return errorCode;
}

/* ========================================================================== */
static int ambtve_dve_ntsc_config(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct ambtve_info			*pinfo;
	dram_dve_t				dve;

	pinfo = (struct ambtve_info *)psink->pinfo;

	errorCode = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_GET_DVE, &dve);
	if (errorCode) {
		vout_errorcode();
		goto ambtve_dve_ntsc_config_exit;
	}

	/* Fsc divisor, Fsc= 3.57561149 */
	dve.phi_24_31 = 0x00000021;

	/* PHI = 0x21f07c1f */
	dve.phi_16_23 = 0x000000f0;
	dve.phi_15_8 = 0x0000007c;
	dve.phi_7_0 = 0x0000001f;
	dve.sctoh_31_24 = 0;
	dve.sctoh_23_16 = 0;
	dve.sctoh_15_8 = 0;
	dve.sctoh_7_0 = 0;

	dve.dve_40.s.u_invert = 0;
	dve.dve_40.s.v_invert = 0;
	dve.dve_40.s.t_offset_phase = 0;
	dve.dve_40.s.t_reset_fsc = 0;

	/* compisite */
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	} else {
		/* black level:140 in DVE42 */
		dve.black_lvl = 0x0000008c;
		/* blank level:120 in DVE43 */
		dve.blank_lvl = 0x00000078;
	}

	/* clamp level of 16 in DVE44 */
	dve.clamp_lvl = 0x00000014;

	/* sync level of 8 in DVE45 */
	dve.sync_lvl = 0x00000008;

	//do not use interpolation in DVE46
	dve.dve_46.s.y_interp = 0;
	/* color settings */
	if (AMBA_VIDEO_STANDARD_MODE(sink_mode->mode) == AMBA_VIDEO_MODE_TEST)
		dve.dve_46.s.y_colorbar_en = 1;
	else
		dve.dve_46.s.y_colorbar_en = 0;
	/* zero delay in Y*/
	dve.dve_46.s.t_ydel_adj = 4;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		dve.dve_46.s.t_sel_ylpf = 0;
		dve.dve_46.s.t_ygain_val = 0;
	} else {
		dve.dve_46.s.t_sel_ylpf = 0;
		dve.dve_46.s.t_ygain_val = 1;	//change to 1.25 Y gain
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_47.s.sel_yuv = 1;
	else
		dve.dve_47.s.sel_yuv = 0;
	dve.dve_47.s.pwr_dwn_cv_dac = 0;
	dve.dve_47.s.pwr_dwn_y_dac = 0;
	dve.dve_47.s.pwr_dwn_c_dac = 0;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* NBA of 0 in DVE50 */
		dve.nba = 0;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	} else {
		/* NBA of -60 in DVE50 */
		dve.nba = 0x000000c4;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	}

	/* DVE52 */
	dve.dve_52.s.pal_c_lpf = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_52.s.sel_c_gain = 0;
	else
		dve.dve_52.s.sel_c_gain = 1;

	/*DVE56 */
	dve.dve_56.s.t_hsync_phs = 0;
	dve.dve_56.s.t_vsync_phs = 0;
	/* H/V sync in, 16 bit sep YCbCr */
	dve.dve_56.s.y_tsyn_mode = 1;
	dve.dve_56.s.y_tencd_mode =0;

	/* DVE57 */
	dve.dve_57.s.clk_phs = 0;
	dve.dve_57.s.t_psync_enb = 0;
	dve.dve_57.s.t_psync_phs = 0;
	dve.dve_57.s.unused = 0;
        dve.dve_57.s.vso = 0;

	/* DVE58 */
	dve.vso_7_0 = 0x00;

	/* DVE59 */
	dve.hso_10_8 = 0;
	/* DVE60 */
	dve.hso_7_0 = 0x00;
	/* DVE61 */
	dve.hcl_9_8 = 0x00000003;
	/* DVE62: 0x1716/2-1=0x359 */
	dve.hcl_7_0 = 0x00000059;
	/* DVE65 */
	dve.ccd_odd_15_8 = 0;
	/*DVE66 */
	dve.ccd_odd_7_0 = 0;
	/* DVE67 */
	dve.ccd_even_15_8 = 0;
	/* DVE68 */
	dve.ccd_even_7_0 = 0;
	/* DVE69 */
	dve.cc_enbl = 0;
	/* turn off macrovision in DVE72 */
	dve.mvfcr = 0;
	/*DVE73 */
	dve.mvcsl1_5_0 = 0;
	/*DVE74 */
	dve.mvcls1_5_0 = 0;
	/*DVE75 */
	dve.mvcsl2_5_0 = 0;
	/*DVE76 */
	dve.mvcls2_5_0 = 0;
	/* DVE77 */
	dve.dve_77.s.cs_sp = 0;
	dve.dve_77.s.cs_num = 0;
	dve.dve_77.s.cs_ln = 0;
	/* DVE78 */
	dve.mvpsd_5_0 = 0;
	/*DVE79 */
	dve.mvpsl_5_0 = 0;
	/*DVE80 */
	dve.mvpss_5_0 = 0;
	/*DVE81 */
	dve.mvpsls_14_8 = 0;
	/* DVE82 */
	dve.mvpsls_7_0 = 0;
	/* DVE83 */
	dve.mvpsfs_14_8 = 0;
	/* DVE84 */
	dve.mvpsfs = 0;
	/* DVE85 */
	dve.mvpsagca = 0;
	/* DVE86 */
	dve.mvpsagcb = 0;
	/* DVE87 */
	dve.mveofbpc = 0;

	/* DVE88 */
	dve.dve_88.s.bst_zone_sw1 = 8;
	dve.dve_88.s.bst_zone_sw2 = 0;

	/* DVE89 */
	dve.dve_89.s.bz3_end = 5;
	dve.dve_89.s.adv_bs_en = 1;
	dve.dve_89.s.bz_invert_en = 4;

	/* DVE90 */
	dve.mvpcslimd_7_0 = 0;
	/* DVE91 */
	dve.mvpcslimd_9_8 = 0;

	/* DVE96 */
	dve.dve_96.s.sel_sin = 0;
	dve.dve_96.s.fsc_tst_en = 0;

	/* DVE97 */
	dve.dve_97.s.byp_y_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn on luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 1;
	} else {
		/* turn on the luma lpf */
		dve.dve_97.s.sel_y_lpf = 1; //Full bandwidth
		dve.dve_97.s.ygain_off = 0;
	}

	dve.dve_97.s.sin_cos_en = 0;
	dve.dve_97.s.sel_dac_tst = 0;
	dve.dve_97.s.dig_out_en = 0;

	dve.dve_99.s.byp_c_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn off chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;
		dve.dve_99.s.cgain_off = 1;
	} else {
		/* turn on the chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;  /* Fix the false color */
		dve.dve_99.s.cgain_off = 0;
	}
	dve.mvtms_3_0 = 0;
	dve.hlr_9_8 = 0;
	dve.hlr_7_0 = 0;
	dve.vsmr_4_0 = 0x00010000;

	errorCode = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_SET_DVE, &dve);
	if (errorCode) {
		vout_errorcode();
		goto ambtve_dve_ntsc_config_exit;
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		errorCode = amba_vout_video_source_cmd(psink->source_id,
			AMBA_VIDEO_SOURCE_SET_SCALE_SD_ANALOG_OUT,
			&pinfo->ntsc_analog_info);
		if (errorCode) {
			vout_errorcode();
		}
	}

	AMBA_VOUT_FUNC_EXIT(ambtve_dve_ntsc_config)
}

static int ambtve_dve_pal_config(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct ambtve_info			*pinfo;
	dram_dve_t				dve;

	pinfo = (struct ambtve_info *)psink->pinfo;

	errorCode = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_GET_DVE, &dve);
	if (errorCode) {
		vout_errorcode();
		goto ambtve_dve_pal_config_exit;
	}

	/* Fsc divisor, Fsc= 3.57561149 */
	dve.phi_24_31 = 0x0000002a;
	/* PHI = 0x21f07c1f */
	dve.phi_16_23 = 0x00000009;
	dve.phi_15_8 = 0x0000008a;
	dve.phi_7_0 = 0x000000cb;
	dve.sctoh_31_24 = 0;
	dve.sctoh_23_16 = 0;
	dve.sctoh_15_8 = 0;
	dve.sctoh_7_0 = 0;

	dve.dve_40.s.u_invert = 0;
	dve.dve_40.s.v_invert = 0;
	dve.dve_40.s.t_offset_phase = 0;
	dve.dve_40.s.t_reset_fsc = 0;

	/* compisite */
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	} else {
		/* black level 126 in DVE42 */
		dve.black_lvl = 0x0000007e;
		/* blank level:126 in DVE43 */
		dve.blank_lvl = 0x0000007e;
	}

	/* clamp level of 16 in DVE44 */
	dve.clamp_lvl = 0x00000013;

	/* sync level of 8 in DVE45 */
	dve.sync_lvl = 0x00000010;

	//do not use interpolation in DVE46
	dve.dve_46.s.y_interp = 0;
	/* color settings */
	if (AMBA_VIDEO_STANDARD_MODE(sink_mode->mode) == AMBA_VIDEO_MODE_TEST)
		dve.dve_46.s.y_colorbar_en = 1;
	else
		dve.dve_46.s.y_colorbar_en = 0;
	/* zero delay in Y*/
	dve.dve_46.s.t_ydel_adj = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		dve.dve_46.s.t_sel_ylpf = 0;
		dve.dve_46.s.t_ygain_val = 0;
	} else {
		dve.dve_46.s.t_sel_ylpf = 1;
		dve.dve_46.s.t_ygain_val = 1; //change to 1.25 Y gain
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_47.s.sel_yuv = 1;
	else
		dve.dve_47.s.sel_yuv = 0;
	dve.dve_47.s.pwr_dwn_cv_dac = 0;
	dve.dve_47.s.pwr_dwn_y_dac = 0;
	dve.dve_47.s.pwr_dwn_c_dac = 0;

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* NBA of 0 in DVE50 */
		dve.nba = 0;
		/* PBA of 0 in DVE51 */
		dve.pba = 0;
	} else {
		/* NBA of -60 in DVE50 */
		dve.nba = 0x000000d3;
		/* PBA of 0 in DVE51 */
		dve.pba = 0x0000002d;
	}

	/* DVE52 */
	dve.dve_52.s.pal_c_lpf = 1;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR)
		dve.dve_52.s.sel_c_gain = 0;
	else
		dve.dve_52.s.sel_c_gain = 1;

	/*DVE56 */
	dve.dve_56.s.t_hsync_phs = 0;
	dve.dve_56.s.t_vsync_phs = 0;
	/* H/V sync in, 16 bit sep YCbCr */
	dve.dve_56.s.y_tsyn_mode = 1;
	dve.dve_56.s.y_tencd_mode =1;

	/* DVE57 */
	dve.dve_57.s.clk_phs = 0;
	dve.dve_57.s.t_psync_enb = 0;
	dve.dve_57.s.t_psync_phs = 0;
	dve.dve_57.s.unused = 0;
        dve.dve_57.s.vso = 0;

	/* DVE58 */
	dve.vso_7_0 = 0x00;

	/* DVE59 */
	dve.hso_10_8 = 0;
	/* DVE60 */
	dve.hso_7_0 = 0x00;
	/* DVE61 */
	dve.hcl_9_8 = 0x00000003;
	/* DVE62: 864 * 2 / 2 - 1 = 0x359 */
	dve.hcl_7_0 = 0x0000005f;
	/* DVE65 */
	dve.ccd_odd_15_8 = 0;
	/*DVE66 */
	dve.ccd_odd_7_0 = 0;
	/* DVE67 */
	dve.ccd_even_15_8 = 0;
	/* DVE68 */
	dve.ccd_even_7_0 = 0;
	/* DVE69 */
	dve.cc_enbl = 0;
	/* turn off macrovision in DVE72 */
	dve.mvfcr = 0;
	/*DVE73 */
	dve.mvcsl1_5_0 = 0;
	/*DVE74 */
	dve.mvcls1_5_0 = 0;
	/*DVE75 */
	dve.mvcsl2_5_0 = 0;
	/*DVE76 */
	dve.mvcls2_5_0 = 0;
	/* DVE77 */
	dve.dve_77.s.cs_sp = 0;
	dve.dve_77.s.cs_num = 0;
	dve.dve_77.s.cs_ln = 0;
	/* DVE78 */
	dve.mvpsd_5_0 = 0;
	/*DVE79 */
	dve.mvpsl_5_0 = 0;
	/*DVE80 */
	dve.mvpss_5_0 = 0;
	/*DVE81 */
	dve.mvpsls_14_8 = 0;
	/* DVE82 */
	dve.mvpsls_7_0 = 0;
	/* DVE83 */
	dve.mvpsfs_14_8 = 0;
	/* DVE84 */
	dve.mvpsfs = 0;
	/* DVE85 */
	dve.mvpsagca = 0;
	/* DVE86 */
	dve.mvpsagcb = 0;
	/* DVE87 */
	dve.mveofbpc = 0;

	/* DVE88 */
	dve.dve_88.s.bst_zone_sw1 = 8;
	dve.dve_88.s.bst_zone_sw2 = 0;

	/* DVE89 */
	dve.dve_89.s.bz3_end = 5;
	dve.dve_89.s.adv_bs_en = 1;
	dve.dve_89.s.bz_invert_en = 4;

	/* DVE90 */
	dve.mvpcslimd_7_0 = 0;
	/* DVE91 */
	dve.mvpcslimd_9_8 = 0;

	/* DVE96 */
	dve.dve_96.s.sel_sin = 0;
	dve.dve_96.s.fsc_tst_en = 0;

	/* DVE97 */
	dve.dve_97.s.byp_y_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn on luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;
		dve.dve_97.s.ygain_off = 1;
	} else {
		/* turn on the luma lpf */
		dve.dve_97.s.sel_y_lpf = 1;	//Full bandwidth
		dve.dve_97.s.ygain_off = 0;
	}

	dve.dve_97.s.sin_cos_en = 0;
	dve.dve_97.s.sel_dac_tst = 0;
	dve.dve_97.s.dig_out_en = 0;

	dve.dve_99.s.byp_c_ups = 0;
	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		/* turn off chroma lpf */
		dve.dve_99.s.byp_c_lpf = 1;
		dve.dve_99.s.cgain_off = 1;
	} else {
		/* turn on the chroma lpf */
		dve.dve_99.s.byp_c_lpf = 0;
		dve.dve_99.s.cgain_off = 0;
	}
	dve.mvtms_3_0 = 0;
	dve.hlr_9_8 = 0;
	dve.hlr_7_0 = 0;
	dve.vsmr_4_0 = 0x00010000;

	errorCode = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_SET_DVE, &dve);
	if (errorCode) {
		vout_errorcode();
		goto ambtve_dve_pal_config_exit;
	}

	if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_YPBPR) {
		errorCode = amba_vout_video_source_cmd(psink->source_id,
			AMBA_VIDEO_SOURCE_SET_SCALE_SD_ANALOG_OUT,
			&pinfo->pal_analog_info);
		if (errorCode) {
			vout_errorcode();
		}
	}

	AMBA_VOUT_FUNC_EXIT(ambtve_dve_pal_config)
}

static int ambtve_init_480i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(YUV525I_Y_PELS_PER_LINE * 2, 262, 263,
		ambtve_init_480i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 3, 0,
		0x3ffd, 858, 0x3fff, 858,
		ambtve_init_480i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(274, 1713, 22, 261, 0,
		ambtve_init_480i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_480i_arch)

	AMBA_VOUT_ANALOG_CONFIG(VD_INTERLACE, VD_480I60, ANALOG_ACT_HIGH,
		ANALOG_ACT_HIGH, ambtve_init_480i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_SD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD,
		ambtve_init_480i_arch)

	AMBA_VOUT_DVE_NTSC_CONFIG(ambtve_init_480i_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_480i_arch)
}

static int ambtve_init_576i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
	struct amba_vout_hv_size_info		sink_hv;
	struct amba_vout_hv_sync_info		sink_sync;
	struct amba_vout_window_info		sink_window;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_SET_HV(YUV625I_Y_PELS_PER_LINE * 2, 312, 313,
		ambtve_init_576i_arch)

	AMBA_VOUT_SET_HVSYNC(0, 1,
		0, 1, 0, 1,
		0, 0, 3, 0,
		0x3ffd, 864, 0x3fff, 864,
		ambtve_init_576i_arch)

	AMBA_VOUT_SET_ACTIVE_WIN_480I_576I(286, 1725, 24, 311, 0,
		ambtve_init_576i_arch)

	AMBA_VOUT_SET_VIDEO_SIZE(ambtve_init_576i_arch)

	AMBA_VOUT_ANALOG_CONFIG(VD_INTERLACE, VD_576I50, ANALOG_ACT_HIGH,
		ANALOG_ACT_HIGH, ambtve_init_576i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_SD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_SD,
		ambtve_init_576i_arch)

	AMBA_VOUT_DVE_PAL_CONFIG(ambtve_init_576i_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_576i_arch)
}

static int ambtve_init_480p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_PROGRESSIVE, VD_480P60, ANALOG_ACT_HIGH,
		ANALOG_ACT_HIGH, ambtve_init_480p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_480p_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_480p_arch)
}

static int ambtve_init_576p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_PROGRESSIVE, VD_576P50, ANALOG_ACT_LOW,
		ANALOG_ACT_LOW, ambtve_init_576p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_576p_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_576p_arch)
}

static int ambtve_init_720p_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_PROGRESSIVE, VD_720P60, ANALOG_ACT_LOW,
		ANALOG_ACT_LOW, ambtve_init_720p_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_720p_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_720p_arch)
}

static int ambtve_init_720p50_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_PROGRESSIVE, VD_720P50, ANALOG_ACT_LOW,
		ANALOG_ACT_LOW, ambtve_init_720p50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_720p50_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_720p50_arch)
}

static int ambtve_init_1080i_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_INTERLACE, VD_1080I60, ANALOG_ACT_LOW,
		ANALOG_ACT_LOW, ambtve_init_1080i_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_1080i_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_1080i_arch)
}

static int ambtve_init_1080i50_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;
        vd_config_t				sink_cfg;
	struct amba_video_source_csc_info	sink_csc;

	AMBA_VOUT_ANALOG_CONFIG(VD_INTERLACE, VD_1080I50, ANALOG_ACT_LOW,
		ANALOG_ACT_LOW, ambtve_init_1080i50_arch)

	AMBA_VOUT_SET_CSC(AMBA_VIDEO_SOURCE_CSC_ANALOG,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_HD,
		AMBA_VIDEO_SOURCE_CSC_ANALOG_CLAMP_HD,
		ambtve_init_1080i50_arch)

	AMBA_VOUT_FUNC_EXIT(ambtve_init_1080i50_arch)
}

static int ambtve_pre_setmode_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;

	return errorCode;
}

static int ambtve_post_setmode_arch(struct __amba_vout_video_sink *psink,
	struct amba_video_sink_mode *sink_mode)
{
	int					errorCode = 0;

	return errorCode;
}

