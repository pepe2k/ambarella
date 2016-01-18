/**
 * @file system/include/eeprom/is25c64b.h
 *
 * History:
 *    2011/05/02 - [Evan Chen] created file
 *
 * Copyright (C) 2011-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __IS26C64B_H__
#define __IS26C64B_H__

#define SPI_EEPROM		1	/* byte-wise eraseable */
#define SPI_FLASH		2	/* block-wise eraseable */

/****************************************************************/
/* SPI specification.						*/
/****************************************************************/
#define EEPROM_TYPE		SPI_EEPROM
#define EEPROM_CHIP_SIZE	(64 * 1024 / 8)
#define EEPROM_PAGE_SIZE	32
#define EEPROM_FREQ_HZ		2000000
#define EEPROM_ADDR_BITS	15	/* Effective address bits */
#define EEPROM_ADDR_BYTES	2
#define EEPROM_SPI_MODE		3
#define EEPROM_DATA_BITS	8	/* 8 bits data frame. */
#define EEPROM_NAME		"INTEGRATED SILICON EEPROM IS26C64B_8KB"

/****************************************************************/
/* These are basic instructions for common eeprom (SPI_EEPROM). */
/****************************************************************/
#define EEPROM_IS_WREN		0x06	/* Set write enable latch */
#define EEPROM_IS_WRDI		0x04	/* Reset write enable latch */
#define EEPROM_IS_RDSR		0x05	/* Read status register */
#define EEPROM_IS_WRSR		0x01	/* write status register */
#define EEPROM_IS_READ		0x03	/* Read data from memory array */
#define EEPROM_IS_WRITE		0x02	/* Write data to memory array */

#endif

