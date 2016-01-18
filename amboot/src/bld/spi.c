/**
 * system/src/peripheral/spi.c
 *
 * History:
 *    2005/03/28 - [Eric Lee] created file
 *    2006/03/02 - [Allen Wang] changed to the IRQ interrupt
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <basedef.h>
#include <ambhw.h>
#include <bldfunc.h>
#include <peripheral/spi.h>
#define CURR_SPI_REG(spi_inst, offset)	spi_get_reg_addr((spi_inst), (offset))
#define DATA_FIFO_SIZE(spi_inst)	get_spi_fifo_size(spi_inst)

/* Enable SSI 2 and 3 */
#define ENA_SSI_EN2_EN3		0x00000002

/* Config SSI_CLK */
#define SPI_RX_TIMEOUT		0xffffffff	/* Number of FIFO entries */

#define POLLING_TIMEOUT		0x10000000

#define DATA_FIFO_SIZE_16		0x10
#define DATA_FIFO_SIZE_32		0x20
#define DATA_FIFO_SIZE_128		0x80


typedef struct spi_ctrl_info_s {
#if (SPI_SUPPORT_TSSI_MODE)
        u8	tssi_mode;	/* TSSI modes */
#endif
        u8      op_mode;        /* SPI modes */
	u8	scpol;          /* serial clock polarity */
	u8 	scph;           /* serial clock phase */
	u8 	frf;            /* frame formats */
	u8	dfs;            /* Data frame size */
	u8 	cfs;		/* Control frame size,
				   A2 valid */
	u16	sckdv;          /* SSI clock divider */
	u32	baudrate;	/* master baudrate */

	u8	mhs;		/* Microwire handsaking */
	u8 	mdd;		/* Microwire control, data
				   direction */
	u8 	mwmod;		/* Microwire transfer mode */
} spi_ctrl_info_t;


static spi_ctrl_info_t spi_ctrl_info[SPI_INSTANCES][SPI_MAX_SLAVE_ID + 1];

#if (SPI_SUPPORT_TISSP_NSM == 1)
/* Mapping of GPIO pins and SPI IDs */
static u8 spi_gpio_map[8] = {
	SSI0_EN0, SSI0_EN1, SSIO_EN2, SSIO_EN3,
	SSI_4_N,  SSI_5_N,  SSI_6_N,  SSI_7_N
};

/* Mapping of frame formats and spi modes */
static u8 spi_frm_format[9] = {
	FRF_MOTO, FRF_MOTO, FRF_MOTO, FRF_MOTO, 	/* Motorola mode */
	FRF_TI,						/* TI mode */
	FRF_NSM, FRF_NSM, FRF_NSM, FRF_NSM		/* NS microwave mode */
};

# if (SPI_INSTANCES > 2)
static u8 spi3_gpio_map[8] = {
	SSI3_EN0, SSI3_EN1, SSI3_EN2, SSI3_EN3,
	SSI3_EN4, SSI3_EN5, SSI3_EN6, SSI3_EN7
};
#endif

#else

static u8 spi_gpio_map[4] = {
	SSI0_EN0, SSI0_EN1, SSIO_EN2, SSIO_EN3
};

static u8 spi_frm_format[4] = {
	FRF_MOTO, FRF_MOTO, FRF_MOTO, FRF_MOTO 	       /* Motorola mode */
};

#endif

static void set_spi_baud_rate(u8 spi_inst, u8 spi_id, u32 baud_rate);

/**
 * Select SPI Master/Slave BASE, than return correct address
 */
static u32 spi_get_reg_addr(u8 spi_inst, u32 offset)
{
	switch (spi_inst) {
		case SPI_MASTER1:
			return SPI_REG(offset);
#if (SPI_INSTANCES > 1)
		case SPI_MASTER2:
			return SPI2_REG(offset);
#endif
# if (SPI_INSTANCES > 2)
		case SPI_MASTER3:
			return SPI3_REG(offset);
#endif
	}

	return 0xffffffff;
}

static u16 get_spi_fifo_size(u8 spi_inst)
{
#if(CHIP_REV == I1)
	switch(spi_inst) {
		case SPI_MASTER1:
		case SPI_MASTER3:
		case SPI_SLAVE1:
			return DATA_FIFO_SIZE_128;
		case SPI_MASTER2:
		case SPI_MASTER4:
			return DATA_FIFO_SIZE_32;
	}
	return 0xffff;
#else
	return DATA_FIFO_SIZE_16;
#endif
}

/**
 * Enable SPI slave with SPI_ID
 */
static void spi_ena(u8 spi_inst, u8 id, u8 spi_mode)
{
	/* enable slave device with spi_id to communicate */
	if (spi_inst == SPI_MASTER1)
		writel(CURR_SPI_REG(spi_inst, SPI_SER_OFFSET), (0x01 << id));
#if (SPI_INSTANCES > 1)
	else if (spi_inst == SPI_MASTER2) {
		if (spi_ctrl_info[spi_inst][id].tssi_mode == 0)
			writel(CURR_SPI_REG(spi_inst, SPI_SER_OFFSET),
				(0x01 << id));
		else {
			/* enable tssi mode */
			writel(TSSI_CTRL_REG,
				spi_ctrl_info[spi_inst][id].tssi_mode);
		}
	}
#endif
# if (SPI_INSTANCES > 2)
	else if (spi_inst == SPI_MASTER3) {
		writel(CURR_SPI_REG(spi_inst, SPI_SER_OFFSET), (0x01 << id));
	}
#endif
}

