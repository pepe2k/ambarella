/**
 * @file system/include/flash/inor/parts.h
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

#ifndef __FLASH_INOR_PARTS_H__
#define __FLASH_INOR_PARTS_H__

/******************************************************************************/
/* NOR                                                                        */
/******************************************************************************/
#if defined(CONFIG_NOR_E28F128)
#include <flash/inor/e28f128j3a.h>
#elif defined(CONFIG_NOR_E28F640)
#include <flash/inor/e28f640j3a.h>
#elif defined(CONFIG_NOR_E28F320)
#include <flash/inor/e28f320j3a.h>
#elif defined(CONFIG_NOR_JS28F320)
#include <flash/inor/js28f320j3d75.h>
#else

#define NOR_CONTROL		0x0

#define NOR_SECTOR_SIZE		0
#define NOR_CHIP_SIZE		0
#define NOR_TOTAL_SECTORS	0
#define NOR_DEVICES		0
#define NOR_NAME		"NOR_NONE"

#define NOR_TAS			0
#define NOR_TCS			0
#define NOR_TDS			0
#define NOR_TAH			0
#define NOR_TCH			0
#define NOR_TDH			0
#define NOR_TWP			0
#define NOR_TWH			0
#define NOR_TRC			0
#define NOR_TADELAY		0
#define NOR_TCDELAY		0
#define NOR_TRPDELAY		0
#define NOR_TRDELAY		0
#define NOR_TABDELAY		0
#define NOR_TWHR		0
#define NOR_TIR			0
#define NOR_TRHZ		0
#define NOR_TRPH		0

#endif

#endif
