/*******************************************************************************
 * priv.h
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

#ifndef PRIV_H_
#define PRIV_H_

#include <basetypes.h>
#include "ambas_vin.h"
#include "iav_drv.h"
#include "utils.h"

#define PM_MB_BUF_NUM              (8)
#define PM_PIXEL_BUF_NUM           (2)
#define PM_MAX_BUF_NUM             (PM_MB_BUF_NUM)

#define OVERLAY_MAX_STREAM_NUM     (4)
#define OVERLAY_MAX_AREA_NUM       (MAX_NUM_OVERLAY_AREA)
#define OVERLAY_BUF_NUM            (5)

#define TRACE_RECT(str, rect) \
	do {	\
		TRACE("%s\t(w %d, h %d, x %d, y %d)\n", \
			str, rect.width, rect.height, rect.x, rect.y); \
	} while(0)

#define TRACE_RECTP(str, rect) \
	do {	\
		TRACE("%s\t(w %d, h %d, x %d, y %d)\n", \
			str, rect->width, rect->height, rect->x, rect->y); \
	} while(0);

typedef enum {
	PM_MB_MAIN = 0,
	PM_PIXEL,
	PM_PIXELRAW,
	PM_MB_STREAM,
} PM_TYPE;

typedef enum {
	OP_ADD,
	OP_REMOVE,
	OP_UPDATE,
} OP;

typedef struct pm_buffer_s {
	int domain_width;
	int domain_height;
	int id;
	int pitch;
	int width;
	int height;
	u32 bytes;
	u8* addr[PM_MAX_BUF_NUM];
} pm_buffer_t;

typedef struct pm_node_s {
	int id;
	iav_rect_ex_t rect;
	u32 state;
	int redraw;
	struct pm_node_s* next;
} pm_node_t;

typedef struct overlay_buffer_s {
	int data_id;
	u32 bytes;
	u8* clut_addr;
	u8* data_addr[OVERLAY_BUF_NUM];
	int bg_entry;
	int pm_entry;
	iav_rect_ex_t rect_mb;
	int pm_overlapped;
} overlay_buffer_t;

typedef struct pm_record_s {
	pm_node_t* node_in_vin;
	int node_num;
	pm_buffer_t buffer[IAV_STREAM_MAX_NUM_IMPL];
} pm_record_t;

typedef struct overlay_record_s {
	overlay_buffer_t buffer[OVERLAY_MAX_STREAM_NUM][OVERLAY_MAX_AREA_NUM];
	int max_stream_num;
} overlay_record_t;

typedef struct vproc_param_s {
	iav_mmap_info_t pm_mem;
	iav_mmap_info_t overlay_mem;
	iav_system_resource_setup_ex_t resource;
	iav_privacy_mask_info_ex_t pm_info;
	struct amba_video_info vin;
	iav_source_buffer_setup_ex_t srcbuf_setup;
	iav_source_buffer_format_ex_t srcbuf[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_encode_format_ex_t stream[IAV_STREAM_MAX_NUM_IMPL];

	pm_record_t pm_record;
	overlay_record_t overlay_record;

	iav_privacy_mask_setup_ex_t* pm;
	iav_digital_zoom_ex_t* dptz[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_ca_warp_t* cawarp;
	iav_stream_privacy_mask_t* streampm[IAV_STREAM_MAX_NUM_IMPL];
	iav_stream_offset_t* streamofs[IAV_STREAM_MAX_NUM_IMPL];
	overlay_insert_ex_t* overlay[IAV_STREAM_MAX_NUM_IMPL];

	iav_apply_flag_t apply_flag[IAV_VIDEO_PROC_NUM];
	iav_video_proc_t ioctl_pm;
	iav_video_proc_t ioctl_dptz[IAV_ENCODE_SOURCE_TOTAL_NUM];
	iav_video_proc_t ioctl_cawarp;
	iav_video_proc_t ioctl_streamofs[IAV_STREAM_MAX_NUM_IMPL];
	iav_video_proc_t ioctl_overlay[IAV_STREAM_MAX_NUM_IMPL];
	iav_video_proc_t ioctl_streampm[IAV_STREAM_MAX_NUM_IMPL];

} vproc_param_t;

// priv.c
int get_pm_type(void);

int is_id_in_map(const int i, const u32 map);

pm_node_t* create_pm_node(int mask_id, const iav_rect_ex_t* mask_rect);
void set_pm_node_to_redraw(pm_node_t* node);
void clear_pm_node_from_redraw(pm_node_t* node);
int is_pm_node_to_redraw(pm_node_t* node);

int is_rect_overlap(const iav_rect_ex_t* a, const iav_rect_ex_t* b);
void get_overlap_rect(iav_rect_ex_t* overlap, const iav_rect_ex_t* a,
    const iav_rect_ex_t* b);

int rounddown_to_mb(const int pixels);
int roundup_to_mb(const int pixels);
void rect_to_rectmb(iav_rect_ex_t* rect_mb, const iav_rect_ex_t* rect);
void rectmb_to_rect(iav_rect_ex_t* rect, const iav_rect_ex_t* rect_mb);
int rect_vin_to_main(iav_rect_ex_t* rect_in_main,
    const iav_rect_ex_t* rect_in_vin);
int rect_main_to_srcbuf(iav_rect_ex_t* rect_in_srcbuf,
    const iav_rect_ex_t* rect_in_main, const int src_id);
void flip_stream_pm_overlay(iav_rect_ex_t* rect_in_stream,
	const int stream_id);
int rect_srcbuf_to_stream(iav_rect_ex_t* rect_in_stream,
    const iav_rect_ex_t* rect_in_srcbuf,
    const int stream_id);
int rect_vin_to_stream(iav_rect_ex_t* rect_in_stream,
    const iav_rect_ex_t* rect_in_vin,
    const int stream_id);
void rect_main_to_vin(iav_rect_ex_t* rect_in_vin,
    const iav_rect_ex_t* rect_in_main);

// pm_mb.c
int operate_mbmain_pm(const OP op, const int mask_id,
	iav_rect_ex_t* rect_in_main);
int operate_mbmain_mctf(mctf_param_t* mctf);
int operate_mbmain_color(yuv_color_t* color);
int init_mbmain(void);
int deinit_mbmain(void);

int operate_mbstream_pmoverlay(const OP op, const int mask_id,
    iav_rect_ex_t* rect_in_main);
int operate_mbstream_color(yuv_color_t* color);
int init_mbstream(void);
int deinit_mbstream(void);

int get_mbstream_format(u32 streams);
void prepare_mbstream_unmask(const int stream_id);
void draw_mbstream_pm(const int stream_id, const iav_rect_ex_t* rect_mb);
void erase_mbstream_pm(const int stream_id, const iav_rect_ex_t* rect_mb);

int disable_pm(void);
int disable_stream_pm(void);

// pm_pixel.c
int operate_pixel_pm(const OP op, const int mask_id, iav_rect_ex_t* rect);
int operate_pixelraw_pm(const OP op, const int mask_id, iav_rect_ex_t* rect);
int init_pixel(void);
int deinit_pixel(void);

int init_pixelraw(void);
int deinit_pixelraw(void);

// dptz.c
int operate_dptz(iav_digital_zoom_ex_t* dptz);
int init_dptz(void);
int deinit_dptz(void);

// cawarp.c
int operate_cawarp(cawarp_param_t* cawarp);
int init_cawarp(void);
int deinit_cawarp(void);
int disable_cawarp(void);

// overlay.c
int operate_overlay(overlay_param_t* overlay);
int operate_mbstream_overlaypm(const OP op, const int stream_id,
    overlay_param_t* overlay);
int init_overlay(void);
int deinit_overlay(void);
void update_mbstream_buffer(const int stream_id, OP overlay_op);
int disable_overlay(void);

// encofs.c
int operate_encofs(offset_param_t* offset);
int init_encofs(void);
int deinit_encofs(void);

// do_ioctl.c
int ioctl_map_pm(void);
int ioctl_map_overlay(void);
int ioctl_get_pm_info(void);
int ioctl_get_srcbuf_format(const int source_id);
int ioctl_get_srcbuf_setup(void);
int ioctl_get_vin_info(void);
int ioctl_gdma_copy(iav_gdma_copy_ex_t* gdma);
int ioctl_get_system_resource(void);
int ioctl_get_stream_format(const int stream_id);
int ioctl_get_pm(void);
int ioctl_get_dptz(const int source_id);
int ioctl_get_stream_pm(const int stream_id);
int ioctl_get_stream_offset(const int stream_id);
int ioctl_get_stream_overlay(const int stream_id);

int ioctl_apply(void);

int ioctl_cfg_pm(void);
int ioctl_cfg_dptz(const int buf_id);
int ioctl_cfg_cawarp(void);
int ioctl_cfg_stream_pm(const u32 streams);
int ioctl_cfg_stream_overlay(const u32 streams);
int ioctl_cfg_stream_offset(const int stream_id);

extern vproc_param_t vproc;

#endif /* PRIV_H_ */
