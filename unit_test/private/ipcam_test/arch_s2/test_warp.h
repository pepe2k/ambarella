/********************************************************************
 * test_warp.h
 *
 * History:
 *	2012/03/23 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ********************************************************************/

#ifndef __TEST_WARP_H__
#define __TEST_WARP_H__


#define FILENAME_LENGTH	(32)

typedef enum {
	MAP_GRID_TYPE_FIRST = 0,
	MAP_GRID_HORIZONTAL = 0,
	MAP_GRID_VERTICAL,
	MAX_MAP_GRID_TYPE,
	MAP_GRID_TYPE_LAST = MAX_MAP_GRID_TYPE,
	MAP_GRID_TYPE_INVALID = 255,
} MAP_GRID_TYPE;

enum {
	FISHEYE_WARP_CONTROL = 0,
	FISHEYE_WARP_PROC = 1,
};

typedef struct warp_region_s {
	int	input_src_rid;
	int	input_src_rid_flag;

	int	input_width;
	int	input_height;
	int	input_size_flag;

	int	input_offset_x;
	int	input_offset_y;
	int	input_offset_flag;

	int	output_width;
	int	output_height;
	int	output_size_flag;

	int	output_offset_x;
	int	output_offset_y;
	int	output_offset_flag;

	int	rotate;
	int	rotate_flag;

	int	hor_grid_col;
	int	hor_grid_row;
	int	hor_grid_size_flag;

	int	hor_hor_grid_spacing;
	int	hor_hor_grid_flag;
	int	hor_ver_grid_spacing;
	int	hor_ver_grid_flag;

	char	hor_file[FILENAME_LENGTH];
	int	hor_file_flag;

	int	ver_grid_col;
	int	ver_grid_row;
	int	ver_grid_size_flag;

	int	ver_hor_grid_spacing;
	int	ver_hor_grid_flag;
	int	ver_ver_grid_spacing;
	int	ver_ver_grid_flag;

	char	ver_file[FILENAME_LENGTH];
	int	ver_file_flag;
} warp_region_t;

typedef struct warp_region_dptz_s {
	int	input_w;
	int	input_h;
	int	input_size_flag;
	int	input_x;
	int	input_y;
	int	input_offset_flag;
	int	output_w;
	int	output_h;
	int	output_size_flag;
	int	output_x;
	int	output_y;
	int	output_offset_flag;
} warp_region_dptz_t;

static int current_area = -1;
static int current_buffer = -1;
static MAP_GRID_TYPE grid_type = MAP_GRID_TYPE_INVALID;
static warp_region_t warp_region[MAX_NUM_WARP_AREAS];
static warp_region_dptz_t warp_region_dptz[MAX_NUM_WARP_AREAS];

