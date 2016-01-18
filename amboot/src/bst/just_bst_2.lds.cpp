/**
 * system/src/bst/just_bst_2.lds.cpp
 *
 * This linker script is used for linking the 2nd-half of BST that is
 * relocated to the DRAM. It is used for debugging purposed only.
 *
 * History:
 *    2005/03/08 - [Charles Chiou] created file
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
ENTRY(start_post_relocate)
SECTIONS
{
	. = AMBOOT_BST_RAM_START;
	.text : {
		postreloc*.o (.text)
		bstnand*.o (.text)
		bstonenand*.o (.text)
		bstnor*.o (.text)
		bstsnor*.o (.text)
		bstemmc*.o (.text)
	}

	. = ALIGN(256);
	__fiodma_desc_addr = .;
	. = . + 0x100;
	. = ALIGN(256);
	__fiodma_rept_addr = .;
}
