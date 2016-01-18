/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441/arch_a7/adv7441a_arch.h
 *
 * History:
 *    2011/10/19 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __ADV7441A_ARCH_H__
#define __ADV7441A_ARCH_H__

#define ADV7441A_PREFER_EMBMODE

#define ADV7441A_1080P60_601_CAP_START_X		(0x0094)
#define ADV7441A_1080P60_601_CAP_START_Y		(0x0024)
#define ADV7441A_1080P60_601_CAP_SYNC_START		(0x8040)

#define ADV7441A_1080P50_601_CAP_START_X		(0x0094)
#define ADV7441A_1080P50_601_CAP_START_Y		(0x0024)
#define ADV7441A_1080P50_601_CAP_SYNC_START		(0x801b)

#define ADV7441A_1080I60_601_CAP_START_X		(0x0094)
#define ADV7441A_1080I60_601_CAP_START_Y		(0x0010)
#define ADV7441A_1080I60_601_CAP_SYNC_START		(0x8060)

#define ADV7441A_1080I50_601_CAP_START_X		(0x0074)
#define ADV7441A_1080I50_601_CAP_START_Y		(0x0010)
#define ADV7441A_1080I50_601_CAP_SYNC_START		(0x8030)

#define ADV7441A_720P60_601_CAP_START_X			(0x00dc)
#define ADV7441A_720P60_601_CAP_START_Y			(0x0016)
#define ADV7441A_720P60_601_CAP_SYNC_START		(0x801b)

#define ADV7441A_720P50_601_CAP_START_X			(0x00dc)
#define ADV7441A_720P50_601_CAP_START_Y			(0x0016)
#define ADV7441A_720P50_601_CAP_SYNC_START		(0x8038)

#define ADV7441A_576P_601_CAP_START_X			(0x0034)
#define ADV7441A_576P_601_CAP_START_Y			(0x0020)
#define ADV7441A_576P_601_CAP_SYNC_START		(0x8000)

#define ADV7441A_480P_601_CAP_START_X			(0x0034)
#define ADV7441A_480P_601_CAP_START_Y			(0x0020)
#define ADV7441A_480P_601_CAP_SYNC_START		(0x8000)

#define ADV7441A_576I_656_CAP_START_X			(0)
#define ADV7441A_576I_656_CAP_START_Y			(12)
#define ADV7441A_576I_656_CAP_SYNC_START		(0x8000)

#define ADV7441A_576I_601_CAP_START_X			(0x0040)
#define ADV7441A_576I_601_CAP_START_Y			(0x0012)
#define ADV7441A_576I_601_CAP_SYNC_START		(0x8000)

#define ADV7441A_480I_656_CAP_START_X			(0)
#define ADV7441A_480I_656_CAP_START_Y			(12)
#define ADV7441A_480I_656_CAP_SYNC_START		(0x8000)

#define ADV7441A_480I_601_CAP_START_X			(0x003a)
#define ADV7441A_480I_601_CAP_START_Y			(0x0008)
#define ADV7441A_480I_601_CAP_SYNC_START		(0x8000)

#define ADV7441A_VGA_CAP_START_X			(48)
#define ADV7441A_VGA_CAP_START_Y			(33)
#define ADV7441A_VGA_CAP_SYNC_START			(0x8000)

#define ADV7441A_SVGA_CAP_START_X			(88)
#define ADV7441A_SVGA_CAP_START_Y			(23)
#define ADV7441A_SVGA_CAP_SYNC_START			(0x8000)

#define ADV7441A_XGA_CAP_START_X			(160)
#define ADV7441A_XGA_CAP_START_Y			(29)
#define ADV7441A_XGA_CAP_SYNC_START			(0x8000)

#define ADV7441A_SXGA_CAP_START_X			(248)
#define ADV7441A_SXGA_CAP_START_Y			(38)
#define ADV7441A_SXGA_CAP_SYNC_START			(0x8000)

#define ADV7441A_TBD_CAP_START_X			(0)
#define ADV7441A_TBD_CAP_START_Y			(0)
#define ADV7441A_TBD_CAP_SYNC_START			(0x8000)

#define ADV7441A_EMBMODE_CAP_START_X			(0)
#define ADV7441A_EMBMODE_CAP_START_Y			(0)
#define ADV7441A_EMBMODE_CAP_SYNC_START			(0x8000)

#endif /* __ADV7441A_ARCH_H__ */
