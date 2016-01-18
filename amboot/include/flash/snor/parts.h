/**
 * @file system/include/flash/snor/parts.h
 *
 * History:
 *    2009/08/17 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FLASH_SNOR_PARTS_H__
#define __FLASH_SNOR_PARTS_H__

#include <flash/snor/command.h>

#if defined(CONFIG_NOR_S29GL256P)
#include <flash/snor/s29gl256p.h>
#else
#define SNOR_MANID			0x0
#define SNOR_DEVID			0x0
#define SNOR_ID2			0x0
#define SNOR_ID3			0x0
#endif

#define SNOR_ID		((unsigned long)			   \
			 (SNOR_ID3 << 24)  | (SNOR_ID2 << 16) |    \
			 (SNOR_DEVID << 8) | SNOR_MANID)
#endif