/**
 * This function is polling SSI to not busy
 */
static void polling_spi_status(u8 spi_inst)
{
	u32 reg = 1;
	u32 fifo_frame = 0;
	int x;

	/* waiting fifo empty */
	/* check busy bit of status register */
	for (x = 0; x < POLLING_TIMEOUT; x++) {

		fifo_frame = readl(CURR_SPI_REG(spi_inst, SPI_TXFLR_OFFSET));
		reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET)) &
				SPI_SR_BUSY;

		if ((fifo_frame == 0) && (reg == 0))
			break;

	}

	if (reg == 0x1) {
		for (;;);
	}

	if (fifo_frame != 0x0) {
	}
}

/**
 * This function is to wait for SSI to be not busy and disable it
 *
 * @returns - 0 no errors, -1 errors occurred
 */
static int spi_dis(u8 spi_inst, u32 mask)
{
        u32 reg = 0, reg1 = 0;

        /* wait for SSI to be not busy and disable it */
	polling_spi_status(spi_inst);

#if (SPI_INSTANCES > 1)
	if (spi_ctrl_info[spi_inst][0].tssi_mode != 0) {
		/* reset tssi mode */
		spi_ctrl_info[spi_inst][0].tssi_mode = 0;
		writel(TSSI_CTRL_REG, 0);
	}
#endif
	/* double check if any errors occured */
	reg1 = readl(CURR_SPI_REG(spi_inst, SPI_ISR_OFFSET)) & mask;
	reg = readl(CURR_SPI_REG(spi_inst, SPI_ICR_OFFSET));

	/* disabled SPI */
	if ((spi_inst & SPIS_SHIFT_BASE) == 0)
		writel(CURR_SPI_REG(spi_inst, SPI_SER_OFFSET), 0);

	writel(CURR_SPI_REG(spi_inst, SPI_SSIENR_OFFSET), 0);

	return reg1;
}

/**
 * SPI CTRL0 register mux
 */
static u32 spi_ctrl_0_mux(u8 spi_inst, u8 spi_id, u8 tx_rx_mode)
{
	u32 reg = 0;

	if ((spi_inst & SPIS_SHIFT_BASE) == 0) { /*Master */
		reg = ( (spi_ctrl_info[spi_inst][spi_id].cfs << 12)	|
			(tx_rx_mode << 8)				|
			(spi_ctrl_info[spi_inst][spi_id].scpol << 7)	|
			(spi_ctrl_info[spi_inst][spi_id].scph << 6)	|
			(spi_ctrl_info[spi_inst][spi_id].frf << 4)	|
			(spi_ctrl_info[spi_inst][spi_id].dfs)
		      );
	}

	return reg;
}

/**
 * SPI Microwire Ctrl register mux
 */
static u32 spi_mwcr_mux(u8 spi_inst, u8 spi_id, u8 tx_rx_mode)
{
#if (SPI_SUPPORT_TISSP_NSM == 1)

	u32 reg;

	/* National Semi. Microwire */
	if ((spi_inst & SPIS_SHIFT_BASE) == 0) {/* Master */

		if (tx_rx_mode == SPI_WRITE_ONLY)
			spi_ctrl_info[spi_inst][spi_id].mdd = 1;
		else { /* tx_rx_mode == SPI_READ_ONLY */
			spi_ctrl_info[spi_inst][spi_id].mdd = 0;
		}
		/* MMCR */
		reg = ( (spi_ctrl_info[spi_inst][spi_id].mhs << 2)	|
			(spi_ctrl_info[spi_inst][spi_id].mdd << 1)	|
			(spi_ctrl_info[spi_inst][spi_id].mwmod)
		      );
	}

	return reg;
#else
	return 0;
#endif
}

/**
 * Initial and set SPI interface with the device configuration
 */
static void spi_set_reg(u8 spi_inst, u8 spi_id, u8 tx_rx_mode, u8 txftlr,
			u8 rxftlr, u8 int_mask)
{
        u32 reg = 0;

	/* wait for SPI isn't busy */
	polling_spi_status(spi_inst);

	/* disable SPI*/
	writel(CURR_SPI_REG(spi_inst, SPI_SSIENR_OFFSET), 0);

	/* the SPI is always of the master mode */
	reg = spi_ctrl_0_mux(spi_inst, spi_id, tx_rx_mode);
	writel(CURR_SPI_REG(spi_inst, SPI_CTRLR0_OFFSET), reg);

#if (SPI_SUPPORT_TISSP_NSM == 1)
	/* National Semi. Microwire */
	if ((spi_inst & SPIS_SHIFT_BASE) == 0) {/* Master */
		if ((spi_ctrl_info[spi_inst][spi_id].op_mode >= NSM_MODE0) &&
	            (spi_ctrl_info[spi_inst][spi_id].op_mode <= NSM_MODE3)) {

			reg = spi_mwcr_mux(spi_inst, spi_id, tx_rx_mode);
			writel(CURR_SPI_REG(spi_inst, SPI_MWCR_OFFSET), reg);
		}
	}
#endif

	/* BAUDR : baud rate select */
	if ((spi_inst & SPIS_SHIFT_BASE) == 0) { /* Master */
		writel(CURR_SPI_REG(spi_inst, SPI_BAUDR_OFFSET),
					spi_ctrl_info[spi_inst][spi_id].sckdv);
	}
	/* TXFTLR : transmit FIFO threshold */
	writel(CURR_SPI_REG(spi_inst, SPI_TXFTLR_OFFSET), txftlr);

	/* RXFTLR : receive FIFO threshold */
	writel(CURR_SPI_REG(spi_inst, SPI_RXFTLR_OFFSET), rxftlr);

	/* Only mask txe interrupt */
	writel(CURR_SPI_REG(spi_inst, SPI_IMR_OFFSET), int_mask);

	/* enable the SSI */
	writel(CURR_SPI_REG(spi_inst, SPI_SSIENR_OFFSET), 1);

}

