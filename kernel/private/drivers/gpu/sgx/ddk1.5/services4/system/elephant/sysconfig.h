/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

#include <mach/hardware.h>
#include <hal/hal.h>
#include "syscommon.h"

#define VS_PRODUCT_NAME	"IONE"

#define SYS_SGX_CLOCK_SPEED		amb_get_3d_clock_frequency(HAL_BASE_VP)

#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(100)
#define SYS_SGX_PDS_TIMER_FREQ				(1000)

#if !defined(SYS_SGX_ACTIVE_POWER_LATENCY_MS)
#define SYS_SGX_ACTIVE_POWER_LATENCY_MS		(1)
#endif


#define SYS_IONE_SGX_REGS_SYS_PHYS_BASE  0xF0020000

#define SYS_IONE_SGX_REGS_SIZE           0x4000

#define SYS_IONE_SGX_IRQ				 153

#define SYS_IONE_GP11TIMER_ENABLE_SYS_PHYS_BASE  0x48088024
#define SYS_IONE_GP11TIMER_REGS_SYS_PHYS_BASE	 0xe800b000
#define SYS_IONE_GP11TIMER_TSICR_SYS_PHYS_BASE	 0x48088040


#endif