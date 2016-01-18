/**
 * system/src/bld/idc.c
 *
 * Implement the driver APIs for IDC interface under boot loader.
 * IDC can communicate with the device with IDC
 *
 * History:
 *	2005/04/21 - [Allen Wang] created file.
 *	2011/02/16 - [Jack Huang] ported idc driver from kernel to bld.
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <basedef.h>
#include <ambhw.h>
#include <bldfunc.h>
#include <peripheral/idc.h>

/* IDC ports using master controllers */
#define IDC_MASTER1	0
#define IDC_MASTER2	1
#define IDC_MASTER3	2

/* IDC operation modes */
#define IDC_HIGH_MODE_400KHZ	0
#define IDC_FAST_MODE_100KHZ	1
#define IDC_NORMAL_MODE_10KHZ	2

#define IDC_ACK_MASK 		0x00000001

#define IDC_INT_MAX_WAIT_LOOP	24000	/* This is used temporarily */
#define FIFO_BUF_SIZE 63

#define CURR_IDC_REG(idc, offset) 	idc_get_reg_addr(idc, offset)

#define _IDC_1		0
#if (IDC_INSTANCES >= 2)
#define _IDC_2		1
#endif
#if (IDC_INSTANCES >= 3)
#define _IDC_3		2
#endif

static void idc_access_seq(u8 idc, u8 read_write, u8 idc_slave_type,
			   u16 addr, u16 subaddr, u16 *pdata, u16 data_size);
static void idc_set_extended_addr(u8 idc, u16 addr);
static void idc_set_8bit_addr(u8 idc, u16 addr);
static void idc_burst_writew(u8 idc, u16 *pdata, u8 fifo_fulness, u16 nwords);
static void idc_burst_writeb(u8 idc, u16 *pdata, u8 fifo_fulness, u16 nbytes);
static int idc_wait_interrupt_pending(u8 idc, u32 ctrl_reg_addr,
				      u32 idc_ctrl_reg);
static void idc_put_fifo_words(u8 idc, u16 *pdata, u16 nwords);
static void idc_put_fifo_bytes(u8 idc, u16 *pdata, u16 nbytes);

/* Controllers-Ports mapping */
static u8 idc_port_map_to_ctrl(u8 idc)
{
	switch (idc) {
		case IDC_MASTER1:
			return _IDC_1;
#if (CHIP_REV == A5) || (CHIP_REV == A6) || (CHIP_REV == A5S) || \
	(CHIP_REV == A7) || (CHIP_REV == I1) || (CHIP_REV == S2)
		case IDC_MASTER2:
			return _IDC_2;
#elif (CHIP_REV == A2S) || (CHIP_REV == A5L)
		case IDC_MASTER2:
			return _IDC_1;
#endif
#if (CHIP_REV == S2)
		case IDC_MASTER3:
			return	_IDC_3;
#elif (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
	(CHIP_REV == I1)
		case IDC_MASTER3:
			return	_IDC_1;
#endif
	}
	return 0xff;
}

/* Select IDC BASE, than return correct address*/
static u32 idc_get_reg_addr(u8 idc, u32 offset)
{
	switch (idc) {
		case _IDC_1:
			return IDC_REG(offset);
#if (IDC_INSTANCES >= 2)
		case _IDC_2:
			return IDC2_REG(offset);
#endif
#if (IDC_INSTANCES >= 3)
		case _IDC_3:
			return IDC3_REG(offset);
#endif
	}

	return 0xffffffff;
}