/**
 * SPI slave interrupt handler
 */
void spi_slave_handler(void *arg)
{
}


/**
 * This function is called by peripheral_init() for initialization only
 */
void spi_init(void)
{
}

/**
 * Fill the TX FIFO up with the data to be sent prior to the starting of the
 * SPI TX/RX
 */
static void spi_fill_tx_fifo(u8 spi_inst, u16 w_size, u16 r_size, u16 *buf
			   , u32 *widx)
{
        u32 preload = DATA_FIFO_SIZE(spi_inst); /* fill the FIFO first */
	u32 postload = DATA_FIFO_SIZE(spi_inst);

	if (w_size > r_size) {
	        if (w_size < preload) {
	                preload = w_size;
	                postload = w_size;
	        }
        } else {
                if (w_size < preload) {
	                preload = w_size;
	                if (r_size < DATA_FIFO_SIZE(spi_inst))
	                        postload = r_size;
	        }
        }

        while ((*widx) < preload) {

		writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), buf[*widx]);
		(*widx)++;
        }

	while ((*widx) < postload ) {
		writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), SPI_DUMMY_DATA);
		(*widx)++;
	}
}

/**
 * set SPI baud rate register
 */
static void set_spi_baud_rate(u8 spi_inst, u8 spi_id, u32 baud_rate)
{
	u32 amba_freq = 0;
	u16 sckdv = 0;
	switch (spi_inst) {
		case SPI_MASTER1:
			amba_freq = get_ssi_freq_hz();
			break;
#if (SPI_INSTANCES > 1)
		case SPI_MASTER2:
			amba_freq = get_ssi2_freq_hz();
			break;
#endif
# if (SPI_INSTANCES > 2)
		case SPI_MASTER3:
			amba_freq = get_ssi_freq_hz();
			break;
#endif
#if (SPI_SLAVE_INSTANCES >= 1)
		case SPI_SLAVE1:
			amba_freq = get_ssi2_freq_hz();
			break;
#endif
		default:
			break;
	}

	if (baud_rate >= 0xffffffff) /*Disable clock output */
		sckdv = 0;
	else
		sckdv = (u16)(((amba_freq / baud_rate) + 0x01) & 0xfffe);

	if (sckdv > 65534)
		sckdv = 65534;

	/* SCKDV shall be an even number */
	if ((spi_inst & SPIS_SHIFT_BASE) == 0) /* Master */
		spi_ctrl_info[spi_inst][spi_id].sckdv = sckdv;
}

/**
 * set SPI pin mux
 */
static void set_spi_pin_mux(u8 spi_inst, u8 spi_id)
{
	switch (spi_inst) {
		case SPI_MASTER1:
			/* To enable the GPIO PIN as HW_FUNC */
			gpio_config_hw(spi_gpio_map[spi_id]);

#if (SPI_EN2_EN3_ENABLED_BY_HOST_ENA_REG == 1)
			/* To enable SSI_EN2 and SSI_EN3 */
			if ((spi_id == 2) || (spi_id == 3)) {
				writel(HOST_ENABLE_REG,
			               readl(HOST_ENABLE_REG) |
					     ENA_SSI_EN2_EN3);
#if (SPI_SUPPORT_TISSP_NSM == 1)
			} else if ((spi_id >= 4) && (spi_id <= 7)) {
				writel(GPIO2_AFSEL_REG,
		      		       readl(GPIO2_AFSEL_REG) &
					     (~(0x1 << (18 + spi_id - 4))));
#endif
			}
#endif

#if (SPI_EN2_ENABLED_BY_GPIO2_AFSEL_REG == 1)
			/* To enable SSI_EN2 */
			if (spi_id == 2) {
				writel(GPIO2_AFSEL_REG,
				       readl(GPIO2_AFSEL_REG) | (0x1 << 22));
#if (SPI_SUPPORT_TISSP_NSM == 1)
			} else if ((spi_id >= 4) && (spi_id <= 7)) {
				writel(GPIO2_AFSEL_REG,
				       readl(GPIO2_AFSEL_REG) &
					     (~(0x1 << (18 + spi_id - 4))));
#endif
			}
#endif

#if (SPI_EN4_7_ENABLED_BY_GPIO1_AFSEL_REG == 1)
			if ((spi_id >= 4) && (spi_id <= 7)) {
				gpio_config_hw(GPIO(63));
			}
#endif
			break;
#if (SPI_INSTANCES > 1)
		case SPI_MASTER2:
			/* To enable the GPIO PIN as HW_FUNC */
			gpio_config_hw(SSI2CLK);
		        gpio_config_hw(SSI2MOSI);
		        gpio_config_hw(SSI2MISO);
		        gpio_config_hw(SSI2_0EN);
			break;
#endif
# if (SPI_INSTANCES > 2)
		case SPI_MASTER3:
			/* To enable the GPIO PIN as HW_FUNC */
			gpio_config_hw(SSI3_CLK);
		        gpio_config_hw(SSI3_MOSI);
		        gpio_config_hw(SSI3_MISO);
			gpio_config_hw(spi3_gpio_map[spi_id]);
			break;
#endif
		default:
			break;
	}

}

