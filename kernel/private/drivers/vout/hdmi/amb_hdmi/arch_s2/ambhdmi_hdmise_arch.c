/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/arch_a5s/ambhdmi_hdmise_arch.c
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
#if LINUX_VERSION_CODE <  KERNEL_VERSION(2,6,38)
typedef enum audio_in_freq_e
{
	AUDIO_SF_reserved = 0,
	AUDIO_SF_96000,
	AUDIO_SF_48000,
	AUDIO_SF_44100,
	AUDIO_SF_32000,
	AUDIO_SF_24000,
	AUDIO_SF_22050,
	AUDIO_SF_16000,
	AUDIO_SF_12000,
	AUDIO_SF_11025,
	AUDIO_SF_8000,
} AUDIO_IN_FREQ_t;
#endif

static void ambhdmi_hdmise_set_arch(struct ambhdmi_sink *phdmi_sink,
	const amba_hdmi_video_timing_t *vt)
{
	u32 regbase	= (u32)phdmi_sink->regbase;
	u32 reg, val;

	/* Use HDCP & EESS */
	reg = regbase + HDMI_CLOCK_GATED_OFFSET;
	val = amba_readl(reg);
	val |= HDMI_CLOCK_GATED_HDCP_CLOCK_EN;
	amba_writel(reg, val);

	reg = regbase + HDCP_HDCPCE_CTL_OFFSET;
	val = HDCP_HDCPCE_CTL_KEY_SPACE(1) | HDMI_HDCPCE_CTL_USE_EESS(1)
		| HDMI_HDCPCE_CTL_HDCPCE_EN;
	amba_writel(reg, val);
}

