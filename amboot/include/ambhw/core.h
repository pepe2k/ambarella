/*
 * ambhw/core.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
 *
 * Copyright (C) 2006-2007, Ambarella, Inc.
 */

#ifndef __AMBHW_CORE_H__
#define __AMBHW_CORE_H__

#if !defined(__KERNEL__)

#define CLINE	5			/* 32 byte cache line size */
#define NWAY	2			/* 4 way cache */
#define I7SET	5
#define I7WAY	30
#define NCSIZE	4			/* number of times of cache size */

#if defined(CONFIG_MMP2_ARM926EJS)
#define CSIZE	15			/* 32 KB cache size */
#else
#define CSIZE	14			/* 16 KB cache size */
#endif
#define SWAY	(CSIZE - NWAY)		/* size of way in bytes = 1 << SWAY */
#define NSET	(CSIZE - NWAY - CLINE)	/* cache lines per way = 1 << NSET */

#ifndef __cacheline_aligned
#if defined(__arm)
#define __cacheline_aligned	__align(1 << CLINE)
#else
#define __cacheline_aligned	__attribute__((aligned(1 << CLINE))
#endif
#endif

#if defined(__ARM926EJS__)
#define __ARMV5_MMU__
#else
#define __ARMV6_MMU__
#endif

#if defined(__ARMV5_MMU__)
#define L1SECT(addr, ap, domain, cached, buffered)		\
	((addr & 0xfff00000) |					\
	 (ap << 10) |						\
	 (domain << 5) |					\
	 (cached << 3) |					\
	 (buffered << 2) |					\
	 0x12)
#define L1COARSE(addr, domain)					\
	((addr & 0xfffffc00) |					\
	 (domain << 5) |					\
	 0x11)
#define L2SMALL(addr, ap0, ap1, ap2, ap3, cached, buffered)	\
	((addr & 0xfffff000) |					\
	 (ap3 << 10) |						\
	 (ap2 << 8) |						\
	 (ap1 << 6) |						\
	 (ap0 << 4) |						\
	 (cached << 3) |					\
	 (buffered << 2) |					\
	 0x2)
#endif

#if defined(__ARMV6_MMU__)
#define L1SECT_V6(addr, ng, s, apx, ap, tex, p, domain, xn, cached, buffered) \
	((addr & 0xfff00000) |					\
	 (ng << 17) |						\
	 (s << 16) |						\
	 (apx << 15) |						\
	 (tex << 12) |						\
	 (ap << 10) |						\
	 (p << 9) |						\
	 (domain << 5) |					\
	 (xn << 4) |						\
	 (cached << 3) |					\
	 (buffered << 2) |					\
	 0x02)
#define L1COARSE_V6(addr, p, domain)				\
	((addr & 0xfffffc00) |					\
	 (p << 9) |						\
	 (domain << 5) |					\
	 0x01)
#define L2SMALL_V6(addr, ng, s, apx, tex, ap, cached, buffered, xn)	\
	((addr & 0xfffff000) |						\
	 (ng << 11) |							\
	 (s << 10) |							\
	 (apx << 9) |							\
	 (tex << 6) |							\
	 (ap << 4) |							\
	 (cached << 3) |						\
	 (buffered << 2) |						\
	 (0x2) |							\
	 xn)
#endif

#endif  /* !__KERNEL__ */

#endif
