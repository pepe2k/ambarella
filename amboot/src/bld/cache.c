/**
 * system/src/comsvc/cache.c
 *
 * History:
 *    2005/05/26 - [Charles Chiou] created file
 *    2005/08/29 - [Chien-Yang Chien] assumed maintenance of this module
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>

#define ENABLE_DRAIN_WRITE_BUF

void flush_all_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" :
				      "=r " (addr_start));
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 1" :
				      "=r " (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void clean_flush_all_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c14, 1" : :
				      "r" (addr_start));
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void flush_i_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c5, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void flush_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start;

	K_ASSERT((((u32) addr) & ((0x1 << CLINE) - 1)) == 0);
	K_ASSERT((size & ((0x1 << CLINE) - 1)) == 0);

	addr_start = (u32) addr;
	size = size >> CLINE;

	while ((int) size > 0) {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c6, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	}
}

void clean_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void clean_flush_d_cache_region(void *addr, unsigned int size)
{
	u32 addr_start, addr_end;
	u32 misalign;

	addr_start = ((u32) addr) & ~((0x1 << CLINE) - 1);
	addr_end = ((u32) addr + size) & ~((0x1 << CLINE) - 1);

	misalign = ((u32) addr + size) & ((0x1 << CLINE) - 1);
	if (misalign)
		addr_end += (0x1 << CLINE);

	size = (addr_end - addr_start) >> CLINE;

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c14, 1" : :
				      "r" (addr_start));
		addr_start += (0x1 << CLINE);
		size--;
	} while ((int) size > 0);
}

void drain_write_buffer(u32 addr)
{
	addr = addr & ~((0x1 << CLINE) - 1);

	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 4" : : "r" (addr));
}

void clean_d_cache(void *addr, unsigned int size)
{
	if (size >= (0x1 << CSIZE) * NCSIZE) {
		_clean_d_cache();
	} else {
		clean_d_cache_region(addr, size);
	}

#if defined(ENABLE_DRAIN_WRITE_BUF)
	drain_write_buffer((u32)addr);
#endif
}

void flush_d_cache(void *addr, unsigned int size)
{
	flush_d_cache_region(addr, size);
}

void clean_flush_d_cache(void *addr, unsigned int size)
{
	if (size >= (0x1 << CSIZE) * NCSIZE) {
		_clean_flush_d_cache();
	} else {
		clean_flush_d_cache_region(addr, size);
	}

#if defined(ENABLE_DRAIN_WRITE_BUF)
	drain_write_buffer((u32)addr);
#endif
}

/**
 * Lockdown one way in the i-cache at the addr with size. The maximun ways
 * in the i-cache to lockdown is 3 ways for ARM926EJ-S. There must be one
 * way to be reserved for general usage at least.
 *
 * @param addr - pointer to addr desired to lockdown code
 * @param size - size in bytes of the code
 * @returns - locked way bit (if size > 0 || size < size of way)
 *	    - next available way bit (if size == 0)
 *	    - 0x8 (if no avaiable ways for lockdown)
 *	    - -1 (if size > size of way)
 */
int lock_i_cache_region(void *addr, unsigned int size)
{
	u32 c9f, locks, unlocks;
	int rval = 0;

	size = size + (u32) addr;
	addr = (void *) (((u32) addr) & ~((0x1 << CLINE) - 1));

	if (size & ((0x1 << CLINE) - 1))
		size = ((size - (u32) addr) >> CLINE) + 1;
	else
		size = (size - (u32) addr) >> CLINE;

	if (size > (0x1 << NSET)) {
		rval = -1;
		goto lock_i_cache_done;
	}


	__asm__ __volatile__ ("mrc p15, 0, %0, c9, c0, 1" : "=r " (c9f));

	/***********************************/
	/* lock the unused ways in i-cache */
	/***********************************/

	locks = c9f & 0xf;
	unlocks = ~locks;

	if (unlocks & 0x1) {
		c9f |= ~0x1;
		unlocks = 0x1;
	} else if (unlocks & 0x2) {
		c9f |= ~0x2;
		unlocks = 0x2;
	} else if (unlocks & 0x4) {
		c9f |= ~0x4;
		unlocks = 0x4;
	} else {
		rval = 0x8;
		goto lock_i_cache_done;
	}

	if (size == 0) {
		rval = unlocks;
		goto lock_i_cache_done;
	}

	__asm__ __volatile__ ("mcr p15, 0, %0, c9, c0, 1" : : "r" (c9f));

	/***********************************************/
	/* load code into cachelines in the unlock way */
	/***********************************************/

	do {
		__asm__ __volatile__ ("mcr p15, 0, %0, c7, c13, 1" : :
				      "r" ((u32) addr));
		addr = (void *) (((u32) addr) + (0x1 << CLINE));
		size--;
	} while (size > 0);

	/************************************************************/
	/* lock the desirable code and unlock other ways in i-cache */
	/************************************************************/

	c9f &= ~0xf;
	c9f |= (locks | unlocks);

	__asm__ __volatile__ ("mcr p15, 0, %0, c9, c0, 1" : : "r" (c9f));

	rval = unlocks;

lock_i_cache_done:

	return rval;
}

/**
 * Unlock any locked I-cache way(s).
 *
 * @param ways - The index to way.
 */
void unlock_i_cache_ways(unsigned int ways)
{
	ways &= 0xf;

	__asm__ __volatile__ ("mcr p15, 0, %0, c9, c0, 1" : : "r" (ways));
}

/**
 * Preload Instruction
 *
 * @param addr - pointer to addr desired to preload
 * @param size - size in bytes of the code
 * @returns - 0 OK
 *	    - -1 (if size > size of way)
 */
int pli_cache_region(void *addr, unsigned int size)
{
	int rval = 0;

	size = size + (u32)addr;
	addr = (void *)(((u32)addr) & ~((0x1 << CLINE) - 1));
	if (size & ((0x1 << CLINE) - 1)) {
		size = ((size - (u32)addr) >> CLINE) + 1;
	} else {
		size = (size - (u32)addr) >> CLINE;
	}

	if (size > (0x1 << CSIZE)) {
		rval = -1;
		goto pli_cache_region_exit;
	}

	do {
		__asm__ __volatile__ (
			"mcr p15, 0, %0, c7, c13, 1" : : "r" ((u32)addr));
		addr = (void *)(((u32)addr) + (0x1 << CLINE));
		size--;
	} while (size > 0);

pli_cache_region_exit:
	return rval;
}

