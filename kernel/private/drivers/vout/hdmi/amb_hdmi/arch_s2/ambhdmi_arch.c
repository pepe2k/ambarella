/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/arch_a5s/ambhdmi_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Initial revision
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static struct ambhdmi_instance_info hdmi_instance = {
	.name		= "HDMI",
	.phdmi_sink	= NULL,
	.source_id	= AMBA_VOUT_SOURCE_STARTING_ID + 1,
	.sink_type	= AMBA_VOUT_SINK_TYPE_HDMI,
	.io_mem		= {
		.start	= HDMI_BASE,
		.end	= HDMI_BASE + 0x1000 - 1,
		.flags	= IORESOURCE_MEM,
	},
	.irq		= {
		.start	= HDMI_IRQ,
		.end	= HDMI_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
	.mode_list	= {
		AMBA_VIDEO_MODE_VGA,
		AMBA_VIDEO_MODE_480I,
		AMBA_VIDEO_MODE_576I,
		AMBA_VIDEO_MODE_D1_NTSC,
		AMBA_VIDEO_MODE_D1_PAL,
		AMBA_VIDEO_MODE_720P,
		AMBA_VIDEO_MODE_720P_PAL,
		AMBA_VIDEO_MODE_1080I,
		AMBA_VIDEO_MODE_1080I_PAL,
		AMBA_VIDEO_MODE_1080P24,
		AMBA_VIDEO_MODE_1080P25,
		AMBA_VIDEO_MODE_1080P30,
		AMBA_VIDEO_MODE_1080P,
		AMBA_VIDEO_MODE_1080P_PAL,
		AMBA_VIDEO_MODE_HDMI_NATIVE,
		AMBA_VIDEO_MODE_720P24,
		AMBA_VIDEO_MODE_720P25,
		AMBA_VIDEO_MODE_720P30,
		AMBA_VIDEO_MODE_2160P30,
		AMBA_VIDEO_MODE_2160P25,
		AMBA_VIDEO_MODE_2160P24,
		AMBA_VIDEO_MODE_2160P24_SE,

		AMBA_VIDEO_MODE_MAX
	}
};