/* set idc pin mux */
static void set_idc_pin_mux(u8 idc)
{
#if (CHIP_REV == A2S)
	switch (idc) {
		case IDC_MASTER1:
			/* GPIO2.AFSEL[23] == 0 (IDC)*/
			writel(GPIO2_AFSEL_REG,
			       readl(GPIO2_AFSEL_REG) & (~(0x1 << 23)));
			break;
		case IDC_MASTER2:
			/* GPIO2.AFSEL[23] == 1 (DDC)*/
			writel(GPIO2_AFSEL_REG,
			       readl(GPIO2_AFSEL_REG) | (0x1 << 23));
			break;
	}
#elif (CHIP_REV == A5S) || (CHIP_REV == I1)
	switch (idc) {
		case IDC_MASTER1:
			/* GPIO1.AFSEL[4] == 0 (IDC1)*/
			writel(GPIO1_AFSEL_REG,
			       readl(GPIO1_AFSEL_REG) & (~(0x1 << 4)));
			break;
		case IDC_MASTER3:
			/* GPIO1.AFSEL[4] == 1 (IDC3)*/
			writel(GPIO1_AFSEL_REG,
			       readl(GPIO1_AFSEL_REG) | (0x1 << 4));
			break;
	}
#elif (CHIP_REV == A7) || (CHIP_REV == S2)
	switch (idc) {
		case IDC_MASTER1:
			/* GPIO0.AFSEL[17] == 0 (IDC1)*/
			writel(GPIO0_AFSEL_REG,
			       readl(GPIO0_AFSEL_REG) & (~(0x1 << 17)));
			break;
		case IDC_MASTER3:
			/* GPIO0.AFSEL[17] == 1 (IDC3)*/
			writel(GPIO0_AFSEL_REG,
			       readl(GPIO0_AFSEL_REG) | (0x1 << 17));
			break;
	}
#elif (CHIP_REV == A5L)
	switch (idc) {
		case IDC_MASTER1:
			/* GPIO1.AFSEL[31] == 0 (IDC)*/
			writel(GPIO1_AFSEL_REG,
			       readl(GPIO1_AFSEL_REG) & (~(0x1 << 31)));
			/* GPIO2.AFSEL[31] == 0 (IDC)*/
			writel(GPIO2_AFSEL_REG,
			       readl(GPIO2_AFSEL_REG) & (~(0x1 << 31)));
			break;
		case IDC_MASTER2:
			/* GPIO1.AFSEL[31] == 0 (DDC)*/
			writel(GPIO1_AFSEL_REG,
			       readl(GPIO1_AFSEL_REG) & (~(0x1 << 31)));
			/* GPIO2.AFSEL[31] == 1 (DDC)*/
			writel(GPIO2_AFSEL_REG, readl(GPIO2_AFSEL_REG) |
				(unsigned int)(0x1 << 31));
			break;
		case IDC_MASTER3:
			/* GPIO1.AFSEL[31] == 1 (IDC1)*/
			writel(GPIO1_AFSEL_REG, readl(GPIO1_AFSEL_REG) |
				(unsigned int)(0x1 << 31));
			break;
	}
#endif
}

/**
 * IDC/IDC2/IDC3/I2C read
 * Use combined mode for IDC read.
 */
void idc_master_read(u8 idc, u16 idc_slave_type, u16 addr, u16 subaddr,
		u16 *pdata, u16 data_size)
{
	u8 idc_ctrl = idc_port_map_to_ctrl(idc);
	K_ASSERT(idc_ctrl < IDC_INSTANCES);

	/* set idc pin mux */
	set_idc_pin_mux(idc);


	idc_access_seq( idc_ctrl,
			1,
			idc_slave_type,
		    	addr,
		    	subaddr,
		    	pdata,
		    	data_size);

} /* end idc_master_read() */

/**
 * IDC/IDC2/IDC3/I2C write
 */
void idc_master_write(u8 idc, u16 idc_slave_type, u16 addr, u16 subaddr,
		u16 *pdata, u16 data_size)
{
	u8 idc_ctrl = idc_port_map_to_ctrl(idc);
	K_ASSERT(idc_ctrl < IDC_INSTANCES);

	/* set idc pin mux */
	set_idc_pin_mux(idc);

	/* Write/transmit */
	idc_access_seq( idc_ctrl,
			0,
			idc_slave_type,
			addr,
			subaddr,
			pdata,
			data_size);
} /* end idc_master_write() */

/**
 * IDC/IDC2/IDC3/I2C write sequence using a 63-entry byte wide FIFO
 */
