/**********************************************************************
 *
 * mw_aaa_params.h
 *
 * History:
 *	2012/12/10 - [Jingyang Qiu] Created this file
 *
 * Copyright (C) 2012 - 2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 *********************************************************************/

#ifndef __SENSOR_ADJ_AEB_FILE_H__
#define __SENSOR_ADJ_AEB_FILE_H__

#include "mw_struct.h"
typedef enum  {
	LIST_CHROMA_SCALE,
	LIST_RGB2YUV,
	LIST_ADJ_PARAM,
	LIST_MANUAL_LE,
	LIST_TILE_CONFIG,
	LIST_LINES,
	LIST_AWB_PARAM,
	LIST_AE_AGC_DGAIN,
	LIST_AE_SHT_DGAIN,
	LIST_DLIGHT,
	LIST_SENSOR_CONFIG,
	LIST_TOTAL_NUM,
} LIST_ID;

/// 32 bytes header
typedef struct ambarella_adj_aeb_bin_header_s {
	u32	magic_number;
	u16	header_ver_major;
	u16	header_ver_minor;
	u32	header_size;
	u32	payload_size;
	u32	sensor_id;
	u16	reserved[6];
} ambarella_adj_aeb_bin_header_t;

///////count data size
typedef struct ambarella_data_struct_header_s {
	u32	struct_size;
	u16	struct_total_num;
	u16	struct_align;
} ambarella_data_struct_header_t;

typedef struct ambarella_data_header_s {
	u8	file_name[FILE_NAME_LENGTH];
	ambarella_data_struct_header_t	struct_type[LIST_TOTAL_NUM];
} ambarella_data_header_t;


typedef struct ambarella_adj_aeb_bin_parse_s {
	ambarella_adj_aeb_bin_header_t	bin_header;
	ambarella_data_header_t	data_header;
} ambarella_adj_aeb_bin_parse_t;

typedef struct aaa_files_s {
	char		adj_file[FILE_NAME_LENGTH];
	char		aeb_file[FILE_NAME_LENGTH];
	char		lens_file[FILE_NAME_LENGTH];
} aaa_files_t;

typedef struct sensor_model_s {
	u32		sensor_id;
	char		name[32];
	u32		default_fps;
	u32		current_fps;
	u32		vin_mode;
	u32		sensor_op_mode;
	u32		offset_num[LIST_TOTAL_NUM];
	aaa_files_t	load_files;
	u32		lens_id;
	u32		sensor_slow_shutter;
} sensor_model_t;

ambarella_adj_aeb_bin_header_t G_adj_bin_header;

#endif

