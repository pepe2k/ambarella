/**
 * system/include/hotboot.h
 *
 * History:
 *    2008/10/30 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __HOTBOOT_H__
#define __HOTBOOT_H__

/*
 * HotBoot is a method where the chip is reset while memory content before
 * the reset is retained. We write special pattern into pre-defined memory
 * locations to communicate to the boot loader (AMBoot) to alter its loading
 * behavior.
 *
 * +---------------------+
 * | MAGIC0              |
 * +---------------------+
 * | Bit Patterns        |
 * +---------------------+
 * | Rserved             |
 * +---------------------+
 * | MAGIC1              |
 * +---------------------+
 *
 * At the moment, the hotboot area is 4 words in length.
 */

#define HOTBOOT_MEM_ADDR	(AMBOOT_BLD_RAM_START + 0x7fff0)
#define HOTBOOT_MAGIC0		0x14cd78a0
#define HOTBOOT_MAGIC1		0x319837fb

/* The following bit fields is a mask for "active" loading partitions */

#define HOTBOOT_PBA	0x01
#define HOTBOOT_PRI	0x02
#define HOTBOOT_SEC	0x04
#define HOTBOOT_BAK	0x08
#define HOTBOOT_RMD	0x10
#define HOTBOOT_ROM	0x20
#define HOTBOOT_DSP	0x40
#define HOTBOOT_USB	0x80

#endif
