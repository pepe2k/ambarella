/*
 * Filename : ar0331_docmd.c
 *
 * History:
 *    2011/07/11 - [Haowei Lo] Create
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static void ar0331_dump_reg(struct __amba_vin_source *src)
{

	u32 i;
	u16 reg_to_dump_init[] = {
		0x0400,0x0402,0x0404,0x0406,0x0500,0x0600,0x0602,0x0604,0x0606,0x0608,0x060A,0x060C,0x060E,0x0610,0x1000,0x1004,0x1006,0x1008,0x100A,0x1080,
		0x1084,0x1086,0x1088,0x1100,0x1104,0x1108,0x110A,0x110C,0x1110,0x1114,0x1116,0x1118,0x111C,0x1120,0x1122,0x1124,0x1128,0x112C,0x1130,0x1134,
		0x1136,0x1140,0x1142,0x1144,0x1146,0x1148,0x114A,0x1160,0x1162,0x1164,0x1168,0x116C,0x116E,0x1170,0x1174,0x1180,0x1182,0x1184,0x1186,0x11C0,
		0x11C2,0x11C4,0x11C6,0x1200,0x1204,0x1206,0x1208,0x120A,0x1300,0x1400,0x1402,0x1404,0x1406,0x1408,0x140A,0x140C,0x140E,0x1410,0x3000,0x3002,
		0x3004,0x3006,0x3008,0x300A,0x300C,0x3010,0x3012,0x3014,0x3016,0x3018,0x301A,0x301C,0x301D,0x301E,0x3021,0x3022,0x3023,0x3024,0x3026,0x3028,
		0x302A,0x302C,0x302E,0x3030,0x3032,0x3034,0x3036,0x3038,0x303A,0x303B,0x303C,0x3040,0x3044,0x3046,0x3048,0x304A,0x304C,0x304E,0x3050,0x3052,
		0x3054,0x3056,0x3058,0x305A,0x305C,0x305E,0x3060,0x3062,0x3064,0x3066,0x3068,0x306A,0x306C,0x306E,0x3070,0x3072,0x3074,0x3076,0x3078,0x307A,
		0x3080,0x30A0,0x30A2,0x30A4,0x30A6,0x30A8,0x30AA,0x30AC,0x30AE,0x30B0,0x30B2,0x30B4,0x30BC,0x30C0,0x30C2,0x30C4,0x30C6,0x30C8,0x30CA,0x30CC,
		0x30CE,0x30D0,0x30D2,0x30D4,0x30D6,0x30D8,0x30DA,0x30DC,0x3130,0x3132,0x3134,0x3136,0x3138,0x313A,0x313C,0x313E,0x315C,0x315E,0x3160,0x3162,
		0x3164,0x3166,0x3168,0x316A,0x316C,0x316E,0x3170,0x3172,0x3174,0x3176,0x3178,0x318A,0x318C,0x318E,0x3190,0x31A0,0x31A2,0x31A4,0x31A6,0x31A8,
		0x31AA,0x31AC,0x31AE,0x31B0,0x31B2,0x31B4,0x31B6,0x31B8,0x31BA,0x31BC,0x31BE,0x31C0,0x31C2,0x31C4,0x31C6,0x31C8,0x31CA,0x31CC,0x31CE,0x31DA,
		0x31DC,0x31DE,0x31E0,0x31E2,0x31E4,0x31E8,0x31EA,0x31EC,0x31EE,0x31F2,0x31F4,0x31F6,0x31F8,0x31FA,0x31FC,0x31FE,0x3600,0x3602,0x3604,0x3606,
		0x3608,0x360A,0x360C,0x360E,0x3610,0x3612,0x3614,0x3616,0x3618,0x361A,0x361C,0x361E,0x3620,0x3622,0x3624,0x3626,0x3640,0x3642,0x3644,0x3646,
		0x3648,0x364A,0x364C,0x364E,0x3650,0x3652,0x3654,0x3656,0x3658,0x365A,0x365C,0x365E,0x3660,0x3662,0x3664,0x3666,0x3680,0x3682,0x3684,0x3686,
		0x3688,0x368A,0x368C,0x368E,0x3690,0x3692,0x3694,0x3696,0x3698,0x369A,0x369C,0x369E,0x36A0,0x36A2,0x36A4,0x36A6,0x36C0,0x36C2,0x36C4,0x36C6,
		0x36C8,0x36CA,0x36CC,0x36CE,0x36D0,0x36D2,0x36D4,0x36D6,0x36D8,0x36DA,0x36DC,0x36DE,0x36E0,0x36E2,0x36E4,0x36E6,0x3700,0x3702,0x3704,0x3706,
		0x3708,0x370A,0x370C,0x370E,0x3710,0x3712,0x3714,0x3716,0x3718,0x371A,0x371C,0x371E,0x3720,0x3722,0x3724,0x3726,0x3780,0x3782,0x3784,0x3800,
		0x39E8,0x39EA,0x39EC,0x39EE,0x39F0,0x39F2,0x39F4,0x39F6,0x39F8,0x39FA,0x39FC,0x39FE,0x3E00,0x3E02,0x3E04,0x3E06,0x3E08,
		0x3E0A,0x3E0C,0x3E0E,0x3E10,0x3E12,0x3E14,0x3E16,0x3E18,0x3E1A,0x3E1C,0x3E1E,0x3E20,0x3E22,0x3E24,0x3E26,0x3E28,0x3E2A,0x3E2C,0x3E2E,0x3E30,
		0x3E32,0x3E34,0x3E36,0x3E38,0x3E3A,0x3E3C,0x3E3E,0x3E40,0x3E42,0x3E44,0x3E46,0x3E48,0x3E4A,0x3E4C,0x3E4E,0x3E50,0x3E52,0x3E54,0x3E56,0x3E58,
		0x3E90,0x3E92,0x3E94,0x3E96,0x3E98,0x3E9A,0x3E9C,0x3E9E,0x3EA0,0x3EA2,0x3EB0,0x3EB2,0x3EB4,0x3EB6,0x3EB8,0x3ECC,0x3ECE,0x3ED0,0x3ED2,0x3ED4,
		0x3ED6,0x3ED8,0x3EDA,0x3EDC,0x3EDE,0x3EE0,0x3EE2,0x3EE4,0x3EE6,0x3EE8,0x3EEA,0x3EEC,0x3EEE,0x3EF0,0x3F06,0x30FE,0x30BA,0x3042,0x3088,0x3086
	};


	for(i=0; i<sizeof(reg_to_dump_init)/sizeof(reg_to_dump_init[0]); i++) {
		u16 reg_addr = reg_to_dump_init[i];
		u16 reg_val;

		ar0331_read_reg(src, reg_addr, &reg_val);    // Read sensor revision number
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}

}

static void _ar0331_set_reg_0x301A(struct __amba_vin_source *src, u32 restart_frame, u32 streaming, u32 reset)
{
	u16 reg_0x301A; // 0x405c

	reg_0x301A =
		0 << 12                 |    // 1: disable serial interface (HiSPi)
		0 << 11                 |    // force_pll_on, 0 : pll will be powered down in standby mode
		0 << 10                 |    // DONT restart if bad frame is detected
		0 << 9                  |    // The sensor will produce bad frames as some register changed
		0 << 8                  |    // input buffer related to GPI0/1/2/3 inputs are powered down & disabled
		0 << 7                  |    // Parallel data interface is enabled (dep on bit[6])
		1 << 6                  |    // Parallel interface is driven
		1 << 4                  |    //reset_reg_unused
		1 << 3                  |    // Forbids to change value of SMIA registers
		(streaming>0) << 2      |    // Put the sensor in streaming mode
		(restart_frame>0) << 1  |    // Causes sensor to truncate frame at the end of current row and start integrating next frame
		(reset>0) << 0;                      // Set the bit initiates a reset sequence
	// Typically, in normal streamming mode (group_hold=0, restart_frame=1, standby=0), the value is 0x50DE
	ar0331_write_reg(src, 0x301A, reg_0x301A); // AR0331_RESET
}

static void ar0331_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct ar0331_info *pinfo;

	pinfo = (struct ar0331_info *) src->pinfo;

	vin_dbg("ar0331_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = ar0331_video_info_table[index].format_index;

	for (i = 0; i < AR0331_VIDEO_FORMAT_REG_NUM; i++) {
		if (ar0331_video_format_tbl.reg[i] == 0)
			break;

		ar0331_write_reg(src,
				ar0331_video_format_tbl.reg[i],
				ar0331_video_format_tbl.table[format_index].data[i]);
	}

	if (ar0331_video_format_tbl.table[format_index].ext_reg_fill)
		ar0331_video_format_tbl.table[format_index].ext_reg_fill(src);
}


static void ar0331_fill_share_regs(struct __amba_vin_source *src)
{
	int i, ar0331_share_regs_size;
	const struct ar0331_reg_table *reg_tbl;
	struct ar0331_info	*pinfo = (struct ar0331_info *) src->pinfo;

	switch(pinfo->op_mode) {
	case AMBA_VIN_LINEAR_MODE:
		ar0331_share_regs_size = AR0331_LINEAR_SHARE_REG_SZIE;
		reg_tbl = ar0331_linear_share_regs;
		vin_info("Pure linear mode\n");
		break;
	case AR0331_16BIT_HDR_MODE:
		ar0331_share_regs_size = AR0331_16BITS_HDR_SHARE_REG_SIZE;
		reg_tbl = ar0331_16bits_hdr_share_regs;
		vin_info("16bits HDR mode(ALTM bypass)\n");
		break;
	case AMBA_VIN_HDR_MODE:
	default:
		ar0331_share_regs_size = AR0331_HDR_SHARE_REG_SZIE;
		reg_tbl = ar0331_hdr_share_regs;
		vin_info("12bits HDR mode\n");
		break;
	}

	for (i = 0; i < ar0331_share_regs_size; i++) {
		if(reg_tbl[i].reg == 0xffff) {
			msleep(reg_tbl[i].data);
		} else {
			ar0331_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
		}
	}
}

static void ar0331_start_streaming(struct __amba_vin_source *src)
{
	_ar0331_set_reg_0x301A(src, 0/*restart_frame*/, 1/*streaming*/, 0/*reset*/); //Enable Streaming
}

