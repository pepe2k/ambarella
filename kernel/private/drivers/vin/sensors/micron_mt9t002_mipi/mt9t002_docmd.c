/*
 * Filename : mt9t002_docmd.c
 *
 * History:
 *    2012/03/23 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 * This file is produced by perl.
 */

static void _mt9t002_set_reg_0x301A(struct __amba_vin_source *src, u32 restart_frame, u32 streaming, u32 reset)
{
	u16 reg_0x301A; // 0x405c

	reg_0x301A =
		0 << 12                 |    // 1: disable serial interface (HiSPi)
		0 << 11                 |    // force_pll_on
		0 << 10                 |    // DONT restart if bad frame is detected
		0 << 9                  |    // The sensor will produce bad frames as some register changed
		0 << 8                  |    // input buffer related to GPI0/1/2/3 inputs are powered down & disabled
		0 << 7                  |    // Parallel data interface is enabled (dep on bit[6])
		1 << 6                  |    // Parallel interface is driven
		1 << 4                  |    // reset_reg_unused
		1 << 3                  |    // Forbids to change value of SMIA registers
		(streaming>0) << 2      |    // Put the sensor in streaming mode
		(restart_frame>0) << 1  |    // Causes sensor to truncate frame at the end of current row and start integrating next frame
		(reset>0) << 0;                      // Set the bit initiates a reset sequence
	// Typically, in normal streamming mode (group_hold=0, restart_frame=1, standby=0), the value is 0x50DE
	mt9t002_write_reg(src, 0x301A, reg_0x301A); // MT9T002_RESET
}

static void mt9t002_dump_reg(struct __amba_vin_source *src)
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

		mt9t002_read_reg(src, reg_addr, &reg_val);    // Read sensor revision number
		vin_dbg("REG= 0x%04X, 0x%04X\n", reg_addr, reg_val);
	}
}

#if 0
void mt9t002_dump_sequencer(struct __amba_vin_source *src)
{
	int i;
	u8 reg_high = 0xFF;
	u8 reg_low = 0xFF;

	_mt9t002_set_reg_0x301A(src, 0, 0, 0); //Disable Streaming
	mt9t002_write_reg(src, 0x3088, 0xC000);
	DRV_PRINT("----- Sequencer -----\n");
	for (i = 1; i <= 157 /* max entry num of sequencers */; i++) {
		mt9t002_readb_reg(src, 0x3086, &reg_high);
		mt9t002_readb_reg(src, 0x3086, &reg_low);
		DRV_PRINT("%4d 0x%04X\n", i, (reg_high << 8) | reg_low);
	}
}
#endif

static void mt9t002_fill_video_format_regs(struct __amba_vin_source *src)
{
	int i;
	u32 index;
	u32 format_index;
	struct mt9t002_info *pinfo;

	pinfo = (struct mt9t002_info *) src->pinfo;

	vin_dbg("mt9t002_fill_video_format_regs \n");
	index = pinfo->current_video_index;
	format_index = mt9t002_video_info_table[index].format_index;

	for (i = 0; i < MT9T002_VIDEO_FORMAT_REG_NUM; i++) {
		if (mt9t002_video_format_tbl.reg[i] == 0)
			break;

		mt9t002_write_reg(src,
				mt9t002_video_format_tbl.reg[i],
				mt9t002_video_format_tbl.table[format_index].data[i]);
	}

	if (mt9t002_video_format_tbl.table[format_index].ext_reg_fill)
		mt9t002_video_format_tbl.table[format_index].ext_reg_fill(src);
}

