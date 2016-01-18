/**
 * @file system/include/flash/flpart.h
 *
 * History:
 *    2005/03/03 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __FLASH_FLPART_H__
#define __FLASH_FLPART_H__

#include <board.h>

#if (ONENAND_NOR_SUPPORT == 0)
#include <flash/slcnand/parts.h>
#include <flash/inor/parts.h>

#define CONFIG_ONENAND_NONE	1
#define CONFIG_SNOR_NONE	1
#else
#include <flash/slcnand/parts.h>
#include <flash/onenand/parts.h>
#include <flash/inor/parts.h>
#include <flash/snor/parts.h>
#endif /* ONENAND_NOR_SUPPORT */

#if defined(CONFIG_NOR_NONE) && defined(CONFIG_SNOR_NONE) &&  \
    defined(CONFIG_NAND_NONE) && defined(CONFIG_ONENAND_NONE)
#define	FLASH_NONE		1
#endif

#if defined(CONFIG_NOR_NONE) && defined(CONFIG_SNOR_NONE)
#define	NOR_FLASH_NONE		1
#endif

#if defined(CONFIG_NAND_NONE) && defined(CONFIG_ONENAND_NONE)
#define	NAND_FLASH_NONE		1
#endif

#endif
