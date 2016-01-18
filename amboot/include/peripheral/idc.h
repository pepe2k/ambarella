/**
 * system/include/peripheral/idc.h
 *
 * Head file of IDC driver APIs. IDC can communicate with the device with I2C
 *
 *
 * History:
 *	2005/02/05 - [Allen Wang] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __IDC_H__
#define __IDC_H__

#include <basedef.h>
#include <ambhw.h>
//#include <ipc/ipc_mutex_def.h>
//#include <ipc/ipc_mutex.h>
//#include <kutil.h>

/*  I2C slave types */
enum idc_type
{
	IDC0	=  0,	/*  8-bit addr,     no subaddr/regaddr,  8-bit data*/
	IDC1	=  1,	/*  8-bit addr,     no subaddr/regaddr, 16-bit data*/
	IDC2	=  2,	/*  8-bit addr,  8-bit subaddr/regaddr,  8-bit data*/
	IDC3	=  3,	/*  8-bit addr,  8-bit subaddr/regaddr, 16-bit data*/
	IDC4	=  4,	/*  8-bit addr, 16-bit subaddr/regaddr,  8-bit data*/
	IDC5	=  5,	/*  8-bit addr, 16-bit subaddr/regaddr, 16-bit data*/
	IDC6	=  6,	/* 16-bit addr,     no subaddr/regaddr,  8-bit data*/
	IDC7	=  7,	/* 16-bit addr,     no subaddr/regaddr, 16-bit data*/
	IDC8	=  8,	/* 16-bit addr,  8-bit subaddr/regaddr,  8-bit data*/
	IDC9	=  9,	/* 16-bit addr,  8-bit subaddr/regaddr, 16-bit data*/
	IDC10	= 10,	/* 16-bit addr, 16-bit subaddr/regaddr,  8-bit data*/
	IDC11	= 11	/* 16-bit addr, 16-bit subaddr/regaddr, 16-bit data*/
	};

/**
 * IDC slave handler
 */
typedef void (*idcs_handler)(void);

/**
 * The idcs_param_s type is data structure used to
 * get data from idc_slave module.
 */
typedef struct idcs_param_s {
	idcs_handler tx_hdl;
	idcs_handler rx_hdl;
	u32 tx_buf_size;	/* output size */
	u32 rx_buf_size;	/* input size */
	u16 *rx_buf;		/* input buffer */
	u16 *tx_buf;		/* output buffer */
	u16 mode;		/* transfer mode */
	u8 slave_addr;
} idcs_param_t;

/* IDC ports using master controllers */
#define IDC_MASTER1	0
#define IDC_MASTER2	1
#define IDC_MASTER3	2

/* IDC ports using slave controllers */
#define IDC_SLAVE1	0x80

/* IDC operation modes */
#define IDC_HIGH_MODE_400KHZ	0
#define IDC_FAST_MODE_100KHZ	1
#define IDC_NORMAL_MODE_10KHZ	2

#if defined(CONFIG_SUPPORT_BOSS) && !defined(__BUILD_AMBOOT__)
#define IDC_GLOBAL_MUTEX_LOCK(x) 	{				\
		ipc_mutex_lock(G_idc_ctrl_map_mutex_id[(x)]);		\
}

#define IDC_GLOBAL_MUTEX_UNLOCK(x) 	{				\
		ipc_mutex_unlock(G_idc_ctrl_map_mutex_id[(x)]);		\
}
#else
#define IDC_GLOBAL_MUTEX_LOCK(x)	wai_sem(G_idc[(x)].semid)
#define IDC_GLOBAL_MUTEX_UNLOCK(x)	sig_sem(G_idc[(x)].semid)
#endif

/**
 * Configure the IDC/IDC@ operation modes
 *
 * @param mode - IDC operation modes
 */
#define idc_config(mode)		idc_dclk_config(IDC_MASTER1, mode)
#if (CHIP_REV == A2S) || (CHIP_REV == A5) || (CHIP_REV == A6) || \
	(CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == A5L) || \
	(CHIP_REV == I1) || (CHIP_REV == S2)
#define idc2_config(mode)		idc_dclk_config(IDC_MASTER2, mode)
#endif

/* These APIs could become obsolete in near future.
   Please use new APIs as below */
#define idc_read(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_read(IDC_MASTER1, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc2_read(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_read(IDC_MASTER2, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc3_read(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_read(IDC_MASTER3, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc_write(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_write(IDC_MASTER1, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc2_write(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_write(IDC_MASTER2, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc3_write(idc_slave_type, addr, subaddr, pdata, data_size)	\
	idc_master_write(IDC_MASTER3, idc_slave_type, addr, subaddr,	\
			pdata, data_size)

#define idc_burst_write(idc_slave_type, addr, subaddr, pdata, data_size)  \
	idc_master_burst_write(IDC_MASTER1, idc_slave_type, addr, subaddr,\
			pdata, data_size)

#define idc2_burst_write(idc_slave_type, addr, subaddr, pdata, data_size) \
	idc_master_burst_write(IDC_MASTER2, idc_slave_type, addr, subaddr,\
			pdata, data_size)