static void ar0331_stop_streaming(struct __amba_vin_source *src)
{
	_ar0331_set_reg_0x301A(src, 0/*restart_frame*/, 0/*streaming*/, 0/*reset*/); //Stop Streaming
}

static void ar0331_sw_reset(struct __amba_vin_source *src)
{
		/* >> TODO << */
	_ar0331_set_reg_0x301A(src, 0/*restart_frame*/, 0/*streaming*/, 1/*reset*/); //reset sensor
	msleep(100);
}

static void ar0331_reset(struct __amba_vin_source *src)
{
	struct ar0331_info *pinfo;
	pinfo = (struct ar0331_info *) src->pinfo;

	AMBA_VIN_HW_RESET();
	ar0331_sw_reset(src);
	pinfo->init_flag = 1;
}

static void ar0331_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct ar0331_info *pinfo;
	const struct ar0331_pll_reg_table 	*pll_reg_table;
	const struct ar0331_reg_table 		*pll_tbl;

	pinfo = (struct ar0331_info *) src->pinfo;

	vin_dbg("ar0331_fill_video_fps_regs\n");
	pll_reg_table = &ar0331_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < AR0331_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			ar0331_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int ar0331_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct ar0331_info  *pinfo;

	pinfo = (struct ar0331_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int ar0331_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u16				data_val;

	struct ar0331_info 			*pinfo;
	const struct ar0331_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct ar0331_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= ar0331_video_info_table[mode_index].format_index;
	pll_reg_table	= &ar0331_pll_tbl[pinfo->pll_index];

	errCode |= ar0331_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= ar0331_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	active_lines = ar0331_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("ar0331_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}

static int ar0331_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int 					errCode = 0;
	u32 					index;
	u32 					format_index;
	struct ar0331_info 			*pinfo;

	pinfo = (struct ar0331_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= AR0331_VIDEO_INFO_TABLE_SZIE) {
		p_video_info->width = 0;
		p_video_info->height = 0;
		p_video_info->fps = 0;
		p_video_info->format = 0;
		p_video_info->type = 0;
		p_video_info->bits = 0;
		p_video_info->ratio = 0;
		p_video_info->system = 0;
		p_video_info->rev = 0;
		p_video_info->pattern= 0;

		errCode = -EPERM;
	} else {
		format_index = ar0331_video_info_table[index].format_index;
		p_video_info->width = ar0331_video_info_table[index].def_width;
		p_video_info->height = ar0331_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = ar0331_video_format_tbl.table[format_index].format;
		p_video_info->type = ar0331_video_format_tbl.table[format_index].type;
		/* FIX ME */
		if(pinfo->op_mode == AR0331_16BIT_HDR_MODE){
			p_video_info->bits = AMBA_VIDEO_BITS_14;
		} else {
			p_video_info->bits = ar0331_video_format_tbl.table[format_index].bits;
		}
		p_video_info->ratio = ar0331_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int ar0331_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct ar0331_info *pinfo;

	pinfo = (struct ar0331_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int ar0331_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof (amba_vin_shutter_info_t));
	return 0;
}

static int ar0331_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	struct ar0331_info *pinfo = (struct ar0331_info *) src->pinfo;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < AR0331_VIDEO_MODE_TABLE_SZIE; i++) {
		if (ar0331_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = AR0331_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = ar0331_video_mode_table[i].still_index;
			format_index = ar0331_video_info_table[index].format_index;

			p_mode_info->video_info.width =
				ar0331_video_format_tbl.table[format_index].width;
			p_mode_info->video_info.height =
				ar0331_video_format_tbl.table[format_index].height;
			p_mode_info->video_info.fps =
				ar0331_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format =
				ar0331_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type =
				ar0331_video_format_tbl.table[format_index].type;
			/* FIX ME */
			if(pinfo->op_mode == AR0331_16BIT_HDR_MODE){
				p_mode_info->video_info.bits = AMBA_VIDEO_BITS_14;
			} else {
				p_mode_info->video_info.bits =
					ar0331_video_format_tbl.table[format_index].bits;
			}
			p_mode_info->video_info.ratio =
				ar0331_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system =
				AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int ar0331_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;

	errCode = ar0331_read_reg(src, 0x3000, ss_id);

	return errCode;
}

static int ar0331_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	/* >> TODO << */
	return 0;
}

static int ar0331_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{

	return 0;

}

static int ar0331_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> TODO, for RGB mode only << */
	return 0;

}

