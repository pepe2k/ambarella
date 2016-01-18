/**
 * system/include/peripheral/spi.h
 *
 *  Driver APIs for SPI
 *
 * History:
 *    2005/03/28 - [Eric Lee] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef  __SPI_H__
#define  __SPI_H__

#include <ambhw.h>

/* SPI INSTANCE ID */
#define SPI_MASTER1	0
#define SPI_MASTER2	1
#define SPI_MASTER3	2
#define SPI_MASTER4	3
#define SPI_SLAVE1	0x80
#define SPIS_SHIFT_BASE	0x80

/* SPI modes */
#define SPI_MODE0  	0     	/* Motorola SCPOL = low,   SCPH = low */
#define SPI_MODE1  	1     	/* Motorola SCPOL = low,   SCPH = high */
#define SPI_MODE2  	2     	/* Motorola SCPOL = high,  SCPH = low */
#define SPI_MODE3  	3     	/* Motorola SCPOL = high,  SCPH = high */
/* SPI modes for A2 and later*/
#define TI_SSP_MODE	4	/* TI SSP */
#define NSM_MODE0	5	/* NS Microwire, NHS = 0, MWMODE = 0 */
#define NSM_MODE1	6	/* NS Microwire, NHS = 0, MWMODE = 1 */
#define NSM_MODE2	7	/* NS Microwire, NHS = 1, MWMODE = 0 (read) */
#define NSM_MODE3 	8	/* NS Microwire, NHS = 1, MWMODE = 1 (read) */

/* Frame formats */
#define FRF_MOTO	0	/* Frame format: Motorola */
#define FRF_TI		1	/* Frame format: TI, A2 only */
#define FRF_NSM		2	/* Frame format: NS micro-wire, A2 only */
#define FRF_RSV		3	/* Frame format: Reserved */

#define SPI_MAX_CFS	16
#define SPI_MAX_DFS	16

#define SPI_WRITE_READ  0	/* Motorola SPI and TI SSP */
#define SPI_WRITE_ONLY  1	/* Motorola SPI, TI SSP and NSM */
#define SPI_READ_ONLY   2	/* Motorola SPI, TI SSP and NSM */

/* TSSI modes */
#define TSSI_EN_IDSP_LAST_PIXEL	0x10
#define TSSI_EN_SENSOR_VSYNC	0x20
#define TSSI_EN_IDSP_VSYNC	0x40
#define TSSI_EN_FORCED_DRAIN	0x80

#define SPI_DUMMY_DATA		0x0		/* Dummy Word, SCPOL = 1/0 */
#define SPI_MSTIS_MASK		0x00000020
#define SPI_RXFIS_MASK 		0x00000010
#define SPI_RXOIS_MASK 		0x00000008
#define SPI_RXUIS_MASK 		0x00000004
#define SPI_TXOIS_MASK 		0x00000002
#define SPI_TXEIS_MASK 		0x00000001
#define SPI_COMBINED_MASK	0x0000001f
#define SPI_TX_MASK             0x00000003
#define SPI_RX_MASK             0x0000001c
#define SPI_SR_BUSY		0x1
#define SPI_SR_TX_FIFO_N_FULL	0x2
#define SPI_SR_TX_FIFO_EMPTY	0x4
#define SPI_SR_RX_FIFO_N_EMPTY	0x8
#define SPI_SR_RX_FIFO_FULL	0x10

#define SPI_CS_POL_HIGH		1
#define SPI_CS_POL_LOW		0

/* These APIs could become obsolete in near future. 
   Please use new APIs as below */
#define spi_config(spi_id, spi_mode, cfs_dfs, baud_rate)		\
	spi_master_config(SPI_MASTER1, spi_id, spi_mode, cfs_dfs, 	\
			  baud_rate)

#define spi2_config(spi_id, spi_mode, cfs_dfs, baud_rate)		\
	spi_master_config(SPI_MASTER2, spi_id, spi_mode, cfs_dfs, 	\
			  baud_rate)

#define spi_write(spi_id, buffer, n_size)				\
	spi_master_write(SPI_MASTER1, spi_id, buffer, n_size)

#define spi2_write(spi_id, buffer, n_size)				\
	spi_master_write(SPI_MASTER2, spi_id, buffer, n_size)

#define spi_read(spi_id, buffer, n_size)				\
	spi_master_read(SPI_MASTER1, spi_id, buffer, n_size)

#define spi2_read(spi_id, buffer, n_size)				\
	spi_master_read(SPI_MASTER2, spi_id, buffer, n_size)

#define spi_write_read(spi_id, w_buffer, r_buffer, w_size, r_size)	\
	spi_master_write_read(SPI_MASTER1, spi_id, w_buffer, r_buffer,	\
			      w_size, r_size)

#define spi2_write_read(spi_id, w_buffer, r_buffer, w_size, r_size)	\
	spi_master_write_read(SPI_MASTER2, spi_id, w_buffer, r_buffer,	\
			      w_size, r_size)

/**
 * Callback type for spi event handler
 */
typedef void (*spi_slave_event_handler)(void);

/**
 * The spi_slave_param_s type is data structure used to 
 * get data from spi_slave module.
 */
typedef struct spi_slave_param_s {
	u16 mode;		/* transfer mode */
	u32 tx_buf_size;	/* output size */
	u32 rx_buf_size;	/* input size */
	u16 *rx_buf;		/* input buffer */
	u16 *tx_buf;		/* output buffer */
	spi_slave_event_handler hdl;
} spi_slave_param_t;

__BEGIN_C_PROTO__