void idc_master_burst_write(u8 idc, u16 idc_slave_type, u16 addr, u16 subaddr,
		     u16 *pdata, u16 data_size)
{
	u8 fifo_fulness;
	u8 idc_ctrl = idc_port_map_to_ctrl(idc);

	K_ASSERT(idc_ctrl < IDC_INSTANCES);

	/* set idc pin mux */
	set_idc_pin_mux(idc);

	/* START and IDC slave address */
	if (idc_slave_type > 0x05) {
		/* push START command to FIFO */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMCTRL_OFFSET), 0x04);

		/* push address to FIFO */
		/* Send out the MS byte of extended slave address to write */
		/* 0x11110 (A9)(A8)(0) */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET),
			0xf0 | ( (addr >> 8) & 0x07 ));

		/* Send out the LS byte of extended slave address to write */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET), (addr & 0xff));

		fifo_fulness = 3;

	} else {
		/* push START command to FIFO */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMCTRL_OFFSET), 0x04);

		/* push address to FIF : 7-bit address + 1-bit write (0) */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET), (addr & 0xff));

		fifo_fulness = 2;
	}

	/* Send the sub-address or register address */
	if ((idc_slave_type == 0x04) | (idc_slave_type == 0x05) |
	    (idc_slave_type == 0x0c) | (idc_slave_type == 0x0d) ) {
		/* Send out two bytes of slave subaddress to read */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET),
			(subaddr >> 8) & 0xff);
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET),
			subaddr & 0xff);

		fifo_fulness += 2;

	} else if ((idc_slave_type == 0x02) | (idc_slave_type == 0x03)|
	  	   (idc_slave_type == 0x0a) | (idc_slave_type == 0x0b) ) {
		/* Send out one byte of slave address to read */
		writel(CURR_IDC_REG(idc_ctrl, IDC_FMDATA_OFFSET),
			subaddr & 0xff);
		fifo_fulness ++;
	}

	/* Read data from the slave device */
	if ( idc_slave_type & 0x01 ) {
		/* read/write 16-bit data */
		idc_burst_writew(idc_ctrl, pdata, fifo_fulness, data_size);

	} else {
		/* read/write 8-bit data */
		idc_burst_writeb(idc_ctrl, pdata, fifo_fulness, data_size);
	}


} /* end idc_master_burst_write() */

/**
 * Configure the IDC operation modes
 */
void idc_dclk_config(u8 idc, int mode)
{
	/*
	The relationship between the PCLK and IDC_period
	is shown as the following equivalence.

	IDC_period = idc_clk/(4*(nscale+1))

	In order to meet the IDC's requirement, we need
	to restrict that IDC_period could not be greater
	than 400k. Here, restrict the IDC_period
	not greater than 400k.

	Hence,

	IDC_period = idc_clk/(4*(nscale+1)+IDC_INTERNAL_DELAY_CLK) <= 400k

	==> idc_clk/400k <= 4*(nscale+1)+IDC_INTERNAL_DELAY_CLK

	==> (idc_clk-IDC_INTERNAL_DELAY_CLK*400k)/400k <= 4*(nscale+1)

	==> nscale >= (idc_clk-IDC_INTERNAL_DELAY_CLK*400k)/1600k - 1

	we choose: nscale = (idc_clk-IDC_INTERNAL_DELAY_CLK*400k)/1600k - 1;

	Setttings
	400kHz, SCL: nscale =
	(u16) ((idc_clk - IDC_INTERNAL_DELAY_CLK * 400000 + 800000) / 1600000);
	100kHz, SCL: nscale =
	(u16) ((idc_clk - IDC_INTERNAL_DELAY_CLK * 100000 + 200000) / 400000);
	10kHz, SCL:  nscale =
	(u16) ((idc_clk - IDC_INTERNAL_DELAY_CLK * 10000 + 20000) / 40000);

	nscale -= 1;
	*/

	u16 nscale;
	u32 idc_clk = get_apb_bus_freq_hz();

	switch (mode) {
	case IDC_FAST_MODE_100KHZ:
		nscale = (u16) ((idc_clk -
				 (IDC_INTERNAL_DELAY_CLK * 100000) +
				 200000) / 400000);
		break;
	case IDC_NORMAL_MODE_10KHZ:
		nscale = (u16) ((idc_clk -
				 (IDC_INTERNAL_DELAY_CLK * 10000) +
				 20000) / 40000);
		break;
	default:
		nscale = (u16) ((idc_clk -
				 (IDC_INTERNAL_DELAY_CLK * 400000) +
				 800000) / 1600000);
	};

	nscale -= 1;

	/* LS byte */
	writel(CURR_IDC_REG(idc, IDC_PSLL_OFFSET),
		(nscale & 0xff));
	/* MS byte */
	writel(CURR_IDC_REG(idc, IDC_PSLH_OFFSET),
		((nscale & 0xff00) >> 8));
}