static void mt9t002_fill_rev_regs(struct __amba_vin_source *src)
{
	int i, mt9t002_rev_reg_size;
	const struct mt9t002_reg_table *reg_tbl;
	struct mt9t002_info *pinfo;

	pinfo = (struct mt9t002_info *) src->pinfo;
	//FIXME, update it if there is new version
	vin_info("OTPM version:%d\n", pinfo->otpm_ver);
	switch(pinfo->otpm_ver) {
		case 1://AR0330 Rev1, OTPM_Ver1
			mt9t002_rev_reg_size = MT9T002_REV1_REG_SIZE;
			reg_tbl = mt9t002_rev1_regs;
			break;
		case 2://AR0330 Rev2.0, OTPM_Ver2
			mt9t002_rev_reg_size = MT9T002_REV2_REG_SIZE;
			reg_tbl = mt9t002_rev2_regs;
			break;
		case 3://AR0330 Rev2.1, OTPM_Ver3
			mt9t002_rev_reg_size = MT9T002_REV3_REG_SIZE;
			reg_tbl = mt9t002_rev3_regs;
			break;
		case 4://AR0330 Rev2.1, OTPM_Ver4
			mt9t002_rev_reg_size = MT9T002_REV4_REG_SIZE;
			reg_tbl = mt9t002_rev4_regs;
			break;
		case 5://AR0330 Rev2.1, OTPM_Ver5
			mt9t002_rev_reg_size = MT9T002_REV5_REG_SIZE;
			reg_tbl = mt9t002_rev5_regs;
			break;
		default:
			mt9t002_rev_reg_size = 0;/* Do nothing */
			vin_info("Unsupported OTPM version, may need to update the driver");
			break;
	}

	for (i = 0; i < mt9t002_rev_reg_size; i++) {
		mt9t002_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void mt9t002_fill_share_regs(struct __amba_vin_source *src)
{
	int i;
	const struct mt9t002_reg_table *reg_tbl = mt9t002_share_regs;

	mt9t002_fill_rev_regs(src);

	for (i = 0; i < MT9T002_SHARE_REG_SIZE; i++) {
		mt9t002_write_reg(src, reg_tbl[i].reg, reg_tbl[i].data);
	}
}

static void mt9t002_start_streaming(struct __amba_vin_source *src)
{
	_mt9t002_set_reg_0x301A(src, 0/*restart_frame*/, 1/*streaming*/, 0/*reset*/); //Enable Streaming
}

static void mt9t002_sw_reset(struct __amba_vin_source *src)
{
		/* >> TODO << */
	_mt9t002_set_reg_0x301A(src, 0/*restart_frame*/, 0/*streaming*/, 1/*reset*/); //reset sensor
	msleep(100);
}

static void mt9t002_reset(struct __amba_vin_source *src)
{
	AMBA_VIN_HW_RESET();
	mt9t002_sw_reset(src);
}

static void mt9t002_fill_pll_regs(struct __amba_vin_source *src)
{

	int i;
	struct mt9t002_info *pinfo;
	const struct mt9t002_pll_reg_table 	*pll_reg_table;
	const struct mt9t002_reg_table 		*pll_tbl;

	pinfo = (struct mt9t002_info *) src->pinfo;

	vin_dbg("mt9t002_fill_video_fps_regs\n");
	pll_reg_table = &mt9t002_pll_tbl[pinfo->pll_index];
	pll_tbl = pll_reg_table->regs;

	if (pll_tbl != NULL) {
		for (i = 0; i < MT9T002_VIDEO_PLL_REG_TABLE_SIZE; i++) {
			mt9t002_write_reg(src, pll_tbl[i].reg, pll_tbl[i].data);
		}
	}
}

static int mt9t002_get_video_mode( struct __amba_vin_source *src, enum amba_video_mode *p_mode)
{
	int errCode = 0;
	struct mt9t002_info  *pinfo;

	pinfo = (struct mt9t002_info *)src->pinfo;
	*p_mode = pinfo->current_vin_mode;

	return errCode;
}

static int mt9t002_get_vblank_time( struct __amba_vin_source *src,
	u32 *ptime)
{
	int				errCode		= 0;
	int				mode_index;
	u32				format_index;
	u64				v_btime;
	u32				active_lines, frame_length_lines;
	u32				line_length;
	u16				data_val;

	struct mt9t002_info 			*pinfo;
	const struct mt9t002_pll_reg_table 	*pll_reg_table;

	pinfo		= (struct mt9t002_info *)src->pinfo;
	mode_index	= pinfo->current_video_index;
	format_index	= mt9t002_video_info_table[mode_index].format_index;
	pll_reg_table	= &mt9t002_pll_tbl[pinfo->pll_index];

	errCode |= mt9t002_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mt9t002_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	active_lines = mt9t002_video_format_tbl.table[format_index].height;

	v_btime = (u64)line_length * (u64)(frame_length_lines - active_lines) * 1000000000;
	DO_DIV_ROUND(v_btime, (u64)pll_reg_table->pixclk); //ns
	*ptime = v_btime;

	vin_dbg("mt9t002_get_vblank_time() \n"
		"line_length: %d,\nframe_length_lines: %d,\nactive_lines: %d,\nptime: %d\n",
		line_length, frame_length_lines, active_lines, *ptime);

	return errCode;
}

static int mt9t002_get_video_info(struct __amba_vin_source *src, struct amba_video_info *p_video_info)
{
	int 					errCode = 0;
	u32 					index;
	u32 					format_index;
	struct mt9t002_info 			*pinfo;

	pinfo = (struct mt9t002_info *) src->pinfo;

	index = pinfo->current_video_index;

	if (index >= MT9T002_VIDEO_INFO_TABLE_SZIE) {
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
		format_index = mt9t002_video_info_table[index].format_index;
		p_video_info->width = mt9t002_video_info_table[index].def_width;
		p_video_info->height = mt9t002_video_info_table[index].def_height;
		p_video_info->fps = pinfo->frame_rate;
		p_video_info->format = mt9t002_video_format_tbl.table[format_index].format;
		p_video_info->type = mt9t002_video_format_tbl.table[format_index].type;
		p_video_info->bits = mt9t002_video_format_tbl.table[format_index].bits;
		p_video_info->ratio = mt9t002_video_format_tbl.table[format_index].ratio;
		p_video_info->system = AMBA_VIDEO_SYSTEM_AUTO;
		p_video_info->rev = 0;
		p_video_info->pattern= pinfo->bayer_pattern;
	}

	return errCode;
}

static int mt9t002_get_agc_info(struct __amba_vin_source *src, amba_vin_agc_info_t * p_agc_info)
{
	int errCode = 0;
	struct mt9t002_info *pinfo;

	pinfo = (struct mt9t002_info *)src->pinfo;
	*p_agc_info = pinfo->agc_info;

	return errCode;
}
static int mt9t002_get_shutter_info(struct __amba_vin_source *src, amba_vin_shutter_info_t * pshutter_info)
{
	memset(pshutter_info, 0x00, sizeof (amba_vin_shutter_info_t));
	return 0;
}

static int mt9t002_check_video_mode(struct __amba_vin_source *src, struct amba_vin_source_mode_info *p_mode_info)
{
	int errCode = 0;
	int i;
	u32 index;
	u32 format_index;

	p_mode_info->is_supported = 0;
	memset(p_mode_info->fps_table, 0, p_mode_info->fps_table_size);
	memset(&p_mode_info->video_info, 0, sizeof (p_mode_info->video_info));

	for (i = 0; i < MT9T002_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9t002_video_mode_table[i].mode == p_mode_info->mode) {
			if (p_mode_info->mode == AMBA_VIDEO_MODE_AUTO)
				p_mode_info->mode = MT9T002_VIDEO_MODE_TABLE_AUTO;
			p_mode_info->is_supported = 1;

			index = mt9t002_video_mode_table[i].still_index;
			format_index = mt9t002_video_info_table[index].format_index;

			p_mode_info->video_info.width =
				mt9t002_video_info_table[index].def_width;
			p_mode_info->video_info.height =
				mt9t002_video_info_table[index].def_height;
			p_mode_info->video_info.fps =
				mt9t002_video_format_tbl.table[format_index].max_fps;
			p_mode_info->video_info.format =
				mt9t002_video_format_tbl.table[format_index].format;
			p_mode_info->video_info.type =
				mt9t002_video_format_tbl.table[format_index].type;
			p_mode_info->video_info.bits =
				mt9t002_video_format_tbl.table[format_index].bits;
			p_mode_info->video_info.ratio =
				mt9t002_video_format_tbl.table[format_index].ratio;
			p_mode_info->video_info.system =
				AMBA_VIDEO_SYSTEM_AUTO;
			p_mode_info->video_info.rev = 0;
			break;
		}
	}

	return errCode;
}

static int mt9t002_query_sensor_id(struct __amba_vin_source *src, u16 * ss_id)
{
	int errCode = 0;

	errCode = mt9t002_read_reg(src, 0x3000, ss_id);
	return errCode;
}

static int mt9t002_query_sensor_version(struct __amba_vin_source *src, u16 * ver)
{
	int errCode = 0;
	u16 otpm_ver1, otpm_ver2;
	u8  otpm_ver = 0;
	struct mt9t002_info *pinfo;
	pinfo = (struct mt9t002_info *) src->pinfo;

	errCode |= mt9t002_read_reg(src, 0x30F0, &otpm_ver1);
	errCode |= mt9t002_read_reg(src, 0x3072, &otpm_ver2);
	//FIXME, update it if there is new version
	if(otpm_ver1 == 0x1200) {
		otpm_ver = 1;//AR0330 Rev1, OTPM_Ver1
	}else if(otpm_ver1 == 0x1208) {
		switch(otpm_ver2){
			case 0x0:
				otpm_ver = 2;//AR0330 Rev2.0, OTPM_Ver2
				break;
			case 0x6:
				otpm_ver = 3;//AR0330 Rev2.1, OTPM_Ver3
				break;
			case 0x7:
				otpm_ver = 4;//AR0330 Rev2.1, OTPM_Ver4
				break;
			case 0x8:
				otpm_ver = 5;//AR0330 Rev2.1, OTPM_Ver5
				break;
			default:
				otpm_ver = 0;
				break;
		}
	}
	if(otpm_ver == 0){
		vin_info("Unsupported OTPM version 0x%x:0x%x, need to update the driver\n", otpm_ver1, otpm_ver2);
	} else {
		vin_info("OTPM version:%d\n", otpm_ver);
	}
	*ver = otpm_ver;
	pinfo->otpm_ver = otpm_ver;

	return errCode;
}

static int mt9t002_set_still_mode(struct __amba_vin_source *src, struct amba_vin_src_still_info *p_info)
{

	return 0;

}

static int mt9t002_set_low_ligth_agc(struct __amba_vin_source *src, u32 agc_index)
{
		/* >> TODO, for RGB mode only << */
	return 0;

}

static int mt9t002_set_shutter_time(struct __amba_vin_source *src, u32 shutter_time)
{
	u64 exposure_time_q9;
	u16 data_val;
	u32 line_length, frame_length_lines;
	u16 shutter_width;
	int errCode = 0;

	struct mt9t002_info *pinfo;
	const struct mt9t002_pll_reg_table *pll_reg_table;

	pinfo		= (struct mt9t002_info *)src->pinfo;
	pll_reg_table	= &mt9t002_pll_tbl[pinfo->pll_index];

	exposure_time_q9 = shutter_time; 		// time*512*1000000

	errCode |= mt9t002_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mt9t002_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	exposure_time_q9 = exposure_time_q9 * (u64)pll_reg_table->pixclk;

	DO_DIV_ROUND(exposure_time_q9, line_length);
	DO_DIV_ROUND(exposure_time_q9, 512000000);

	shutter_width = (u16) exposure_time_q9;

	/* check if num_time_h > max_lines */
	if(shutter_width > (frame_length_lines - 1) ){
		shutter_width = frame_length_lines - 1; // To avoid exposure time greater than a frame
		vin_dbg("warning: Exposure time is too long\n");
	}

	errCode |= mt9t002_write_reg(src, 0x3012, shutter_width );

	pinfo->current_shutter_time = shutter_time;

	vin_dbg("shutter_width:%d\n", shutter_width);

	return errCode;
}

static int mt9t002_set_shutter_time_row(struct __amba_vin_source *src, u32 row)
{
	u64 exposure_time_q9;
	u16 data_val;
	u32 line_length, frame_length_lines;
	u32 shutter_width;
	int errCode = 0;

	struct mt9t002_info *pinfo;
	const struct mt9t002_pll_reg_table *pll_reg_table;

	pinfo		= (struct mt9t002_info *)src->pinfo;
	pll_reg_table	= &mt9t002_pll_tbl[pinfo->pll_index];

	errCode |= mt9t002_read_reg(src, 0x300C, &data_val);
	line_length = (u32) data_val;
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}

	errCode |= mt9t002_read_reg(src, 0x300A, &data_val);
	frame_length_lines = (u32) data_val;

	shutter_width = row;

	/* FIXME: shutter width: 1 ~ (Frame format(V) - 1) */
	if((shutter_width < 1) ||(shutter_width  > frame_length_lines - 1) ) {
		vin_warn("row number %d err, valid range is %d - %d\n", shutter_width, 1, frame_length_lines - 1);
		return -EPERM;
	}

	errCode |= mt9t002_write_reg(src, 0x3012, (u16)shutter_width);

	exposure_time_q9 = shutter_width;

	exposure_time_q9 = exposure_time_q9 * (u64)line_length * 512000000;
	DO_DIV_ROUND(exposure_time_q9, pll_reg_table->pixclk);

	pinfo->current_shutter_time = (u32)exposure_time_q9;

	vin_dbg("V:%d, shutter_width:%d, shutter_time:%d\n", frame_length_lines, shutter_width, (u32)exposure_time_q9);

	return errCode;
}

