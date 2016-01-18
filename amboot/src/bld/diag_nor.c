/**
 * system/src/bld/diag_nand.c
 *
 * History:
 *    2005/09/21 - [Chien-Yang Chen] created file
 *    2007/10/11 - [Charles Chiou] Added PBA partition
 *
 * Copyright (C) 2004-2007, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#define __FLDRV_IMPL__
#include <fio/firmfl.h>

#if !defined(CONFIG_NOR_NONE)

void diag_nor_verify(int argc, char *argv[])
{
	putstr("Not implement!\r\n");
}

#else  /* !CONFIG_NOR_NONE */

void diag_nor_verify(int argc, char *argv[])
{
	putstr("NOR disabled!\r\n");
}

#endif  /* CONFIG_NOR_NONE */

