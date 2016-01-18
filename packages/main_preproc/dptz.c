/*******************************************************************************
 * dptz.c
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

static int G_dptz_inited;

static int check_dptz(iav_digital_zoom_ex_t* dptz)
{
	int src_width, src_height;

	if (!dptz) {
		ERROR("DPTZ is NULL.\n");
		return -1;
	}
	if (dptz->source < IAV_ENCODE_SOURCE_MAIN_BUFFER
	    || dptz->source > IAV_ENCODE_SOURCE_FOURTH_BUFFER) {
		ERROR("Invalid DPTZ source buffer id [%d].\n", dptz->source);
		return -1;
	}
	if (dptz->source == IAV_ENCODE_SOURCE_MAIN_BUFFER) {
		src_width = vproc.vin.width;
		src_height = vproc.vin.height;
	} else {
		src_width = vproc.srcbuf[0].size.width;
		src_height = vproc.srcbuf[0].size.height;
	}
	if (!dptz->input.width) {
		dptz->input.width = src_width;
	}
	if (!dptz->input.height) {
		dptz->input.height = src_height;
	}
	if ((dptz->input.x + dptz->input.width > src_width)
	    || (dptz->input.y + dptz->input.height > src_height)) {
		ERROR("DPTZ input window (x %d, y %d, w %d, h %d) is "
			"out of %s %dx%d.\n", dptz->input.x, dptz->input.y,
		    dptz->input.width, dptz->input.height,
		    dptz->source == IAV_ENCODE_SOURCE_MAIN_BUFFER ? "vin" :
		        "main buffer", src_width, src_height);
		return -1;
	}
	return 0;
}

int init_dptz(void)
{
	int i;

	//always reflesh DPTZ parameter
	for (i = 0; i <= IAV_ENCODE_SOURCE_FOURTH_BUFFER; i++) {
		if (ioctl_get_dptz(i) < 0) {
			return -1;
		}
	}

	if (unlikely(!G_dptz_inited)) {
		do {
			if (ioctl_get_vin_info() < 0) {
				break;
			}
			DEBUG("vin %dx%d\n", vproc.vin.width, vproc.vin.height);
			for (i = 0; i <= IAV_ENCODE_SOURCE_FOURTH_BUFFER; i++) {
				if (ioctl_get_srcbuf_format(i) < 0) {
					break;
				}
				DEBUG("buffer %d %dx%d, input %dx%d+%dx%d\n",
				    vproc.dptz[i]->source, vproc.srcbuf[i].size.width,
				    vproc.srcbuf[i].size.height, vproc.dptz[i]->input.width,
				    vproc.dptz[i]->input.height, vproc.dptz[i]->input.x,
				    vproc.dptz[i]->input.y);
			}
			G_dptz_inited = 1;
		} while (0);
	}
	return G_dptz_inited ? 0 : -1;
}

int deinit_dptz(void)
{
	if (G_dptz_inited) {
		G_dptz_inited = 0;
		DEBUG("done\n");
	}
	return G_dptz_inited ? -1 : 0;
}

int operate_dptz(iav_digital_zoom_ex_t* dptz)
{
	if (init_dptz() < 0)
		return -1;

	if (check_dptz(dptz) < 0)
		return -1;

	*vproc.dptz[dptz->source] = *dptz;

	return ioctl_cfg_dptz(dptz->source);
}