static int ar0331_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{

	u64 exposure_time_q9;
	u16 data_val;
	u32 line_length, frame_length_lines;
	u16 shutter_width;
	int errCode = 0;
	u16 t1t2_ratio = 0, mode=0, max_sht_width = 0;

	struct ar0331_info *pinfo;
	const struct ar0331_pll_reg_table *pll_reg_table;

	pinfo		= (struct ar0331_info *)src->pinfo;
	pll_reg_table	= &ar0331_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time; 		// time*512*1000000

	errCode |= ar0331_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= ar0331_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	errCode |= ar0331_read_reg(src, 0x3082, &data_val);
	mode = data_val&0x1;
	t1t2_ratio = 1<<(((data_val&0xc)>>2) + 2);
	max_sht_width = MIN(70*t1t2_ratio, frame_length_lines-71);

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u16) exposure_time_q9;

	/* check if num_time_h > max_lines */
	if(mode==1)	{//ERS linear mode
		if(shutter_width > (frame_length_lines - 1) )
			shutter_width = frame_length_lines - 1;
	} else {	//HDR mode
		if(shutter_width < (t1t2_ratio/2))
			shutter_width = t1t2_ratio/2;
		else if(shutter_width > max_sht_width)
			shutter_width = max_sht_width;
	}

	errCode |= ar0331_write_reg(src, 0x3012, shutter_width );
	pinfo->current_shutter_time = shutter_time;

	vin_dbg("shutter_width:%d\n", shutter_width);

	return errCode;
}

