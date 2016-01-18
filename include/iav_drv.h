
/*
 * iav_drv.h
 *
 * History:
 *	2008/04/02 - [Oliver Li] created file
 *	2011/06/10 - [Jian Tang] modified file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __IAV_DRV_H
#define __IAV_DRV_H

#if defined(CONFIG_CPU_CORTEXA9_HF)
#  if !((( __GNUC__  == 4) && (  __GNUC_MINOR__ >= 7)) || (__GNUC__  > 4))
#    error  "Hard float needs Linaro Toolchain GCC4.8 to compile"
#  endif
#elif defined(CONFIG_CPU_CORTEXA9) && \
      (((( __GNUC__  == 4) && (  __GNUC_MINOR__ >= 7)) || (__GNUC__  > 4)))
#  error  "Soft float is not supported by GCC version > 4.7.4"
#endif

#define IAV_IOC_MAGIC			'v'
#define IAV_IOC_AUDIT_MAGIC		'a'
#define IAV_IOC_ENCODE_MAGIC		'e'
#define IAV_IOC_DECODE_MAGIC		'd'
#define IAV_IOC_DECODE_MAGIC2		'D'
#define IAV_IOC_TRANSCODE_MAGIC	't'
#define IAV_IOC_OVERLAY_MAGIC		'o'
#define IAV_IOC_DUPLEX_MAGIC		'L'
#define DUMP_IDSP_0	0
#define DUMP_IDSP_1	1
#define DUMP_IDSP_2	2
#define DUMP_IDSP_3	3
#define DUMP_IDSP_4	4
#define DUMP_IDSP_5	5
#define DUMP_IDSP_6	6
#define DUMP_IDSP_7	7
#define DUMP_IDSP_100	100
#define DUMP_FPN		200
#define DUMP_VIGNETTE	201
#define MAX_DUMP_BUFFER_SIZE 256*1024
#define IDSP_RAM_SIZE 0x05800000

#include "iav_audit_drv.h"
#include "iav_decode_drv.h"
#include "iav_encode_drv.h"
#include "iav_duplex_drv.h"
#include "iav_ioctl_arch.h"
#include "iav_struct_arch.h"

// arg of IAV_IOC_ENTER_IDLE
enum {
	IAV_GOTO_IDLE_NORMAL = 0,
	IAV_GOTO_IDLE_GIVEUP_DSP = 1,
	IAV_GOTO_IDLE_TAKEOVER_DSP = 2,
};


#endif // __IAV_DRV_H


