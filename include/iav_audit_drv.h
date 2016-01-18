/*
 * iav_audit_drv.h
 *
 * History:
 *	2012/07/25 - [Jian Tang] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_AUDIT_DRV_H__
#define __IAV_AUDIT_DRV_H__


/**********************************************************
 *
 *	Audit APIs - IAV_IOC_AUDIT_MAGIC
 *
 *********************************************************/

enum {
	// For Audit global setting, from 0x00 to 0x1F
	IOC_SET_AUDIT_SETUP = 0x00,
	IOC_GET_AUDIT_SETUP = 0x01,

	// For Audit module setting, from 0x20 to 0x7F
	IOC_GET_AUDIT_IRQ = 0x21,
	IOC_GET_AUDIT_BITS_INFO = 0x22,
	IOC_GET_AUDIT_MV_STATIS = 0x23,

	// Reserved, from 0x80 to 0xFF
};

#define _AUDIT_IO(IOCTL)		_IO(IAV_IOC_AUDIT_MAGIC, IOCTL)
#define _AUDIT_IOW(IOCTL, param)		_IOW(IAV_IOC_AUDIT_MAGIC, IOCTL, param)
#define _AUDIT_IOR(IOCTL, param)		_IOR(IAV_IOC_AUDIT_MAGIC, IOCTL, param)
#define _AUDIT_IOWR(IOCTL, param)		_IOWR(IAV_IOC_AUDIT_MAGIC, IOCTL, param)

#define IAV_IOC_SET_AUDIT_SETUP		_AUDIT_IOW(IOC_SET_AUDIT_SETUP, struct iav_audit_setup_s *)
#define IAV_IOC_GET_AUDIT_SETUP		_AUDIT_IOR(IOC_GET_AUDIT_SETUP, struct iav_audit_setup_s *)

#define IAV_IOC_GET_AUDIT_IRQ			_AUDIT_IOWR(IOC_GET_AUDIT_IRQ, struct iav_audit_irq_s *)
#define IAV_IOC_GET_AUDIT_BITS_INFO	_AUDIT_IOWR(IOC_GET_AUDIT_BITS_INFO, struct iav_audit_bits_info_s *)
#define IAV_IOC_GET_AUDIT_MV_STATIS	_AUDIT_IOWR(IOC_GET_AUDIT_MV_STATIS, struct iav_audit_mv_statis_s *)


/**********************************************************
 *
 *			Structure definitions
 *
 *********************************************************/

typedef struct iav_audit_setup_s {
} iav_audit_setup_t;


typedef struct iav_audit_irq_s {
} iav_audit_irq_t;


typedef struct iav_audit_bits_info_s {
} iav_audit_bits_info_t;


typedef struct iav_audit_mv_statis_s {
} iav_audit_mv_statis_t;

#endif	// __IAV_AUDIT_DRV_H__

