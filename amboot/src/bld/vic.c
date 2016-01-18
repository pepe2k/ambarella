/**
 * system/src/bld/vic.c
 *
 * Vector interrupt controller related utilities.
 *
 * History:
 *    2005/07/26 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>

/**
 * Enable IRQ/FIQ interupts.
 */
void enable_interrupts(void)
{
	unsigned long tmp;

	__asm__ __volatile__ (
		"mrs %0, cpsr\n"
		"bic %0, %0, #0x80\n"
		"msr cpsr_c, %0"
		: "=r" (tmp)
		:
		: "memory");
}

/**
 * Disable IRQ/FIQ interrupts.
 */
void disable_interrupts(void)
{
	unsigned long old;
	unsigned long tmp;

	__asm__ __volatile__ (
		"mrs %0, cpsr\n"
		"orr %1, %0, #0xc0\n"
		"msr cpsr_c, %1"
		: "=r" (old), "=r" (tmp)
		:
		: "memory");
}

/**
 * Initialize the VIC.
 */
void vic_init(void)
{
	disable_interrupts();

	/* Set VIC sense and event type for each entry
	 * note: we initialize udc vbus irq type here */
	writel(VIC_SENSE_REG, 0x00000001);
	writel(VIC_BOTHEDGE_REG, 0x00000000);
	writel(VIC_EVENT_REG, 0x00000001);
#if (VIC_INSTANCES >= 2)
	writel(VIC2_SENSE_REG, 0x00000000);
	writel(VIC2_BOTHEDGE_REG, 0x00000000);
	writel(VIC2_EVENT_REG, 0x00000000);
#endif
#if (VIC_INSTANCES >= 3)
	writel(VIC3_SENSE_REG, 0x00000000);
	writel(VIC3_BOTHEDGE_REG, 0x00000000);
	writel(VIC3_EVENT_REG, 0x00000000);
#endif
	/* Disable all IRQ */
	writel(VIC_INT_SEL_REG, 0x00000000);
	writel(VIC_INTEN_REG, 0x00000000);
	writel(VIC_INTEN_CLR_REG, 0xffffffff);
	writel(VIC_EDGE_CLR_REG, 0xffffffff);
#if (VIC_INSTANCES >= 2)
	writel(VIC2_INT_SEL_REG, 0x00000000);
	writel(VIC2_INTEN_REG, 0x00000000);
	writel(VIC2_INTEN_CLR_REG, 0xffffffff);
	writel(VIC2_EDGE_CLR_REG, 0xffffffff);
#endif
#if (VIC_INSTANCES >= 3)
	writel(VIC3_INT_SEL_REG, 0x00000000);
	writel(VIC3_INTEN_REG, 0x00000000);
	writel(VIC3_INTEN_CLR_REG, 0xffffffff);
	writel(VIC3_EDGE_CLR_REG, 0xffffffff);
#endif

	enable_interrupts();
}

/**
 * Configure an IRQ vector on the VIC.
 */
void vic_set_type(u32 line, u32 type)
{
	u32 mask = 0x0, sense = 0x0 , bothedges = 0x0, event = 0x0;
	u32 bit = 0x0;

	if (line < 32) {
		mask = ~(0x1 << line);
		bit = (0x1 << line);

		/* Line directly connected to VIC */
		sense = readl(VIC_SENSE_REG);
		bothedges = readl(VIC_BOTHEDGE_REG);
		event = readl(VIC_EVENT_REG);
#if (VIC_INSTANCES >= 2)
	} else if (line < 64) {
		mask = ~(0x1 << (line - VIC2_INT_VEC_OFFSET) );
		bit = (0x1 << (line - VIC2_INT_VEC_OFFSET) );

		/* Line directly connected to VIC */
		sense = readl(VIC2_SENSE_REG);
		bothedges = readl(VIC2_BOTHEDGE_REG);
		event = readl(VIC2_EVENT_REG);
#endif
#if (VIC_INSTANCES >= 3)
	} else {
		mask = ~(0x1 << (line - VIC3_INT_VEC_OFFSET) );
		bit = (0x1 << (line - VIC3_INT_VEC_OFFSET) );

		/* Line directly connected to VIC */
		sense = readl(VIC3_SENSE_REG);
		bothedges = readl(VIC3_BOTHEDGE_REG);
		event = readl(VIC3_EVENT_REG);
#endif
	}

	switch (type) {
	case VIRQ_RISING_EDGE:
		sense &= mask;
		bothedges &= mask;
		event |= bit;
		break;
	case VIRQ_FALLING_EDGE:
		sense &= mask;
		bothedges &= mask;
		event &= mask;
		break;
	case VIRQ_BOTH_EDGES:
		sense &= mask;
		bothedges |= bit;
		event &= mask;
		break;
	case VIRQ_LEVEL_LOW:
		sense |= bit;
		bothedges &= mask;
		event &= mask;
		break;
	case VIRQ_LEVEL_HIGH:
		sense |= bit;
		bothedges &= mask;
		event |= bit;
		break;
	}

	if (line < 32) {
		writel(VIC_SENSE_REG, sense);
		writel(VIC_BOTHEDGE_REG, bothedges);
		writel(VIC_EVENT_REG, event);
#if (VIC_INSTANCES >= 2)
	} else if (line < 64) {
		writel(VIC2_SENSE_REG, sense);
		writel(VIC2_BOTHEDGE_REG, bothedges);
		writel(VIC2_EVENT_REG, event);
#endif
#if (VIC_INSTANCES >= 3)
	} else {
		writel(VIC3_SENSE_REG, sense);
		writel(VIC3_BOTHEDGE_REG, bothedges);
		writel(VIC3_EVENT_REG, event);
#endif
	}
}

