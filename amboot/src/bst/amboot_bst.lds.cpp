/**
 * system/src/bst/amboot_bst.lds.cpp
 *
 * History:
 *    2005/01/26 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(reset)
SECTIONS
{
	. = 0x00000000;

	. = ALIGN(4);
	.text : {
		* (EXCLUDE_FILE (*postreloc*.o *bstnand*.o *bstonenand*.o *bstnor*.o *bstsnor*.o *bstemmc*.o) .text)
	}

	.data . : {
		memsetup*.o(.data)
		__table_end = .;
	}

	.bst_2 AMBOOT_BST_RAM_START : AT (ADDR(.text) + SIZEOF(.text) + SIZEOF(.data)) {
		__postreloc_ram_start = .;
		postreloc*.o (.text)
		bstnand*.o (.text)
		bstonenand*.o (.text)
		bstnor*.o (.text)
		bstsnor*.o (.text)
		bstemmc*.o (.text)
		__postreloc_ram_end = .;

	 }
	__postreloc_start = SIZEOF(.text) + SIZEOF(.data);
	__postreloc_end = SIZEOF(.text) + SIZEOF(.data) +
		(__postreloc_ram_end - __postreloc_ram_start);

	. = ALIGN(256);
	__fiodma_desc_addr = .;
	. = . + 0x100;
	. = ALIGN(256);
	__fiodma_rept_addr = .;
}
