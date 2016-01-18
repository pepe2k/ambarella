/**
 * system/src/bld/diag_adc.c
 *
 * History:
 *    2005/09/25 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <ambhw.h>
#include <hal/hal.h>

/*
 * only A5 support continue mode, A1/A2/A3 does not support
 * #undef ADC_ONE_SHOT	-- continue mode
 * #define ADC_ONE_SHOT -- single mode
 */
#define ADC_ONE_SHOT
/**
 * Run diagnostic on ADC: continuously display the current value.
 */
#if (CHIP_REV == S2)
void amboot_adc_set_slot_ctrl(u8 slot_id, u32 slot_value)
{
	switch (slot_id) {
	case 0:
		writel(ADC_SLOT_CTRL_0_REG, slot_value);
		break;
	case 1:
		writel(ADC_SLOT_CTRL_1_REG, slot_value);
		break;
	case 2:
		writel(ADC_SLOT_CTRL_2_REG, slot_value);
		break;
	case 3:
		writel(ADC_SLOT_CTRL_3_REG, slot_value);
		break;
	case 4:
		writel(ADC_SLOT_CTRL_4_REG, slot_value);
		break;
	case 5:
		writel(ADC_SLOT_CTRL_5_REG, slot_value);
		break;
	case 6:
		writel(ADC_SLOT_CTRL_6_REG, slot_value);
		break;
	case 7:
		writel(ADC_SLOT_CTRL_7_REG, slot_value);
		break;
	}
}

void amboot_adc_set_config(void)
{
	int i = 0;
	int slot_num_reg = 0;

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_ENABLE));

	timer_reset_count(TIMER1_ID);
	while (timer_get_count(TIMER1_ID) <= 2); /* wait 200 milisecond */

	writel(ADC_SLOT_NUM_REG, 0);//set slot number=1
	writel(ADC_SLOT_PERIOD_REG, 60);//set slot period 60
	for(i=0;i <= slot_num_reg; i++)
		amboot_adc_set_slot_ctrl(i, 0xfff);//set slot 0 ctrl 0xfff
}
#endif

void diag_adc(void)
{
	u32 data[ADC_NUM_CHANNELS] = {0};
	u32 old[ADC_NUM_CHANNELS]  = {0};
	u32 i, equal;
	int c=0;

#if defined(ADC16_CTRL_REG)
	writel(ADC16_CTRL_REG, 0x0);
#endif
	uart_putstr("running ADC diagnostics...\r\n");
	uart_putstr("press space key to quit!\r\n");

#if (ADC_NUM_CHANNELS == 8)
	/* SCALER_ADC_REG (default=4) */
	/* clk_au = 27MHz/4 */
//	writel(0x7017009c, 0x0010004);
	/* clk_au = 27MHz/2 */
	writel(0x7017009c, 0x0010002);

	/* ADC Analog (lowest power) */
	writel(0x70170198, 0x031cff);
#endif
#if (ADC_NUM_CHANNELS == 10)
	amb_set_adc_clock_frequency (HAL_BASE_VP, 6000000);
#endif
	/* ADC interface control R/W */

#if (CHIP_REV != A5S) && (CHIP_REV != S2)
	/* ADC reset */
	writel(ADC_RESET_REG, 0x1);
#endif
	/* ADC enable */
#if (CHIP_REV != S2)
	writel(ADC_ENABLE_REG, 0x1);
#else
	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_CLEAR));
	amboot_adc_set_config();
#endif

#if (ADC_NUM_CHANNELS == 8)
	/* ADC data latch, 256 cycles -> previously 4096 */
	writel(0x701A000C, 0xff);

	/* decimation R = 16 */
	writel(0x701A0008, 0xf);
#endif

	/* clear */
#if (CHIP_REV != S2)
	writel(ADC_CONTROL_REG, 0x0);
#endif
	writel(ADC_DATA0_REG, 0x0);
	writel(ADC_DATA1_REG, 0x0);
	writel(ADC_DATA2_REG, 0x0);
	writel(ADC_DATA3_REG, 0x0);