static int ar0331_convert_dgain_ratio(u16 old_dgain, u32 ratio, u16 *new_dgain)
{
	int errCode = 0;
	u32 tmp_dgain;

	tmp_dgain = (old_dgain * ratio)/1024;

	if(tmp_dgain > 0x7FF){//dgain should be less than 15.992
		tmp_dgain = 0x7FF;
		vin_info("Waring: dgain value is too high!\n");
	} else if ((ratio != 0)&&(tmp_dgain == 0)){//0<ratio<1/128, set to 1/128
		tmp_dgain = 1;
	}
	*new_dgain = tmp_dgain;

	vin_dbg("old_dgain=%d, ratio=%d, new_dgain=%d\n", old_dgain, ratio, *new_dgain);

	return errCode;
}

static int ar0331_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	u16 idc_reg;
	struct ar0331_info *pinfo;
	u32 gain_index;
	s32 db_max, db_min;
	s32 db_step;
	u16 new_dgain;
	u8 md_q1[4] = {41,57,76,85};
	u16 data_val, q1_idx;

	vin_dbg("ar0331_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct ar0331_info *) src->pinfo;
	db_max = pinfo->agc_info.db_max;
	db_min = pinfo->agc_info.db_min;
	db_step = pinfo->agc_info.db_step;

	if(agc_db < db_min) {
		vin_warn("gain 0x%x db should be greater than 0x%x db, set to 0x%x db\n",
			agc_db, db_min, db_min);
		agc_db = db_min;
	} else if (agc_db > db_max) {
		vin_warn("gain 0x%x db should be less than 0x%x db, set to 0x%x db\n",
			agc_db, db_max, db_max);
		agc_db = db_max;
	}

	if (pinfo->op_mode == AMBA_VIN_LINEAR_MODE) {// pure linear mode
		gain_index = (agc_db - db_min) / db_step;
	} else {// HDR mode
		gain_index = (db_max - agc_db) / db_step;
	}

	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {

		if (pinfo->op_mode == AMBA_VIN_LINEAR_MODE) {// pure linear mode
			ar0331_write_reg(src, 0x3060, AR0331_LINEAR_GAIN_TABLE[gain_index][AR0331_GAIN_COL_REG_AGAIN]);
			ar0331_write_reg(src, 0x305E, AR0331_LINEAR_GAIN_TABLE[gain_index][AR0331_GAIN_COL_REG_DGAIN]);
		} else {// HDR mode
			idc_reg = AR0331_GAIN_TABLE[gain_index][AR0331_GAIN_COL_REG_AGAIN];
			q1_idx = (idc_reg&0x30)>>4;
			ar0331_write_reg(src, 0x3060/*AR0331_GAIN_COL_REG_AGAIN*/, idc_reg);

			ar0331_read_reg(src, 0x3198, &data_val);
			data_val &= 0xff00;
			data_val |= md_q1[q1_idx%4];
			ar0331_write_reg(src, 0x3198, data_val);

			pinfo->dgain_base = AR0331_GAIN_TABLE[gain_index][AR0331_GAIN_COL_REG_DGAIN];

			/* r dgain */
			ar0331_convert_dgain_ratio(pinfo->dgain_base, pinfo->dgain_r_ratio, &new_dgain);
			ar0331_write_reg(src, 0x305A, new_dgain);

			/* gr/gb dgain, ratio is fixed to 1 */
			ar0331_write_reg(src, 0x3056, pinfo->dgain_base);
			ar0331_write_reg(src, 0x305C, pinfo->dgain_base);

			/* b dgain */
			ar0331_convert_dgain_ratio(pinfo->dgain_base, pinfo->dgain_b_ratio, &new_dgain);
			ar0331_write_reg(src, 0x3058, new_dgain);
		}
		pinfo->current_gain_db = agc_db;
		return 0;
	} else {
		return -1;
	}
}

