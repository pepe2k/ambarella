/**
 * system/src/bld/irq_timer.S
 *
 * History:
 *    2005/08/15 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

.globl	irq_timer1
irq_timer1:
	bl	timer1_handler
	b	irq_done		@ finished processing timer interrupt

.globl	irq_timer2
irq_timer2:
	bl	timer2_handler
	b	irq_done		@ finished processing timer interrupt

.globl	irq_timer3
irq_timer3:
	bl	timer3_handler
	b	irq_done		@ finished processing timer interrupt
