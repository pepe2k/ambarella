/**
 * system/src/bld/amboot_bld.lds.cpp
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <config.h>
#include <bsp.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_trampoline)
SECTIONS
{
	. = AMBOOT_BLD_RAM_START;

	.text : {
		trampoline*.o (.text)

		* (.text*)
		* (.rodata*)

		. = ALIGN(4);
		__cmdlist_start = .;
		* (.cmdlist)
		__cmdlist_end = .;
	}

	. = ALIGN(4);
	.data : {
		* (.data*)
	}

	. = ALIGN(4);
	.bss : {
		__bss_start = .;

		* (.bss)

		__bss_end = .;

		. = ALIGN(4);
		/* Stack for UND mode */
		__und_stack_start = .;
		. = __und_stack_start + 0x100;
		__und_stack_end = .;

		/* Stack for ABT mode */
		__abt_stack_start = .;
		. = __abt_stack_start + 0x100;
		__abt_stack_end = .;

		/* Stack for IRQ mode */
		__irq_stack_start = .;
		. = __irq_stack_start + 0x2000;
		__irq_stack_end = .;

		/* Stack for FIQ mode */
		__fiq_stack_start = .;
		. = __fiq_stack_start + 0x100;
		__fiq_stack_end = .;

		/* Stack for SVC mode */
		__svc_stack_start = .;
		. = __svc_stack_start + 0x100;
		__svc_stack_end = .;

		/* Stack for SYS mode */
		__sys_stack_start = .;
		. = __sys_stack_start + AMBOOT_BLD_STACK_SIZE;
		__sys_stack_end = .;

		/* Heap for BLD */
		__heap_start = .;
		. = __heap_start +  AMBOOT_BLD_HEAP_SIZE;
		__heap_end = .;
	}

	. = ALIGN(4);
	.bss.noinit : {
		* (.bss.noinit*)
	}

	. = ALIGN(0x4000);
	.pagetable (NOLOAD) : {
		/* MMU page table */
		__pagetable_main = .;
		. = __pagetable_main + (4096 * 4);	/* 16 KB of L1 */
		__pagetable_hv = .;
		. = __pagetable_hv + (256 * 4);		/* 1KB of L2 */
		__pagetable_hal = .;
		. = __pagetable_hal + (256 * 4);	/* 1KB of L2 */
		__pagetable_cry = .;
		. = __pagetable_cry + (256 * 4);
		__pagetable_end = .;
	}

	.hugebuf . (NOLOAD) : {
		. = ALIGN(0x100000);
		__bld_hugebuf = .;
	}
}
