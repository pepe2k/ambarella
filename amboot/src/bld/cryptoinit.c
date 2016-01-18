/**
* src/bld/cryptoinit.c
*
* History
* 2012/09/06 - [Jahnson Diao ] created file
*
* Copyright (C) 2012-2012, Ambarella, Inc.
*
* All rights reserved. No Part of this file may be reproduced, stored
* in a retrieval system, or transmitted, in any form, or by any means,
* electronic, mechanical, photocopying, recording, or otherwise,
* without the prior consent of Ambarella, Inc.
*/
#include <bldfunc.h>
#include <ambhw.h>
#include <ambhw/busaddr.h>
#include <cryptoinit.h>

#if defined(AMBARELLA_CRYPTO_REMAP)

int crypto_remap(void)
{

#if defined(AMBARELLA_CRYPTO_REMAP)
		/* the physical address */
		extern u32 __pagetable_main;
		extern u32 __pagetable_cry;
		u32 pgtbl, base, x, size, val;

		/* Disable the MMU */
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));
		x &= ~(0x1);
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));

		pgtbl = (u32) &__pagetable_main;

		/* Setup crypto to point to a corse page table */
		base = CRYPTO_REMAP_ADDR >> 20;
		x = (u32) &__pagetable_cry;

#if defined(__ARM926EJS__)
		x |= 0x11;		/* ARM926EJS */
#endif
#if defined(__ARM1136JS__)
		x |= 0x01;		/* ARM1136JS */
#endif
		writel(pgtbl + (base  * 4), x);

		/* Now setup the 2nd-level pagetable for HAL */
		base = (u32) &__pagetable_cry;
		size = 4096;
		val = CRYPT_PHYS_BASE;
		val &= 0xfffff000;
		val |= 0xff2;
		writel(base , val);

		/* Flush the TLB and re-enable the MMU */
		x = 0x0;
		__asm__ __volatile__ ("mcr p15, 0, %0, c8, c6, 0": : "r" (x));
		__asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r " (x));
		x |= 0x1;
		__asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0": : "r" (x));

#endif
	return 0;
}


#endif
