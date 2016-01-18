/*
 * kernel/private/drivers/ambarella/vout/digital/amb_dbus/ambdbus.c
 *
 * History:
 *    2009/05/21 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
struct ambdbus_info {
	unsigned char __iomem			*regbase;
	struct __amba_vout_video_sink		video_sink;
	u32					*mode_list;
};

struct ambdbus_instance_info {
	const char				name[AMBA_VOUT_NAME_LENGTH];
	struct ambdbus_info			*pdbus_sink;
	int					source_id;
	u32					sink_type;
	struct resource				io_mem;
	enum amba_video_mode			mode_list[AMBA_VIDEO_MODE_NUM];
};

/* ========================================================================== */

/* ========================================================================== */
static int ambdbus_docmd(struct __amba_vout_video_sink *psink,
	enum amba_video_sink_cmd cmd, void *args);

/* ========================================================================== */
#include "arch/ambdbus_arch.c"
#include "ambdbus_docmd.c"

/* ========================================================================== */
static int ambdbus_probe(struct device *dev)
{
	int				errorCode = 0;

	errorCode = ambdbus_probe_arch();

	return errorCode;
}

static int ambdbus_remove(struct device *dev)
{
	int				errorCode = 0;

	errorCode = ambdbus_remove_arch();

	return errorCode;
}

struct amb_driver ambdbus = {
	.probe = ambdbus_probe,
	.remove = ambdbus_remove,
	.driver = {
		.name = "ambdbus",
		.owner = THIS_MODULE,
	}
};

static int __init ambdbus_init(void)
{
	return amb_register_driver(&ambdbus);
}

static void __exit ambdbus_exit(void)
{
	amb_unregister_driver(&ambdbus);
}

module_init(ambdbus_init);
module_exit(ambdbus_exit);

MODULE_DESCRIPTION("Built-in LCD Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

