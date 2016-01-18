/**
 * @file system/include/eeprom/eepart.h
 *
 * History:
 *    2009/2/10 - [Chien-Yang Chen] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __EEPART_H__
#define __EEPART_H__

#include <board.h>

#define SPI_EEPROM_NONE		0
#define EEPROM_READ_BLOCK_SIZE	2048

#if	defined(CONFIG_EEPROM_M95128)
#include <eeprom/m95128.h>
#elif	defined(CONFIG_EEPROM_M95640)
#include <eeprom/m95640.h>
#elif	defined(CONFIG_EEPROM_AT25256A)
#include <eeprom/at25256a.h>
#elif	defined(CONFIG_EEPROM_AT25640A)
#include <eeprom/at25640a.h>
#elif	defined(CONFIG_EEPROM_MX25L4005C)
#include <eeprom/mx25l4005c.h>
#elif	defined(CONFIG_EEPROM_MX25L1005)
#include <eeprom/mx25l1005.h>
#elif	defined(CONFIG_EEPROM_EN25Q128)
#include <eeprom/en25q128b.h>
#elif	defined(CONFIG_EEPROM_IS26C64B)
#include <eeprom/is25c64b.h>
#elif	defined(CONFIG_EEPROM_MC25LC1024)
#include <eeprom/mc25lc1024.h>
#elif   defined(CONFIG_EEPROM_MX25L512E)
#include <eeprom/mx25l512e.h>
#elif   defined(CONFIG_EEPROM_MX25L1006E)
#include <eeprom/mx25l1006e.h>
#elif   defined(CONFIG_EEPROM_S25FL256S)
#include <eeprom/s25fl256s.h>
#else
/* CONFIG_EEPROM_NONE */
#define SPI_EEPROM		1
#define SPI_FLASH		2

#define EEPROM_TYPE		SPI_EEPROM_NONE
#define EEPROM_CHIP_SIZE	0
#define EEPROM_PAGE_SIZE	512
#define EEPROM_FREQ_HZ		0
#define EEPROM_ADDR_BITS	0
#define EEPROM_ADDR_BYTES	0
#define EEPROM_SPI_MODE		0
#define EEPROM_DATA_BITS	0
#define EEPROM_NAME		"EEPROM_NONE"

#define EEPROM_IS_WREN		0
#define EEPROM_IS_WRDI		0
#define EEPROM_IS_RDSR		0
#define EEPROM_IS_WRSR		0
#define EEPROM_IS_READ		0
#define EEPROM_IS_WRITE		0

#endif	/* CONFIG_EEPROM_NONE */

#if (EEPROM_TYPE != SPI_FLASH)
#define EEPROM_TOTAL_BLOCKS	0x1
#define EEPROM_IS_RDID		0
#define EEPROM_IS_BE		0
#define EEPROM_IS_CE		0
#endif

#endif	/* __EEPART_H__ */