/**
 * A1 supports 4 SPI slave devics and A2 supports 8 SPI slave devics
 */
/**
 * Configure the specific Serial Peripheral Interface (SPI) module to different
 * operation modes
 */
void spi_master_config(u8 spi_inst, u8 spi_id, u8 spi_mode, u8 cfs_dfs,
		       u32 baud_rate)
{

#if (SPI_SUPPORT_ENA_PIN_REVERSIBLE_POLARITY == 1)
	u32 data;
#endif
#if (SPI_SUPPORT_TSSI_MODE)
	u32 tssi_mode;
#endif

#if (SPI_SUPPORT_ENA_PIN_REVERSIBLE_POLARITY == 1)
	/*config SSI0_EN0 and SSI0_EN1 default high */
	data = readl(TSSI_POLARITY_INVERT_REG);
	writel(TSSI_POLARITY_INVERT_REG, data & 0xfffffffc);
#endif

#if (SPI_SUPPORT_TSSI_MODE)
	tssi_mode = (spi_mode & 0xf0) >> 4;
	spi_mode &= 0xf;
#endif

#if ((SPI_SUPPORT_TSSI_MODE) && (SPI_INSTANCES > 1))
        spi_ctrl_info[spi_inst][spi_id].tssi_mode	= tssi_mode;
#endif
        spi_ctrl_info[spi_inst][spi_id].op_mode 	= spi_mode;

	spi_ctrl_info[spi_inst][spi_id].baudrate	= baud_rate;

	/* The following two fields are only valid for Motorola mode  */
	spi_ctrl_info[spi_inst][spi_id].scpol 	= (spi_mode >> 1) & 0x01;
	spi_ctrl_info[spi_inst][spi_id].scph 	= (spi_mode & 0x01);

	spi_ctrl_info[spi_inst][spi_id].cfs 	= cfs_dfs >> 4;/*A2 valid*/
	spi_ctrl_info[spi_inst][spi_id].dfs 	= cfs_dfs & 0x0f;
	spi_ctrl_info[spi_inst][spi_id].frf 	= spi_frm_format[spi_mode];

#if ((SPI_SUPPORT_TISSP_NSM == 1) || (SPI_INSTANCES > 1))
	if (spi_mode >= NSM_MODE0 && spi_mode <= NSM_MODE3) {
		spi_ctrl_info[spi_inst][spi_id].mhs 	=
					((spi_mode - NSM_MODE0) >> 1);
		spi_ctrl_info[spi_inst][spi_id].mwmod 	=
					((spi_mode - NSM_MODE0) & 0x01);
	}
#endif

	/* Both SPIs shall use the same settings */
	set_spi_baud_rate(spi_inst, spi_id, baud_rate);

	set_spi_pin_mux(spi_inst, spi_id);

}


/**
 * Write n_size number of data frames (buffer) continuously into slave device
 * with ID 'spi_id'
 */
static int spi_write_moto_ti(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size)
{
        u32 idx = 0;
	u32 reg = 0, reg1 = 0;
	u8 txftlr = DATA_FIFO_SIZE(spi_inst) - 1;
	u8 rxftlr = DATA_FIFO_SIZE(spi_inst) / 2 - 1;

	/* Temporary */
	if (n_size < (txftlr + 1))
		txftlr = n_size - 1;

	spi_set_reg(
	 	    spi_inst,
		    spi_id,
		    SPI_WRITE_ONLY,
		    txftlr,
		    rxftlr,
		    (SPI_TXEIS_MASK | SPI_TXOIS_MASK));

	/* fill the FIFO first */
	while ((idx < DATA_FIFO_SIZE(spi_inst)) && (idx < n_size)) {

		writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), buffer[idx]);
	        idx++;
	}

        /* Enable SPI slave device */
        spi_ena(spi_inst, spi_id, SPI_WRITE_ONLY);

	if (n_size > DATA_FIFO_SIZE(spi_inst)) {
	        while (idx < n_size) {

			reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

	                if (reg & SPI_SR_TX_FIFO_N_FULL) {

				writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
					buffer[idx]);
                  		idx++;
                  	}
                }
	} /* if n_size > DATA_FIFO_SIZE */

        /* Disable the SPI TX */
        reg1 = spi_dis(spi_inst, SPI_TXOIS_MASK);

        if (reg1)
        	return -1;	/* Failed */
        else
		return (0); 	/* Succeeded */
}