#if (ADC_NUM_CHANNELS >= 6)
	writel(ADC_DATA4_REG, 0x0);
	writel(ADC_DATA5_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 8)
	writel(ADC_DATA6_REG, 0x0);
	writel(ADC_DATA7_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 10)
	writel(ADC_DATA8_REG, 0x0);
	writel(ADC_DATA9_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 12)
	writel(ADC_DATA10_REG, 0x0);
	writel(ADC_DATA11_REG, 0x0);
#endif

#ifndef ADC_ONE_SHOT
	/* ADC control mode, continuous, start conversion */
	writel(ADC_CONTROL_REG,
	       (readl(ADC_CONTROL_REG) | ADC_CONTROL_MODE | ADC_CONTROL_START));
	while ((readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);
#else
	/* ADC control mode, single */
	writel(ADC_CONTROL_REG,
	       (readl(ADC_CONTROL_REG) & (~ADC_CONTROL_MODE)));
#endif

	for (;;) {
		if (uart_poll()){
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
			break;
			}
		}
#ifdef ADC_ONE_SHOT
		/* ADC control mode, single, start conversion */
		writel(ADC_CONTROL_REG,
		       (readl(ADC_CONTROL_REG) | ADC_CONTROL_START));
#if (CHIP_REV != S2)
		while ((readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);
#else
		while ((readl(ADC_STATUS_REG) & ADC_CONTROL_STATUS) == 0x0);
#endif
#endif

		/* delay 200 ms for stability */
		timer_reset_count(TIMER1_ID);
		while (timer_get_count(TIMER1_ID) <= 2); /* wait 200 milisecond */

		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			old[i] = data[i];
		}

		/* ADC interface Read from Channel 0, 1, 2, 3 */
#if (ADC_NUM_CHANNELS == 8)
		data[0] = (readl(ADC_DATA0_REG) + 0x8000) & 0xffff;
		data[1] = (readl(ADC_DATA1_REG) + 0x8000) & 0xffff;
		data[2] = (readl(ADC_DATA2_REG) + 0x8000) & 0xffff;
		data[3] = (readl(ADC_DATA3_REG) + 0x8000) & 0xffff;
		data[4] = (readl(ADC_DATA4_REG) + 0x8000) & 0xffff;
		data[5] = (readl(ADC_DATA5_REG) + 0x8000) & 0xffff;
		data[6] = (readl(ADC_DATA6_REG) + 0x8000) & 0xffff;
		data[7] = (readl(ADC_DATA7_REG) + 0x8000) & 0xffff;
#else
		data[0] = readl(ADC_DATA0_REG);
		data[1] = readl(ADC_DATA1_REG);
		data[2] = readl(ADC_DATA2_REG);
		data[3] = readl(ADC_DATA3_REG);
#if (ADC_NUM_CHANNELS >= 6)
		data[4] = readl(ADC_DATA4_REG);
		data[5] = readl(ADC_DATA5_REG);
#endif
#if (ADC_NUM_CHANNELS >= 10)
		data[6] = readl(ADC_DATA6_REG);
		data[7] = readl(ADC_DATA7_REG);
		data[8] = readl(ADC_DATA8_REG);
		data[9] = readl(ADC_DATA9_REG);
#endif
#if (ADC_NUM_CHANNELS >= 12)
		data[10] = readl(ADC_DATA10_REG);
		data[11] = readl(ADC_DATA11_REG);
#endif
#endif

		equal = 1;
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			if (data[i] != old[i]) {
				equal = 0;
				break;
			}
		}

		if (equal) {
			continue;
		}

		uart_putstr("[");
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			uart_putdec(data[i]);
			uart_putstr("] [");
		}
		uart_putdec(i);
		uart_putstr("]          \r");
	}

#ifndef ADC_ONE_SHOT
	/* stop conversion */
	writel(ADC_CONTROL_REG, 0x0);
#endif

#if (CHIP_REV != A5S) && (CHIP_REV != S2)
	writel(ADC_RESET_REG, 0x1);
#endif

#if (CHIP_REV != S2)
	writel(ADC_ENABLE_REG, 0x0);
#else
	writel(ADC_CONTROL_REG,
	       (readl(ADC_CONTROL_REG) & (~ADC_CONTROL_ENABLE)));
#endif