static int ar0331_set_dgain_ratio(struct __amba_vin_source *src, struct amba_vin_dgain_info dgain)
{
	int errCode = 0;
	u16 new_dgain;
	struct ar0331_info *pinfo;

	pinfo = (struct ar0331_info *) src->pinfo;

	/* r */
	if(dgain.r_ratio == 0)
		vin_info("Warning:R dgain ratio is set to 0!\n");
	ar0331_convert_dgain_ratio(pinfo->dgain_base, dgain.r_ratio, &new_dgain);
	ar0331_write_reg(src, 0x305A, new_dgain);
	pinfo->dgain_r_ratio = dgain.r_ratio;

	/* gr */
	if(dgain.gr_ratio == 0)
		vin_info("Warning:Gr dgain ratio is set to 0!\n");
	ar0331_convert_dgain_ratio(pinfo->dgain_base, dgain.gr_ratio, &new_dgain);
	ar0331_write_reg(src, 0x3056, new_dgain);

	/* gb */
	if(dgain.gb_ratio == 0)
		vin_info("Warning:Gb dgain ratio is set to 0!\n");
	ar0331_convert_dgain_ratio(pinfo->dgain_base, dgain.gb_ratio, &new_dgain);
	ar0331_write_reg(src, 0x305C, new_dgain);

	/* b */
	if(dgain.b_ratio == 0)
		vin_info("Warning:B dgain ratio is set to 0!\n");
	ar0331_convert_dgain_ratio(pinfo->dgain_base, dgain.b_ratio, &new_dgain);
	ar0331_write_reg(src, 0x3058, new_dgain);
	pinfo->dgain_b_ratio = dgain.b_ratio;

	return errCode;
}