/**
 * Read nbuf number of data frames (buffer) continuously from slave device
 * with ID 'spi_id'
 */
static int spi_read_moto_ti(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size)
{
	u32 idx, retry;
	u32 reg = 0, reg1 = 0;
	u8 txftlr = DATA_FIFO_SIZE(spi_inst) - 1;
	u8 rxftlr = 0x0;

	/* CTRL1 : number of data frame = NDF + 1 */
	writel(CURR_SPI_REG(spi_inst, SPI_CTRLR1_OFFSET), (n_size - 1));

	spi_set_reg(
	 	    spi_inst,
		    spi_id,
		    SPI_READ_ONLY,
		    txftlr,
		    rxftlr,
		    SPI_RX_MASK);

	/* write a dummy word to begin the transfer */
	writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), SPI_DUMMY_DATA);

	/* get data */
	idx = 0;

        /* Enable SPI slave device */
        spi_ena(spi_inst, spi_id, SPI_READ_ONLY);

        /* Read the data in RX FIFO */
        retry = 0;

        while (idx < n_size) {

		reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

	        if (reg & SPI_SR_RX_FIFO_N_EMPTY) {
			if (readl(CURR_SPI_REG(spi_inst, SPI_RXFLR_OFFSET))) {
				buffer[idx] = readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
				idx++;
				retry = 0;
			}
                } else {
                	retry ++;
                	if (retry >= SPI_RX_TIMEOUT)
                	        break;
                }

	}

        /* Disable the SPI RX */
        reg1 = spi_dis(spi_inst, SPI_RX_MASK);

	if (reg1 || (retry == SPI_RX_TIMEOUT) )
        	return -1;	/* Failed */
        else
		return (0); 	/* Succeeded */
}

/**
 * Perform writing of w_size frame to the external serial device,
 * along with reading of r_size frame of data received from the
 * the same serial device
 */
static int spi_write_read_moto_ti(u8 spi_inst, u8 spi_id, u16 *w_buffer,
				  u16 *r_buffer, u16 w_size, u16 r_size)
{
	u32 widx, ridx, retry = 0;
	u32 reg = 0, reg1 = 0, x;
	u8 txftlr = DATA_FIFO_SIZE(spi_inst) - 1;
	u8 rxftlr = 0x00;
        u32 spi_duration;

	spi_set_reg(
	 	    spi_inst,
		    spi_id,
		    SPI_WRITE_READ,
		    txftlr,
		    rxftlr,
		    (SPI_TXEIS_MASK | SPI_TXOIS_MASK));

        ridx = 0;
	widx = 0;

	/* Find the duration of SPI TX and RX */
	if (w_size >= r_size)
                spi_duration = w_size;
        else
                spi_duration = r_size;

        /* Fill the TX FIFO up with the data to be sent */
        spi_fill_tx_fifo(spi_inst, w_size, r_size, w_buffer, &widx);

        /* Enable SPI slave device */
        spi_ena(spi_inst, spi_id, SPI_WRITE_READ);

	if (w_size > DATA_FIFO_SIZE(spi_inst)) {
	  	while (widx < w_size) {
		        reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

		        if (reg & SPI_SR_TX_FIFO_N_FULL) {
		                writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
					w_buffer[widx]);
		                widx++;
		                if (ridx < r_size) {
		                	if (readl(CURR_SPI_REG(
							spi_inst,
							SPI_RXFLR_OFFSET))) {
  			                	r_buffer[ridx]=
							readl(CURR_SPI_REG(
								spi_inst,
								SPI_DR_OFFSET));
	  			                ridx++;
					}
				} else {
					/* Consume the dummy data */
					reg1 = readl( CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
	  			}
	  		}
		}
        }

        while (widx < spi_duration) {

		reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

		if (reg & SPI_SR_TX_FIFO_N_FULL) {
			writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
					SPI_DUMMY_DATA);
		        widx++;

		        if (ridx < r_size) {
			        if (readl(CURR_SPI_REG(spi_inst,
							SPI_RXFLR_OFFSET))) {
	  			       	r_buffer[ridx] =
						readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
	  			        	ridx++;
	  			}
  			} else {
  			        /* Consume the dummy data */
				reg1 = readl(CURR_SPI_REG(spi_inst,
							  SPI_DR_OFFSET));
  			}
  	        }
        }

        /* Read the remaining data in RX FIFO */
	while (ridx < r_size) {

		reg = readl(CURR_SPI_REG(spi_inst, SPI_RXFLR_OFFSET));

		if (reg) {
			for (x = 0; x < reg; x++)
				r_buffer[ridx++] = readl(CURR_SPI_REG(spi_inst,
							SPI_DR_OFFSET));
			retry = 0;
                } else {
                        retry++;
                	if (retry >= SPI_RX_TIMEOUT) {
                	        break;
                        }
                }
        }

        /* Disable the SPI TX/RX */
	reg1 = spi_dis(spi_inst, SPI_TXOIS_MASK);

	if (reg1)
        	return -1;	/* Failed */
        else
		return (0); 	/* Succeeded */

} /* end spi_write_read */

/**
 * Write n_size number of data frames (buffer) continuously into slave device
 * with ID 'spi_id'
 */
