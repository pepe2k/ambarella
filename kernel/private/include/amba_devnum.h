/*
 * kernel/private/include/amba_devnum.h
 *
 * History:
 *    2012/10/25 - [Jian Tang] Create this file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_DEVNUM_H__
#define __AMBA_DEVNUM_H__

#include <mach/hardware.h>

#define IAV_DEV_MAJOR		AMBA_DEV_MAJOR
#define IAV_DEV_MINOR		0

#define UCODE_DEV_MAJOR		AMBA_DEV_MAJOR
#define UCODE_DEV_MINOR		1

#define DSPLOG_DEV_MAJOR	AMBA_DEV_MAJOR
#define DSPLOG_DEV_MINOR	(AMBA_DEV_MINOR_PUBLIC_END + 9)

#endif	// __AMBA_DEVNUM_H__