static int ar0331_get_dgain_ratio(struct __amba_vin_source *src, struct amba_vin_dgain_info *dgain)
{
	int errCode = 0;
	u16 current_dgain=0, dgain_ratio;

	/* r */
	ar0331_read_reg(src, 0x305A, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	dgain->r_ratio = dgain_ratio;

	/* gr */
	ar0331_read_reg(src, 0x3056, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	dgain->gr_ratio = dgain_ratio;

	/* gb */
	ar0331_read_reg(src, 0x305C, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	dgain->gb_ratio = dgain_ratio;

	/* b */
	ar0331_read_reg(src, 0x3058, &current_dgain);
	dgain_ratio = current_dgain * (1024 / 128);
	dgain->b_ratio = dgain_ratio;

	return errCode;
}


static int ar0331_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int					errCode = 0;
	uint					readmode;
	struct ar0331_info			*pinfo;
	u32					target_bayer_pattern;
	u16					tmp_reg;

	pinfo = (struct ar0331_info *) src->pinfo;

	switch (mirror_mode->bayer_pattern) {
	case AMBA_VIN_SRC_BAYER_PATTERN_AUTO:
		break;

	case AMBA_VIN_SRC_BAYER_PATTERN_RG:
	case AMBA_VIN_SRC_BAYER_PATTERN_BG:
	case AMBA_VIN_SRC_BAYER_PATTERN_GR:
	case AMBA_VIN_SRC_BAYER_PATTERN_GB:
		pinfo->bayer_pattern = mirror_mode->bayer_pattern;
		break;

	default:
		vin_err("do not support bayer pattern: %d\n", mirror_mode->bayer_pattern);
		return -EINVAL;
	}

	switch (mirror_mode->pattern) {
	case AMBA_VIN_SRC_MIRROR_AUTO:
		return 0;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY_VERTICALLY:
		readmode = AR0331_MIRROR_ROW + AR0331_MIRROR_COLUMN;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = AR0331_MIRROR_ROW;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		readmode = AR0331_MIRROR_COLUMN;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_NONE:
		readmode = 0;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	default:
		vin_err("do not support cmd mirror mode\n");
		return -EINVAL;
	}

	errCode |= ar0331_read_reg(src, 0x3040, &tmp_reg);
	tmp_reg &= (~AR0331_MIRROR_MASK);
	tmp_reg |= readmode;
	errCode |= ar0331_write_reg(src, 0x3040, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}
static int ar0331_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
		/* >> TODO, for YUV output mode only << */
	return errCode;
}
static int ar0331_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
		/* >> TODO, for RGB raw output mode only << */
		/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int ar0331_set_fps(struct __amba_vin_source *src, u32 fps)
{

	int 					errCode = 0;
	struct ar0331_info 			*pinfo;
	u32 					index;
	u32 					format_index;
	u64					frame_time_pclk;
	u32  					frame_time = 0;
	u16 					line_length;
	const struct ar0331_pll_reg_table 	*pll_reg_table;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct ar0331_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= AR0331_VIDEO_INFO_TABLE_SZIE) {
		vin_err("ar0331_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto ar0331_set_fps_exit;
	}

	/* ToDo: Add specified PLL index to lower fps */
	format_index = ar0331_video_info_table[index].format_index;
	pll_reg_table = &ar0331_pll_tbl[pinfo->pll_index];

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = ar0331_video_format_tbl.table[format_index].auto_fps;
	if(fps < ar0331_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, ar0331_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto ar0331_set_fps_exit;
	}

	frame_time = fps;

	ar0331_read_reg(src, 0x300C, &line_length);
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);

	DO_DIV_ROUND(frame_time_pclk, (u32) line_length);
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	ar0331_write_reg(src, 0x300A, (u16)( frame_time_pclk & 0xffff));

	ar0331_set_shutter_time(src, pinfo->current_shutter_time);

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	errCode = ar0331_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto ar0331_set_fps_exit;
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

ar0331_set_fps_exit:
	return errCode;
}

static int ar0331_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct ar0331_info *pinfo;
	u32 format_index;

	pinfo = (struct ar0331_info *) src->pinfo;

	if (index >= AR0331_VIDEO_INFO_TABLE_SZIE) {
		vin_err("ar0331_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto ar0331_set_mode_exit;
	}

	errCode |= ar0331_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = ar0331_video_info_table[index].format_index;

	pinfo->cap_start_x = ar0331_video_info_table[index].def_start_x;
	pinfo->cap_start_y = ar0331_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = ar0331_video_info_table[index].def_width;
	pinfo->cap_cap_h = ar0331_video_info_table[index].def_height;
	pinfo->slvs_eav_col = ar0331_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = ar0331_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = ar0331_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = ar0331_video_format_tbl.table[format_index].pll_index;
	/* FIX ME */
	if(pinfo->op_mode == AR0331_16BIT_HDR_MODE) {
		pinfo->pll_index = 1;
	}

	ar0331_print_info(src);
	//set clk_si
	errCode |= ar0331_init_vin_clock(src, &ar0331_pll_tbl[pinfo->pll_index]);

	errCode |= ar0331_set_vin_mode(src);

	/* Stop Streaming */
	ar0331_stop_streaming(src);

	/* Only initialize after reset */
	if (pinfo->init_flag) {
		ar0331_fill_share_regs(src);
		ar0331_fill_pll_regs(src);
		pinfo->init_flag = 0;
	}

	ar0331_fill_video_format_regs(src);

	ar0331_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	ar0331_set_shutter_time(src, AMBA_VIDEO_FPS_60);

	ar0331_set_agc_db(src, pinfo->agc_info.db_min);

	errCode |= ar0331_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

	/* Enable Streaming */
	ar0331_start_streaming(src);

ar0331_set_mode_exit:
	return errCode;
}

static int ar0331_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int 					errCode = -EINVAL;
	int 					i;
	struct ar0331_info 			*pinfo;

	pinfo = (struct ar0331_info *) src->pinfo;

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < AR0331_VIDEO_MODE_TABLE_SZIE; i++) {
		if (ar0331_video_mode_table[i].mode == mode) {
			errCode = ar0331_set_video_index(src, ar0331_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= AR0331_VIDEO_MODE_TABLE_SZIE) {
		vin_err("ar0331_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = ar0331_video_mode_table[i].mode;
		pinfo->mode_type = ar0331_video_mode_table[i].preview_mode_type;
	}

	return errCode;
}


static int ar0331_set_operation_mode(struct __amba_vin_source *src, amba_vin_sensor_op_mode mode)
{
	int 			errCode = 0;
	struct ar0331_info	*pinfo = (struct ar0331_info *) src->pinfo;

	if((mode != AMBA_VIN_LINEAR_MODE) && (mode != AMBA_VIN_HDR_MODE)){
		vin_err("wrong opeartion mode, %d!\n", mode);
		errCode = -EPERM;
	} else {
		pinfo->op_mode = mode;
		if (pinfo->op_mode == AMBA_VIN_LINEAR_MODE) {
			pinfo->agc_info.db_min = 0x01CC506A;	// 1.798102dB(1.23x)
			pinfo->agc_info.db_max = 0x2A000000;	// 42dB
			pinfo->max_agc_index = AR0331_LINEAR_GAIN_42DB;
		} else {
			pinfo->agc_info.db_min = 0x01CC506A;	// 1.798102dB(1.23x)
			pinfo->agc_info.db_max = 0x1E000000;	// 30dB
			pinfo->max_agc_index = AR0331_GAIN_1_23DB;
		}
	}
	if(!errCode) {
		pinfo->init_flag = 1;
		ar0331_reset(src);
		ar0331_set_video_mode(src, pinfo->current_vin_mode);
	}

	return errCode;
}

static int ar0331_get_sensor_temperatue(struct __amba_vin_source *src, u64 * temperature)
{
	int errCode = 0;
	u16 temp_55, temp_70, temp_now, temp_ctrl;
	u64 slope, t0;

	errCode |= ar0331_write_reg(src, 0x30C6, 0x1D2);
	errCode |= ar0331_write_reg(src, 0x30C8, 0x1BD);

	errCode |= ar0331_read_reg(src, 0x30B4, &temp_ctrl);
	temp_ctrl |= 0x11;//set bit0 and bit 4 to enable temperature sensor
	errCode |= ar0331_write_reg(src, 0x30B4, temp_ctrl);

	errCode |= ar0331_read_reg(src, 0x30C6, &temp_70);
	errCode |= ar0331_read_reg(src, 0x30C8, &temp_55);
	errCode |= ar0331_read_reg(src, 0x30B2, &temp_now);
	temp_now &= 0x3FFF;

	slope = 7680000000;//(70 - 55) * 512000000
	if(temp_70 <= temp_55) {
		vin_err("Calibration value is wrong!\n");
		return -EINVAL;
	}
	DO_DIV_ROUND(slope, (temp_70 - temp_55));
	t0 =  28160000000 - (slope * temp_55);//55 * 512000000 - (slope * temp_55)

	*temperature = slope * temp_now + t0;
	vin_dbg("slope:%lld, t0:%lld, temp:%lld\n", slope, t0, *temperature);

	return errCode;
}


static int ar0331_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	u16				sen_id = 0;
	struct ar0331_info	*pinfo;

	pinfo = (struct ar0331_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = ar0331_init_vin_clock(src, &ar0331_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto ar0331_init_hw_exit;
	msleep(10);
	ar0331_reset(src);

	errCode = ar0331_query_sensor_id(src, &sen_id);
	if (errCode)
		goto ar0331_init_hw_exit;
	/*
	if (sen_id != expected id) {
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		goto ar0331_init_hw_exit;
	}*/

	DRV_PRINT("AR0331 sensor ID is 0x%x\n", sen_id);

ar0331_init_hw_exit:
	return errCode;
}

static int ar0331_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct ar0331_info*pinfo;
	pinfo = (struct ar0331_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		ar0331_reset(src);
		break;

	case AMBA_VIN_SRC_SET_POWER:
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_power, (u32)args);
		break;

	case AMBA_VIN_SRC_SUSPEND:
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_reset, 1);
		ambarella_set_gpio_output(
			&ambarella_board_generic.vin0_power, 0);
		break;

	case AMBA_VIN_SRC_RESUME:
		errCode = ar0331_init_hw(src);
		break;
	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_AR0331;
			pub_src->default_mode = AR0331_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = ar0331_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = ar0331_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = ar0331_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = ar0331_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = ar0331_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = ar0331_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = ar0331_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = ar0331_set_still_mode(src, (struct amba_vin_src_still_info *) args);
		break;

	case AMBA_VIN_SRC_GET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_GET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_SET_BLC:
		errCode = amba_vin_adapter_cmd(src->adapid, AMBA_VIN_ADAP_SET_SW_BLC, (void *) args);
		break;

	case AMBA_VIN_SRC_GET_FRAME_RATE:
		*(u32 *)args = pinfo->frame_rate;
		break;

	case AMBA_VIN_SRC_SET_FRAME_RATE:
		errCode = ar0331_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = ar0331_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_DGAIN_RATIO:
		errCode = ar0331_get_dgain_ratio(src, (struct amba_vin_dgain_info *) args);
		break;

	case AMBA_VIN_SRC_SET_DGAIN_RATIO:
		errCode = ar0331_set_dgain_ratio(src, *(struct amba_vin_dgain_info *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = ar0331_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = ar0331_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = ar0331_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = ar0331_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		ar0331_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver = 0;
			u32 *pdata = (u32 *) args;

			errCode = ar0331_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errCode = ar0331_query_sensor_version(src, &sen_ver);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;

			*pdata = (sen_id << 16) | sen_ver;
		}
		exit_AMBA_VIN_SRC_TEST_GET_DEV_ID:
		break;

	case AMBA_VIN_SRC_TEST_GET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u16 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;

			errCode = ar0331_read_reg(src, subaddr, &data);

			reg_data->data = data;
		}
		break;

	case AMBA_VIN_SRC_TEST_SET_REG_DATA:
		{
			struct amba_vin_test_reg_data *reg_data;
			u16 subaddr;
			u16 data = 0;

			reg_data = (struct amba_vin_test_reg_data *) args;
			subaddr = reg_data->reg;
			data = reg_data->data;

			errCode = ar0331_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_GET_OPERATION_MODE:
		*(amba_vin_sensor_op_mode *)args = pinfo->op_mode;
		break;

	case AMBA_VIN_SRC_SET_OPERATION_MODE:
		errCode = ar0331_set_operation_mode(src, *(amba_vin_sensor_op_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = ar0331_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_GET_SENSOR_TEMPERATURE:
		errCode = ar0331_get_sensor_temperatue(src, (u64 *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
