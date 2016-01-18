/*
 * util.h
 *
 * History:
 *	2008/1/25 - [Oliver Li] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef UTIL_H
#define UTIL_H

#include "msg_print.h"

#define KB	(1024)
#define MB	(1024 * 1024)

//#define CONFIG_IAV_ENABLE_PRINTK

// ARM physical address to DSP address
#if defined(CONFIG_PLAT_AMBARELLA_MEM_START_LOW)
#define PHYS_TO_DSP(addr)	(u32)(addr)
#else
#define PHYS_TO_DSP(addr)	(u32)((u32)(addr) & 0x3FFFFFFF)
#endif
#define VIRT_TO_DSP(addr)	PHYS_TO_DSP(virt_to_phys(addr))			// kernel virtual address to DSP address
#define AMBVIRT_TO_DSP(addr)	PHYS_TO_DSP(ambarella_virt_to_phys((u32)addr))	// ambarella virtual address to DSP address

// DSP address to ARM physical address
#if defined(CONFIG_PLAT_AMBARELLA_MEM_START_LOW)
#define DSP_TO_PHYS(addr)	(u32)(addr)
#else
#define DSP_TO_PHYS(addr)	(u32)((u32)(addr) | 0xC0000000)
#endif
#define DSP_TO_VIRT(addr)	phys_to_virt(DSP_TO_PHYS(addr))			// DSP address to kernel virtual address
#define DSP_TO_AMBVIRT(addr)	ambarella_phys_to_virt(DSP_TO_PHYS((u32)addr))	// DSP address to ambarella virtual address

#define clean_d_cache		ambcache_clean_range
#define invalidate_d_cache	ambcache_inv_range

#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif

#ifdef CONFIG_IAV_ENABLE_PRINTK
#define __iav_printk(fmt, args...) \
	DRV_PRINT(KERN_DEBUG fmt, ##args)
#else
#define __iav_printk(fmt, args...)
#endif

#define iav_printk(S...)	__iav_printk(S)
#define iav_dbg_printk(S...)	__iav_printk(S)
#define iav_error(S...)		DRV_PRINT("iav error: "S)
#define iav_warning(S...)		DRV_PRINT(KERN_WARNING "iav warn: "S)
#define LOG_ERROR(S...)			DRV_PRINT(KERN_ERR "iav error: "S)
#define LOG_WARNING(S...)		DRV_PRINT(KERN_WARNING "iav warn: "S)
#define dsp_printk(S...)		DRV_PRINT(KERN_DEBUG "dsp:" S)
#define dsp_error(S...)			DRV_PRINT(KERN_ERR "dsp error: "S)
#define dsp_warning(S...)		DRV_PRINT(KERN_WARNING "dsp warn: "S)


#define PRINT_FUNCTION_NAME	    __iav_printk("iav:  Calling %s() ", __FUNCTION__)

#define IAV_lock_irq(lock, flags)	spin_lock_irqsave(lock, flags)
#define IAV_unlock_irq(lock, flags)	spin_unlock_irqrestore(lock, flags)

#define IAV_ASSERT(x)	BUG_ON(!(x))

#define ROUND_UP(x, n)    ( ((x)+(n)-1u) & ~((n)-1u) )

//32-bit
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )

//64-bit
#define DO_DIV_ROUND(divident, divider)     do {			\
	(divident) += ((divider)>>1);							\
	do_div( (divident) , (divider) );						\
	} while (0)

#endif	// UTIL_H