#define idc3_burst_write(idc_slave_type, addr, subaddr, pdata, data_size) \
	idc_master_burst_write(IDC_MASTER3, idc_slave_type, addr, subaddr,\
			pdata, data_size)

#define idc_read_ddc(segment_pointer, word_offset, pdata, data_size)      \
	idc_master_read_ddc(IDC_MASTER1, segment_pointer, word_offset,    \
			pdata, data_size)

#define idc2_read_ddc(segment_pointer, word_offset, pdata, data_size)     \
	idc_master_read_ddc(IDC_MASTER2, segment_pointer, word_offset,	  \
			pdata, data_size)

#define idc_direct_read(idc_slave_type, addr, subaddr, pdata, data_size)  \
	idc_master_direct_read(IDC_MASTER1, idc_slave_type, addr, subaddr,\
			pdata, data_size)

#define idc2_direct_read(idc_slave_type, addr, subaddr, pdata, data_size) \
	idc_master_direct_read(IDC_MASTER2, idc_slave_type, addr, subaddr,\
			pdata, data_size)

#define idc3_direct_read(idc_slave_type, addr, subaddr, pdata, data_size) \
	idc_master_direct_read(IDC_MASTER3, idc_slave_type, addr, subaddr,\
			pdata, data_size)

__BEGIN_C_PROTO__

/**
 * Read data over the IDC/I2C ports using combined format
 *
 * @param idc - IDC instance number.
 * @param idc_slave_type - Select IDC/I2C slave type.
 * @param addr - IDC device address to read. The 8-bit address cover 1-bit R/W.
 * @param subaddr - IDC device sub-address to read.
 * @param pdata - the pointer of the data array to put the read data
 * @param data_size - the number of words/bytes to read
 */
extern void idc_master_read(u8 idc, u16 idc_slave_type, u16 addr, u16 subaddr,
		     u16 *pdata, u16 data_size);

/**
 * Write data over the IDC/I2C ports
 *
 * @param idc - IDC instance number.
 * @param idc_slave_type - Select IDC/I2C slave type.
 * @param addr - IDC device address to write. The 8-bit address cover 1-bit R/W.
 * @param subaddr - IDC device sub-address to write
 * @param pdata - the pointer of the data array
 * @param data_size - the number of words/bytes to write
 */
extern void idc_master_write(u8 idc, u16 idc_slave_type, u16 addr, u16 subaddr,
		      u16 *pdata, u16 data_size);

/**
 * Write data over the IDC/I2C ports and 63-byte FIFO
 *
 * @param idc - IDC instance number.
 * @param idc_slave_type - Select IDC/I2C slave type.
 * @param addr - IDC device address to write. The 8-bit address cover 1-bit R/W.
 * @param subaddr - IDC device sub-address to write
 * @param pdata - the pointer of the data array
 * @param data_size - the number of words/bytes to write
 */
extern void idc_master_burst_write(u8 idc,u16 idc_slave_type, u16 addr,
			    u16 subaddr, u16 *pdata, u16 data_size);

/**
 * Read data directly over the IDC/I2C ports
 *
 * @param idc - IDC instance number.
 * @param idc_slave_type - Select IDC/I2C slave type.
 * @param addr - IDC device address to read. The 8-bit address cover 1-bit R/W.
 * @param pdata - the pointer of the data array
 * @param data_size - the number of words/bytes to read
 */
extern void idc_master_direct_read(u8 idc, u16 idc_slave_type, u16 addr,
			    u16 *pdata, u16 data_size);

/**
 * IDC read in a EDDC format
 * Read/write data over the DDC/DDC port
 *
 * @param idc - IDC instance number.
 * @param segment_pointer - the pointer of EDID segment
 * @param word_offset - the starting address of the words to read
 * @param pdata - the pointer of the data array
 * @param data_size - the number of bytes to write
 */
extern int idc_master_read_ddc(u8 idc, u16 segment_pointer, u16 word_offset,
			u16 *pdata, u16 data_size);

/**
 * This function is only used for initialization and setup
 * of I2C/IDC interface
 */
extern void idc_init(void);

/**
 * This function is only used to set up the operation modes
 * of I2C/IDC interface
 */
extern void idc_dclk_config(u8 idc, int mode);

/**
 * This function is used to get current idc operation modes
 *
 * @param idc - IDC instance number.
 *
 * @return - current idc operation mode
 */
extern int idc_get_dclk(u8 idc);

/**
 * Set the param of IDC slave interface.
 *
 * @param param - slave configuration parameters
 */
extern void idcs_set_param(idcs_param_t *param);

/**
 * This function is only used for IDC slave enable
 *
 * @return - return slave disable success/fail 0:success, -1:fail.
 */
extern int idc_slave_enable(void);

/**
 * This function is only used for IDC slave disable
 *
 * @return - return slave disable success/fail 0:success, -1:fail.
 */
extern int idc_slave_disable(void);

__END_C_PROTO__

#endif