static int spi_write_nsm(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size)
{
        u32 idx;
	u32 reg = 0, reg1 = 0;
	u8 txftlr = DATA_FIFO_SIZE(spi_inst) - 1;
	u8 rxftlr = DATA_FIFO_SIZE(spi_inst) / 2 - 1;

	/* Temporary */
	if (n_size < (txftlr + 1))
		txftlr = n_size - 1;

	spi_set_reg(
		    spi_inst,
		    spi_id,
		    SPI_WRITE_ONLY,
		    txftlr,
		    rxftlr,
		    (SPI_TXEIS_MASK | SPI_TXOIS_MASK));

	/* fill the FIFO first */
	idx = 0;
	while ((idx < DATA_FIFO_SIZE(spi_inst)) && (idx < n_size)) {

		writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), buffer[idx]);
	        idx++;
	}

        /* Enable SPI slave device */
        spi_ena(spi_inst, spi_id, SPI_WRITE_ONLY);

	if (n_size > DATA_FIFO_SIZE(spi_inst)) {
	        while (idx < n_size) {

			reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

	                if (reg & SPI_SR_TX_FIFO_N_FULL) {
				writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
					buffer[idx]);
                  		idx++;
                  	}
                }
	} /* if n_size > DATA_FIFO_SIZE */

        /* Disable the SPI TX */
        reg1 = spi_dis(spi_inst, SPI_TXOIS_MASK);

        if (reg1)
        	return -1;	/* Failed */
        else
		return (0); 	/* Succeeded */
}

/**
 * Read nbuf number of data frames (buffer) continuously from slave device
 * with ID 'spi_id'
 */
static int spi_read_nsm(u8 spi_inst, u8 spi_id, u16 *w_buffer, u16 *r_buffer,
			u16 r_size)
{
        u16 w_size = 0x0;
	u32 widx, ridx, retry = 0;
	u32 reg = 0, reg1 = 0;
	u8 txftlr = DATA_FIFO_SIZE(spi_inst) - 1;
	u8 rxftlr = 0x00;
        u32 spi_duration;

        /* CTRL1 : number of data frame = NDF + 1 */
	writel(CURR_SPI_REG(spi_inst, SPI_CTRLR1_OFFSET), (r_size - 1));

	spi_set_reg(spi_inst, spi_id, SPI_READ_ONLY, txftlr, rxftlr,
			    (SPI_TXEIS_MASK | SPI_TXOIS_MASK));

	if (spi_ctrl_info[spi_inst][spi_id].mwmod) {
		w_size = 1;		/* Contiuous sequential mode */
	} else {
		w_size = r_size;	/* Non-sequential mode */
	}

        /* Find the duration of SPI TX and RX */
        spi_duration = w_size;

        /* Fill the TX FIFO up with the data to be sent */
	widx = 0;
	ridx = 0;
	while ((widx < DATA_FIFO_SIZE(spi_inst)) && (widx < w_size)) {
		writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET), w_buffer[widx]);
	        widx++;
	}

        /* Enable SPI slave device */
        spi_ena(spi_inst, spi_id, SPI_READ_ONLY);

	if (w_size > DATA_FIFO_SIZE(spi_inst)) {

	  	while (widx < w_size) {
			reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));

			if (reg & SPI_SR_TX_FIFO_N_FULL) {
				writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
					w_buffer[widx]);
				widx++;
				if (ridx < r_size) {
					if (readl(CURR_SPI_REG(spi_inst,
							SPI_RXFLR_OFFSET))) {
	  					r_buffer[ridx] = readl(
							CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
						ridx++;
					}
				} else {
					/* Consume the dummy data */
					reg1 = readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
				}
			}
		}
	}

        while (widx < spi_duration) {
		reg = readl(CURR_SPI_REG(spi_inst, SPI_SR_OFFSET));
		if (reg & SPI_SR_TX_FIFO_N_FULL) {
			writel(CURR_SPI_REG(spi_inst, SPI_DR_OFFSET),
						SPI_DUMMY_DATA);
		        widx++;
		        if (ridx < r_size) {
				if (readl(CURR_SPI_REG(spi_inst,
							SPI_RXFLR_OFFSET))) {
	  				r_buffer[ridx] =
						readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
	  				ridx++;
	  			}
  			} else {
  				/* Consume the dummy data */
				reg1 = readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
  			}
  	        }
        }

        /* Read the remaining data in RX FIFO */
	while (ridx < r_size) {
		if (readl(CURR_SPI_REG(spi_inst, SPI_RXFLR_OFFSET))) {
			r_buffer[ridx] = readl(CURR_SPI_REG(spi_inst,
								SPI_DR_OFFSET));
			ridx++;
		} else {
			retry++;
			if (retry >= SPI_RX_TIMEOUT) {
				break;
			}
		}
	}

        /* Disable the SPI TX/RX */
	reg1 = spi_dis(spi_inst, SPI_TXOIS_MASK);

	if (reg1)
        	return -1;	/* Failed */
        else
		return (0); 	/* Succeeded */
}

/**
 * Write n_size number of data frames (buffer) continuously into slave device
 * with ID 'spi_id'
 */
