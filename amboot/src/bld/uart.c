/**
 * system/src/bld/uart.c
 *
 * History:
 *    2005/01/27 - [Charles Chiou] created file
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

/**
 * Initialize the UART.
 */
void uart_init(void)
{
#if	defined(AMBOOT_UART_19200)
	static const int baud = 19200;
#elif	defined(AMBOOT_UART_38400)
	static const int baud = 38400;
#elif	defined(AMBOOT_UART_57600)
	static const int baud = 57600;
#else
	static const int baud = 115200;
#endif
	u32 clk;
	u16 dl;

	rct_set_uart_pll();
	writeb(UART0_REG(UART_SRR_OFFSET), 0x00);

#if	defined(__FPGA__)
	clk = get_apb_bus_freq_hz();
#else
	clk = get_uart_freq_hz();
#endif

	dl = clk * 10 / baud / 16;
	if (dl % 10 >= 5)
		dl = (dl / 10) + 1;
	else
		dl = (dl / 10);

	writeb(UART0_LC_REG, UART_LC_DLAB);
	writeb(UART0_DLL_REG, dl & 0xff);
	writeb(UART0_DLH_REG, dl >> 8);
	writeb(UART0_LC_REG, UART_LC_8N1);
}

/**
 * Put a character out to the UART controller.
 */
void uart_putchar(char c)
{
	while (!(readb(UART0_LS_REG) & UART_LS_TEMT));
	writeb(UART0_TH_REG, c);
}

/**
 * Get a character from the UART controller.
 */
void uart_getchar(char *c)
{
	while (!(readb(UART0_LS_REG) & UART_LS_DR));
	*c = readb(UART0_RB_REG);
}

/**
 * Formated output with a hexadecimal number.
 */
void uart_puthex(u32 h)
{
	int i;
	char c;

	for (i = 7; i >= 0; i--) {
		c = (char) ((h >> (i * 4)) & 0xf);

		if (c > 9)
			c += ('a' - 10);
		else
			c += '0';
		uart_putchar(c);
	}
}

/**
 * show one byte.
 */
void uart_putbyte(u32 h)
{
	int i;
	char c;

	for (i = 1; i >= 0; i--) {
		c = (char) ((h >> (i * 4)) & 0xf);

		if (c > 9)
			c += ('a' - 10);
		else
			c += '0';
		uart_putchar(c);
	}
}

/**
 * Formatted output with a decimal number.
 */
void uart_putdec(u32 d)
{
	int leading_zero = 1;
	u32 divisor, result, remainder;

	if (d > 1000000000) {
		uart_putchar('?');  /* FIXME! */
		return;
	}

	remainder = d;
	for (divisor = 1000000000; divisor > 0; divisor /= 10) {
		result = 0;
		if (divisor <= remainder) {
			for (;(result * divisor) <= remainder; result++);
			result--;
			remainder -= (result * divisor);
		}

		if (result != 0 || divisor == 1)
			leading_zero = 0;
		if (leading_zero == 0)
			uart_putchar((char) (result + '0'));
	}
}

/**
 * Output a NULL-terminated string.
 */
int uart_putstr(const char *str)
{
	int rval = 0;


	if (str == ((void *) 0x0))
		return rval;

	while (*str != '\0') {
		uart_putchar(*str);
		str++;
		rval++;
	}

	return rval;
}

#if (UART_INSTANCES >= 2)
void uart1_init(void)
{

	static const int baud = 115200;
	u32 clk;
	u16 dl;

	rct_set_uart_pll();
	writeb(UART0_REG(UART_SRR_OFFSET), 0x00);

#if	defined(__FPGA__)
	clk = get_apb_bus_freq_hz();
#else
	clk = get_uart_freq_hz();
#endif

	dl = clk * 10 / baud / 16;
	if (dl % 10 >= 5)
		dl = (dl / 10) + 1;
	else
		dl = (dl / 10);

	writeb(UART1_LC_REG, UART_LC_DLAB);
	writeb(UART1_DLL_REG, dl & 0xff);
	writeb(UART1_DLH_REG, dl >> 8);
	writeb(UART1_LC_REG, UART_LC_8N1);
}

