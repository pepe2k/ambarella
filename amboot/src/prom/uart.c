/**
 * system/src/prom/uart.c
 *
 * History:
 *    2008/05/23 - [Dragon Chiang] created file
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>

#ifdef __DEBUG_BUILD__

#if (CHIP_REV == A5S) || (CHIP_REV == A7) || (CHIP_REV == I1)
#define PROM_REF_CLK_FREQ	24000000
#define	UART_SCALER		13
#else
#define	UART_SCALER		2
#define PROM_REF_CLK_FREQ	27000000
#endif

/**
 * Initialize the UART.
 */
void uart_init(void)
{
	static const int baud = 115200;
	u32 clk;
	u16 dl;

	/* Initiaize the UART */
	writel(CG_UART_REG, UART_SCALER);
	clk = PROM_REF_CLK_FREQ / readl(CG_UART_REG);
	dl = clk * 10 / baud / 16;
	if (dl % 10 >= 5)
		dl = (dl / 10) + 1;
	else
		dl = (dl / 10);

	/* Set baud rate */
	writeb(UART0_LC_REG, UART_LC_DLAB);
	writeb(UART0_DLL_REG, dl & 0xff);
	writeb(UART0_DLH_REG, dl >> 8);
	writeb(UART0_LC_REG, UART_LC_8N1);

	/* Initialize UART GPIO */
	writel(GPIO0_AFSEL_REG, (readl(GPIO0_AFSEL_REG) | 0x0000c000));
	writel(GPIO0_ENABLE_REG, (readl(GPIO0_ENABLE_REG) | 0x0000c003));
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
		uart_putchar('?');
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

/**
 * Poll to see if a character is available.
 */
int uart_poll(void)
{
	/* Check for data ready bit */
	return readl(UART0_LS_REG) & UART_LS_DR;
}

/**
 * Read a character in UART controller.
 */
int uart_read(void)
{
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
	while (readl(UART0_LS_REG) & UART_LS_DR)
		readl(UART0_RB_REG);
}

/**
 * Wait for escape character(s): line feed, carriage return, escape.
 */
int uart_wait_escape(void)
{
	int c;

	for (;;) {
		if (uart_poll()) {
			c = uart_read();
			if (c == 0x0a || c == 0x0d || c == 0x1b) {
				return 1;
			}
		}
	}

	return 0;
}


/**
 * Get a string from UART.
 */
int uart_getstr(char *str, int n)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;
	int skipnl = 1;

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {};

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
int uart_getcmd(char *cmd, int n)
{
	int c;
	int i;
	int nread;
	int maxread = n - 1;

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {};

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
int uart_getblock(char *buf, int n)
{
	int c, i, nread;
	int maxread = n;

	for (nread = 0, i = 0; nread < maxread;) {
		while (uart_poll() == 0) {};

		c = uart_read();
		if (c < 0)
			return c;

		buf[i++] = (char) c;
		nread++;
	}

	return nread;
}
#endif
