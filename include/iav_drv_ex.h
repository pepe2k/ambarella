
/*
 * iav_drv_ex.h
 *
 * History:
 *	2008/6/30 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/*
 *	user-friendly APIs
 */

#ifndef __KERNEL__

typedef struct iav_bits_info_s {
	u32	frame_num;
	u32	PTS;
	u32	pic_type	: 3;
	u32	level_idc	: 3;
	u32	ref_idc		: 1;
	u32	pic_struct	: 1;

	u32	pic_size;	/* total picture size */
	u8	*start_addr;	/* start address of 1st part */
	u32	size;		/* size of first part */
	int	more;		/* 1: there's data in 2nd part */
	u8	*start_addr2;	/* start address of 2nd part */
	u32	size2;		/* size of 2nd part */
} iav_bits_info_t;

typedef struct iav_bs_fifo_info_s {
	u32	count;		/* input & output
				 * input - how many items in descs
				 * output - items filled
				 */
	iav_bits_info_t *descs;
} iav_bs_fifo_info_t;


static inline int iav_read_bsb(int fd_iav, iav_bs_fifo_info_t *fifo_info)
{
    return -1;
}

#endif

