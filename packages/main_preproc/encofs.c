/*******************************************************************************
 * encofs.c
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

#include <basetypes.h>
#include "ambas_common.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#include "lib_vproc.h"
#include "priv.h"

static int G_encofs_inited;

static int check_encofs(const offset_param_t* offset)
{
	int src_id;
	iav_reso_ex_t* srcbuf;
	iav_encode_format_ex_t* stream;

	if (!offset) {
		ERROR("Offset is NULL.\n");
		return -1;
	}
	if (offset->stream_id < 0 || offset->stream_id > IAV_STREAM_MAX_NUM_IMPL) {
		ERROR("Invalid stream offset id [%d].\n", offset->stream_id);
		return -1;
	}
	if (ioctl_get_stream_format(offset->stream_id) < 0) {
		return -1;
	}
	src_id = vproc.stream[offset->stream_id].source;
	if (ioctl_get_srcbuf_format(src_id) < 0) {
		return -1;
	}
	srcbuf = &vproc.srcbuf[src_id].size;
	stream = &vproc.stream[offset->stream_id];
	if ((offset->x + stream->encode_width > srcbuf->width)
		|| (offset->y + stream->encode_height > srcbuf->height)) {
		ERROR("Stream %d size %dx%d + offset %dx%d exceeds the source "
			"buffer %d size %x%d.\n", offset->stream_id,
			stream->encode_width, stream->encode_height, offset->x, offset->y,
			src_id, srcbuf->width, srcbuf->height);
		return -1;
	}

	return 0;
}

int init_encofs(void)
{
	int i;

	if (unlikely(!G_encofs_inited)) {
		do {
			for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; i++) {
				if (ioctl_get_stream_offset(i) < 0) {
					break;
				}
				DEBUG("stream %d offset %dx%d\n", i, vproc.streamofs[i]->x,
				    vproc.streamofs[i]->y);
			}
			G_encofs_inited = 1;
		} while (0);
	}
	return G_encofs_inited ? 0 : -1;
}

int deinit_encofs(void)
{
	if (G_encofs_inited) {
		G_encofs_inited = 0;
		DEBUG("done\n");
	}
	return G_encofs_inited ? -1 : 0;
}

int operate_encofs(offset_param_t* offset)
{
	if (init_encofs() < 0)
		return -1;

	if (check_encofs(offset) < 0)
		return -1;
	if((offset->x != ROUND_DOWN(offset->x, 2))
		|| (offset->y != ROUND_DOWN(offset->y, 4))) {
		ERROR("stream_id %d: offset x %d must be multiple of 2, y %d must be multiple of 4 \n",
			offset->stream_id, offset->x, offset->y);
	}

	vproc.streamofs[offset->stream_id]->x = ROUND_DOWN(offset->x, 2);
	vproc.streamofs[offset->stream_id]->y = ROUND_DOWN(offset->y, 4);

	return ioctl_cfg_stream_offset(offset->stream_id);
}
