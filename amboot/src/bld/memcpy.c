/**
 * system/src/bld/memcpy.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
 *    2005/03/08 - [Charles Chiou] replaced with ARM linux implementation.
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>

#if 0

void *memcpy(void *dst, const void *src, unsigned int n)
{
	unsigned char *_dst = (unsigned char *) dst;
	const unsigned char *_src = (const unsigned char *) src;

	while (n-- > 0)
		*_dst++ = *_src++;

	return dst;
}

#else

/*-------------------------------------------------------------------------
 void *memcpy(void* dst, const void* src, unsigned int n)

 Copy block of memory

 Input	: void* dst		: destination address
	  const void* src	: source address
 	  unsigned int n	: memory size to be copied
 Output	: none
 Return	: destination address
 Note	: none
-------------------------------------------------------------------------*/
void *memcpy(void* dst, const void* src, unsigned int n)
{
	if (n)
	{
		switch (((unsigned int)src | (unsigned int)dst | n) & 3)
		{
		case 0: {
			unsigned int *xp = (unsigned int *)src;
			unsigned int *wp = (unsigned int *)dst;
			do {
				*wp++ = *xp++;
			} while ((n -= 4) != 0);
			break;
		}

		case 2: {
			unsigned short *xp = (unsigned short *)src;
			unsigned short *wp = (unsigned short *)dst;
			do {
				*wp++ = *xp++;
			} while ((n -= 2) != 0);
			break;
		}

		default: {
			unsigned char *xp = (unsigned char *)src;
			unsigned char *wp = (unsigned char *)dst;
			do {
				*wp++ = *xp++;
			} while ((n -= 1) != 0);
			break;
		}
		}
	}

	return dst;
}

#endif
