/**
 * system/src/bld/timer.c
 *
 * History:
 *    2005/08/01 - [Charles Chiou] - created file
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

unsigned int timer1_count = 0;
unsigned int timer2_count = 0;
unsigned int timer3_count = 0;

/**
 * Handler of timer 1 IRQ.
 */
void timer1_handler(void)
{
	timer1_count++;
	vic_ackint(TIMER1_INT_VEC);
}

/**
 * Handler of timer 2 IRQ.
 */
void timer2_handler(void)
{
	timer2_count++;
	vic_ackint(TIMER2_INT_VEC);
}

/**
 * Handler of timer 3 IRQ.
 */
void timer3_handler(void)
{
	timer3_count++;
	vic_ackint(TIMER3_INT_VEC);
}

/**
 * Initialize the timer to start ticking.
 */
void timer_init(void)
{
	u32 apb_freq = get_apb_bus_freq_hz();
	unsigned int cnt;

	timer1_count = 0;
	timer2_count = 0;
	timer3_count = 0;

	/* Reset all timers */
	writel(TIMER_CTR_REG, 0x0);

	/* Setup VIC */
	vic_set_type(TIMER1_INT_VEC, VIRQ_RISING_EDGE);
	vic_enable(TIMER1_INT_VEC);

	vic_set_type(TIMER2_INT_VEC, VIRQ_RISING_EDGE);
	vic_enable(TIMER2_INT_VEC);

	/* Reset timer control register */
	writel(TIMER_CTR_REG, 0x0);

	/* Setup timer 1: 100 millisecond */
	cnt = apb_freq / 10;
	writel(TIMER1_STATUS_REG, cnt);
	writel(TIMER1_RELOAD_REG, cnt);
	writel(TIMER1_MATCH1_REG, 0x0);
	writel(TIMER1_MATCH2_REG, 0x0);

	/* Setup timer 2: 1 millisecond */
	cnt = apb_freq / 1000;
	writel(TIMER2_STATUS_REG, cnt);
	writel(TIMER2_RELOAD_REG, cnt);
	writel(TIMER2_MATCH1_REG, 0x0);
	writel(TIMER2_MATCH2_REG, 0x0);

	/* Setup timer 3: only used for polling */
	writel(TIMER3_STATUS_REG, 0xffffffff);
	writel(TIMER3_RELOAD_REG, 0xffffffff);
	writel(TIMER3_MATCH1_REG, 0x0);
	writel(TIMER3_MATCH2_REG, 0x0);

	/* Start timer */
	writel(TIMER_CTR_REG, 0x5);
}

void timer_enable(int tmr_id)
{
	u32 val;

	val = readl(TIMER_CTR_REG);

	switch (tmr_id) {
	case TIMER1_ID:
		writel(TIMER_CTR_REG, val | 0x5);
		break;
	case TIMER2_ID:
		writel(TIMER_CTR_REG, val | 0x50);
		break;
	case TIMER3_ID:
		writel(TIMER_CTR_REG, val | 0x500);
		break;
	default:
		K_ASSERT(0);
	}
}

void timer_disable(int tmr_id)
{
	u32 val;

	val = readl(TIMER_CTR_REG);

	switch (tmr_id) {
	case TIMER1_ID:
		writel(TIMER_CTR_REG, val & (~0xf));
		break;
	case TIMER2_ID:
		writel(TIMER_CTR_REG, val & (~0xf0));
		break;
	case TIMER3_ID:
		writel(TIMER_CTR_REG, val & (~0xf00));
		writel(TIMER3_STATUS_REG, 0xffffffff);
		writel(TIMER3_RELOAD_REG, 0xffffffff);
		break;
	default:
		K_ASSERT(0);
	}
}

/**
 * Reset the count.
 */
void timer_reset_count(int tmr_id)
{
	switch (tmr_id) {
	case TIMER1_ID:
		timer1_count = 0;
		break;
	case TIMER2_ID:
		timer2_count = 0;
		break;
	case TIMER3_ID:
		timer3_count = 0;
		break;
	default:
		K_ASSERT(0);
	}
}

/**
 * Get current count.
 */
u32 timer_get_count(int tmr_id)
{
	switch (tmr_id) {
	case TIMER1_ID:
		return timer1_count;
	case TIMER2_ID:
		return timer2_count;
	case TIMER3_ID:
		return timer3_count;
	default:
		K_ASSERT(0);
	}
}

u32 timer_get_tick(int tmr_id)
{
	switch (tmr_id) {
	case TIMER1_ID:
		return readl(TIMER1_STATUS_REG);
	case TIMER2_ID:
		return readl(TIMER2_STATUS_REG);
	case TIMER3_ID:
		return readl(TIMER3_STATUS_REG);
	default:
		K_ASSERT(0);
	}
}

u32 timer_tick2ms(u32 s_tck, u32 e_tck)
{
	u32 apb_freq = get_apb_bus_freq_hz();
	return (s_tck - e_tck) / (apb_freq / 1000);
}

void timer_dly_ms(int tmr_id, u32 dly_tim)
{
	u32 cur_tim;
	u32 s_tck, e_tck;

	timer_disable(tmr_id);
	timer_enable(tmr_id);
	s_tck = timer_get_tick(tmr_id);
	while (1) {
		e_tck = timer_get_tick(tmr_id);
		cur_tim = timer_tick2ms(s_tck, e_tck);
		if (cur_tim >= dly_tim)
			break;
	}
}