/**
 * This function is called by peripheral_init() for initialization only
 */
void idc_init(void)
{
	/* Disable IDC/I2C */
	writel(CURR_IDC_REG(_IDC_1, IDC_ENR_OFFSET), 0);
	idc_dclk_config(_IDC_1, IDC_FAST_MODE_100KHZ);
	/* Enable IDC/I2C */
	writel(CURR_IDC_REG(_IDC_1, IDC_ENR_OFFSET), 1);

#if (IDC_INSTANCES >= 2)
	/* Disable IDC2/I2C */
	writel(CURR_IDC_REG(_IDC_2, IDC_ENR_OFFSET), 0);
	idc_dclk_config(_IDC_2, IDC_FAST_MODE_100KHZ);
	/* Enable IDC2/I2C */
	writel(CURR_IDC_REG(_IDC_2, IDC_ENR_OFFSET), 1);
#endif

#if (IDC_INSTANCES >= 3)
	/* Disable IDC3/I2C */
	writel(CURR_IDC_REG(_IDC_3, IDC_ENR_OFFSET), 0);
	idc_dclk_config(_IDC_3, IDC_FAST_MODE_100KHZ);
	/* Enable IDC3/I2C */
	writel(CURR_IDC_REG(_IDC_3, IDC_ENR_OFFSET), 1);
#endif
}

/**
 * ACK, clear interrupt, transmit or receive data. The one-byte data transfer
 * is done successfully with ARM interrupt
 */
static int idc_wait_interrupt_pending(u8 idc, u32 ctrl_reg_addr,
				      u32 idc_ctrl_reg)
{
	/* Polling mode*/
	u32 nreg_value = 0;
	int wait_loop = IDC_INT_MAX_WAIT_LOOP;

	writel(ctrl_reg_addr, idc_ctrl_reg);

	K_ASSERT(idc < IDC_INSTANCES);

	do {
		nreg_value = readl(CURR_IDC_REG(idc, IDC_CTRL_OFFSET));

		wait_loop --;

	} while (( (nreg_value & 0x00000002) == 0) && (wait_loop > 0 ) );

	return 0;
} /* idc_wait_interrupt_pending() */

/* Read 2-byte data from IDC bus by polling */
static void idc_read_one_word(u8 idc, u16 *wdata, u8 nack)
{
	/* ACK and clear interrupt*/
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   0);

	*wdata = readl(CURR_IDC_REG(idc, IDC_DATA_OFFSET)); /* MS byte */

	*wdata <<= 8;
	/* ACK and clear interrupt*/
	/* last_byte: 0 - controller_nack (nack) */
	/* 	      1 - controller_ack (ack) */
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   nack);

	*wdata += readl(CURR_IDC_REG(idc, IDC_DATA_OFFSET)); /* MS byte */

} /* end idc_read_one_word() */

/* Read byte data from IDC and then merge every two bytes to single word.
   The word data is placed to the array *pdata. */
static void idc_readw(u8 idc, u16 *pdata, u16 nwords)
{
	u16 nidx;

	for (nidx = 0; nidx < (nwords - 1); ++nidx ) {
		idc_read_one_word(idc, &pdata[nidx], 0);
	}

	idc_read_one_word(idc, &pdata[nidx], 1);

} /* end idc_readw () */

/* Read 1-byte data from IDC bus by polling */
static void idc_read_one_byte(u8 idc, u16 *bdata, u8 nack)
{
	/* ACK and clear interrupt*/
	/* last_byte: 0 - controller_nack (nack) */
	/* 	      1 - controller_ack (ack) */
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   nack);

	/* Read single byte */
	*bdata = (u16) readl(CURR_IDC_REG(idc, IDC_DATA_OFFSET));

} /* end idc_read_one_byte() */

