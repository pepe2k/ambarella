/**
 * system/src/comsvc/hotboot.c
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

#include <basedef.h>
#include <ambhw.h>
#include <kutil.h>
#include <hotboot.h>

void hotboot(u32 pattern)
{
	arm_dis_int();
	writel(HOTBOOT_MEM_ADDR + 0x0, HOTBOOT_MAGIC0);
	writel(HOTBOOT_MEM_ADDR + 0x4, pattern);
	writel(HOTBOOT_MEM_ADDR + 0x8, 0x0);
	writel(HOTBOOT_MEM_ADDR + 0xc, HOTBOOT_MAGIC1);
	rct_reset_chip();
}
