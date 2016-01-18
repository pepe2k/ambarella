/*
 * kernel/private/include/amba_iav.h
 *
 * History:
 *    2008/08/03 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_IAV_H
#define __AMBA_IAV_H

#define	IAV_VIN_DISABLED		0x00
#define	IAV_VIN_ENABLED_FOR_VIDEO	0x01
#define	IAV_VIN_ENABLED_FOR_STILL	0x02

struct amba_iav_vin_info {
	u32				enabled;
	int				active_src_id;
	struct amba_vin_src_capability	capability;
};

struct amba_iav_vout_info {
	u32				enabled;
	int				active_sink_id;
	struct amba_video_sink_mode	active_mode;
	struct amba_video_info		video_info;
};

#endif	//__AMBA_IAV_H