/**
 * Enable a line.
 */
void vic_enable(u32 line)
{
	u32 r;

	if (line < 32) {
		r = readl(VIC_INTEN_REG);
		r |= (0x1 << line);
		writel(VIC_INTEN_REG, r);
#if (VIC_INSTANCES >= 2)
	} else if (line < (VIC2_INT_VEC_OFFSET + 32)) {
		r = readl(VIC2_INTEN_REG);
		r |= (0x1 << (line - VIC2_INT_VEC_OFFSET) );
		writel(VIC2_INTEN_REG, r);
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < (VIC3_INT_VEC_OFFSET + 32)) {
		r = readl(VIC3_INTEN_REG);
		r |= (0x1 << (line - VIC3_INT_VEC_OFFSET) );
		writel(VIC3_INTEN_REG, r);
#endif
	}
}

/**
 * Disable a line.
 */
void vic_disable(u32 line)
{
	if (line < 32) {
		writel(VIC_INTEN_CLR_REG, (0x1 << line));
#if (VIC_INSTANCES >= 2)
	} else if (line < (VIC2_INT_VEC_OFFSET + 32)) {
		writel(VIC2_INTEN_CLR_REG,
					(0x1 << (line - VIC2_INT_VEC_OFFSET)));
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < (VIC3_INT_VEC_OFFSET + 32)) {
		writel(VIC3_INTEN_CLR_REG,
					(0x1 << (line - VIC3_INT_VEC_OFFSET)));
#endif
	}
}

/**
 * Acknowledge an interrupt.
 */
void vic_ackint(u32 line)
{
	if (line < 32) {
		writel(VIC_EDGE_CLR_REG, (0x1 << line));
#if (VIC_INSTANCES >= 2)
	} else if (line < (VIC2_INT_VEC_OFFSET + 32)) {
		writel(VIC2_EDGE_CLR_REG,
					(0x1 << (line - VIC2_INT_VEC_OFFSET)));
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < (VIC3_INT_VEC_OFFSET + 32)) {
		writel(VIC3_EDGE_CLR_REG,
					(0x1 << (line - VIC3_INT_VEC_OFFSET)));
#endif
	}
}

void vic_sw_set(u32 line)
{
	u32 r;

	if (line < 32) {
		r = readl(VIC_SOFTEN_REG);
		r |= (0x1 << line);
		writel(VIC_SOFTEN_REG, r);
#if (VIC_INSTANCES >= 2)
	} else if (line < (VIC2_INT_VEC_OFFSET + 32)) {
		r = readl(VIC2_SOFTEN_REG);
		r |= (0x1 << (line - VIC2_INT_VEC_OFFSET));
		writel(VIC2_SOFTEN_REG, r);
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < (VIC3_INT_VEC_OFFSET + 32)) {
		r = readl(VIC3_SOFTEN_REG);
		r |= (0x1 << (line - VIC3_INT_VEC_OFFSET));
		writel(VIC2_SOFTEN_REG, r);
#endif
	}
}

void vic_sw_clr(u32 line)
{
	u32 r;

	if (line < 32) {
		r = readl(VIC_SOFTEN_CLR_REG);
		r |= (0x1 << line);
		writel(VIC_SOFTEN_CLR_REG, r);
#if (VIC_INSTANCES >= 2)
	} else if (line < (VIC2_INT_VEC_OFFSET + 32)) {
		r = readl(VIC2_SOFTEN_CLR_REG);
		r |= (0x1 << (line - VIC2_INT_VEC_OFFSET));
		writel(VIC2_SOFTEN_CLR_REG, r);
#endif
#if (VIC_INSTANCES >= 3)
	} else if (line < (VIC3_INT_VEC_OFFSET + 32)) {
		r = readl(VIC3_SOFTEN_CLR_REG);
		r |= (0x1 << (line - VIC3_INT_VEC_OFFSET));
		writel(VIC3_SOFTEN_CLR_REG, r);
#endif
	}
}

