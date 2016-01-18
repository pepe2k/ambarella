/*******************************************************************************
 * priv.c
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#include "lib_vproc.h"
#include "priv.h"

const char* PM_TYPE_STR[] = { "MB Main", "Pixel", "Pixel Raw", "MB Stream" };

int get_pm_type(void)
{
	int type = -1;

	if (ioctl_get_pm_info() < 0) {
		return -1;
	}
	switch (vproc.pm_info.unit) {
		case IAV_PRIVACY_MASK_UNIT_MB:
			type = (
			    vproc.pm_info.domain == IAV_PRIVACY_MASK_DOMAIN_MAIN ?
			        PM_MB_MAIN : PM_MB_STREAM);
			break;
		case IAV_PRIVACY_MASK_UNIT_PIXEL:
			type = PM_PIXEL;
			break;
		case IAV_PRIVACY_MASK_UNIT_PIXELRAW:
			type = PM_PIXELRAW;
			break;
		default:
			break;
	}
	if (type < 0) {
		ERROR("Cannot handle privacy mask unit %d, domain %d.\n",
		    vproc.pm_info.unit, vproc.pm_info.domain);
	} else {
		DEBUG("==== %s ======\n", PM_TYPE_STR[type]);
	}
	return type;
}

pm_node_t* create_pm_node(const int mask_id, const iav_rect_ex_t* mask_rect)
{
	pm_node_t* new_node = (pm_node_t*) malloc(sizeof(pm_node_t));
	if (likely(new_node)) {
		memset(new_node, 0, sizeof(pm_node_t));
		new_node->id = mask_id;
		if (likely(mask_rect)) {
			new_node->rect = *mask_rect;
		}
	} else {
		ERROR("cannot malloc node.\n");
	}
	return new_node;
}

inline int is_rect_overlap(const iav_rect_ex_t* a, const iav_rect_ex_t* b)
{
	return ((MAX(a->x + a->width, b->x + b->width) - MIN(a->x, b->x))
	< (a->width + b->width))&& ((MAX(a->y + a->height, b->y + b->height)
			- MIN(a->y, b->y)) < (a->height + b->height));
}

inline void get_overlap_rect(iav_rect_ex_t* overlap,
    const iav_rect_ex_t* a,
    const iav_rect_ex_t* b)
{
	overlap->x = MAX(a->x, b->x);
	overlap->y = MAX(a->y, b->y);
	overlap->width = MIN(a->x + a->width, b->x + b->width) - overlap->x;
	overlap->height = MIN(a->y + a->height, b->y + b->height) - overlap->y;
}

inline void set_pm_node_to_redraw(pm_node_t* node)
{
	node->redraw = 1;
}

inline void clear_pm_node_from_redraw(pm_node_t* node)
{
	node->redraw = 0;
}

inline int is_pm_node_to_redraw(pm_node_t* node)
{
	return node->redraw != 0;
}

inline int roundup_to_mb(const int pixels)
{
	return ROUND_UP(pixels, 16) / 16;
}

inline int rounddown_to_mb(const int pixels)
{
	return ROUND_DOWN(pixels, 16) / 16;
}

inline void rect_to_rectmb(iav_rect_ex_t* rect_mb, const iav_rect_ex_t* rect)
{
	rect_mb->x = rounddown_to_mb(rect->x);
	rect_mb->y = rounddown_to_mb(rect->y);
	rect_mb->width = roundup_to_mb(rect->x + rect->width) - rect_mb->x;
	rect_mb->height = roundup_to_mb(rect->y + rect->height) - rect_mb->y;
}

inline void rectmb_to_rect(iav_rect_ex_t* rect, const iav_rect_ex_t* rect_mb)
{
	rect->x = rect_mb->x * 16;
	rect->y = rect_mb->y * 16;
	rect->width = rect_mb->width * 16;
	rect->height = rect_mb->height * 16;
}

int rect_vin_to_main(iav_rect_ex_t* rect_in_main,
    const iav_rect_ex_t* rect_in_vin)
{
	iav_rect_ex_t* input = &vproc.dptz[IAV_ENCODE_SOURCE_MAIN_BUFFER]->input;
	iav_reso_ex_t* output = &vproc.srcbuf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;

	TRACE_RECTP("rect in vin", rect_in_vin);
	if (!is_rect_overlap(rect_in_vin, input)) {
		TRACE("not in main buffer.\n");
		return -1;
	}
	get_overlap_rect(rect_in_main, rect_in_vin, input);
	// Convert offset to dptz I input
	rect_in_main->x = (rect_in_main->x - input->x) * output->width
	    / input->width;
	rect_in_main->y = (rect_in_main->y - input->y) * output->height
	    / input->height;
	rect_in_main->width = rect_in_main->width * output->width / input->width;
	rect_in_main->height = rect_in_main->height * output->height
	    / input->height;

	TRACE_RECTP("\t\t=> rect in main buffer", rect_in_main);
	return 0;

}

void rect_main_to_vin(iav_rect_ex_t* rect_in_vin,
    const iav_rect_ex_t* rect_in_main)
{
	iav_rect_ex_t* input = &vproc.dptz[IAV_ENCODE_SOURCE_MAIN_BUFFER]->input;
	iav_reso_ex_t* output = &vproc.srcbuf[IAV_ENCODE_SOURCE_MAIN_BUFFER].size;

	rect_in_vin->x = input->x + input->width * rect_in_main->x / output->width;
	rect_in_vin->width = input->width * rect_in_main->width / output->width;
	rect_in_vin->y = input->y
	    + input->height * rect_in_main->y / output->height;
	rect_in_vin->height = input->height * rect_in_main->height / output->height;

	TRACE_RECTP("rect in main", rect_in_main);
	TRACE_RECTP("\t\t=> rect in vin", rect_in_vin);
}

int rect_main_to_srcbuf(iav_rect_ex_t* rect_in_srcbuf,
    const iav_rect_ex_t* rect_in_main, const int src_id)
{
	iav_rect_ex_t* input = &vproc.dptz[src_id]->input;
	iav_reso_ex_t* output = &vproc.srcbuf[src_id].size;

	if (src_id == IAV_ENCODE_SOURCE_MAIN_BUFFER) {
		*rect_in_srcbuf = *rect_in_main;
		return 0;
	}

	TRACE_RECTP("rect in main buffer", rect_in_main);
	if (!is_rect_overlap(rect_in_main, input)) {
		TRACE("not in source buffer %d.\n", src_id);
		return -1;
	}
	get_overlap_rect(rect_in_srcbuf, rect_in_main, input);
	// Convert offset to srcbuf input
	rect_in_srcbuf->x = (rect_in_srcbuf->x - input->x) * output->width
	    / input->width;
	rect_in_srcbuf->y = (rect_in_srcbuf->y - input->y) * output->height
	    / input->height;
	rect_in_srcbuf->width = rect_in_srcbuf->width * output->width
	    / input->width;
	rect_in_srcbuf->height = rect_in_srcbuf->height * output->height
	    / input->height;

	TRACE_RECTP("\t\t=> rect in source buffer", rect_in_srcbuf);
	return 0;
}

void flip_stream_pm_overlay(iav_rect_ex_t* rect_in_stream, const int stream_id)
{
	if ((0 == vproc.stream[stream_id].vflip) && (1 == vproc.stream[stream_id].hflip)) {
		/* hflip */
		rect_in_stream->x = vproc.stream[stream_id].encode_width -
			rect_in_stream->x - rect_in_stream->width;
	} else if ((1 == vproc.stream[stream_id].vflip) && (0 == vproc.stream[stream_id].hflip)) {
		/* vflip */
		rect_in_stream->y = vproc.stream[stream_id].encode_height -
			rect_in_stream->y - rect_in_stream->height;
	} else if ((1 == vproc.stream[stream_id].vflip) && (1 == vproc.stream[stream_id].hflip)) {
		/* 180 */
		rect_in_stream->x = vproc.stream[stream_id].encode_width -
			rect_in_stream->x - rect_in_stream->width;
		rect_in_stream->y = vproc.stream[stream_id].encode_height -
			rect_in_stream->y - rect_in_stream->height;
	}
}