void uart1_putchar(char c)
{
	while (!(readb(UART1_LS_REG) & UART_LS_TEMT));
	writeb(UART1_TH_REG, c);
}

int uart1_putstr(const char *str)
{
	int rval = 0;


	if (str == ((void *) 0x0))
		return rval;

	while (*str != '\0') {
		uart1_putchar(*str);
		str++;
		rval++;
	}

	return rval;
}
#else
void uart1_init(void)
{}

void uart1_putchar(char c)
{}

int uart1_putstr(const char *str)
{}
#endif

/**
 * Poll to see if a character is available.
 */
int uart_poll(void) {
	/* Check for data ready bit */
	return readl(UART0_LS_REG) & UART_LS_DR;
}

/**
 * Read a character in UART controller.
 */
int uart_read(void) {
	if (readl(UART0_LS_REG) & UART_LS_DR)
		return readl(UART0_RB_REG);
	else
		return -1;
}

/**
 * Flush UART input.
 */
void uart_flush_input(void)
{
	unsigned int dump;
	while (readl(UART0_LS_REG) & UART_LS_DR) {
		dump = readl(UART0_RB_REG);
	}
}

/**
 * Wait for escape character(s): line feed, carriage return, escape.
 */
int uart_wait_escape(int time)
{
	int c;
	u32 t_now;

	timer_reset_count(TIMER2_ID);
	timer_enable(TIMER2_ID);

	for (;;) {
		t_now = timer_get_count(TIMER2_ID);
		/* wait 2 milisecond */
		if (t_now >= time)
			break;

		if (uart_poll()) {
			c = uart_read();
			if (c == 0x0a || c == 0x0d || c == 0x1b) {
				timer_disable(TIMER2_ID);
				return 1;
			}
		}
	}

	timer_disable(TIMER2_ID);

	return 0;
}


/**
 * Get a string from UART.
 */
int uart_getstr(char *str, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;
	u32 t_now;
	int skipnl = 1;

	timer_reset_count(TIMER1_ID);

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = timer_get_count(TIMER1_ID);
			if ((timeout > 0) && (t_now > timeout * 10)) {
				str[i++] = '\0';
				return nread;
			}
		}

		c = uart_read();
		if (c < 0) {
			str[i++] = '\0';
			return c;
		}

		if ((skipnl == 1) && (c != '\r') && (c != '\n'))
			skipnl = 0;

		if (skipnl == 0) {
			if ((c == '\r') || (c == '\n')) {
				str[i++] = '\0';
				return nread;
			} else {
				str[i++] = (char) c;
				nread++;
			}
		}
	}

	return nread;
}

/**
 * Get command from UART.
 */
int uart_getcmd(char *cmd, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;
	u32 t_now;

	timer_reset_count(TIMER1_ID);

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = timer_get_count(TIMER1_ID);

			if ((timeout > 0) && (t_now > timeout * 10)) {
				cmd[i++] = '\0';
				return nread;
			}
		}

		c = uart_read();
		if (c < 0) {
			cmd[i++] = '\0';
			uart_putchar('\n');
			return c;
		}

		if ((c == '\r') || (c == '\n')) {
			cmd[i++] = '\0';

			uart_putstr("\r\n");
			return nread;
		} else if (c == 0x08) {
			if(i > 0) {
				i--;
				nread--;
				uart_putstr("\b \b");
			}
		} else {
			cmd[i++] = c;
			nread++;

			uart_putchar(c);
		}
	}

	return nread;
}

/**
 * Get a block of bytes from UART.
 */
int uart_getblock(char *buf, int n, int timeout)
{
	int c;
	int i;
	int nread;
	int maxread = n;
	u32 t_now;

	timer_reset_count(TIMER1_ID);

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {
			t_now = timer_get_count(TIMER1_ID);
			if ((timeout > 0) && (t_now > timeout * 10)) {
				return nread;
			}
		}

		c = uart_read();
		if (c < 0)
			return c;

		buf[i++] = (char) c;
		nread++;
	}

	return nread;
}