/**
 * Configure the specific Serial Peripheral Interface (SPI) module.
 * 
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices(0 - max number of SPI enable pins)
 *		   A2 allows 2 slave-select output at the same time.
 *		   ID = (4-bit ID_EN0 + 4-bit ID_EN1). When only 1 slave to
 *		   enable, set ID_EN0 = ID_EN1.
 * @param spi_mode - SPI mode to specify where data should be latched
 *                    for transitions. There are the combinations of
 *                    SCPOL and SCPH for Motorola SPI. 4 combinations of NHS
 * 		      and MWMOD for National Semi. Micro-wire.
 * @param fs - control frame size plus data frame length.
 *		      The control words and data transceived shall be
 * 		      right-justified.
 * @param baud_rate - frequency of the serial clock (SCK)
 *                    to regulate the data transfer. If baud_rate = 0xffffffff,
 *		      the serial clock output is disabled.
 *
 */
extern void spi_master_config(u8 spi_inst, u8 spi_id, u8 spi_mode, u8 fs, 
		       u32 baud_rate);

/**
 * Write n_size number of data frames (buffer) continuously into slave device
 * with ID 'spi_id'
 *
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices (0 - max number of SPI enable pins)
 * @param buffer - Pointer to the data buffer array for writing
 * @param n_size - number of data frames (buffer) to write (transmit)
 * @param return - spi_write is success/fail. 0:success, -1:fail
 *
 */
extern int spi_master_write(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size);

/**
 * Read nbuf number of data frames (buffer) continuously from slave device
 * with ID 'spi_id'
 *
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices (0 - max number of SPI enable pins)
 * @param buffer - Pointer to the data buffer array for receiving
 * @param n_size - number of data frames (buffer) to read (receive)
 * @param return - spi_read is success/fail. 0:success, -1:fail
 */
extern int spi_master_read(u8 spi_inst, u8 spi_id, u16 *buffer, u16 n_size);

/**
 * Perform writing of w_size frame to the external serial device,
 * along with reading of r_size frame of data received from the
 * the same serial device
 *
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices (0 - max number of SPI enable pins)
 * @param w_buffer - Pointer to the data buffer array for writing
 * @param r_buffer - Pointer to the data buffer array for receiving
 * @param w_size - number of data frames (buffer) to write (transmit)
 * @param r_size - number of data frames (buffer) to read (receive)
 * @param return - spi_write_read is success/fail. 0:success, -1:fail
 */
extern int spi_master_write_read(u8 spi_inst, u8 spi_id, u16 *w_buffer, 
				 u16 *r_buffer, u16 w_size, u16 r_size);

/**
 * Configure the specific Serial Peripheral Interface (SPI_SLAVE) module.
 *
 * @param spi_mode - SPI_slave mode to specify where data should be latched
 *                    for transitions. There are the combinations of
 *                    SCPOL and SCPH for Motorola SPI. 4 combinations of NHS
 * 		      and MWMOD for National Semi. Micro-wire.
 * @param cfs_dfs - control frame size plus data frame length.
 *		      The control words and data transceived shall be
 * 		      right-justified.
 *
 */
extern void spi_slave_config(u8 spi_mode, u8 cfs_dfs);

/**
 * Set the param of SPI slave interface.
 *
 * @param parameter - Structure contains the callback function and 
 *		      slave param
 */
extern void spi_slave_set_param(spi_slave_param_t *parameter);

/**
 * This function only used for get the current param of spi slave module
 *
 * @return slave parameter
 * @see also spi_slave_param_t
 */
extern spi_slave_param_t *spi_slave_get_param(void);

/**
 * Configure the polarity of spi enable line at high or low
 *
 * @param spi_inst - different instance module
 * @param spi_id - slave enable siganl id(slave)
 * @param cs - default enable signal high/low 
 *                        0, spi enable default low 
 *                        1, spi enable default high 
 */
extern void spi_config_ena_pin_polarity(u8 spi_inst, u8 spi_id, int cs);

/**
 * This function is called by peripheral_init() for initialization only
 */
extern void spi_init(void);

/**
 * Config SPI CLK delay cycle time after slave enable
 * @param spi_inst - different instance module
 * @param delay_cycle - delay cycle count after slave enable
 */

extern void spi_config_ena_delay (u8 spi_inst, u16 delay_cycle);
/**
 * Send the corresponding semaphore ID with spi_inst
 * 
 * @param spi_inst - different instance module
 */
int spi_get_semaphore_id(u8 spi_inst);

/*
 * Config Micro-Wire start bit high or low
 * @param spi_inst - different instance module
 * @param high_low - set start bit, 0 : low, 1 : high
 */

extern void spi_config_nsm_handshaking_start_bit(u8 spi_inst, u8 high_low);
/**
 * A function to write and read using gpio as SPI_EN pin
 *
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices (0 - max number of SPI enable pins)
 * @param w_buffer - Pointer to the data buffer array for writing
 * @param r_buffer - Pointer to the data buffer array for receiving
 * @param w_size - number of data frames (buffer) to write (transmit)
 * @param r_size - number of data frames (buffer) to read (receive)
 */
int spi_write_read_gpio_en(u8 spi_inst,
			    u8 spi_id,
			    u16 *w_buffer,
			    u16 *r_buffer,
			    u16 w_size,
			    u16 r_size);

/**
 * A function to write using gpio as SPI_EN pin
 *
 * @param spi_inst - different instance module
 * @param spi_id - ID of SPI slave devices (0 - max number of SPI enable pins)
 * @param buffer - Pointer to the data buffer array for writing
 * @param size - number of data frames (buffer) to write (transmit)
 */
int spi_write_gpio_en(u8 spi_inst,
		       u8 spi_id,
		       u16 *buffer,
		       u16 size);

__END_C_PROTO__

#endif  /* __SPI_H__ */
