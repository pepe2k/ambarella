/**
 * amba_dsp_common.cpp
 *
 * History:
 *  2011/07/12 - [Zhi He] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"

extern "C" {
#include <basetypes.h>
#include "ambas_common.h"
#include "iav_drv.h"
#include "ambas_vin.h"
#include "ambas_vout.h"
}

#include "am_dsp_if.h"

static SVoutModeDef G_vout_modes[] =
{
    {
        AMBA_VIDEO_MODE_320_288, 320, 288
    },
    {
        AMBA_VIDEO_MODE_360_240, 360, 240
    },
    {
        AMBA_VIDEO_MODE_360_288, 360, 288
    },
    {
        AMBA_VIDEO_MODE_480_240, 480, 240
    },
    {
        AMBA_VIDEO_MODE_720_240, 720, 240
    },
    {
        AMBA_VIDEO_MODE_960_240, 960, 240
    },
    {
        AMBA_VIDEO_MODE_VGA, 640, 480
    },
    {
        AMBA_VIDEO_MODE_SVGA, 800, 600
    },
    {
        AMBA_VIDEO_MODE_XGA, 1024, 768
    },
    {
        AMBA_VIDEO_MODE_SXGA, 1280, 1024
    },
    {
        AMBA_VIDEO_MODE_UXGA, 1600, 1200
    },
    {
        AMBA_VIDEO_MODE_QXGA, 2048, 1536
    },
    {
        AMBA_VIDEO_MODE_WVGA, 800, 480
    },
    {
        AMBA_VIDEO_MODE_WSVGA, 1024, 600
    },
    {
        AMBA_VIDEO_MODE_1280_960, 1280, 960
    },
    {
        AMBA_VIDEO_MODE_WXGA, 1280, 800
    },
    {
        AMBA_VIDEO_MODE_WSXGA, 1440, 900
    },
    {
        AMBA_VIDEO_MODE_WSXGAP, 1680, 1050
    },
    {
        AMBA_VIDEO_MODE_WUXGA, 1920, 1200
    },
    {
        AMBA_VIDEO_MODE_320_240, 320, 240
    },
    {
        AMBA_VIDEO_MODE_HVGA, 320, 480
    },
    {
        AMBA_VIDEO_MODE_480I, 720, 480
    },
    {
        AMBA_VIDEO_MODE_576I, 720, 576
    },
    {
        AMBA_VIDEO_MODE_D1_NTSC, 720, 480
    },
    {
        AMBA_VIDEO_MODE_D1_PAL, 720, 576
    },
    {
        AMBA_VIDEO_MODE_720P, 1280, 720
    },
    {
        AMBA_VIDEO_MODE_720P_PAL, 1280, 720
    },
    {
        AMBA_VIDEO_MODE_1080I, 1920, 1080
    },
    {
        AMBA_VIDEO_MODE_1080I_PAL, 1920, 1080
    },
};

AM_ERR GetVoutSize(AM_INT mode, AM_UINT* width, AM_UINT* height)
{
    size_t i;
    for (i = 0; i < sizeof(G_vout_modes) / sizeof(G_vout_modes[0]); i++) {
        if (G_vout_modes[i].mode == mode) {
            *width = G_vout_modes[i].width;
            *height = G_vout_modes[i].height;
            return ME_OK;
        }
    }

    AM_ERROR("not supported mode %d in GetVoutSize, need add to support list.\n", mode);
    return ME_NOT_SUPPORTED;
}

AM_ERR GetSinkId(AM_INT iavfd, AM_INT type, AM_INT* pSinkId)
{
    struct amba_vout_sink_info  sink_info;

    AM_INT num = 1, i = 0;
    if (ioctl(iavfd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
        perror("IAV_IOC_VOUT_GET_SINK_NUM");
        return ME_ERROR;
    }

    if (num < 1) {
        printf("Please load vout driver!\n");
        return ME_ERROR;
    }

    for (i = num - 1; i >= 0; i--) {
        sink_info.id = i;
        if (ioctl(iavfd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
            perror("IAV_IOC_VOUT_GET_SINK_INFO");
            return ME_ERROR;
        }

        AM_PRINTF("sink %d is %s\n", sink_info.id, sink_info.name);

        if ((sink_info.sink_type == type) &&
            (sink_info.source_id == 1))
            *pSinkId = sink_info.id;
    }
    return ME_OK;
}

AM_INT RectOutofRange(AM_INT size_x, AM_INT size_y, AM_INT offset_x, AM_INT offset_y, AM_INT range_size_x, AM_INT range_size_y, AM_INT range_offset_x, AM_INT range_offset_y)
{
    if (offset_x < range_offset_x || offset_y < range_offset_y || offset_x > (range_size_x + range_offset_x) || offset_y > (range_size_y + range_offset_y)) {
        return 1;
    }

    if ((offset_x + size_x) < range_offset_x || (offset_y + size_y) < range_offset_y || (offset_x + size_x) > (range_size_x + range_offset_x) || (offset_y + size_y) > (range_size_y + range_offset_y)) {
        return 2;
    }

    return 0;
}