int spi_master_write(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size)
{
	if (spi_ctrl_info[spi_inst][spi_id].frf <= FRF_TI) {
		return (spi_write_moto_ti (spi_inst, spi_id, buffer, n_size));
	} else {
		return (spi_write_nsm (spi_inst, spi_id, buffer, n_size));
	}
}

/**
 * Read nbuf number of data frames (buffer) continuously from slave device
 * with ID 'spi_id'
 */
int spi_master_read(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size)
{
	if (spi_ctrl_info[spi_inst][spi_id].frf <= FRF_TI) {
		return (spi_read_moto_ti (spi_inst, spi_id, buffer, n_size));
	} else {
		return (spi_read_nsm (spi_inst, spi_id, buffer, buffer,
				      n_size));
	}
}

/**
 * Perform writing of w_size frame to the external serial device,
 * along with reading of r_size frame of data received from the
 * the same serial device
 */
int spi_master_write_read(u8 spi_inst, u8 spi_id, u16 *w_buffer, u16 *r_buffer,
			u16 w_size, u16 r_size)
{
	int val = -1;


	if (spi_ctrl_info[spi_inst][spi_id].frf <= FRF_TI) {
		val = spi_write_read_moto_ti (spi_inst, spi_id, w_buffer,
						r_buffer, w_size, r_size);
	}

	return val;
}

/**
 * Configure the polarity of spi enable line at high or low
 */
void spi_config_ena_pin_polarity(u8 spi_inst, u8 spi_id, int cs)
{
#if (SPI_SUPPORT_ENA_PIN_REVERSIBLE_POLARITY == 1)
	u32 data;

	switch(spi_inst) {
		case SPI_MASTER1:
			data = readl(TSSI_POLARITY_INVERT_REG);
			if (cs == SPI_CS_POL_HIGH) {
				//set id default high
				data &= (~(1 << spi_id));
			} else if (cs == SPI_CS_POL_LOW) {
				//set id default low
				data |= (1 << spi_id);
			writel(TSSI_POLARITY_INVERT_REG, data);
			}
			break;
	}
#endif
}

#if defined(CONFIG_BSP_JIG) || defined(CONFIG_BSP_WOW3)

/**
 * Send the semaphore ID to IFP
 */
int spi_get_semaphore_id(void)
{
	return G_spi[0];
}

/**
 * A function to write and read simultaneously for IFP
 */
int ifp_spi_write_read(u8 spi_id,
	               u16 *w_buffer,
	               u16 *r_buffer,
	               u16 w_size,
	               u16 r_size)
{
        return (spi_write_read_moto_ti(SPI_MASTER1, spi_id, w_buffer, r_buffer,
				       w_size, r_size));
}

#endif

#if defined(BOARD_SUPPORT_SPI_PMIC_WM831X)

#ifndef SPI_PMIC_BUS_ID
#define SPI_PMIC_BUS_ID				(SPI_MASTER3)
#endif

#ifndef SPI_PMIC_CS_ID
#define SPI_PMIC_CS_ID				(3)
#endif

#ifndef SPI_PMIC_CS_GPIO
#define SPI_PMIC_CS_GPIO			(SSI3_EN3)
#endif

#ifndef SPI_PMIC_SPI_MODE
#define SPI_PMIC_SPI_MODE			(SPI_MODE0)
#endif

#ifndef SPI_PMIC_CFS_DFS
#define SPI_PMIC_CFS_DFS			(15)
#endif

#ifndef SPI_PMIC_BAUD_RATE
#define SPI_PMIC_BAUD_RATE			(500000)
#endif

void spi_pmic_wm831x_config(void)
{
	spi_master_config(SPI_PMIC_BUS_ID, SPI_PMIC_CS_ID,
		SPI_PMIC_SPI_MODE, SPI_PMIC_CFS_DFS,
		SPI_PMIC_BAUD_RATE);
	gpio_config_sw_out(SPI_PMIC_CS_GPIO);
	gpio_set(SPI_PMIC_CS_GPIO);
}

int spi_pmic_wm831x_reg_read(u16 reg_addr, u16 *reg_value)
{
	u32					reg = 0;
	u32					ridx = 0;
	int					ret = 0;

	gpio_clr(SPI_PMIC_CS_GPIO);

	spi_set_reg(SPI_PMIC_BUS_ID, SPI_PMIC_CS_ID, SPI_WRITE_READ,
		(DATA_FIFO_SIZE(SPI_PMIC_BUS_ID) - 1), 0x00,
		(SPI_TXEIS_MASK | SPI_TXOIS_MASK));

	writel(CURR_SPI_REG(SPI_PMIC_BUS_ID, SPI_DR_OFFSET),
		(reg_addr | 0x8000));
	writel(CURR_SPI_REG(SPI_PMIC_BUS_ID, SPI_DR_OFFSET),
		SPI_DUMMY_DATA);

	spi_ena(SPI_PMIC_BUS_ID, SPI_PMIC_CS_ID, SPI_WRITE_READ);
	while (ridx < 2) {
		reg = readl(CURR_SPI_REG(SPI_PMIC_BUS_ID,
			SPI_RXFLR_OFFSET));
		if (reg) {
			reg = readl(CURR_SPI_REG(SPI_PMIC_BUS_ID,
				SPI_DR_OFFSET));
			if (ridx == 1)
				*reg_value = reg;
			ridx++;
		}
	}
	reg = spi_dis(SPI_PMIC_BUS_ID, SPI_TXOIS_MASK);
	if (reg)
		ret = -1;

	gpio_set(SPI_PMIC_CS_GPIO);

	return ret;
}