int rect_srcbuf_to_stream(iav_rect_ex_t* rect_in_stream,
    const iav_rect_ex_t* rect_in_srcbuf, const int stream_id)
{
	iav_rect_ex_t stream_in_srcbuf;

	stream_in_srcbuf.x = vproc.streamofs[stream_id]->x;
	stream_in_srcbuf.y = vproc.streamofs[stream_id]->y;
	stream_in_srcbuf.width = vproc.stream[stream_id].encode_width;
	stream_in_srcbuf.height = vproc.stream[stream_id].encode_height;

	TRACE_RECTP("rect in source buffer", rect_in_srcbuf);
	if (!is_rect_overlap(rect_in_srcbuf, &stream_in_srcbuf)) {
		TRACE("not in stream %d.\n", stream_id);
		return -1;
	}
	get_overlap_rect(rect_in_stream, rect_in_srcbuf, &stream_in_srcbuf);
	rect_in_stream->x -= stream_in_srcbuf.x;
	rect_in_stream->y -= stream_in_srcbuf.y;
	if ((IAV_ENCODE_HIGH_MP_FULL_PERF_MODE == vproc.resource.encode_mode)
		&& (PM_MB_STREAM == get_pm_type())) {
		flip_stream_pm_overlay(rect_in_stream, stream_id);
	}

	TRACE_RECTP("rect in stream", rect_in_stream);
	return 0;

}

int rect_vin_to_stream(iav_rect_ex_t* rect_in_stream,
    const iav_rect_ex_t* rect_in_vin, const int stream_id)
{
	int ret = -1;
	iav_rect_ex_t rect_in_main, rect_in_srcbuf;
	TRACE_RECTP("rect in vin", rect_in_vin);

	do {
		if (rect_vin_to_main(&rect_in_main, rect_in_vin) < 0) {
			break;
		}

		if (rect_main_to_srcbuf(&rect_in_srcbuf, &rect_in_main,
		    vproc.stream[stream_id].source) < 0) {
			break;
		}

		if (rect_srcbuf_to_stream(rect_in_stream, &rect_in_srcbuf, stream_id)
		    < 0) {
			break;
		}
		TRACE_RECTP("rect in stream", rect_in_stream);
		ret = 0;
	} while (0);

	if (ret) {
		TRACE("not in stream %d.\n", stream_id);
	}

	return ret;
}

inline int is_id_in_map(const int i, const u32 map)
{
	return map & (1 << i);
}