/* Read byte data from IDC and place each byte data to the array *pdata. */
static void idc_readb(u8 idc, u16 *pdata, u16 nbytes)
{
	u16 nidx;

	for ( nidx = 0; nidx < (nbytes - 1); ++nidx ) {
		idc_read_one_byte(idc, &pdata[nidx], 0);
	}
	idc_read_one_byte(idc, &pdata[nidx], 1);

} /* end idc_readb () */

/* Write all of the data word by word */
static void idc_writew(u8 idc, u16 *pdata, u16 nwords)
{
	u16 nidx;

	for (nidx = 0; nidx < nwords; ++nidx ){
		/* MS data byte */
		writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET),
		       ((pdata[nidx] >> 8) & 0xff));

		/* ACK, clear interrupt and send data */
		idc_wait_interrupt_pending(idc,
					   CURR_IDC_REG(idc,
							IDC_CTRL_OFFSET),
					   0);

		/* LS data byte */
		writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET),
		       (pdata[nidx] & 0xff));

		/* ACK, clear interrupt and send data */
		idc_wait_interrupt_pending(idc,
					   CURR_IDC_REG(idc,
							IDC_CTRL_OFFSET),
					   0);
	}
} /* end idc_writew () */

/* Write all of the data byte by byte */
static void idc_writeb(u8 idc, u16 *pdata, u16 data_size)
{
	u16 nidx;

	for (nidx = 0; nidx < data_size; ++nidx) {
		/* Write single byte */
		writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET),
		       (pdata[nidx] & 0xff));

		/* ACK, clear interrupt and send data */
		idc_wait_interrupt_pending(idc,
					   CURR_IDC_REG(idc,
							IDC_CTRL_OFFSET),
					   0);
	}
} /* end idc_writeb () */

/* Read/write data over the IDC/IDC port */
static void idc_access_seq(u8 idc, u8 read_write, u8 idc_slave_type,
			   u16 addr, u16 subaddr, u16 *pdata, u16 data_size)
{
        u16 waddr;
 	/*
 	programming sequence :

        (1) Read, read_write = 1

        -> START
        -> slave address MS (7+1w)
        -> A (1)
        -> slave address LS (8) (optinal)
        -> A (1) (for 16-bit mode)
        -> slave sub-address MS (8) (optinal)
        -> A (1)
        -> slave sub-address LS (8) (for 16-bit mode) optinal
        -> A (1) (for 16-bit mode)
        -> repeat START
        -> slave address MS (7+1r)
        -> A (1)
        -> slave address LS (8) (optinal)
        -> A (1) (for 16-bit mode)
        -> {DATA(8/16) -> A(1)} ^(data_size-1)
        -> DATA(8/16)->NA(1)
        -> STOP

        (2) Write, read_write = 0

        -> START
        -> slave address MS (7+1w)
        -> A (1)
        -> slave address LS (8) (optinal)
        -> A (1) (for 16-bit mode)
        -> slave sub-address MS (8) (optinal)
        -> A (1)
        -> slave sub-address LS (8) (for 16-bit mode) (optinal)
        -> A (1) (for 16-bit mode)
        -> {DATA(8/16) -> A(1)} ^(data_size)
        -> STOP
        */

        waddr = addr - read_write;

	/* START and IDC slave address */
	if (idc_slave_type > 0x05) {
		/* Send out two bytes of slave address to read
		   under START condittion */
		idc_set_extended_addr(idc, waddr);
	} else {
		/* Send out one byte of slave address to read
		   under START condittion */
		idc_set_8bit_addr(idc, waddr);
	}

	/* Send the sub-address */
	if ((idc_slave_type == 0x04) | (idc_slave_type == 0x05) |
	    (idc_slave_type == 0x0a) | (idc_slave_type == 0x0b) ) {

		/* Send out two bytes of slave subaddress to read */
		idc_writew(idc, &subaddr, 1);

	} else if ((idc_slave_type == 0x02) | (idc_slave_type == 0x03)|
	  	   (idc_slave_type == 0x08) | (idc_slave_type == 0x09) ) {

		/* Send out one byte of slave address to read */
		idc_writeb(idc, &subaddr, 1);
	}

	/* Repeat START and IDC slave address for combined mode */
	if (read_write == 1) {
		if (idc_slave_type > 0x05) {
			/* Send out two bytes of slave address to read */
			/* under START condittion */
			idc_set_extended_addr(idc, addr);
		} else {
			/* Send out one byte of slave address to read */
			/* under START condittion */
			idc_set_8bit_addr(idc, addr);
		}
	}

	/* Read/Write data from the slave device */
	if ( idc_slave_type & 0x01 ) {
		/* read/write 16-bit data */
		if (read_write == 1) {
			idc_readw(idc, pdata, data_size);
		} else
			idc_writew(idc, pdata, data_size);

	} else {
		/* read/write 8-bit data */
		if (read_write == 1) {
			idc_readb(idc, pdata, data_size);
		} else {
			idc_writeb(idc, pdata, data_size);
		}
	}

	/* STOP */
	writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x08);

} /* idc_access_seq() */

