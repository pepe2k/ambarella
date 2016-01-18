/**
 * system/src/bld/diag_ir.c
 *
 * History:
 *    2005/09/25 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>

extern void rct_set_ir_pll(void);

/**
 * Run diagnostic on IR: continuously display the current value.
 */
void diag_ir(void)
{
	int i;
	u32 data;

	/* Set IR sampling frequency */
	rct_set_ir_pll();

	/* config IR GPIO */
	gpio_config_hw(IR_IN);

	uart_putstr("running IR diagnostics...\r\n");
	uart_putstr("press any key to quit!\r\n");

	writel(IR_CONTROL_REG, IR_CONTROL_RESET);
	writel(IR_CONTROL_REG, IR_CONTROL_ENB);

	for (i = 0; ; i++) {
		if (uart_poll())
			break;

		while (readl(IR_STATUS_REG) == 0x0);
		data = readl(IR_DATA_REG);
		uart_putdec(data);
		uart_putchar(' ');
		if (i == 8) {
			uart_putstr("\r\n");
			i = 0;
		}
	}

	writel(IR_CONTROL_REG, 0x0);

	uart_putstr("\r\ndone!\r\n");
}