static int mt9t002_set_agc_db(struct __amba_vin_source *src, s32 agc_db)
{
	u16 idc_reg;
	struct mt9t002_info *pinfo;
	u32 gain_index;
	s32 db_max;
	s32 db_step;

	vin_dbg("mt9t002_set_agc_db: 0x%x\n", agc_db);

	pinfo = (struct mt9t002_info *) src->pinfo;
	db_max = pinfo->agc_info.db_max;
	db_step = pinfo->agc_info.db_step;

	gain_index = (db_max - agc_db) / db_step;

	if (gain_index > MT9T002_GAIN_0DB)
		gain_index = MT9T002_GAIN_0DB;
	if ((gain_index >= pinfo->min_agc_index) && (gain_index <= pinfo->max_agc_index)) {

		idc_reg = MT9T002_GAIN_TABLE[gain_index][MT9T002_GAIN_COL_REG_AGAIN];
		mt9t002_write_reg(src, 0x3060/*MT9T002_GAIN_COL_REG_AGAIN*/, idc_reg);

		idc_reg = MT9T002_GAIN_TABLE[gain_index][MT9T002_GAIN_COL_REG_DGAIN];
		mt9t002_write_reg(src, 0x305E/*MT9T002_GAIN_COL_REG_DGAIN*/, idc_reg);

		pinfo->current_gain_db = agc_db;

		return 0;
	} else{
		return -1;
	}
}
static int mt9t002_set_mirror_mode(struct __amba_vin_source *src, struct amba_vin_src_mirror_mode *mirror_mode)
{
	int					errCode = 0;
	uint					readmode;
	struct mt9t002_info			*pinfo;
	u32					target_bayer_pattern;
	u16					tmp_reg;

	pinfo = (struct mt9t002_info *) src->pinfo;

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
		readmode = MT9T002_MIRROR_ROW + MT9T002_MIRROR_COLUMN;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_HORRIZONTALLY:
		readmode = MT9T002_MIRROR_ROW;
		target_bayer_pattern = AMBA_VIN_SRC_BAYER_PATTERN_GR;
		break;

	case AMBA_VIN_SRC_MIRROR_VERTICALLY:
		readmode = MT9T002_MIRROR_COLUMN;
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

	errCode |= mt9t002_read_reg(src, 0x3040, &tmp_reg);
	tmp_reg &= (~MT9T002_MIRROR_MASK);
	tmp_reg |= readmode;
	errCode |= mt9t002_write_reg(src, 0x3040, tmp_reg);

	if (mirror_mode->bayer_pattern == AMBA_VIN_SRC_BAYER_PATTERN_AUTO) {
		pinfo->bayer_pattern = target_bayer_pattern;
	}

	return errCode;
}
static int mt9t002_set_anti_flicker(struct __amba_vin_source *src, int anti_option)
{
	int errCode = 0;
	/* >> TODO, for YUV output mode only << */
	return errCode;
}
static int mt9t002_set_slowshutter_mode(struct __amba_vin_source *src, int mode)
{
	int errCode = 0;
	/* >> TODO, for RGB raw output mode only << */
	/* >> Note, this function will be removed latter, use set fps instead << */
	return errCode;
}