/* Put 16-bit data into 63-byte fifo */
static void idc_put_fifo_words(u8 idc, u16 *pdata, u16 nwords)
{
	u32 nidx;

	K_ASSERT( nwords < 32 );

	for (nidx = 0; nidx < nwords; ++nidx) {
		/* MS data byte */
		writel(CURR_IDC_REG(idc, IDC_FMDATA_OFFSET),
		       (pdata[nidx] >> 8 & 0xff));
		/* LS data byte */
		writel(CURR_IDC_REG(idc, IDC_FMDATA_OFFSET),
		       (pdata[nidx] & 0xff));
	}
} /* end idc_put_fifo_words */

/* Put 8-bit data into 63-byte fifo */
static void idc_put_fifo_bytes(u8 idc, u16 *pdata, u16 nbytes)
{
	u32 nidx;

	K_ASSERT( nbytes < 63 );

	for (nidx = 0; nidx < nbytes; ++nidx)
		writel(CURR_IDC_REG(idc, IDC_FMDATA_OFFSET),
		       (pdata[nidx] & 0xff));
}

/* Burstly put 16-bit data into 63-byte fifo till the end of
   transferring last word */
static void idc_burst_writew(u8 idc, u16 *pdata, u8 fifo_fulness,
			     u16 nwords)
{
	u32 remaining_bytes;

	/* Keep a even number of 8-bit slots for write */
	remaining_bytes = (FIFO_BUF_SIZE - fifo_fulness);
	remaining_bytes -= remaining_bytes & 0x1;
	remaining_bytes >>= 1; /* 1 word = 2 bytes */

	do {
		if ( nwords  >  remaining_bytes ) {
			/* Put data to FIFO */
			idc_put_fifo_words(idc, pdata, remaining_bytes);

			/* ACK, clear interrupt, transmit data */
			writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x0);

			idc_wait_interrupt_pending(idc,
				CURR_IDC_REG(idc, IDC_FMCTRL_OFFSET),
						   0x02);

			/* Check the number of data to be sent */
			nwords -=  remaining_bytes;

			/* 31 words in space, one byte is kept
			   for 'if' command */
			remaining_bytes = (FIFO_BUF_SIZE >> 1);

		} else {
			/* Put data to FIFO */
			idc_put_fifo_words(idc, pdata, nwords);

			nwords = 0; /* All data are trasmitted */

			/* NACK, clear interrupt, stop to transmit data */
			writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x0);

			idc_wait_interrupt_pending(idc,
				CURR_IDC_REG(idc, IDC_FMCTRL_OFFSET),
						   0x0a);
		}
		/* Clear the 63-entry FIFO */
		writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x01);

	} while ( nwords > 0 );

} /* end idc_burst_writew () */

/* Burstly put 16-bit data into 63-byte fifo till the end of
   transferring last byte */
