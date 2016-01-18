/**
 * @file system/include/eeprom/mx25l4005c.h
 *
 * History:
 *    2010/04/12 - [Evan Chen] created file
 *
 * Copyright (C) 2010-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __MX25L4005C_H__
#define __MX25L4005C_H__

#define SPI_EEPROM		1	/* byte-wise eraseable */
#define SPI_FLASH		2	/* block-wise eraseable */

/****************************************************************/
/* SPI specification.						*/
/****************************************************************/
#define EEPROM_TYPE		SPI_FLASH
#define EEPROM_CHIP_SIZE	(4096 * 1024 / 8)
#define EEPROM_PAGE_SIZE	256
#define EEPROM_FREQ_HZ		10000000
#define EEPROM_ADDR_BITS	19	/* Effective address bits */
#define EEPROM_ADDR_BYTES	3
#define EEPROM_SPI_MODE		3
#define EEPROM_DATA_BITS	8	/* 8 bits data frame. */
#define EEPROM_NAME		"MACRONIX EEPROM MX25L4005C_512KB"

/****************************************************************/
/* These are basic instructions for common eeprom (SPI_EEPROM). */
/****************************************************************/
#define EEPROM_IS_WREN		0x06	/* Set write enable latch */
#define EEPROM_IS_WRDI		0x04	/* Reset write enable latch */
#define EEPROM_IS_RDSR		0x05	/* Read status register */
#define EEPROM_IS_WRSR		0x01	/* write status register */
#define EEPROM_IS_READ		0x03	/* Read data from memory array */
#define EEPROM_IS_WRITE		0x02	/* Write data to memory array */

/****************************************************************/
/* Command for SPI FLASH(No use for SPI EEPROM).		*/
/****************************************************************/
#define EEPROM_TOTAL_BLOCKS	8
#define EEPROM_IS_RDID		0x9f	/* Output manufacturer and device ID */
#define EEPROM_IS_BE		0x52	/* Block erase */
#define EEPROM_IS_CE		0x60	/* Chip erase */

#endif

