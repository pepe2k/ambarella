/**
 * system/src/bld/bldnet.c
 *
 * Networking support in AMBoot.
 *
 * History:
 *    2006/10/18 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID     
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <bldfunc.h>
#include <fio/firmfl.h>
#include <netdev/tftp.h>

#if (ETH_INSTANCES >= 1)

extern void eth_init(bld_net_if_t *, bld_eth_dev_t *, u8 *);
extern void eth_filter_source_address(struct bld_eth_dev_s *dev, u8 *hwaddr);
extern void eth_pass_source_address(struct bld_eth_dev_s *dev);

bld_net_if_t net_if __attribute__ ((aligned(32)));

u16 htons(u16 n)
{
	return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

u16 ntohs(u16 n)
{
	return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

u32 htonl(u32 n)
{
	return ((n & 0xff) << 24) |
		((n & 0xff00) << 8) | 
		((n & 0xff0000) >> 8) |
		((n & 0xff000000) >> 24);
}

u32 ntohl(u32 n)
{
	return ((n & 0xff) << 24) |
		((n & 0xff00) << 8) | 
		((n & 0xff0000) >> 8) |
		((n & 0xff000000) >> 24);
}

/**
 * Compute check-sum for IP header.
 */
static u16 ip_chksum(void *_buf, int len)
{
	u16 *buf = (u16 *) _buf;
	u32 sum = 0;
	len >>= 1;
	while (len--) {
		sum += *(buf++);
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	return ((~sum) & 0x0000ffff);
}

/**
 * Compute check-sum for UDP header.
 */
static u16 udp_chksum(udphdr_t *udphdr)
{
	u32 sum = 0x0;
	u16 *s;
	int size;
	u32 addr;

	addr = udphdr->iphdr.src;
	sum += addr >> 16;
	sum += addr & 0xffff;
	addr = udphdr->iphdr.dest;
	sum += addr >> 16;
	sum += addr & 0xffff;
	sum += udphdr->iphdr.protocol << 8;	/* endian swap */
	size = udphdr->len;
	sum += size;

	size = ntohs(size);
	s = (u16 *) ((u8 *) udphdr + sizeof(iphdr_t));
	while (size > 1) {
		sum += *s;
		s++;
		size -= 2;
	}
	if (size)
		sum += *(u8 *) s;

	sum = (sum & 0xffff) + (sum >> 16);	/* add overflow counts */
	sum = (sum & 0xffff) + (sum >> 16);	/* once again */

	return ~sum;
}

static u8 my_icmp_data[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
};

static int bld_net_handle_arp(struct bld_net_if_s *nif,
			      void *frame, int framelen)
{
	arpreq_t *arp = (arpreq_t *) frame;

	if (ntohs(arp->hwtype) != 0x0001 ||
	    ntohs(arp->protocol) != 0x0800 ||
	    arp->hwlen != 6 ||
	    arp->protolen != 4)
		return -100;

	if ((ntohs(arp->opcode) == 0x0001) && (arp->tipaddr == nif->ip)) {
		/* Received an ARP request - reply to sender */
		u8 *txbuf;
		arpreq_t *arpans;

		txbuf = (u8 *)nif->dev.get_tx_frame_buf(&nif->dev);
		arpans = (arpreq_t *)txbuf;

		memcpy(arpans->ethhdr.dst, arp->ethhdr.src, 6);
		memcpy(arpans->ethhdr.src, nif->hwaddr, 6);
		arpans->ethhdr.type = htons(0x806);
		arpans->hwtype = htons(0x0001);
		arpans->protocol = htons(0x0800);
		arpans->hwlen = 6;
		arpans->protolen = 4;
		arpans->opcode = htons(0x0002);
		memcpy(arpans->shwaddr, nif->hwaddr, 6);
		arpans->sipaddr = nif->ip;
		memcpy(arpans->thwaddr, arp->ethhdr.src, 6);
		arpans->tipaddr = arp->sipaddr;

		nif->dev.send_frame(&nif->dev, sizeof(*arpans));
	} else if (ntohs(arp->opcode) == 0x0002) {
		/* Received an ARP reply - decide where to save this entry */
		bld_ip_t ip = arp->sipaddr;

		if ((nif->arptab.gw.ip == 0x0) && (nif->gw == ip)) {
			nif->arptab.gw.ip = ip;
			memcpy(nif->arptab.gw.hwaddr, arp->shwaddr, 6);
		}

		if ((nif->arptab.icmp.ip == 0x0) && (nif->icmp_cln.ip == ip)) {
			nif->arptab.icmp.ip = ip;
			memcpy(nif->arptab.icmp.hwaddr, arp->shwaddr, 6);
		}

		if ((nif->arptab.udp.ip == 0x0) &&
			(nif->udp_cln.dst_ip == ip)) {
			nif->arptab.udp.ip = ip;
			memcpy(nif->arptab.udp.hwaddr, arp->shwaddr, 6);
		}
	} else {
		/* Drop the packet */
		return -101;
	}

	return 0;
}

static int bld_net_handle_icmp(struct bld_net_if_s *nif,
			       void *frame, int framelen)
{
	ethhdr_t *ethhdr = (ethhdr_t *) frame;
	icmp_echo_hdr_t *icmp = (icmp_echo_hdr_t *)
		((u8 *) frame + sizeof(*ethhdr));

	if (icmp->type == 0x08 &&
	    icmp->code == 0x00) {
		/* Received an ICMP echo request - reply to sender */
		u8 *txbuf;
		ethhdr_t *ethhdr_rep;
		icmp_echo_hdr_t *icmp_rep;
		int iphdrlen;

		txbuf = (u8 *)nif->dev.get_tx_frame_buf(&nif->dev);
		ethhdr_rep = (ethhdr_t *)txbuf;
		icmp_rep = (icmp_echo_hdr_t *)((u8 *)txbuf + sizeof(*ethhdr));

		memcpy(txbuf, frame, framelen);
		memcpy(ethhdr_rep->dst, ethhdr->src, 6);
		memcpy(ethhdr_rep->src, ethhdr->dst, 6);
		icmp_rep->iphdr.ident = htons(nif->ip_ident++);
		icmp_rep->iphdr.ttl = 255;
		icmp_rep->iphdr.chksum = 0x0;
		icmp_rep->iphdr.src = icmp->iphdr.dest;
		icmp_rep->iphdr.dest = icmp->iphdr.src;
		icmp_rep->type = 0;
		icmp_rep->chksum = 0x0;

		iphdrlen = (icmp_rep->iphdr.verhdrlen & 0xf) << 2;
		icmp_rep->iphdr.chksum =
			ip_chksum((void *)&icmp_rep->iphdr, iphdrlen);
		icmp_rep->chksum =
			ip_chksum(((u8 *)icmp_rep) + sizeof(iphdr_t),
				  framelen - 14 - iphdrlen);

		nif->dev.send_frame(&nif->dev, framelen);
	} else if (icmp->type == 0x00 &&
		   icmp->code == 0x00) {
		/* Received an ICMP echo reply */
		u8 *icmpdata;

		icmpdata = ((u8 *)frame) + sizeof(*ethhdr) + sizeof(*icmp);
		if ((framelen == 74) && (ntohs(icmp->id) == 0x0200) &&
			(ntohs(icmp->seqno) == nif->icmp_cln.seqno) &&
			(memcmp(icmpdata, my_icmp_data, 32) == 0)) {
			nif->icmp_cln.poll_var = 1;
		}
	} else {
		/* Drop the packet */
		return -200;
	}

	return 0;
}

static int bld_net_handle_udp(struct bld_net_if_s *nif,
			      void *frame, int framelen)
{
	ethhdr_t *ethhdr = (ethhdr_t *) frame;
	udphdr_t *udphdr = (udphdr_t *) ((u8 *) frame + sizeof(*ethhdr));
	u8 *udpdata = (u8 *) udphdr + sizeof(*udphdr);
	int udpdata_len = ntohs(udphdr->len) - 8;
	int copied_len;

	if (udphdr->iphdr.src != nif->udp_cln.dst_ip ||
	    ntohs(udphdr->dest) != nif->udp_cln.src_port) {
		/* Drop packet from unkown sender... */
		return -300;
	}

	if (nif->udp_cln.user_buf_valid) {
		if (udpdata_len > nif->udp_cln.user_buf_len)
			copied_len = nif->udp_cln.user_buf_len;
		else
			copied_len = udpdata_len;
		memcpy(nif->udp_cln.user_buf, udpdata, copied_len);
		nif->udp_cln.user_buf_len = copied_len;
		nif->udp_cln.user_src_port = udphdr->src;
		nif->udp_cln.user_buf_valid = 0;
	} else if (!nif->udp_cln.sys_buf_used) {
		memcpy(nif->udp_cln.sys_buf, udpdata, udpdata_len);
		nif->udp_cln.sys_buf_len = udpdata_len;
		nif->udp_cln.sys_src_port = udphdr->src;
		nif->udp_cln.sys_buf_used = 1;
	} else {
		/* Drop the packet ... */
		return -301;
	}
	
	return 0;
}

static int bld_net_handle_ip(struct bld_net_if_s *nif,
			     void *frame, int framelen)
{
	ethhdr_t *ethhdr = (ethhdr_t *) frame;
	iphdr_t *iphdr = (iphdr_t *) ((u8 *) frame + sizeof(*ethhdr));
	icmp_echo_hdr_t *icmphdr;
	udphdr_t *udphdr;
	int iphdrlen;
	u16 chksum;

	/* IP version must be 4 */
	if ((iphdr->verhdrlen >> 4) != 4)
		return -1;

	/* Do a sanity check on the header length */
	iphdrlen = (iphdr->verhdrlen & 0xf) << 2;
	if (iphdrlen > framelen)
		return -2;

	/* Verify the IP header checksum */
	chksum = ip_chksum(iphdr, iphdrlen);
	if (chksum != 0x0)
		return -3;

	/* Only support unfragmented packet */
	/* Mask out the "DontFragment" flag (0x4000) */
	if ((ntohs(iphdr->frags) & ~0x4000) != 0x0)
		return -4;

	/* Only handle IP destined to ourself or a broadcast packet */
	if (iphdr->dest != nif->ip &&
	    iphdr->dest != 0xffffffff)
		return -5;

	switch (iphdr->protocol) {
	case 0x01:
		/* Verify the ICMP checksum */
		icmphdr = (icmp_echo_hdr_t *) ((u8 *) frame + sizeof(*ethhdr));
		chksum = ip_chksum(((u8 *) icmphdr) + sizeof(iphdr_t),
				   framelen - 14 - iphdrlen);
		if (chksum != 0x0)
			return -6;

		bld_net_handle_icmp(nif, frame, framelen);
		break;
	case 0x11:
		/* Verify the UDP checksum */
		udphdr = (udphdr_t *) ((u8 *) frame + sizeof(*ethhdr));
		chksum = udp_chksum(udphdr);
		if (chksum != 0x0)
			return -7;

		bld_net_handle_udp(nif, frame, framelen);
		break;
	default:
		return -99;
	}

	return 0;
}

static void bld_net_send_arp_req(struct bld_net_if_s *nif, bld_ip_t ip)
{
	u8 *txbuf;
	arpreq_t *arpreq;

	disable_interrupts();
	txbuf = (u8 *)nif->dev.get_tx_frame_buf(&nif->dev);
	arpreq = (arpreq_t *)txbuf;

	arpreq->ethhdr.dst[0] = 0xff;
	arpreq->ethhdr.dst[1] = 0xff;
	arpreq->ethhdr.dst[2] = 0xff;
	arpreq->ethhdr.dst[3] = 0xff;
	arpreq->ethhdr.dst[4] = 0xff;
	arpreq->ethhdr.dst[5] = 0xff;
	memcpy(arpreq->ethhdr.src, nif->hwaddr, 6);
	arpreq->ethhdr.type = htons(0x0806);
	arpreq->hwtype = htons(0x0001);
	arpreq->protocol = htons(0x0800);
	arpreq->hwlen = 6;
	arpreq->protolen = 4;
	arpreq->opcode = htons(0x0001);
	memcpy(arpreq->shwaddr, nif->hwaddr, 6);
	arpreq->sipaddr = nif->ip;
	arpreq->thwaddr[0] = 0xff;
	arpreq->thwaddr[1] = 0xff;
	arpreq->thwaddr[2] = 0xff;
	arpreq->thwaddr[3] = 0xff;
	arpreq->thwaddr[4] = 0xff;
	arpreq->thwaddr[5] = 0xff;
	arpreq->tipaddr = ip;

	nif->dev.send_frame(&nif->dev, sizeof(*arpreq));
	enable_interrupts();
}

static void bld_net_send_icmp_req(struct bld_net_if_s *nif, bld_ip_t ip)
{
	u8 *txbuf;
	ethhdr_t *ethhdr;
	icmp_echo_hdr_t *icmp;
	int framelen = 74;

	disable_interrupts();
	txbuf = (u8 *)nif->dev.get_tx_frame_buf(&nif->dev);
	ethhdr = (ethhdr_t *)txbuf;
	icmp = (icmp_echo_hdr_t *)((u8 *)txbuf + sizeof(*ethhdr));

	memcpy(ethhdr->dst, nif->arptab.icmp.hwaddr, 6);
	memcpy(ethhdr->src, nif->hwaddr, 6);
	ethhdr->type = htons(0x0800);
	icmp->iphdr.verhdrlen = 0x45;
	icmp->iphdr.service = 0x00;
	icmp->iphdr.len = htons(60);
	icmp->iphdr.ident = htons(nif->ip_ident++);
	icmp->iphdr.frags = htons(0x0);
	icmp->iphdr.ttl = 255;
	icmp->iphdr.protocol = 0x01;
	icmp->iphdr.chksum = 0x0;
	icmp->iphdr.src = nif->ip;
	icmp->iphdr.dest = ip;
	icmp->type = 0x08;
	icmp->code = 0x00;
	icmp->chksum = 0x0;
	icmp->id = htons(0x0200);
	icmp->seqno = htons(nif->icmp_cln.seqno);
	memcpy(txbuf + sizeof(*ethhdr) + sizeof(*icmp), my_icmp_data, 32);

	icmp->iphdr.chksum = ip_chksum((void *)&icmp->iphdr, sizeof(iphdr_t));
	icmp->chksum = ip_chksum(((u8 *)icmp) + sizeof(iphdr_t),
		sizeof(icmp_echo_hdr_t) - sizeof(iphdr_t) + 32);

	nif->dev.send_frame(&nif->dev, framelen);
	enable_interrupts();
}

/**
 * Initialize the AMBoot IP stack.
 */
void bld_net_init(int verbose)
{
	bld_net_if_t *nif = &net_if;
	flpart_table_t ptb;
	netdev_t *active_eth;

#if defined(BOARD_DISABLE_ETH)
	return;
#endif

	flprog_get_part_table(&ptb);

#if defined(BOARD_USE_ETH1) && (ETH_INSTANCES >= 2)
	active_eth = &ptb.dev.eth[1];
#else
	if (!rct_is_eth_enabled()) {
		if (verbose)
			putstr("Eth isn't enabled by power on config!\r\n");
		return;
	}
	active_eth = &ptb.dev.eth[0];
#endif
	if (active_eth->mac[0] == 0x0 && active_eth->mac[1] == 0x0 &&
		active_eth->mac[2] == 0x0 && active_eth->mac[3] == 0x0 &&
		active_eth->mac[4] == 0x0 && active_eth->mac[5] == 0x0) {
		if (verbose)
			putstr("Invalid: mac!\r\n");
		return;
	}
	if (active_eth->ip == 0x0) {
		if (verbose)
			putstr("Invalid: ip!\r\n");
		return;
	}
	if (active_eth->mask == 0x0) {
		if (verbose)
			putstr("Invalid: mask!\r\n");
		return;
	}

	memcpy(nif->hwaddr, active_eth->mac, 6);
	nif->ip = active_eth->ip;
	nif->mask = active_eth->mask;
	nif->gw = active_eth->gw;
	nif->handle_arp = bld_net_handle_arp;
	nif->handle_ip = bld_net_handle_ip;
	memzero(&nif->dev, sizeof(nif->dev));
	nif->dev.link_speed = SPEED_100;
	nif->dev.link_duplex = DUPLEX_FULL;
#if defined(BOARD_USE_ETH1) && (ETH_INSTANCES >= 2)
	nif->dev.regbase = (unsigned char *)ETH2_BASE;
	nif->dev.irq = ETH2_IRQ;
#else
	nif->dev.regbase = (unsigned char *)ETH_BASE;
	nif->dev.irq = ETH_IRQ;
#endif

	eth_init(nif, &nif->dev, nif->hwaddr);
}

/**
 * Disable the network.
 */
void bld_net_down(void)
{
	bld_net_if_t *nif = &net_if;

	if (nif->dev.nif == NULL)
		return;

	nif->dev.reset(&nif->dev);
}

/**
 * Wait for network interface to be ready.
 */
int bld_net_wait_ready(unsigned int tmo)
{
	bld_net_if_t *nif = &net_if;
	u32 t_now;

	if (nif->dev.nif == NULL)
		return -999;

	tmo = tmo / 100;
	timer_reset_count(TIMER1_ID);
	while (nif->dev.is_link_up(&nif->dev) == 0) {
		t_now = timer_get_count(TIMER1_ID);
		if ((tmo > 0) && (t_now > tmo)) {
			return -1;  /* No ARP reply */
		}
	}

	t_now = 0;
	timer_reset_count(TIMER1_ID);
	while (t_now < 1) {
		t_now = timer_get_count(TIMER1_ID);
	}

	return 0;
}

/**
 * Ping (ICMP echo) a target IP address.
 */
int bld_ping(u32 dst_ip, unsigned int tmo)
{
	bld_net_if_t *nif = &net_if;
	u32 t_now;

	if (nif->dev.nif == NULL)
		return -999;

	tmo = tmo / 100;
	timer_reset_count(TIMER1_ID);

	/* Send ARP request if entry is not already in table */
	if (nif->icmp_cln.ip != dst_ip ||
		(nif->arptab.icmp.hwaddr[0] == 0x0 &&
		nif->arptab.icmp.hwaddr[1] == 0x0 &&
		nif->arptab.icmp.hwaddr[2] == 0x0 &&
		nif->arptab.icmp.hwaddr[3] == 0x0 &&
		nif->arptab.icmp.hwaddr[4] == 0x0 &&
		nif->arptab.icmp.hwaddr[5] == 0x0)) {
		nif->icmp_cln.ip = dst_ip;
		memzero(&nif->arptab.icmp, sizeof(nif->arptab.icmp));
		bld_net_send_arp_req(nif, dst_ip);

		while (nif->arptab.icmp.ip != dst_ip) {
			t_now = timer_get_count(TIMER1_ID);
			if ((tmo > 0) && (t_now > tmo)) {
				return -1;  /* No ARP reply*/
			}
		}
	}

	/* Send ICMP request */
	nif->icmp_cln.seqno++;
	nif->icmp_cln.poll_var = 0;
	bld_net_send_icmp_req(nif, dst_ip);

	while (nif->icmp_cln.poll_var == 0) {
		t_now = timer_get_count(TIMER1_ID);
		if ((tmo > 0) && (t_now > tmo)) {
			return -2;  /* No ICMP echo reply*/
		}
	}

	return 0;
}

/**
 * Bind a UDP port.
 */
int bld_udp_bind(bld_ip_t dst_ip, u16 src_port)
{
	static u16 my_src_port = 1024;
	bld_net_if_t *nif = &net_if;
	unsigned int tmo = DEFAULT_NET_WAIT_TMO;
	u32 t_now;

	if (nif->dev.nif == NULL)
		return -999;

	if (nif->udp_cln.src_port != 0) {
		/* Already bound! */
		return -1;
	}

	/* Assign src_port */
	if (src_port == 0) {
		nif->udp_cln.src_port = my_src_port;
		my_src_port++;
		if (my_src_port < 1024)
			my_src_port = 1024;
	} else {
		nif->udp_cln.src_port = src_port;
	}

	tmo = tmo / 100;
	timer_reset_count(TIMER1_ID);

	/* Send ARP request if entry is not already in table */
	if (nif->udp_cln.dst_ip != dst_ip ||
		(nif->arptab.udp.hwaddr[0] == 0x0 &&
		nif->arptab.udp.hwaddr[1] == 0x0 &&
		nif->arptab.udp.hwaddr[2] == 0x0 &&
		nif->arptab.udp.hwaddr[3] == 0x0 &&
		nif->arptab.udp.hwaddr[4] == 0x0 &&
		nif->arptab.udp.hwaddr[5] == 0x0)) {
		nif->udp_cln.dst_ip = dst_ip;
		memzero(&nif->arptab.udp, sizeof(nif->arptab.udp));
		bld_net_send_arp_req(nif, dst_ip);

		while (nif->arptab.udp.ip != dst_ip) {
			t_now = timer_get_count(TIMER1_ID);
			if ((tmo > 0) && (t_now > tmo)) {
				nif->udp_cln.src_port = 0;
				return -1;  /* No ARP reply */
			}
		}
	}

	eth_filter_source_address(&nif->dev, nif->arptab.udp.hwaddr);

	return 0;
}

/**
 * Close a UDP port.
 */
void bld_udp_close(void)
{
	bld_net_if_t *nif = &net_if;

	if (nif->dev.nif == NULL)
		return;

	eth_pass_source_address(&nif->dev);
	nif->udp_cln.src_port = 0;
}

/**
 * Send a UDP packet.
 */
int bld_udp_send(u16 dst_port, void *packet, int packetlen)
{
	bld_net_if_t *nif = &net_if;
	u8 *txbuf;
	ethhdr_t *ethhdr;
	udphdr_t *udphdr;
	int framelen;

	if (nif->dev.nif == NULL)
		return -999;

	if (nif->udp_cln.src_port == 0)
		return -1;

	disable_interrupts();
	txbuf = (u8 *)nif->dev.get_tx_frame_buf(&nif->dev);
	ethhdr = (ethhdr_t *)txbuf;
	udphdr = (udphdr_t *)((u8 *)txbuf + sizeof(*ethhdr));

	memcpy(ethhdr->dst, nif->arptab.udp.hwaddr, 6);
	memcpy(ethhdr->src, nif->hwaddr, 6);
	ethhdr->type = htons(0x0800);

	udphdr->iphdr.verhdrlen = 0x45;
	udphdr->iphdr.service = 0x00;
	udphdr->iphdr.len = htons(packetlen + sizeof(*udphdr));
	udphdr->iphdr.ident = htons(nif->ip_ident++);
	udphdr->iphdr.frags = htons(0x0);
	udphdr->iphdr.ttl = 64;
	udphdr->iphdr.protocol = 0x11;
	udphdr->iphdr.chksum = 0x0;
	udphdr->iphdr.src = nif->ip;
	udphdr->iphdr.dest = nif->udp_cln.dst_ip;

	udphdr->src = htons(nif->udp_cln.src_port);
	udphdr->dest = htons(dst_port);
	udphdr->len = htons(packetlen + sizeof(*udphdr) - sizeof(iphdr_t));
	udphdr->chksum = 0x0;
	memcpy((u8 *) udphdr + sizeof(*udphdr), packet, packetlen);

	udphdr->iphdr.chksum =
		ip_chksum((void *) &udphdr->iphdr, 20);
	udphdr->chksum = udp_chksum(udphdr);

	framelen = packetlen + sizeof(*ethhdr) + sizeof(*udphdr);

	nif->dev.send_frame(&nif->dev, framelen);
	enable_interrupts();

	return packetlen;
}

/**
 * Receive an UDP packet.
 */
int bld_udp_recv(u16 *src_port, void *packet, int packetlen, int tmo)
{
	bld_net_if_t *nif = &net_if;
	int copied_len;
	u32 t_now;

	if (nif->dev.nif == NULL)
		return -999;

	if (nif->udp_cln.src_port == 0)
		return -1;

	if (nif->udp_cln.sys_buf_used) {
		if (packetlen < nif->udp_cln.sys_buf_len)
			copied_len = packetlen;
		else
			copied_len = nif->udp_cln.sys_buf_len;
		memcpy(packet, nif->udp_cln.sys_buf, copied_len);
		*src_port = ntohs(nif->udp_cln.sys_src_port);
		nif->udp_cln.sys_buf_used = 0;

		return copied_len;
	} else {
		nif->udp_cln.user_buf = packet;
		nif->udp_cln.user_buf_len = packetlen;
		nif->udp_cln.user_buf_valid = 1;

		tmo = tmo / 100;
		timer_reset_count(TIMER1_ID);

		while (nif->udp_cln.user_buf_valid) {
			t_now = timer_get_count(TIMER1_ID);
			if ((tmo > 0) && (t_now > tmo)) {
				return -1;  /* No UDP reception */
			}
		}

		*src_port = ntohs(nif->udp_cln.user_src_port);
		return nif->udp_cln.user_buf_len;
	}

	return -2;
}

int bld_tftp_load(u32 address, char *filename, int verbose)
{
	int					rval = -99999999;
	u32					dladdr = address;
	int					transferred = 0;
	int					filenamelen;
	tftp_header_t				*tftphdr;
	char					*stuff;
	u8					buf[BOARD_ETH_FRAMES_SIZE];
	u16					block;
	int					tries;
	u16					port;
	int					len;

	filenamelen = strlen(filename);
	if ((address < DRAM_START_ADDR) ||
		(address > DRAM_START_ADDR + DRAM_SIZE - TFTP_SEGSIZE) ||
		(filenamelen == 0) || (filenamelen > TFTP_SEGSIZE)) {
		goto tftp_load_exit;
	}
	if (verbose) {
		putstr("downloading [");
		putstr(filename);
		putstr("]:\r\n");
	}

	tftphdr = (tftp_header_t *)buf;
	tftphdr->opcode = htons(TFTP_RRQ);
	stuff = (char *)tftphdr->u.stuff;
	strncpy(stuff, filename, filenamelen + 1);
	stuff[filenamelen] = '\0';
	stuff += (filenamelen + 1);
	strncpy(stuff, "octet", 6);
	stuff[5] = '\0';

	port = TFTP_PORT;
	rval = bld_udp_send(port, buf, (2 + (filenamelen + 1) + 6));
	if (rval < 0)
		goto tftp_load_done;
	for (block = 1; ; block++) {
		for (tries = 0; tries < MAX_TFTP_RECV_TRIES; tries++) {
			rval = bld_udp_recv(&port, buf, sizeof(buf),
				DEFAULT_NET_WAIT_TMO);
			if (rval < 0)
				continue;
			if (ntohs(tftphdr->opcode) != TFTP_DATA)
				continue;
			if (ntohs(tftphdr->u.block) != block)
				continue;
			goto tftp_load_got_good_block;
		}
		if (tries >= MAX_TFTP_RECV_TRIES)
			goto tftp_load_done;

tftp_load_got_good_block:
		len = rval - 4;
		transferred += len;

		/* Send acknolwedgement */
		tftphdr->opcode = htons(TFTP_ACK);
		tftphdr->u.block = htons(block);
		rval = bld_udp_send(port, buf, 4);
		if (rval < 0)
			goto tftp_load_done;
		if (((block % 512) == 0) && verbose)
			putstr(".");
		if (((block % 32768) == 0) && verbose)
			putstr("\r\n");
		memcpy((void *)dladdr, tftphdr->data, len);
		dladdr += len;
		if (len < TFTP_SEGSIZE) {
			rval = 0;
			break;
		}
	}

tftp_load_done:
	if (rval == 0) {
		if (verbose) {
			putstr(" got ");
			putdec(transferred);
			putstr(" bytes\r\n");
		}
		rval = transferred;
	} else if (rval != -99999999) {
		if (verbose)
			putstr(" failed!\r\n");
		rval = -1;
	}

tftp_load_exit:
	return rval;
}

#endif  /* ETH_INSTANCES == 1 */

