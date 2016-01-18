/*******************************************************************************
 * lib_vproc.h
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

#ifndef LIB_VPROC_H_
#define LIB_VPROC_H_

#ifdef __cplusplus
extern "C" {
#endif


#define OVERLAY_CLUT_BYTES        (1024)

#define DEFAULT_COLOR_Y          12
#define DEFAULT_COLOR_U          128
#define DEFAULT_COLOR_V          128

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

// Don't change the order of vuy
typedef struct yuv_color_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;        // Unused for privacy mask
} yuv_color_t;

typedef struct pm_param_s {
	int enable;
	int id;
	iav_rect_ex_t rect;
} pm_param_t;

typedef struct mctf_param_s {
	int strength;
	iav_rect_ex_t rect;
} mctf_param_t;

typedef struct cawarp_param_s {
	int data_num;
	float* data;
	float red_factor;
	float blue_factor;
	int center_offset_x;  // lens offset to VIN center. Left shift is negative while right shift is positive.
	int center_offset_y;  // lens offset to VIN center. Up shift is negative while down shift is positive.
} cawarp_param_t;

typedef struct offset_param_s {
	int stream_id;
	int x;
	int y;
} offset_param_t;

typedef enum {
	OSD_CLUT_8BIT = 0,
	OSD_AYUV_16BIT = 1,
	OSD_ARGB_16BIT = 2,
	OSD_TYPE_NUM,
} OSD_TYPE;

typedef struct overlay_param_s {
	int enable;
	OSD_TYPE type;
	int stream_id;
	int area_id;
	iav_rect_ex_t rect;
	u8* clut_addr;
	u8* data_addr;
	u16 pitch;
	u8 clut_bg_entry;    // background entry in clut
	u8 clut_pm_entry;    // one entry not used in overlay data which is reserved as pm
} overlay_param_t;

int vproc_pm(pm_param_t* mask_in_main);
int vproc_pm_color(yuv_color_t* mask_color);

int vproc_mctf(mctf_param_t* mctf);
int vproc_dptz(iav_digital_zoom_ex_t* dptz);
int vproc_cawarp(cawarp_param_t* cawarp);

int vproc_stream_prepare(int stream_id);
int vproc_stream_overlay(overlay_param_t* overlay);
int vproc_stream_offset(offset_param_t* offset);

int vproc_exit(void);
int vproc_version(version_t* version);


#ifdef __cplusplus
}
#endif

#endif /* LIB_VPROC_H_ */

