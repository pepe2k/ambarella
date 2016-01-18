/**
 * system/src/bld/eeprom.c
 *
 * History:
 *    2008/01/16 - [Dragon Chiang] - created file
 *    2009/02/11 - [Chien-Yang Chen] - reconstructed to boost the performance
 *    and support multiple instances.
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <peripheral/eeprom.h>
#include <peripheral/spi.h>

#if defined(ENABLE_EEPROM)

#define EEPROM_BUF_SIZE		0x10000
#if (((EEPROM_READ_BLOCK_SIZE + 5) * 2) > EEPROM_BUF_SIZE)
#error	Wrong eeprom buffer configuration!
#endif

static u8 g_eebuf[EEPROM_BUF_SIZE]
__attribute__ ((aligned(32), section(".bss.noinit")));

static struct eeprom_s L_config[EEPROM_ID_MAX]
__attribute__ ((aligned(32), section(".bss.noinit")));

/*******************************/
/* EEPROM low level functions. */
/*******************************/
static int __eeprom_set_status(struct eeprom_s *config, unsigned char value)
{
	int rval;
	unsigned short wdata[2];

	/* write enable */
	wdata[0] = EEPROM_IS_WREN;
	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata, 0x1);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -4;
	}
	if (rval < 0) {
		putstr("spi_write(): WREN failed\r\n");
		return -1;
	}

	/*
	 * |<    8 bits    >|<    8 bits    >|
	 *  <  instruction > <     data     >
	 */
	wdata[0] = EEPROM_IS_WRSR;
	wdata[1] = (unsigned short) value;

	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata, 0x2);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -4;
	}
	if (rval < 0) {
		putstr("spi_write(): WRSR failed\r\n");
		return -2;
	}

	/* write disable */
	wdata[0] = EEPROM_IS_WRDI;
	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata, 0x1);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -4;
	}
	if (rval < 0) {
		putstr("spi_write(): WRDI failed\r\n");
		return -3;
	}

	return rval;
}

/*
 * Get EEPROM status. (low level function)
 */
static int __eeprom_get_status(struct eeprom_s *config)
{
	int rval;
	unsigned short rdata[2];
	unsigned short wdata[2];

	/*
	 * |<    8 bits    >|<    8 bits    >|
	 *  <  instruction >
	 *                   <     data     >
	 */

	wdata[0] = EEPROM_IS_RDSR;

	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write_read(config->bus.spi.chan_id, wdata, rdata,
				      0x1, 0x2);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -2;
	}

	if (rval < 0) {
		putstr("spi_write_read(): RDSR failed\r\n");
		return -1;
	}

	return ((int) rdata[1]);
}

static int __eeprom_read_in_block(struct eeprom_s *config,
				  unsigned int addr,
				  unsigned char *buf,
				  int len)
{
	int i, offset, rval;
	unsigned short wdata[5];
	unsigned short *rdata = (unsigned short *) g_eebuf;

	offset = addr & (config->rblock_size - 1);
	if (offset + len > config->rblock_size) {
		/* out of range of read block */
		return -1;
	}

	/*
	 * <instruction><address>
	 * 			 <data>
	 */

	offset = config->addr_size + 1;
	wdata[0] = EEPROM_IS_READ;

	switch (config->addr_size) {
	case 2:
		wdata[1] = ((addr >> 8) & 0xff);
		wdata[2] =  (addr       & 0xff);
		break;
	case 3:
		wdata[1] = ((addr >> 16) & 0xff);
		wdata[2] = ((addr >>  8) & 0xff);
		wdata[3] =  (addr        & 0xff);
		break;
	default:
		return -2;
	}

	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write_read(config->bus.spi.chan_id, wdata, rdata,
				      offset, (offset + len));
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -1;
	}
	if (rval < 0) {
		putstr("spi_write_read(): READ failed\r\n");
		return -3;
	}

	for (i = 0; i < len; i++) {
		buf[i] = (unsigned char) rdata[offset + i];
	}

	return 0;
}