#if defined(ADC16_CTRL_REG)
	writel(ADC16_CTRL_REG, 0x2);
#endif
	uart_putstr("\r\ndone!\r\n");
}

/**
 * Test adc gyro mode.
 */
void diag_adc_gyro(void)
{
	u32 data[8] = {0};
	u32 old[8] = {0};

#if defined(ADC16_CTRL_REG)
	writel(ADC16_CTRL_REG, 0x0);
#endif
	uart_putstr("running ADC GYRO mode diagnostics...\r\n");
	uart_putstr("press any key to quit!\r\n");

#if (ADC_NUM_CHANNELS == 8)
	/* SCALER_ADC_REG (default=4) */
	/* clk_au = 27MHz/4 */
//	writel(0x7017009c, 0x0010004);
	/* clk_au = 27MHz/2 */
	writel(0x7017009c, 0x0010002);

	/* ADC Analog (lowest power) */
	writel(0x70170198, 0x031cff);
#endif

	/* ADC interface control R/W */

#if (CHIP_REV != A5S)
	/* ADC reset */
	writel(ADC_RESET_REG, 0x1);
#endif
	/* ADC enable */
	writel(ADC_CONTROL_REG, ADC_CONTROL_START);


#if (ADC_NUM_CHANNELS == 8)
	/* ADC data latch, 256 cycles -> previously 4096 */
	writel(0x701A000C, 0xff);

	/* decimation R = 16 */
	writel(0x701A0008, 0xf);
#endif

	/* delay 200 ms for stability */
	timer_reset_count(TIMER1_ID);
	while (timer_get_count(TIMER1_ID) <= 2); /* wait 200 milisecond */

	/* ADC control mode, continuous, start conversion */
	writel(ADC_CONTROL_REG,
	       (readl(ADC_CONTROL_REG) | ADC_CONTROL_GYRO_SAMPLE_MODE));

	while ((readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);

	for (;;) {
		if (uart_poll())
			break;

		/* delay 200 ms for stability */
		timer_reset_count(TIMER1_ID);
		while (timer_get_count(TIMER1_ID) <= 2); /* wait 200 milisecond */

		old[0] = data[0];
		old[1] = data[1];
		old[2] = data[2];
		old[3] = data[3];
		old[4] = data[4];
		old[5] = data[5];
		old[6] = data[6];
		old[7] = data[7];

		data[0] = readl(ADC_DATA4_SAMPLE0_REG);
		data[1] = readl(ADC_DATA4_SAMPLE1_REG);
		data[2] = readl(ADC_DATA4_SAMPLE2_REG);
		data[3] = readl(ADC_DATA4_SAMPLE3_REG);
		data[4] = readl(ADC_DATA5_SAMPLE0_REG);
		data[5] = readl(ADC_DATA5_SAMPLE1_REG);
		data[6] = readl(ADC_DATA5_SAMPLE2_REG);
		data[7] = readl(ADC_DATA5_SAMPLE3_REG);

		if (data[0] == old[0] &&
		    data[1] == old[1] &&
		    data[2] == old[2] &&
		    data[3] == old[3] &&
		    data[4] == old[4] &&
		    data[5] == old[5] &&
		    data[6] == old[6] &&
		    data[7] == old[7])
			continue;

		uart_putstr("[");
		uart_putdec(data[0]);
		uart_putstr("] [");
		uart_putdec(data[1]);
		uart_putstr("] [");
		uart_putdec(data[2]);
		uart_putstr("] [");
		uart_putdec(data[3]);

		uart_putstr("] [");
		uart_putdec(data[4]);
		uart_putstr("] [");
		uart_putdec(data[5]);
		uart_putstr("] [");
		uart_putdec(data[6]);
		uart_putstr("] [");
		uart_putdec(data[7]);

		uart_putstr("]          \r");
	}

	/* stop conversion */
	writel(ADC_CONTROL_REG, 0x0);

#if (CHIP_REV != A5S)
	writel(ADC_RESET_REG, 0x1);
#endif

#if defined(ADC16_CTRL_REG)
	writel(ADC16_CTRL_REG, 0x2);
#endif
	uart_putstr("\r\ndone!\r\n");
}