static int mt9t002_set_fps(struct __amba_vin_source *src, u32 fps)
{
	int 					errCode = 0;
	struct mt9t002_info 			*pinfo;
	u32 					index;
	u32 					format_index;
	u64					frame_time_pclk;
	u32					frame_time = 0;
	u16 					line_length;
	u8					current_pll_index = 0;
	const struct mt9t002_pll_reg_table 	*pll_reg_table;
	struct amba_vin_irq_fix			vin_irq_fix;

	pinfo = (struct mt9t002_info *) src->pinfo;
	index = pinfo->current_video_index;

	if (index >= MT9T002_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9t002_set_fps index = %d!\n", index);
		errCode = -EPERM;
		goto mt9t002_set_fps_exit;
	}

	format_index = mt9t002_video_info_table[index].format_index;

	if (fps == AMBA_VIDEO_FPS_AUTO)
		fps = mt9t002_video_format_tbl.table[format_index].auto_fps;
	if(fps < mt9t002_video_format_tbl.table[format_index].max_fps){
		vin_err("The max supported fps is %d\n",
			DIV_ROUND(512000000, mt9t002_video_format_tbl.table[format_index].max_fps));
		errCode = -EPERM;
		goto mt9t002_set_fps_exit;
	}

	frame_time = fps;

	/* ToDo: Add specified PLL index*/
	switch(fps) {
	case AMBA_VIDEO_FPS_29_97:
		current_pll_index = 0;
		break;
	default:
		current_pll_index = 1;
		break;
	}

	if(pinfo->pll_index != current_pll_index){
		pinfo->pll_index = current_pll_index;
		mt9t002_fill_pll_regs(src);
		errCode = mt9t002_init_vin_clock(src, &mt9t002_pll_tbl[pinfo->pll_index]);
		if (errCode)
			goto mt9t002_set_fps_exit;
	}

	pll_reg_table = &mt9t002_pll_tbl[pinfo->pll_index];

	errCode = mt9t002_read_reg(src, 0x300C, &line_length);
	if(errCode || line_length == 0) {
		vin_err("line length is 0\n");
		return -EINVAL;
	}
	frame_time_pclk =  frame_time * (u64)(pll_reg_table->pixclk);

	DO_DIV_ROUND(frame_time_pclk, (u32) line_length);
	if(unlikely(!line_length)) {
		vin_err("line length is 0!\n");
		return -EIO;
	}
	DO_DIV_ROUND(frame_time_pclk, 512000000);

	mt9t002_write_reg(src, 0x300A, (u16)( frame_time_pclk & 0xffff));

	mt9t002_set_shutter_time(src, pinfo->current_shutter_time);

	pinfo->frame_rate = fps;

	//set vblank time
	vin_irq_fix.mode = AMBA_VIN_VSYNC_IRQ_IDSP;
	errCode = mt9t002_get_vblank_time(src, &vin_irq_fix.delay);
	if (errCode)
		goto mt9t002_set_fps_exit;
	errCode = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_FIX_ARCH_VSYNC, &vin_irq_fix);