/*
 * Read EEPROM data to buffer. (low level function)
 */
static int __eeprom_read(struct eeprom_s *config,
			 unsigned int addr,
			 unsigned char *buf,
			 int len)
{
	int i, rval, offset;
	int blocks, first_pblock_size, last_pblock_size;

	offset = addr & (config->rblock_size - 1);
	if (offset == 0)
		first_pblock_size = 0;
	else
		first_pblock_size = config->rblock_size - offset;

	if (len >= first_pblock_size) {
		blocks = (len - first_pblock_size) / config->rblock_size;
		last_pblock_size =
			(len - first_pblock_size) % config->rblock_size;
	} else {
		first_pblock_size = len;
		blocks = 0;
		last_pblock_size = 0;
	}

	if (first_pblock_size) {
		rval = __eeprom_read_in_block(config, addr,
					buf, first_pblock_size);
		if (rval < 0) {
			putstr("read first_pblock_size failed\r\n");
			return rval;
		}
		buf += first_pblock_size;
		addr += first_pblock_size;
	}

	for (i = 0; i < blocks; i++) {
		rval = __eeprom_read_in_block(config, addr,
					buf, config->rblock_size);
		if (rval < 0) {
			putstr("read blocks (");
			putdec(i);
			putstr(") failed\r\n");
			return rval;
		}
		buf += config->rblock_size;
		addr += config->rblock_size;
	}

	if (last_pblock_size) {
		rval = __eeprom_read_in_block(config, addr,
					buf, last_pblock_size);
		if (rval < 0) {
			putstr("read last_pblock_size failed\r\n");
			return rval;
		}
	}

	return 0;
}

static int __eeprom_write_in_page(struct eeprom_s *config,
				  unsigned int addr,
				  unsigned char *buf,
				  int len)
{
	int i, offset, rval;
	unsigned short *wdata = (unsigned short *) g_eebuf;

	offset = addr & (config->page_size - 1);
	if (offset + len > config->page_size) {
		/* out of range of page */
		return -1;
	}

	/* write enable */
	wdata[0] = EEPROM_IS_WREN;
	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata, 0x1);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -5;
	}

	if (rval < 0) {
		putstr("spi_write(): WREN failed\r\n");
		return -1;
	}

	/*
	 * <instruction><address><data>
	 */
	wdata[0] = EEPROM_IS_WRITE;

	switch (config->addr_size) {
	case 2:
		wdata[1] = ((addr >> 8) & 0xff);
		wdata[2] = ( addr       & 0xff);
		break;
	case 3:
		wdata[1] = ((addr >> 16) & 0xff);
		wdata[2] = ((addr >>  8) & 0xff);
		wdata[3] = ( addr        & 0xff);
		break;
	default:
		return -2;
	}

	offset = config->addr_size + 1;
	for (i = 0; i < len; i++) {
		wdata[offset + i] =  (unsigned short) buf[i];
	}

	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata,  offset + len);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -5;
	}

	if (rval < 0) {
		putstr("spi_write(): WRITE failed\r\n");
		return -3;
	}

	/* Poll for ready. */
	while (__eeprom_get_status(config) & 0x1);

	/* write disable */
	wdata[0] = EEPROM_IS_WRDI;
	if (config->bus_type == BUS_TYPE_SPI) {
		rval = spi_write(config->bus.spi.chan_id, wdata, 0x1);
	} else {
		putstr("unsupport eeprom bus type.\r\n");
		return -1;
	}
	if (rval < 0) {
		putstr("spi_write(): WRDI failed\r\n");
		return -4;
	}

	return 0;
}

/*
 * Write buffer data to EEPROM. (low level function)
 */
