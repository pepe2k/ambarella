/**
 * system/src/bld/mmu.S
 *
 * History:
 *    2005/03/10 - [Charles Chiou] created file
 *    2009/01/12 - [Charles Chiou] merged mmu.c & pagetable.c into mmu.S
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

.globl mmu_init
mmu_init:
	/* Set the interrupt vectors address to 0xffff0000 - 0xffff001c */
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #0x2000
	mcr	p15, 0, r0, c1, c0, 0

	/********************/
	/* Setup page table */
	/********************/

#if	(PHY_BUS_MAP_TYPE == 1)
	/* Map DRAM up to the range specified by the configuration */
	ldr	r0, pagetable_main
	mov	r1, #0x0
	mov	r2, #0x0
#if	(DRAM_SIZE == 0x02000000)
	mov	r3, #0x020	/* 256 Mbit (32 MB) */
#elif	(DRAM_SIZE == 0x04000000)
	mov	r3, #0x040	/* 512 Mbit (64 MB) */
#elif	(DRAM_SIZE == 0x08000000)
	mov	r3, #0x080	/* 1 Gbit (128 MB) */
#elif	(DRAM_SIZE == 0x10000000)
	mov	r3, #0x100	/* 2 Gbit (256 MB) */
#elif	(DRAM_SIZE == 0x20000000)
	mov	r3, #0x200	/* 4 Gbit (512 MB) */
#elif	(DRAM_SIZE == 0x40000000)
	mov	r3, #0x400	/* 8 Gbit (1024 MB) */
#elif	(DRAM_SIZE == 0x80000000)
	mov	r3, #0x800	/* 16 Gbit (2048 MB) */
#endif
4:
	mov	r2, r1, lsl #20		/* Address */
	orr	r2, r2, #0xc00		/* Access = R/W */
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x1e
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x0e
#endif
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	4b

	/* Clear from dram end to 0xdff (~ 0xdfefffff) */
1:
	mov	r2, #0x0
	ldr	r3, =0xdff
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	1b

	/* Map 0xdff - 0xf01 (0xdff00000 - 0xf00fffff) as peripheral address */
	ldr	r3, =0xf01
2:
	mov	r2, r1, lsl #20		/* Address */
	orr	r2, r2, #0xc00		/* Access = R/W */
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x12
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x02
#endif
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	2b

	/* Clear 0xf01 - 0xfff (0xf0100000 - 0xffefffff) */
	mov	r2, #0x0
	mov	r3, #0xff
	orr	r3, r3, #0xf00
3:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	3b

	/* Setup 0xfff00000 to point to a corse page table */
	ldr	r2, pagetable_hv
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x11		/* ARM926EJS */
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x01		/* ARM1136JS */
#endif
	str	r2, [r0]

	/* Now setup the 2nd-level pagetable for the 0xfff00000 section */
	ldr	r0, pagetable_hv

	/* Clear 0x00 - 0xef (0xfff00000 - 0xfffeffff) */
	mov	r1, #0x0
	mov	r2, #0x0
	mov	r3, #0xf0
10:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	10b

	/* Map 0xf0 */
	/* Map 0xffff0000 to DRAM_START_ADDR */
	mov	r2, #DRAM_START_ADDR
	orr	r2, r2, #0xe
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1

	/* Clear 0xf1 - 0xff */
	mov	r3, #0x100
	mov	r2, #0x0
11:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	11b
#else
	/* Clear 0x000 - 0x600 (0x00000000 - 0x5fffffff) */
	ldr	r0, pagetable_main
	mov	r1, #0x0
	mov	r2, #0x0
	mov	r3, #0x600
1:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	1b

	/* Map 0x600 - 0x800 (0x60000000 - 0x7fffffff) as peripheral address */
	mov	r3, #0x800
2:
	mov	r2, r1, lsl #20		/* Address */
	orr	r2, r2, #0xc00		/* Access = R/W */
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x12
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x02
#endif
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	2b

	/* Clear 0x800 - 0xc00 (0x00000000 - 0xbfffffff) */
	mov	r2, #0x0
	mov	r3, #0xc00
3:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	3b

	/* Map DRAM up to the range specified by the configuration */
#if	(DRAM_SIZE == 0x02000000)
	mov	r3, #0xc20	/* 256 Mbit (32 MB) */
#elif	(DRAM_SIZE == 0x04000000)
	mov	r3, #0xc40	/* 512 Mbit (64 MB) */
#elif	(DRAM_SIZE == 0x08000000)
	mov	r3, #0xc80	/* 1 Gbit (128 MB) */
#elif	(DRAM_SIZE == 0x10000000)
	mov	r3, #0xd00	/* 2 Gbit (256 MB) */
#elif	(DRAM_SIZE == 0x20000000)
	mov	r3, #0xe00	/* 4 Gbit (512 MB) */
#elif	(DRAM_SIZE == 0x40000000)
	mov	r3, #0xff0	/* 8 Gbit (1024 MB) should be 0x1000*/
#endif
4:
	mov	r2, r1, lsl #20		/* Address */
	orr	r2, r2, #0xc00		/* Access = R/W */
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x1e
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x0e
#endif
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	4b

	/* Clear from end of DRAM to 0xfff (0xffefffff) */
	mov	r2, #0x0
	mov	r3, #0xff
	orr	r3, r3, #0xf00
5:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	5b

	/* Setup 0xfff00000 to point to a corse page table */
	ldr	r2, pagetable_hv
#if defined(__ARM926EJS__)
	orr	r2, r2, #0x11		/* ARM926EJS */
#endif
#if defined(__ARM1136JS__)
	orr	r2, r2, #0x01		/* ARM1136JS */
#endif
	str	r2, [r0]

	/* Now setup the 2nd-level pagetable for the 0xfff00000 section */
	ldr	r0, pagetable_hv

	/* Clear 0x00 - 0xef (0xfff00000 - 0xfffefffff) */
	mov	r1, #0x0
	mov	r2, #0x0
	mov	r3, #0xf0
10:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	10b

	/* Map 0xf0 */
	/* Map 0xffff0000 to DRAM_START_ADDR */
	mov	r2, #DRAM_START_ADDR
	orr	r2, r2, #0xe
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1

	/* Clear 0xf1 - 0xff */
	mov	r3, #0x100
	mov	r2, #0x0
11:
	str	r2, [r0]
	add	r0, r0, #0x4
	add	r1, r1, #0x1
	teq	r1, r3
	bne	11b

#if	(PHY_BUS_MAP_TYPE == 2)
	/* Overwrite 0xe0 page */
	/* Map 0xfffe0000 to DRAM_PHYS_BASE */

	ldr	r0, pagetable_hv
	ldr	r3, =0x380
	orr	r0, r0, r3
	ldr	r2, =DRAM_PHYS_BASE
	ldr	r3, =0xff2
	orr	r2, r2, r3
	str	r2, [r0]
#endif
#endif
	/* Read control register */
	mrc	p15, 0, r0, c1, c0, 0

	/* Make sure MMU is in disabled state first */
	bic	r0, r0, #0x1
	mcr	p15, 0, r0, c1, c0, 0

	/* Set the page table */
	ldr	r0, pagetable_main;
	mcr	p15, 0, r0, c2, c0, 0

	/* Flush all cache */
	mov	r0, #0x0
	mcr	p15, 0, r0, c7, c6, 0

	/* Flush all TLB */
	mcr	p15, 0, r0, c8, c6, 0

	/* Set domain 0 as client and access permission controlled by */
	/* descriptor. Other domains have no access permission */
	mov	r0, #0x1
	mcr	p15, 0, r0, c3, c0, 0

	/* Set the R (read only) bit for those pages with ap = 0 as */
	/* read only pages */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x100
	orr	r0, r0, #0x200
	mcr	p15, 0, r0, c1, c0, 0

	/* Read control register */
	mrc	p15, 0, r0, c1, c0, 0

	/* Enable MMU */
	orr	r0, r0, #0x1
	mcr	p15, 0, r0, c1, c0, 0

	mov	pc, lr

pagetable_main:		.word	__pagetable_main
pagetable_hv:		.word	__pagetable_hv