int spi_pmic_wm831x_reg_write(u16 reg_addr, u16 reg_value)
{
	u32					reg = 0;
	int					ret = 0;

	gpio_clr(SPI_PMIC_CS_GPIO);

	spi_set_reg(SPI_PMIC_BUS_ID, SPI_PMIC_CS_ID, SPI_WRITE_ONLY,
		(DATA_FIFO_SIZE(SPI_PMIC_BUS_ID) - 1),
		(DATA_FIFO_SIZE(SPI_PMIC_BUS_ID) / 2 - 1),
		(SPI_TXEIS_MASK | SPI_TXOIS_MASK));
	writel(CURR_SPI_REG(SPI_PMIC_BUS_ID, SPI_DR_OFFSET), reg_addr);
	writel(CURR_SPI_REG(SPI_PMIC_BUS_ID, SPI_DR_OFFSET), reg_value);

	spi_ena(SPI_PMIC_BUS_ID, SPI_PMIC_CS_ID, SPI_WRITE_ONLY);
	do {
		reg = readl(CURR_SPI_REG(SPI_PMIC_BUS_ID, SPI_SR_OFFSET));
	} while(!(reg & SPI_SR_TX_FIFO_EMPTY));
	reg = spi_dis(SPI_PMIC_BUS_ID, SPI_TXOIS_MASK);
	if (reg)
		ret = -1;

	gpio_set(SPI_PMIC_CS_GPIO);

	return ret;
}

void amboot_wm831x_setreg(u16 reg, u16 val, u16 mask, u16 shift)
{
	u16					pmic_reg = 0;

	spi_pmic_wm831x_reg_read(reg, &pmic_reg);
#if defined(DEBUG_PMIC_ACCESS)
	putstr("WM831x read [0x");
	puthex(reg);
	putstr("]:0x");
	puthex(pmic_reg);
	putstr("\r\n");
#endif
	pmic_reg &= mask;
	pmic_reg |= (val << shift);
#if defined(DEBUG_PMIC_ACCESS)
	putstr("WM831x write[0x");
	puthex(reg);
	putstr("]:0x");
	puthex(pmic_reg);
	putstr("\r\n");
#endif
	spi_pmic_wm831x_reg_write(reg, pmic_reg);
}

int amboot_wm831x_reg_unlock(void)
{
	int ret;
	ret = spi_pmic_wm831x_reg_write(0x4008, 0x9716);
	if (ret != 0) {
		putstr("Failed to unlock registers!\r\n");
	}
	return ret;
}

void amboot_wm831x_reg_lock(void)
{
	int ret;

	ret = spi_pmic_wm831x_reg_write(0x4008, 0);
	if (ret != 0) {
		putstr("Failed to lock registers!\r\n");
	}
}

void amboot_wm831x_set_slp_slot(u16 reg, u16 slot)
{
	amboot_wm831x_setreg(reg, slot,
		WM831X_SLP_SLOT_MASK, WM831X_SLP_SLOT_SHIFT);
}

int amboot_wm831x_auxadc_read(int input)
{
	int src, timeout,ret;
	u16 temp;

	//set aux ENA
	spi_pmic_wm831x_reg_read(0x402e,  &temp);
	spi_pmic_wm831x_reg_write(0x402e, (temp | 0x8000));

	//set aux source
	src = input;
	spi_pmic_wm831x_reg_read(0x402f,  &temp);
	spi_pmic_wm831x_reg_write(0x402f, 1 << src);

	//set cvt ENA
	spi_pmic_wm831x_reg_read(0x402e,  &temp);
	spi_pmic_wm831x_reg_write(0x402e, (temp | 0x4000));

	timeout = 100;
	while (timeout) {
		timer_dly_ms(TIMER3_ID, 10);
		//read status1
		spi_pmic_wm831x_reg_read(0x4011,  &temp);
		if (temp < 0) {
			uart_putstr("read status error ");
			uart_putstr("\r\n");
			ret = -1;
			goto disable;
		}

		/* Did it complete? */
		if (temp & 0x0100) {
			spi_pmic_wm831x_reg_write(0x4011, 0x0100);
			break;
		}
		timeout--;
	}

	spi_pmic_wm831x_reg_read(0x402d,  &temp);
	if (temp < 0) {
		uart_putstr("Failed to read AUXADC data ");
		uart_putstr("\r\n");
	} else {
		src = ((temp & 0xf000)
		       >> 12) - 1;

		if (src == 14)
			src = 15;

		if (src != input) {
			uart_putstr("Data from source ");
			uart_putdec(src);
			uart_putstr("not ");
			uart_putdec(input);
			uart_putstr("\r\n");
			ret = -1;
			goto disable;
		} else {
			temp &= 0x0fff;
		}
	}

	ret = temp * 1465;

disable:
	spi_pmic_wm831x_reg_read(0x402e,  &temp);
	temp = temp & (!(0x8000));
	spi_pmic_wm831x_reg_write(0x402e, temp);
	return ret;
}
#endif /* BOARD_SUPPORT_SPI_PMIC_WM831X */