static Mapping WarpMap[] = {
	/* Warp region 0 */
	{"a0_input_size_update",	&warp_region[0].input_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_input_width",		&warp_region[0].input_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_input_height",		&warp_region[0].input_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_input_offset_update",	&warp_region[0].input_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_input_offset_x",	&warp_region[0].input_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_input_offset_y",	&warp_region[0].input_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_size_update",	&warp_region[0].output_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_width",		&warp_region[0].output_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_height",		&warp_region[0].output_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_offset_update",	&warp_region[0].output_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_offset_x",	&warp_region[0].output_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_output_offset_y",	&warp_region[0].output_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_rotate_update",	&warp_region[0].rotate_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_rotate",			&warp_region[0].rotate,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_hor_grid_size_update",	&warp_region[0].hor_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_hor_grid_col",		&warp_region[0].hor_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0, },
	{"a0_hor_grid_row",		&warp_region[0].hor_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0, },
	{"a0_hor_file_update",		&warp_region[0].hor_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_hor_file",			warp_region[0].hor_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a0_hor_hor_grid_update",	&warp_region[0].hor_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_hor_hor_grid_spacing",	&warp_region[0].hor_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a0_hor_ver_grid_update",	&warp_region[0].hor_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_hor_ver_grid_spacing",	&warp_region[0].hor_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a0_ver_grid_size_update",	&warp_region[0].ver_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_ver_grid_col",		&warp_region[0].ver_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a0_ver_grid_row",		&warp_region[0].ver_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a0_ver_file_update",		&warp_region[0].ver_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_ver_file",			&warp_region[0].ver_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a0_ver_hor_grid_update",	&warp_region[0].ver_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_ver_hor_grid_spacing",	&warp_region[0].ver_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a0_ver_ver_grid_update",	&warp_region[0].ver_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a0_ver_ver_grid_spacing",	&warp_region[0].ver_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },

	/* Warp region 1 */
	{"a1_input_size_update",	&warp_region[1].input_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_input_width",		&warp_region[1].input_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_input_height",		&warp_region[1].input_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_input_offset_update",	&warp_region[1].input_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_input_offset_x",	&warp_region[1].input_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_input_offset_y",	&warp_region[1].input_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_size_update",	&warp_region[1].output_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_width",		&warp_region[1].output_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_height",		&warp_region[1].output_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_offset_update",	&warp_region[1].output_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_offset_x",	&warp_region[1].output_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_output_offset_y",	&warp_region[1].output_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_rotate_update",	&warp_region[1].rotate_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_rotate",			&warp_region[1].rotate,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_hor_grid_size_update",	&warp_region[1].hor_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_hor_grid_col",		&warp_region[1].hor_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a1_hor_grid_row",		&warp_region[1].hor_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a1_hor_file_update",		&warp_region[1].hor_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_hor_file",			&warp_region[1].hor_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a1_hor_hor_grid_update",	&warp_region[1].hor_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_hor_hor_grid_spacing",	&warp_region[1].hor_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a1_hor_ver_grid_update",	&warp_region[1].hor_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_hor_ver_grid_spacing",	&warp_region[1].hor_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a1_ver_grid_size_update",	&warp_region[1].ver_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_ver_grid_col",		&warp_region[1].ver_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a1_ver_grid_row",		&warp_region[1].ver_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a1_ver_file_update",		&warp_region[1].ver_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_ver_file",			&warp_region[1].ver_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a1_ver_hor_grid_update",	&warp_region[1].ver_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_ver_hor_grid_spacing",	&warp_region[1].ver_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a1_ver_ver_grid_update",	&warp_region[1].ver_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a1_ver_ver_grid_spacing",	&warp_region[1].ver_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },

	/* Warp region 2 */
	{"a2_input_size_update",	&warp_region[2].input_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_input_width",		&warp_region[2].input_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_input_height",		&warp_region[2].input_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_input_offset_update",	&warp_region[2].input_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_input_offset_x",	&warp_region[2].input_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_input_offset_y",	&warp_region[2].input_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_size_update",	&warp_region[2].output_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_width",		&warp_region[2].output_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_height",		&warp_region[2].output_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_offset_update",	&warp_region[2].output_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_offset_x",	&warp_region[2].output_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_output_offset_y",	&warp_region[2].output_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_rotate_update",	&warp_region[2].rotate_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_rotate",			&warp_region[2].rotate,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_hor_grid_size_update",	&warp_region[2].hor_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_hor_grid_col",		&warp_region[2].hor_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a2_hor_grid_row",		&warp_region[2].hor_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a2_hor_file_update",		&warp_region[2].hor_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_hor_file",			&warp_region[2].hor_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a2_hor_hor_grid_update",	&warp_region[2].hor_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_hor_hor_grid_spacing",	&warp_region[2].hor_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a2_hor_ver_grid_update",	&warp_region[2].hor_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_hor_ver_grid_spacing",	&warp_region[2].hor_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a2_ver_grid_size_update",	&warp_region[2].ver_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_ver_grid_col",		&warp_region[2].ver_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a2_ver_grid_row",		&warp_region[2].ver_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a2_ver_file_update",		&warp_region[2].ver_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_ver_file",			&warp_region[2].ver_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a2_ver_hor_grid_update",	&warp_region[2].ver_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_ver_hor_grid_spacing",	&warp_region[2].ver_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a2_ver_ver_grid_update",	&warp_region[2].ver_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a2_ver_ver_grid_spacing",	&warp_region[2].ver_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },

	/* Warp region 3 */
	{"a3_input_size_update",	&warp_region[3].input_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_input_width",		&warp_region[3].input_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_input_height",		&warp_region[3].input_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_input_offset_update",	&warp_region[3].input_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_input_offset_x",	&warp_region[3].input_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_input_offset_y",	&warp_region[3].input_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_size_update",	&warp_region[3].output_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_width",		&warp_region[3].output_width,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_height",		&warp_region[3].output_height,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_offset_update",	&warp_region[3].output_offset_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_offset_x",	&warp_region[3].output_offset_x,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_output_offset_y",	&warp_region[3].output_offset_y,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_rotate_update",	&warp_region[3].rotate_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_rotate",			&warp_region[3].rotate,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_hor_grid_size_update",	&warp_region[3].hor_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_hor_grid_col",		&warp_region[3].hor_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a3_hor_grid_row",		&warp_region[3].hor_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a3_hor_file_update",		&warp_region[3].hor_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_hor_file",			&warp_region[3].hor_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a3_hor_hor_grid_update",	&warp_region[3].hor_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_hor_hor_grid_spacing",	&warp_region[3].hor_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a3_hor_ver_grid_update",	&warp_region[3].hor_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_hor_ver_grid_spacing",	&warp_region[3].hor_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a3_ver_grid_size_update",	&warp_region[3].ver_grid_size_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_ver_grid_col",		&warp_region[3].ver_grid_col,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	32.0 },
	{"a3_ver_grid_row",		&warp_region[3].ver_grid_row,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	48.0 },
	{"a3_ver_file_update",		&warp_region[3].ver_file_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_ver_file",			&warp_region[3].ver_file,		MAP_TO_STRING,	0.0,	NO_LIMIT,	0.0,	0.0,	FILENAME_LENGTH },
	{"a3_ver_hor_grid_update",	&warp_region[3].ver_hor_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_ver_hor_grid_spacing",	&warp_region[3].ver_hor_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },
	{"a3_ver_ver_grid_update",	&warp_region[3].ver_ver_grid_flag,	MAP_TO_S32,	0.0,	NO_LIMIT,	0.0,	0.0, },
	{"a3_ver_ver_grid_spacing",	&warp_region[3].ver_ver_grid_spacing,	MAP_TO_S32,	0.0,	MIN_MAX_LIMIT,	0.0,	128.0, },

	{NULL,	NULL,	-1,	0.0,	0,	0.0,	0.0, },
};

static s16 wt_hor[MAX_NUM_WARP_AREAS][MAX_WARP_TABLE_SIZE] = {
	[0] = {
 4376, 3880, 3428, 3016, 2644, 2304, 1996, 1720, 1468, 1244, 1040,  848,  676,  516,  368,  228,
   92,  -40, -176, -316, -460, -616, -780, -960,-1160,-1380,-1620,-1888,-2180,-2512,-2872,    0,

 4128, 3652, 3212, 2820, 2456, 2132, 1840, 1576, 1344, 1128,  940,  764,  604,  468,  332,  208,
   84,  -40, -160, -284, -416, -552, -700, -872,-1056,-1260,-1484,-1740,-2020,-2332,-2676,    0,

 3896, 3436, 3008, 2624, 2280, 1972, 1692, 1444, 1224, 1024,  848,  692,  548,  416,  296,  184,
   76,  -36, -144, -252, -372, -496, -636, -792, -956,-1144,-1356,-1600,-1864,-2164,-2492,    0,

 3684, 3224, 2820, 2448, 2120, 1816, 1556, 1316, 1112,  924,  760,  620,  488,  368,  264,  164,
   68,  -32, -124, -224, -328, -440, -568, -704, -864,-1044,-1240,-1460,-1712,-2000,-2320,    0,

 3480, 3040, 2640, 2280, 1964, 1676, 1428, 1204, 1012,  836,  688,  548,  428,  328,  232,  140,
   60,  -28, -108, -196, -288, -388, -504, -628, -780, -944,-1132,-1336,-1580,-1848,-2156,    0,

 3292, 2864, 2476, 2132, 1820, 1548, 1308, 1096,  912,  748,  608,  484,  376,  284,  200,  120,
   52,  -24,  -96, -168, -252, -340, -440, -560, -696, -852,-1024,-1228,-1452,-1716,-2008,    0,

 3120, 2700, 2324, 1980, 1684, 1428, 1200, 1000,  820,  668,  536,  424,  328,  244,  172,  104,
   44,  -20,  -80, -144, -216, -296, -384, -496, -620, -760, -924,-1120,-1340,-1580,-1868,    0,

 2952, 2548, 2180, 1856, 1576, 1316, 1100,  900,  736,  592,  472,  372,  284,  212,  144,   88,
   36,  -16,  -68, -124, -184, -252, -332, -436, -548, -680, -840,-1020,-1232,-1476,-1740,    0,

 2812, 2412, 2052, 1740, 1464, 1216, 1004,  816,  664,  524,  412,  320,  240,  180,  120,   72,
   32,  -16,  -56, -104, -156, -216, -288, -380, -480, -604, -756, -932,-1132,-1364,-1632,    0,

 2676, 2280, 1936, 1632, 1360, 1124,  916,  744,  592,  468,  360,  276,  204,  148,   96,   60,
   24,  -12,  -48,  -88, -132, -180, -252, -324, -424, -544, -680, -848,-1044,-1268,-1528,    0,

 2556, 2172, 1836, 1540, 1272, 1036,  840,  676,  532,  408,  312,  232,  172,  124,   84,   52,
   20,  -12,  -36,  -64, -104, -148, -212, -280, -376, -480, -616, -772, -964,-1176,-1428,    0,

 2456, 2080, 1740, 1448, 1188,  968,  776,  612,  480,  360,  276,  200,  140,  100,   64,   40,
   16,   -8,  -28,  -52,  -84, -128, -180, -244, -324, -436, -560, -712, -892,-1104,-1348,    0,

 2368, 1996, 1668, 1380, 1120,  900,  712,  560,  428,  320,  240,  168,  116,   76,   52,   28,
   12,   -8,  -20,  -40,  -68, -100, -144, -212, -288, -388, -508, -656, -832,-1036,-1276,    0,

 2288, 1932, 1600, 1316, 1064,  852,  672,  512,  396,  288,  208,  140,   96,   64,   36,   20,
    8,   -4,  -16,  -32,  -52,  -84, -120, -176, -248, -348, -468, -608, -776, -976,-1220,    0,

 2224, 1868, 1548, 1264, 1016,  808,  624,  480,  360,  256,  184,  120,   76,   44,   24,   12,
    4,   -4,  -12,  -24,  -40,  -64, -104, -156, -228, -316, -428, -564, -736, -940,-1168,    0,

 2180, 1828, 1504, 1224,  976,  768,  600,  448,  328,  236,  164,  100,   68,   40,   24,    8,
    0,   -4,   -8,  -12,  -24,  -52,  -88, -140, -204, -296, -400, -540, -708, -896,-1128,    0,

 2144, 1784, 1476, 1200,  956,  752,  576,  432,  312,  216,  148,   92,   56,   32,   12,    4,
    4,    0,   -4,  -12,  -20,  -44,  -72, -124, -192, -272, -384, -516, -680, -876,-1104,    0,

 2132, 1772, 1456, 1180,  940,  736,  560,  420,  304,  208,  136,   84,   44,   20,    8,    4,
    0,   -4,    0,   -4,  -20,  -32,  -68, -116, -180, -264, -372, -504, -664, -860,-1084,    0,

 2128, 1768, 1452, 1176,  936,  732,  560,  420,  304,  208,  140,   84,   48,   24,   12,    4,
    0,    0,   -4,   -8,  -20,  -40,  -72, -116, -180, -264, -372, -504, -664, -856,-1080,    0,

 2136, 1772, 1456, 1180,  940,  736,  564,  420,  304,  208,  136,   84,   44,   24,    8,    0,
    0,    0,    0,    0,  -16,  -40,  -68, -116, -180, -264, -372, -504, -668, -860,-1084,    0,

 2152, 1800, 1480, 1200,  960,  752,  576,  432,  312,  224,  148,   88,   56,   28,   16,    4,
    0,    0,   -4,  -12,  -24,  -40,  -80, -124, -192, -280, -384, -520, -684, -876,-1104,    0,

 2184, 1832, 1508, 1224,  980,  780,  600,  448,  336,  236,  164,  108,   64,   36,   16,    4,
    4,   -4,   -8,  -12,  -28,  -56,  -88, -140, -208, -296, -408, -540, -708, -908,-1128,    0,

 2228, 1880, 1552, 1268, 1024,  812,  636,  480,  360,  260,  184,  128,   80,   48,   24,   16,
    4,   -4,  -12,  -24,  -40,  -68, -112, -164, -228, -316, -440, -576, -740, -944,-1168,    0,

 2300, 1936, 1604, 1320, 1076,  856,  672,  520,  396,  288,  204,  148,   96,   64,   40,   24,
    8,   -4,  -16,  -28,  -52,  -84, -128, -180, -256, -356, -468, -608, -780, -988,-1220,    0,

 2376, 2008, 1672, 1384, 1132,  912,  724,  564,  432,  328,  236,  176,  120,   80,   48,   32,
   12,   -8,  -20,  -44,  -68, -104, -148, -216, -296, -392, -512, -656, -836,-1040,-1280,    0,

 2468, 2088, 1756, 1464, 1200,  972,  784,  616,  484,  368,  280,  208,  144,  100,   68,   40,
   16,   -8,  -28,  -56,  -84, -128, -180, -248, -332, -440, -564, -716, -900,-1108,-1356,    0,

 2572, 2188, 1852, 1548, 1288, 1048,  848,  680,  536,  416,  320,  244,  172,  120,   88,   48,
   20,  -12,  -40,  -68, -104, -152, -216, -284, -380, -488, -624, -784, -972,-1188,-1444,    0,

 2696, 2300, 1948, 1648, 1372, 1136,  928,  752,  600,  472,  368,  280,  208,  148,  104,   60,
   24,  -12,  -48,  -88, -132, -188, -252, -332, -436, -548, -688, -860,-1048,-1276,-1544,    0,

 2828, 2428, 2064, 1752, 1472, 1228, 1016,  828,  672,  536,  420,  324,  252,  180,  124,   76,
   32,  -16,  -60, -104, -156, -224, -296, -384, -492, -612, -764, -944,-1148,-1376,-1648,    0,

 2972, 2568, 2200, 1868, 1584, 1332, 1112,  912,  744,  608,  480,  372,  292,  212,  148,   92,
   36,  -20,  -68, -124, -188, -260, -344, -440, -556, -688, -848,-1032,-1240,-1484,-1756,    0,

 3136, 2716, 2344, 2004, 1700, 1440, 1208, 1012,  832,  676,  548,  432,  332,  252,  176,  108,
   44,  -20,  -84, -148, -220, -304, -396, -504, -628, -772, -944,-1136,-1356,-1596,-1884,    0,

 3316, 2884, 2496, 2148, 1836, 1560, 1324, 1108,  920,  756,  616,  492,  384,  288,  204,  124,
   52,  -24,  -96, -172, -256, -348, -448, -568, -700, -860,-1032,-1240,-1472,-1732,-2024,    0,

 3508, 3064, 2664, 2300, 1984, 1692, 1440, 1224, 1020,  848,  692,  560,  436,  332,  232,  144,
   60,  -28, -112, -196, -292, -396, -512, -640, -784, -952,-1144,-1352,-1596,-1868,-2176,    0,

 3708, 3252, 2844, 2468, 2140, 1836, 1572, 1328, 1120,  944,  776,  628,  492,  376,  264,  164,
   68,  -32, -128, -228, -336, -448, -572, -716, -872,-1056,-1252,-1476,-1728,-2020,-2340,    0,

 3868, 3412, 2984, 2600, 2260, 1952, 1676, 1432, 1204, 1008,  840,  680,  540,  412,  292,  180,
   72,  -36, -140, -248, -368, -492, -628, -776, -940,-1132,-1340,-1576,-1844,-2144,-2468,    0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	}
};

static s16 wt_ver[MAX_NUM_WARP_AREAS][MAX_WARP_TABLE_SIZE] = {
	[0] = {
 2352, 2220, 2092, 1976, 1868, 1768, 1676, 1592, 1516, 1452, 1392, 1336, 1292, 1260, 1232, 1220,
 1204, 1208, 1216, 1228, 1248, 1280, 1320, 1364, 1424, 1488, 1560, 1644, 1728, 1828, 1932,    0,

 2092, 1972, 1852, 1744, 1640, 1544, 1456, 1376, 1308, 1244, 1188, 1140, 1096, 1072, 1048, 1036,
 1016, 1020, 1032, 1040, 1060, 1088, 1120, 1172, 1224, 1284, 1348, 1428, 1512, 1604, 1700,    0,

 1860, 1744, 1632, 1524, 1432, 1344, 1260, 1184, 1120, 1060, 1008,  968,  928,  900,  876,  860,
  848,  852,  856,  868,  892,  916,  956,  996, 1040, 1096, 1160, 1236, 1312, 1400, 1492,    0,

 1648, 1536, 1432, 1336, 1248, 1160, 1088, 1016,  956,  900,  852,  812,  780,  744,  728,  716,
  704,  700,  704,  720,  740,  760,  800,  832,  884,  936,  996, 1056, 1132, 1212, 1300,    0,

 1452, 1348, 1252, 1160, 1080, 1000,  928,  864,  812,  760,  716,  672,  636,  616,  596,  580,
  572,  568,  572,  580,  600,  628,  660,  696,  744,  792,  848,  904,  972, 1044, 1128,    0,

 1276, 1180, 1088, 1004,  928,  856,  792,  732,  680,  632,  588,  548,  520,  496,  480,  460,
  460,  456,  460,  468,  488,  508,  536,  576,  616,  664,  712,  772,  828,  900,  976,    0,

 1116, 1028,  944,  864,  792,  728,  672,  616,  564,  520,  476,  444,  416,  396,  380,  364,
  360,  356,  364,  368,  388,  408,  432,  468,  504,  548,  592,  648,  708,  764,  836,    0,

  968,  888,  808,  740,  680,  616,  564,  508,  464,  420,  388,  356,  332,  312,  292,  284,
  276,  276,  276,  292,  300,  320,  340,  376,  408,  448,  492,  544,  596,  656,  712,    0,

  836,  764,  692,  632,  576,  516,  468,  420,  380,  340,  308,  280,  256,  240,  220,  208,
  204,  208,  212,  220,  236,  248,  268,  300,  328,  364,  404,  452,  496,  552,  608,    0,

  716,  648,  588,  536,  480,  432,  384,  344,  304,  272,  244,  216,  196,  180,  160,  152,
  148,  148,  156,  164,  176,  188,  212,  232,  260,  292,  328,  368,  412,  464,  516,    0,

  608,  548,  496,  448,  400,  352,  312,  276,  244,  212,  188,  164,  144,  132,  120,  116,
  100,  104,  104,  108,  124,  136,  156,  176,  204,  232,  264,  300,  340,  380,  428,    0,

  508,  460,  412,  368,  328,  288,  252,  220,  192,  164,  144,  124,  104,   96,   84,   76,
   72,   68,   68,   80,   88,  104,  116,  136,  156,  184,  212,  240,  276,  312,  352,    0,

  420,  380,  340,  300,  264,  228,  200,  172,  148,  124,  108,   88,   76,   64,   56,   52,
   44,   44,   44,   52,   60,   72,   84,  100,  120,  140,  164,  192,  220,  252,  288,    0,

  336,  304,  272,  240,  208,  180,  156,  132,  112,   92,   80,   64,   52,   44,   32,   28,
   28,   24,   28,   36,   40,   48,   56,   72,   84,  104,  124,  148,  172,  196,  228,    0,

  260,  236,  208,  184,  160,  136,  116,  100,   84,   68,   56,   44,   32,   24,   20,   16,
    8,    8,   16,   20,   24,   32,   40,   48,   64,   76,   92,  108,  128,  152,  176,    0,

  192,  172,  152,  132,  116,  100,   84,   68,   56,   48,   36,   28,   24,   16,   12,    8,
    0,    8,   12,    8,   12,   20,   24,   32,   44,   56,   64,   80,   92,  108,  128,    0,

  124,  112,  100,   88,   76,   64,   56,   44,   36,   28,   24,   16,   12,   12,    8,    4,
    4,    0,    4,    8,    8,   12,   16,   20,   28,   32,   40,   52,   60,   72,   84,    0,

   60,   56,   48,   44,   36,   32,   28,   24,   20,   16,   12,    8,    8,    4,    4,    4,
    0,    4,    0,    4,    4,    4,    8,   12,   12,   16,   20,   24,   28,   36,   40,    0,

   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,
    0,    0,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,   -4,    0,

  -68,  -64,  -56,  -48,  -40,  -36,  -32,  -24,  -20,  -16,  -12,   -8,   -8,   -4,   -4,    0,
    0,    0,    0,    0,   -4,   -8,   -8,  -12,  -16,  -20,  -24,  -28,  -32,  -40,  -44,    0,

 -132, -120, -104,  -92,  -80,  -68,  -56,  -48,  -40,  -32,  -24,  -16,  -16,   -8,   -8,   -4,
    0,    0,   -4,   -8,   -8,  -12,  -16,  -24,  -28,  -36,  -44,  -52,  -64,  -76,  -88,    0,

 -200, -180, -160, -140, -120, -104,  -88,  -72,  -60,  -48,  -40,  -32,  -20,  -16,   -8,   -4,
   -4,   -8,   -8,   -8,  -12,  -20,  -28,  -36,  -44,  -56,  -68,  -84,  -96, -116, -132,    0,

 -268, -244, -216, -188, -164, -144, -124, -104,  -84,  -72,  -56,  -48,  -36,  -28,  -20,  -16,
  -12,   -8,  -20,  -20,  -24,  -32,  -44,  -52,  -64,  -80,  -96, -116, -136, -156, -180,    0,

 -348, -312, -280, -244, -216, -188, -160, -136, -116,  -96,  -80,  -68,  -52,  -44,  -36,  -32,
  -24,  -24,  -32,  -32,  -40,  -52,  -64,  -72,  -88, -108, -128, -152, -176, -204, -232,    0,

 -428, -388, -348, -308, -272, -240, -208, -180, -152, -128, -108,  -96,  -80,  -64,  -56,  -56,
  -48,  -44,  -48,  -56,  -60,  -72,  -84, -104, -124, -144, -168, -196, -224, -256, -292,    0,

 -520, -468, -424, -380, -336, -296, -260, -224, -196, -168, -148, -128, -108,  -96,  -88,  -80,
  -76,  -72,  -76,  -84,  -92, -108, -120, -140, -160, -188, -216, -248, -284, -320, -360,    0,

 -620, -560, -508, -456, -412, -364, -320, -284, -248, -220, -192, -172, -148, -132, -128, -112,
 -108, -108, -112, -120, -128, -140, -164, -184, -212, -240, -272, -308, -348, -392, -440,    0,

 -732, -664, -600, -548, -492, -440, -396, -352, -312, -280, -248, -220, -204, -184, -172, -160,
 -156, -156, -156, -168, -180, -200, -216, -240, -268, -300, -336, -380, -420, -472, -528,    0,

 -852, -776, -704, -644, -584, -532, -480, -432, -388, -352, -316, -288, -268, -244, -228, -220,
 -216, -212, -220, -228, -236, -260, -280, -308, -340, -372, -412, -460, -512, -564, -624,    0,

 -984, -904, -828, -756, -692, -632, -576, -520, -476, -436, -400, -364, -344, -320, -300, -292,
 -288, -284, -284, -296, -316, -332, -356, -384, -420, -460, -504, -556, -608, -668, -728,    0,

-1132,-1044, -960, -880, -808, -744, -684, -632, -576, -532, -496, -460, -428, -408, -388, -376,
 -372, -368, -376, -380, -404, -424, -448, -480, -520, -560, -612, -664, -724, -780, -852,    0,

-1296,-1200,-1108,-1024, -944, -872, -808, -748, -692, -644, -604, -568, -536, -512, -492, -472,
 -472, -468, -476, -484, -504, -524, -552, -588, -628, -676, -724, -788, -848, -920, -992,    0,

-1476,-1372,-1276,-1180,-1096,-1016, -948, -888, -828, -776, -728, -692, -656, -628, -600, -596,
 -588, -584, -588, -596, -620, -640, -680, -716, -756, -808, -864, -920, -992,-1068,-1148,    0,

-1672,-1560,-1456,-1356,-1268,-1180,-1108,-1032, -968, -924, -872, -832, -792, -764, -740, -724,
 -724, -716, -720, -740, -756, -780, -812, -856, -900, -956,-1012,-1076,-1152,-1232,-1324,    0,

-1832,-1716,-1604,-1500,-1408,-1320,-1240,-1168,-1096,-1036, -992, -944, -912, -884, -860, -836,
 -824, -832, -836, -848, -872, -900, -932, -972,-1016,-1076,-1136,-1208,-1288,-1376,-1464,    0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	}
};


 #endif	// __TEST_WARP_H__

