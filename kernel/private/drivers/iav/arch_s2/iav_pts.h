 /*
 * iav_pts.h
 *
 * History:
 *	2013/03/04 - [Zhaoyang Chen] created file
 *
 * Copyright (C) 2011-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_PTS_H__
#define __IAV_PTS_H__

typedef struct iav_hwtimer_pts_info_s
{
	u32			audio_tick;
	u32			audio_freq;
	u64			hwtimer_tick;
	u32			hwtimer_freq;
	struct file	*fp_timer;
}iav_hwtimer_pts_info_t;

typedef struct iav_pts_info_s
{
	iav_hwtimer_pts_info_t	hw_pts_info;
	u32			hwtimer_enabled;
	u32			reserved;
}iav_pts_info_t;

#endif