static int __eeprom_write(struct eeprom_s *config,
			  unsigned int addr,
			  unsigned char *buf,
			  int len)
{
	int i, rval, offset;
	int pages, first_ppage_size, last_ppage_size;

	offset = addr & (config->page_size - 1);
	if (offset == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = config->page_size - offset;

	if (len >= first_ppage_size) {
		pages = (len - first_ppage_size) / config->page_size;
		last_ppage_size = (len - first_ppage_size) % config->page_size;
	} else {
		first_ppage_size = len;
		pages = 0;
		last_ppage_size = 0;
	}

	if (first_ppage_size) {
		rval = __eeprom_write_in_page(config, addr,
					buf, first_ppage_size);
		if (rval < 0) {
			putstr("write first_ppage_size failed\r\n");
			return rval;
		}
		buf += first_ppage_size;
		addr += first_ppage_size;
	}

	for (i = 0; i < pages; i++) {
		rval = __eeprom_write_in_page(config, addr,
					buf, config->page_size);
		if (rval < 0) {
			putstr("write pages (");
			putdec(i);
			putstr(") failed\r\n");
			return rval;
		}
		buf += config->page_size;
		addr += config->page_size;
	}

	if (last_ppage_size) {
		rval = __eeprom_write_in_page(config, addr,
					buf, last_ppage_size);
		if (rval < 0) {
			putstr("write last_ppage_size failed\r\n");
			return rval;
		}
	}

	return 0;
}

static int __eeprom_erase_block_wise(struct eeprom_s *config,
				     unsigned int addr, int len)
{
	unsigned short wdata[4];

	if (config->bus_type != BUS_TYPE_SPI) {
		putstr("unsupport eeprom bus type");
		return -1;
	}

	/*
	 * <instruction><address><data>
	 */

	/* write enable */
	wdata[0] = EEPROM_IS_WREN;
	if (spi_write(config->bus.spi.chan_id, wdata, 0x1) < 0) {
		putstr("spi_write(): WREN failed");
		return -2;
	}

	if (addr + len >= EEPROM_TOTAL_BLOCKS) {
		/* Erase all */
		wdata[0] = EEPROM_IS_CE;
		if (spi_write(config->bus.spi.chan_id, wdata, 0x1) < 0) {
			putstr("spi_write_read(): CE failed");
			return -2;
		}

		/* Poll for ready. */
		while (__eeprom_get_status(config) & 0x1);
	} else {
		/* Erase block */
		u32 blk;
		wdata[0] = EEPROM_IS_BE;
		for (blk = addr; blk < (addr + len); blk++) {
			switch (config->addr_size) {
			case 2:
				wdata[1] = ((blk >> 8) & 0xff);
				wdata[2] =  (blk       & 0xff);
				break;
			case 3:
				wdata[1] = ((blk >> 16) & 0xff);
				wdata[2] = ((blk >>  8) & 0xff);
				wdata[3] =  (blk        & 0xff);
				break;
			default:
				return -2;
			}
			if (0 > spi_write(config->bus.spi.chan_id, wdata,
					  config->addr_size + 1)) {
				putstr("spi_write_read(): BE failed");
				return -2;
			}
			/* Poll for ready. */
			while (__eeprom_get_status(config) & 0x1);
		}
	}

	/* write disable */
	wdata[0] = EEPROM_IS_WRDI;
	if (spi_write(config->bus.spi.chan_id, wdata, 0x1) < 0) {
		putstr("spi_write(): WRDI failed");
		return -2;
	}

	return 0;
}

static int __eeprom_erase_byte_wise(struct eeprom_s *config,
				    unsigned int addr, int len)
{
	int i, rval, offset;
	int pages, first_ppage_size, last_ppage_size;
	unsigned char *buf;

	offset = addr & (config->page_size - 1);
	if (offset == 0)
		first_ppage_size = 0;
	else
		first_ppage_size = config->page_size - offset;

	if (len >= first_ppage_size) {
		pages = (len - first_ppage_size) / config->page_size;
		last_ppage_size = (len - first_ppage_size) % config->page_size;
	} else {
		first_ppage_size = len;
		pages = 0;
		last_ppage_size = 0;
	}

	buf = g_eebuf;
	memset(buf, 0xff, config->page_size);

	if (first_ppage_size) {
		rval = __eeprom_write_in_page(config, addr,
					buf, first_ppage_size);
		if (rval < 0) {
			putstr("write first_ppage_size failed\r\n");
			return rval;
		}
		addr += first_ppage_size;
	}

	for (i = 0; i < pages; i++) {
		rval = __eeprom_write_in_page(config, addr,
					buf, config->page_size);
		if (rval < 0) {
			putstr("write pages (");
			putdec(i);
			putstr(") failed\r\n");
			return rval;
		}
		addr += config->page_size;
	}

	if (last_ppage_size) {
		rval = __eeprom_write_in_page(config, addr,
					buf, last_ppage_size);
		if (rval < 0) {
			putstr("write last_ppage_size failed\r\n");
			return rval;
		}
	}

	return 0;
}

static int __eeprom_erase(struct eeprom_s *config,
			  unsigned int addr, int len)
{
	if (config->rom_type == SPI_EEPROM)
		return __eeprom_erase_byte_wise(config, addr, len);
	else if (config->rom_type == SPI_FLASH)
		return __eeprom_erase_block_wise(config, addr, len);

	return -1;
}

struct eeprom_s *__eeprom_init(int id)
{
	struct eeprom_s *config = &L_config[id];
	int chan_id;

	memset(config, 0x0, sizeof(struct eeprom_s));

#ifdef PREFER_I2C_EEPROM
	/* I2C configurate parameters */
#else
	/* SPI configurate parameters */
	if (id == EEPROM_ID_DEV0)
		chan_id = EEPROM_DEV0_SPI_CHAN;
	else if (id == EEPROM_ID_DEV1)
		chan_id = EEPROM_DEV1_SPI_CHAN;
	else
		return NULL;

	config->bus_type = BUS_TYPE_SPI;

	config->bus.spi.chan_id    	= chan_id;
	config->bus.spi.mode       	= EEPROM_SPI_MODE;
	config->bus.spi.frame_size 	= EEPROM_DATA_BITS - 1;
	config->bus.spi.baud_rate  	= EEPROM_FREQ_HZ;
#endif
	/* EEPROM spec. */
	config->rom_type	= EEPROM_TYPE;
	config->addr_size  	= EEPROM_ADDR_BYTES;
	config->chip_size  	= EEPROM_CHIP_SIZE;
	config->page_size  	= EEPROM_PAGE_SIZE;

	/* User parameters. (Software configuration) */
	config->rblock_size	= EEPROM_READ_BLOCK_SIZE;

	config->read  =
		(int (*)(struct eeprom_s*, unsigned int,
			 unsigned char *, int)) __eeprom_read;
	config->write =
		(int (*)(struct eeprom_s*, unsigned int,
			 unsigned char *, int)) __eeprom_write;
	config->erase =
		(int (*)(struct eeprom_s*,
			 unsigned int, int)) __eeprom_erase;
	config->set_status =
		(int (*)(struct eeprom_s*, u8)) __eeprom_set_status;
	config->get_status =
		(int (*)(struct eeprom_s*)) __eeprom_get_status;

	return config;
}

/*****************************************************************************/
static int eeprom_init(int id)
{
	static int init[EEPROM_ID_MAX];

	if (id >= EEPROM_ID_MAX)
		return -1;

	if (init[id] == 0) {
		struct eeprom_s *config;

		config = __eeprom_init(id);
		if (config == NULL || config->rom_type == SPI_EEPROM_NONE)
			return -1;

		if (config->bus_type == BUS_TYPE_SPI) {
			struct bus_spi_s *spi = &(config->bus.spi);
			spi_config(spi->chan_id, spi->mode,
				   spi->frame_size, spi->baud_rate);
		} else {
			putstr("unsupport eeprom bus type.\r\n");
			return -1;
		}

		init[id] = 1;
	}

	return 0;
}

int eeprom_erase(int id, unsigned int addr, int len)
{
	int rval;
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	if ((addr + len) > config->chip_size)
		return -2;

	return config->erase(config, addr, len);
}

int eeprom_read(int id, unsigned int addr, unsigned char *buf, int len)
{
	int rval;
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	if ((addr + len) > config->chip_size)
		return -2;

	return config->read(config, addr, buf, len);
}

int eeprom_write(int id, unsigned int addr, unsigned char *buf, int len)
{
	int rval;
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	if ((addr + len) > config->chip_size)
		return -2;

	return config->write(config, addr, buf, len);
}

int eeprom_set_status(int id, unsigned char val)
{
	int rval;
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	return config->set_status(config, val);
}

int eeprom_get_status(int id)
{
	int rval;
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	return config->get_status(config);
}

/*
 * Program binary code in buffer to EEPROM with data check.
 */
int eeprom_prog(int id, unsigned char *payload, unsigned int payload_len,
		int (*prgs_rpt)(int, void *), void *arg)
{
	int rval = -1;
	int wlen, this_wlen, remain_size, percentage;
	u8 *raw_image = payload + sizeof(partimg_header_t);
	unsigned char rbuf[EEPROM_PAGE_SIZE];
	struct eeprom_s *config = &L_config[id];

	rval = eeprom_init(id);
	if (rval < 0) {
		return -1;
	}

	if (payload_len > config->chip_size) {
		putstr("Data is too large, EEPROM has not enough space.\r\n");
		return rval;
	}

	if (config->rom_type == SPI_FLASH) {
		/* erase before program */
		u32 blksz = (config->chip_size / EEPROM_TOTAL_BLOCKS);
		u32 blks = (payload_len % blksz) ? (payload_len / blksz + 1):
						   (payload_len / blksz);
		rval = config->erase(config, 0, blks);
		if (rval < 0)
			return rval;
	}

	wlen = 0;
	while (wlen < payload_len) {
		remain_size = payload_len - wlen;
		if (remain_size > EEPROM_PAGE_SIZE)
			this_wlen = EEPROM_PAGE_SIZE;
		else
			this_wlen = remain_size;

		rval = config->write(config, wlen, raw_image, this_wlen);
		if (rval < 0)
			return rval;

		rval = config->read(config, wlen, rbuf, this_wlen);
		if (rval < 0)
			return rval;

		rval = memcmp(raw_image, rbuf, this_wlen);
		if (rval != 0)
			return rval;

		raw_image += this_wlen;
		wlen += this_wlen;

		if (prgs_rpt) {
			if (wlen >= payload_len)
				percentage = 100;
			else
				percentage = wlen * 100 / payload_len;

			prgs_rpt(percentage, arg);
		}
	}

	return rval;
}

struct eeprom_s *get_eeprom(int id)
{
	if (id >= EEPROM_ID_MAX)
		return NULL;

	return &L_config[id];
}
#else	/* ENABLE_EEPROM */

int eeprom_erase(int id, unsigned int addr, int len)
{
	return -1;
}

int eeprom_read(int id, unsigned int addr, unsigned char *buf, int len)
{
	return -1;
}

int eeprom_write(int id, unsigned int addr, unsigned char *buf, int len)
{
	return -1;
}

int eeprom_set_status(int id, unsigned char val)
{
	return -1;
}

int eeprom_get_status(int id)
{
	return -1;
}

int eeprom_prog(int id, unsigned char *payload, unsigned int payload_len,
		int (*prgs_rpt)(int, void *), void *arg)
{
	return -1;
}

struct eeprom_s *get_eeprom(int id)
{
	return NULL;
}
#endif