mt9t002_set_fps_exit:
	return errCode;
}

static int mt9t002_set_video_index(struct __amba_vin_source *src, u32 index)
{
	int errCode = 0;
	struct mt9t002_info *pinfo;
	u32 format_index;

	pinfo = (struct mt9t002_info *) src->pinfo;

	if (index >= MT9T002_VIDEO_INFO_TABLE_SZIE) {
		vin_err("mt9t002_set_video_index do not support mode %d!\n", index);
		errCode = -EINVAL;
		goto mt9t002_set_mode_exit;
	}
	errCode |= mt9t002_pre_set_vin_mode(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;
	pinfo->current_video_index = index;
	format_index = mt9t002_video_info_table[index].format_index;

	pinfo->cap_start_x = mt9t002_video_info_table[index].def_start_x;
	pinfo->cap_start_y = mt9t002_video_info_table[index].def_start_y;
	pinfo->cap_cap_w = mt9t002_video_info_table[index].def_width;
	pinfo->cap_cap_h = mt9t002_video_info_table[index].def_height;
	pinfo->slvs_eav_col = mt9t002_video_info_table[index].slvs_eav_col;
	pinfo->slvs_sav2sav_dist = mt9t002_video_info_table[index].slvs_sav2sav_dist;
	pinfo->bayer_pattern = mt9t002_video_info_table[index].bayer_pattern;
	//set the specified PLL index for each sensor_mode.
	pinfo->pll_index = mt9t002_video_format_tbl.table[format_index].pll_index;

	mt9t002_print_info(src);
	//set clk_si
	errCode |= mt9t002_init_vin_clock(src, &mt9t002_pll_tbl[pinfo->pll_index]);
	errCode |= mt9t002_set_vin_mode(src);

	//reset IDSP
	mt9t002_vin_reset_idsp(src);
	//Enable MIPI
	mt9t002_vin_mipi_phy_enable(src, MIPI_2LANE); //2 lanes

	mt9t002_fill_pll_regs(src);
	mt9t002_fill_share_regs(src);
	mt9t002_fill_video_format_regs(src);
	mt9t002_set_fps(src, AMBA_VIDEO_FPS_AUTO);

	/* Enable Streaming */
	mt9t002_start_streaming(src);
	msleep(3);
	mt9t002_vin_mipi_reset(src);

	errCode |= mt9t002_post_set_vin_mode(src);
	if (!errCode)
		pinfo->mode_type = AMBA_VIN_SRC_ENABLED_FOR_VIDEO;

mt9t002_set_mode_exit:
	return errCode;
}

static int mt9t002_set_video_mode(struct __amba_vin_source *src, enum amba_video_mode mode)
{
	int 					errCode = -EINVAL;
	int 					i;
	struct mt9t002_info 			*pinfo;
	int 					errorCode = 0;
	static int 				first_set_video_mode = 1;

	if(mode == AMBA_VIDEO_MODE_OFF){
		AMBA_VIN_HW_POWEROFF();
		return 0;
	}else{
		AMBA_VIN_HW_POWERON();
	}

	pinfo = (struct mt9t002_info *) src->pinfo;

	/* Hardware Initialization */
	if (first_set_video_mode) {
		//provide a temporary clk_si for sensor reset at first time called
		errorCode = mt9t002_init_vin_clock(src, &mt9t002_pll_tbl[pinfo->pll_index]);
		if (errorCode)
			goto mt9t002_set_video_mode_exit;
		msleep(10);
		first_set_video_mode = 0;
	}
	mt9t002_reset(src);

	pinfo->mode_type = AMBA_VIN_SRC_DISABLED;

	for (i = 0; i < MT9T002_VIDEO_MODE_TABLE_SZIE; i++) {
		if (mt9t002_video_mode_table[i].mode == mode) {
			errCode = mt9t002_set_video_index(src, mt9t002_video_mode_table[i].preview_index);
			pinfo->mode_index = i;
			break;
		}
	}
	if (i >= MT9T002_VIDEO_MODE_TABLE_SZIE) {
		vin_err("mt9t002_set_video_mode do not support %d, %d!\n", mode, i);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
	}

	if (!errCode) {
		pinfo->current_vin_mode = mt9t002_video_mode_table[i].mode;
		pinfo->mode_type = mt9t002_video_mode_table[i].preview_mode_type;
	}

mt9t002_set_video_mode_exit:
	return errCode;
}

static int mt9t002_init_hw(struct __amba_vin_source *src)
{
	int				errCode = 0;
	u16				sen_id = 0;
	u16 			ver;
	struct mt9t002_info	*pinfo;

	pinfo = (struct mt9t002_info *) src->pinfo;

	/* Hardware Initialization */
	/* Set a default CLK_SI to querry sensor id */
	errCode = mt9t002_init_vin_clock(src, &mt9t002_pll_tbl[pinfo->pll_index]);
	if (errCode)
		goto mt9t002_init_hw_exit;
	msleep(10);
	mt9t002_reset(src);

	errCode = mt9t002_query_sensor_id(src, &sen_id);
	if (errCode)
		goto mt9t002_init_hw_exit;
	DRV_PRINT("MT9T002 sensor ID is 0x%x\n", sen_id);

	errCode = mt9t002_query_sensor_version(src, &ver);
	if (errCode)
		goto mt9t002_init_hw_exit;

mt9t002_init_hw_exit:
	AMBA_VIN_HW_POWEROFF();
	return errCode;
}

static int mt9t002_docmd(struct __amba_vin_source *src, enum amba_vin_src_cmd cmd, void *args)
{
	int errCode = 0;
	struct mt9t002_info*pinfo;
	pinfo = (struct mt9t002_info *) src->pinfo;

	vin_dbg("\t\t---->cmd is %d\n", cmd);
	switch (cmd) {
	case AMBA_VIN_SRC_RESET:
		mt9t002_reset(src);
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
		errCode = mt9t002_init_hw(src);
		break;

	case AMBA_VIN_SRC_GET_INFO:
		{
			struct amba_vin_source_info *pub_src;

			pub_src = (struct amba_vin_source_info *) args;
			pub_src->id = src->id;
			pub_src->adapter_id = adapter_id;
			pub_src->dev_type = src->dev_type;
			strlcpy(pub_src->name, src->name, sizeof (pub_src->name));
			pub_src->sensor_id = SENSOR_MT9T002;
			pub_src->default_mode = MT9T002_VIDEO_MODE_TABLE_AUTO;
			pub_src->total_channel_num = src->total_channel_num;
			pub_src->active_channel_id = src->active_channel_id;
			pub_src->source_type.decoder = AMBA_VIN_CMOS_CHANNEL_TYPE_AUTO;
			pub_src->interface_type = SENSOR_IF_MIPI;
		}
		break;

	case AMBA_VIN_SRC_CHECK_VIDEO_MODE:
		errCode = mt9t002_check_video_mode(src, (struct amba_vin_source_mode_info *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_INFO:
		errCode = mt9t002_get_video_info(src, (struct amba_video_info *) args);
		break;

	case AMBA_VIN_SRC_GET_AGC_INFO:
		errCode = mt9t002_get_agc_info(src, (amba_vin_agc_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_INFO:
		errCode = mt9t002_get_shutter_info(src, (amba_vin_shutter_info_t *) args);
		break;

	case AMBA_VIN_SRC_GET_CAPABILITY:
		errCode = mt9t002_get_capability(src, (struct amba_vin_src_capability *) args);
		break;

	case AMBA_VIN_SRC_GET_VIDEO_MODE:
		errCode = mt9t002_get_video_mode(src,(enum amba_video_mode *)args);
		break;

	case AMBA_VIN_SRC_SET_VIDEO_MODE:
		errCode = mt9t002_set_video_mode(src, *(enum amba_video_mode *) args);
		break;

	case AMBA_VIN_SRC_GET_STILL_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_STILL_MODE:
		errCode = mt9t002_set_still_mode(src, (struct amba_vin_src_still_info *) args);
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
		errCode = mt9t002_set_fps(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_GAIN_DB:
		*(u32 *)args = pinfo->current_gain_db;
		break;
	case AMBA_VIN_SRC_SET_GAIN_DB:
		errCode = mt9t002_set_agc_db(src, *(s32 *) args);
		break;

	case AMBA_VIN_SRC_GET_SHUTTER_TIME:
		*(u32 *)args = pinfo->current_shutter_time;
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME:
		errCode = mt9t002_set_shutter_time(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_GET_LOW_LIGHT_MODE:
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;
		break;

	case AMBA_VIN_SRC_SET_LOW_LIGHT_MODE:
		errCode = mt9t002_set_low_ligth_agc(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_SET_MIRROR_MODE:
		errCode = mt9t002_set_mirror_mode(src, (struct amba_vin_src_mirror_mode *) args);
		break;

	case AMBA_VIN_SRC_SET_ANTI_FLICKER:
		errCode = mt9t002_set_anti_flicker(src, *(u32 *) args);
		break;

	case AMBA_VIN_SRC_TEST_DUMP_REG:
		mt9t002_dump_reg(src);
		break;

	case AMBA_VIN_SRC_TEST_GET_DEV_ID:
		{
			u16 sen_id = 0;
			u16 sen_ver = 0;
			u32 *pdata = (u32 *) args;

			errCode = mt9t002_query_sensor_id(src, &sen_id);
			if (errCode)
				goto exit_AMBA_VIN_SRC_TEST_GET_DEV_ID;
			errCode = mt9t002_query_sensor_version(src, &sen_ver);
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

			errCode = mt9t002_read_reg(src, subaddr, &data);

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

			errCode = mt9t002_write_reg(src, subaddr, data);
		}
		break;

	case AMBA_VIN_SRC_SET_TRIGGER_MODE:
		break;

	case AMBA_VIN_SRC_SET_CAPTURE_MODE:
		break;

	case AMBA_VIN_SRC_SET_SLOWSHUTTER_MODE:
		errCode = mt9t002_set_slowshutter_mode(src, *(int *) args);
		break;

	case AMBA_VIN_SRC_SET_SHUTTER_TIME_ROW:
		errCode = mt9t002_set_shutter_time_row(src, *(u32 *) args);
		break;

	default:
		vin_err("%s-%d do not support cmd %d!\n", src->name, src->id, cmd);
		errCode = AMBA_ERR_FUNC_NOT_SUPPORTED;

		break;
	}

	return errCode;
}