static void idc_burst_writeb(u8 idc, u16 *pdata, u8 fifo_fulness, u16 nbytes)
{
	u32 remaining_bytes = (FIFO_BUF_SIZE - fifo_fulness - 1);

	do {
		if ( nbytes  >  remaining_bytes ) {
			/* Put data to FIFO */
			idc_put_fifo_bytes(idc, pdata, remaining_bytes);

			/* Check the number of data to be sent */
			nbytes -=  remaining_bytes;

			/* ACK, clear interrupt, transmit or receive data */
			writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x0);

			idc_wait_interrupt_pending(idc,
				CURR_IDC_REG(idc, IDC_FMCTRL_OFFSET),
						   0x02);

			/* 62 bytes in space, one is left for 'if' command */
			remaining_bytes = (FIFO_BUF_SIZE - 1);

		} else {
			/* Put data to FIFO */
			idc_put_fifo_bytes(idc, pdata, nbytes);

			/* All data are trasmitted */
			nbytes = 0;

			/* Wait for the ARM interrupt */
			writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x0);

			idc_wait_interrupt_pending(idc,
				CURR_IDC_REG(idc, IDC_FMCTRL_OFFSET),
						   0x0a);
		}

		/* Clear the 63-entry FIFO */
		writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x01);

	} while ( nbytes > 0 );

} /* end idc_burst_writeb () */

/* Set the slave address under 10-bit + 1-bit r/w adressing mode */
static void idc_set_extended_addr(u8 idc, u16 addr)
{
	/* Send out the MS byte of extended slave address to read/write */
	/* 0x11110 (A9)(A8)(0) */
	writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET),
	       0xf0 | ((addr >> 8) & 0x0007));

	/* ACK, clear interrupt, transmit or receive data */
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   0x04);

	/* Send out the LS byte of extended slave address to write */
	writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET), (addr & 0xff));

	/* ACK, clear interrupt, transmit or receive data */
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   0x0);

} /* idc_set_extended_addr() */

/* Set the slave address under 7-bit + 1-bit r/w adressing mode */
static void idc_set_8bit_addr(u8 idc, u16 addr)
{
	/* Write 7-bit address + 1-bit r/w */
	writel(CURR_IDC_REG(idc, IDC_DATA_OFFSET), (addr & 0xff));

	/* ACK, clear interrupt, and send data */
	idc_wait_interrupt_pending(idc,
				   CURR_IDC_REG(idc, IDC_CTRL_OFFSET),
				   0x04);

} /* idc_set_8bit_addr() */

inline int idc_read_only_seq(u8 idc, u8 idc_slave_type, u16 addr,
			     u16 *pdata, u16 data_size)
{
        if (idc_slave_type > 0x05) {
		/* Send out two bytes of slave address to read
		   under START condittion */
		idc_set_extended_addr(idc, addr);
	} else {
		/* Send out one byte of slave address to read
		   under START condittion */
		idc_set_8bit_addr(idc, addr);
	}

	/* Read/Write data from the slave device */
	if ( idc_slave_type & 0x01 ) {
		/* read16-bit data */
		idc_readw(idc, pdata, data_size);
	} else {
		/* read 8-bit data */
		idc_readb(idc, pdata, data_size);
	}

	/* STOP */
	writel(CURR_IDC_REG(idc, IDC_CTRL_OFFSET), 0x08);

	return 0;
}

/**
 * IDC/I2C read only
 */
void idc_master_direct_read(u8 idc, u16 idc_slave_type, u16 addr, u16 *pdata,
			u16 data_size)
{
	u8 idc_ctrl = idc_port_map_to_ctrl(idc);
	K_ASSERT(idc_ctrl < IDC_INSTANCES);

	/* set idc pin mux */
	set_idc_pin_mux(idc);

	idc_read_only_seq(idc_ctrl, idc_slave_type, addr, pdata, data_size);


} /* end idc_master_direct_read() */

#ifdef BOARD_SUPPORT_I2C_PMIC_TPS6586XX

#define tps6586xx_I1_SLAVE_ADDR_WR 		0x68
#define tps6586xx_I1_SLAVE_ADDR_RD 		0x69
void idc_wr_tps6586xx(u16 regaddr, u16 pdata)
{
#if (CHIP_REV == I1)
	idc_write(IDC2, tps6586xx_I1_SLAVE_ADDR_WR, regaddr, &pdata, 1);
#endif
}

void idc_rd_tps6586xx(u16 regaddr, u16 *pdata)
{
#if (CHIP_REV == I1)
    idc_read(IDC2, tps6586xx_I1_SLAVE_ADDR_RD, regaddr, pdata, 1);
#endif
}

#endif



