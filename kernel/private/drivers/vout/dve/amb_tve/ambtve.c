/*
 * kernel/private/drivers/ambarella/vout/dve/amb_tve/ambtve.c
 *
 * History:
 *    2009/05/14 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include "vout_pri.h"

/* ========================================================================== */
u32 ypbpr_format_list[] = {
	AMBA_VIDEO_MODE_480I,
	AMBA_VIDEO_MODE_576I,
	AMBA_VIDEO_MODE_D1_NTSC,
	AMBA_VIDEO_MODE_D1_PAL,
	AMBA_VIDEO_MODE_720P,
	AMBA_VIDEO_MODE_720P_PAL,
	AMBA_VIDEO_MODE_1080I,
	AMBA_VIDEO_MODE_1080I_PAL,
	AMBA_VIDEO_MODE_1080P,
	AMBA_VIDEO_MODE_1080P_PAL,

	AMBA_VIDEO_MODE_MAX
};

u32 cvbs_format_list[] = {
	AMBA_VIDEO_MODE_480I,
	AMBA_VIDEO_MODE_576I,

	AMBA_VIDEO_MODE_MAX
};

/* ========================================================================== */
static int ambtve_docmd(struct __amba_vout_video_sink *psink,
	enum amba_video_sink_cmd cmd, void *args);

/* ========================================================================== */
#include "arch/ambtve_arch.c"
#include "ambtve_docmd.c"

/* ========================================================================== */
static int ambtve_probe(struct device *dev)
{
	int				errorCode = 0;

	errorCode = ambtve_probe_arch();

	return errorCode;
}

static int ambtve_remove(struct device *dev)
{
	int				errorCode = 0;

	errorCode = ambtve_remove_arch();

	return errorCode;
}

struct amb_driver ambtve = {
	.probe = ambtve_probe,
	.remove = ambtve_remove,
	.driver = {
		.name = "ambtve",
		.owner = THIS_MODULE,
	}
};

static int __init ambtve_init(void)
{
	return amb_register_driver(&ambtve);
}

static void __exit ambtve_exit(void)
{
	amb_unregister_driver(&ambtve);
}

module_init(ambtve_init);
module_exit(ambtve_exit);

MODULE_DESCRIPTION("Built-in NTSC/PAL Video Encoder");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

