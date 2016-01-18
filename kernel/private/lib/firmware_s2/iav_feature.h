/*
 * iav_feature.h
 *
 * History:
 *	2012/08/11 - [Jian Tang] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_FEATURE_H__
#define __IAV_FEATURE_H__

driver_version_t iav_driver_info =
{
	.arch	= S2,
	.model	= 1,
	.major	= 4,	/* SDK version 4.0.6 */
	.minor	= 0,
	.patch	= 6,
	.mod_time	= 0x20150603,
	.description = "S2 Flexible Linux",
	.api_version	= API_REVISION_U32,
	.idsp_version	= IDSP_REVISION_U32,
};

#endif	// __IAV_FEATURE_H__

