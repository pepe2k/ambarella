/**
 * system/src/prom/prom.lds.cpp
 *
 * History:
 *    2008/01/16 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "prom.h"

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(reset)
SECTIONS
{
	. = (AMBPROM_RAM_START);
	.text : {
		__prom_start = .;
		start_prom_*.o (.text)
		__prom_end = .;

		. = ALIGN(4);
		* (.text*)

		. = ALIGN(4);
		* (.rodata*)
		prom_start = .;
	}

	.data : {
		. = ALIGN(4);
		* (.data*)
	}

	.bss : {
		. = ALIGN(4);
		__bss_start = .;

		*(.bss)

		/* Stack for SYS mode */
		. = ALIGN(4);
		__sys_stack_start = .;
		. = __sys_stack_start + AMBPROM_STACK_SIZE;
		__sys_stack_end = .;

		__bss_end = .;
	 }
}
