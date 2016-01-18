/**
 * system/src/bld/xmodem.c
 *
 * History:
 *    2005/08/18 - [Charles Chiou] created file
 *    2010/07/20 - [Allen Wang] replacd with source code from FreeBSD 
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
 
/*-
 * Copyright (c) 2006 M. Warner Losh.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This software is derived from software provide by Kwikbyte who specifically
 * disclaimed copyright on the code.  This version of xmodem has been nearly
 * completely rewritten, but the CRC is from the original.
 *
 * $FreeBSD: src/sys/boot/arm/at91/libat91/xmodem.c,v 1.1 2006/04/19 17:16:49 imp Exp $
 */

#include <bldfunc.h>

#define MAX_NUM_RESEND	10

/* Line control codes */
#define SOH	0x01	/* start of header */
#define ACK	0x06	/* Acknowledge */
#define NAK	0x15	/* Negative acknowledge */
#define CAN	0x18	/* Cancel */
#define EOT	0x04	/* end of text */
#define STX 	0x02

#define TO	10

static int getc_with_timeout(unsigned int timeout) {
        char buf[1];
        int n;

        n = uart_getblock(buf, 1, timeout);

        if (n == 1)
                return (buf[0] & 0xff);
        else
                return -1;
}

/*
 * int GetRecord(char *)
 *  This private function receives a x-modem record to the pointer and
 * returns non-zero on success.
 */
static int GetRecord(char blocknum, char *dest, int pkt_sz)
{
	int		size;
	int		ch;
	unsigned	chk, j;
	int 		bn, bn1s;
	static int	resend = 0;

	chk = 0;

	if ((bn = getc_with_timeout(TO)) == -1)
		goto timeout;
	
	if ((bn1s = getc_with_timeout(TO)) == -1) 
		goto timeout;
		
	if (bn != (~bn1s & 0xff))
		goto err;
	
	for (size = 0; size < pkt_sz; ++size) {
		if ((ch = getc_with_timeout(TO)) == -1)
			goto timeout;
		chk = chk ^ ch << 8;
		for (j = 0; j < 8; ++j) {
			if (chk & 0x8000)
				chk = chk << 1 ^ 0x1021;
			else
				chk = chk << 1;
		}
		*dest++ = ch;		
	}
		
	chk &= 0xFFFF;

	if (((ch = getc_with_timeout(TO)) == -1) || 
	    ((ch & 0xff) != ((chk >> 8) & 0xFF)))
		goto err;
	if (((ch = getc_with_timeout(TO)) == -1) || 
	    ((ch & 0xff) != (chk & 0xFF)))
		goto err;
		
	if (bn == ((blocknum - 1) & 0xff)) {
		goto nextpkt;
	} else if (bn != (blocknum & 0xff)) {
		goto err;
	}
	
 			
	uart_putchar(ACK); /* Successfully received */
	return (size);
	
err:
timeout:
	resend++;
	
	if (resend >= MAX_NUM_RESEND) {
		uart_putchar(CAN);
		return -1; /* Cancel the transaction */
	}
		
	uart_flush_input();
	uart_putchar(NAK);

nextpkt:
	uart_putchar(ACK);
	
	return (0); /* Request packet resending or 
	               withdraw the duplicate packet */
}

/*
 * int xmodem_rx(char *)
 *  This global function receives a x-modem transmission consisting of
 * (potentially) several blocks.  Returns the number of bytes received or
 * -1 on error.
 */
int xmodem_recv(char *dest, int buf_len)
{
	int	starting, ch;
	char	packetNumber, *startAddress = dest;
	int	packet_size = 0;

	starting 	= 1;
	packetNumber 	= 1;
		
	uart_flush_input();
	
	while (1) {
		if (starting) {
			uart_putchar('C');
		}
		
		if (((ch = getc_with_timeout(1)) == -1) || 
		     (ch != SOH && ch != EOT && ch != STX))
			continue;
		packet_size = (ch == SOH) ? 128 : 1024; 
						
		if (ch == EOT) {
			uart_putchar(ACK);
			return (dest - startAddress);
		}
		starting = 0;
		// Xmodem packets: SOH PKT# ~PKT# 128-bytes CRC16
		ch = GetRecord(packetNumber, dest, packet_size);
		
		if (ch > 0) {
			packetNumber++;
			dest += ch;
		} else if (ch < 0)
			return (-1);			
	}

	// the loop above should return in all cases
	return (-1);
}
