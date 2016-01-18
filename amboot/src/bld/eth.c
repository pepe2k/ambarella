/**
 * system/src/bld/eth.c
 *
 * Networking support in AMBoot.
 *
 * History:
 *    2006/10/18 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2006, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <ambhw.h>
#include <bldfunc.h>

#ifndef DEAFULT_PHY_ID
#define DEAFULT_PHY_ID			(0)
#endif

#if (ETH_INSTANCES >= 1)

static bld_eth_dev_t *G_eth_dev = NULL;

/**
 * Interrupt handler
 */
void eth_handler(void)
{
	bld_eth_dev_t *dev = G_eth_dev;
	u32 status;

	if (dev == NULL)
		return;

	vic_disable(dev->irq);

	status = readl(dev->regbase + ETH_DMA_STATUS_OFFSET);

	if (status & ETH_DMA_STATUS_FBI) {
		/* Fatal bus error */
		dev->isr.fbi++;
	}

	if (status & ETH_DMA_STATUS_RI) {
		/* Normal receive interrupt */
		int i;
		int fp, lp;
		ethhdr_t *ethhdr;
		int ep_len;

		dev->isr.rx++;

		fp = dev->rx_cur;
		lp = (fp - 1 + BOARD_ETH_RX_FRAMES) % BOARD_ETH_RX_FRAMES;


		for (i = fp; i != lp; i = (i + 1) % BOARD_ETH_RX_FRAMES) {
			dev->rx_cur = i;

			_clean_flush_d_cache();

			if ((dev->rxd[i].status & ETH_RDES0_OWN) == 0x0) {
				ethhdr = (ethhdr_t *) dev->rxd[i].buffer1;
				ep_len = ETH_RDES0_FL(dev->rxd[i].status);
				ep_len -= 4;  /* Strip the 4 byte CRC */

				switch (ntohs(ethhdr->type)) {
				case 0x0806:	/* ARP */
					dev->nif->handle_arp(dev->nif,
						ethhdr, ep_len);
					break;
				case 0x0800:	/* IP */
					dev->nif->handle_ip(dev->nif,
						ethhdr, ep_len);
					break;
				default:
					break;
				}

				dev->rxd[i].status = ETH_RDES0_OWN;
				_clean_d_cache();
			} else {
				/* Break the loop if next descriptor is owned
				by DMA */
				break;
			}
		}
	}

	if (status & ETH_DMA_STATUS_RU) {
		/* Receive underflow */
		int i;

		dev->isr.ru++;
		for (i = 0; i < BOARD_ETH_RX_FRAMES; i++) {
			dev->rxd[i].status = ETH_RDES0_OWN;
		}
		_clean_flush_d_cache();
	}

	if (status & (ETH_DMA_STATUS_ERI | ETH_DMA_STATUS_RWT |
		ETH_DMA_STATUS_OVF)) {
		/* Abnormal receive interrupt */
		dev->isr.rerr++;
	}

	if (status & ETH_DMA_STATUS_RPS) {
		/* Receiver stoppped */
	}

	if (status & ETH_DMA_STATUS_TI) {
		/* Normal transmit interrupt */
		dev->isr.tx++;
	}

	if (status & ETH_DMA_STATUS_TU) {
		/* Transmit underflow interrupt */
		dev->isr.tu++;
	}

	if (status & (ETH_DMA_STATUS_ETI | ETH_DMA_STATUS_UNF |
		ETH_DMA_STATUS_TJT)) {
		/* Abnormal transmit interrupt */
		dev->isr.terr++;
	}

	if (status & ETH_DMA_STATUS_TPS) {
		/* Transmit stopped */
	}

	writel(dev->regbase + ETH_DMA_STATUS_OFFSET, status);
	vic_ackint(dev->irq);
	vic_enable(dev->irq);
}

static void *eth_get_tx_frame_buf(struct bld_eth_dev_s *dev)
{
	if (dev == NULL)
		return NULL;

	return dev->tx[dev->tx_cur].buf;
}

static int eth_send_frame(struct bld_eth_dev_s *dev, int framelen)
{
	if (dev == NULL)
		return -1;

	dev->txd[dev->tx_cur].status |= ETH_TDES0_OWN;
	dev->txd[dev->tx_cur].length &= (~0x7ff);
	dev->txd[dev->tx_cur].length |= ETH_TDES1_TBS1(framelen);

	_clean_flush_d_cache();

	writel(dev->regbase + ETH_DMA_TX_POLL_DMD_OFFSET, 0x1);

	dev->tx_cur = (dev->tx_cur + 1) % BOARD_ETH_TX_FRAMES;

	return 0;
}

static void eth_stop_rx_tx(struct bld_eth_dev_s *dev)
{
	u32					dma_status;
	u32					i = 120000;

	writel(dev->regbase + ETH_MAC_CFG_OFFSET,
		(readl(dev->regbase + ETH_MAC_CFG_OFFSET) & ~ETH_MAC_CFG_RE));
	writel(dev->regbase + ETH_DMA_OPMODE_OFFSET,
		(readl(dev->regbase + ETH_DMA_OPMODE_OFFSET) &
		~(ETH_DMA_OPMODE_SR | ETH_DMA_OPMODE_ST)));
	do {
		dma_status = readl(dev->regbase + ETH_DMA_STATUS_OFFSET);
	} while ((dma_status & (ETH_DMA_STATUS_TS_MASK |
		ETH_DMA_STATUS_RS_MASK)) && --i);
	writel(dev->regbase + ETH_MAC_CFG_OFFSET,
		(readl(dev->regbase + ETH_MAC_CFG_OFFSET) & ~ETH_MAC_CFG_TE));
}

static void eth_reset_dma(struct bld_eth_dev_s *dev)
{
	u32					dma_busmode;
	u32					i = 3000;

	writel(dev->regbase + ETH_DMA_BUS_MODE_OFFSET, ETH_DMA_BUS_MODE_SWR);
	do {
		dma_busmode = readl(dev->regbase + ETH_DMA_BUS_MODE_OFFSET);
	} while ((dma_busmode & ETH_DMA_BUS_MODE_SWR) && --i);
}

static u16 eth_mii_read(struct bld_eth_dev_s *dev, u8 addr, u8 reg)
{
	u32 val;
	u32 retry_counter = 1000;

	val = (ETH_MAC_GMII_ADDR_PA(addr) | ETH_MAC_GMII_ADDR_GR(reg) |
		ETH_MAC_GMII_ADDR_CR_250_300MHZ |
		ETH_MAC_GMII_ADDR_GB);
	writel(dev->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	do {
		val = readl(dev->regbase + ETH_MAC_GMII_ADDR_OFFSET);
		if (retry_counter == 0)
			return 0xFFFF;
		retry_counter--;
	} while ((val & ETH_MAC_GMII_ADDR_GB) == ETH_MAC_GMII_ADDR_GB);

	return (readl(dev->regbase + ETH_MAC_GMII_DATA_OFFSET) & 0xffff);
}

#if 0
static void eth_mii_write(struct bld_eth_dev_s *dev, u8 addr, u8 reg, u16 data)
{
	u32 val;
	u32 retry_counter = 1000;

	val = data & 0xffff;
	writel(dev->regbase + ETH_MAC_GMII_DATA_OFFSET, val);
	val = (ETH_MAC_GMII_ADDR_PA(addr) | ETH_MAC_GMII_ADDR_GR(reg) |
		ETH_MAC_GMII_ADDR_CR_250_300MHZ |
		ETH_MAC_GMII_ADDR_GW |
		ETH_MAC_GMII_ADDR_GB);
	writel(dev->regbase + ETH_MAC_GMII_ADDR_OFFSET, val);

	do {
		val = readl(dev->regbase + ETH_MAC_GMII_ADDR_OFFSET);
		if (retry_counter == 0)
			break;
		retry_counter--;
	} while ((val & ETH_MAC_GMII_ADDR_GB) == ETH_MAC_GMII_ADDR_GB);
}
#endif

static u8 eth_scan_phy_id(struct bld_eth_dev_s *dev)
{
	int i, bOK;
	u16 phy_reg;
	u32 phy_id;

	for (i = 0; i < 32; i++) {
		phy_reg = eth_mii_read(dev, i, 0x02);	//PHYSID1
		phy_id = (phy_reg & 0xffff) << 16;
		phy_reg = eth_mii_read(dev, i, 0x03);	//PHYSID2
		phy_id |= (phy_reg & 0xffff);
		bOK = 1;
		for (; phy_id > 0; phy_id >>= 4) {
			if ((phy_id & 0xf) == 0xf) {
				bOK = 0;
				break;
			}
		}
		if (bOK)
			return i;
	}

	return DEAFULT_PHY_ID;
}

static int eth_is_link_up(struct bld_eth_dev_s *dev)
{
	u8 phy_id;
	u16 bmsr;
	u16 bmcr;
	u16 lpagb = 0;
	u16 lpa;
	u16 adv;
	u32 eth_mac_cfg;

	dev->link_speed = SPEED_0;
	dev->link_duplex = DUPLEX_HALF;

	phy_id = eth_scan_phy_id(dev);

	bmsr = eth_mii_read(dev, phy_id, 0x01);
	if (bmsr == 0xFFFF)
		goto eth_is_link_up_exit;
	bmsr = eth_mii_read(dev, phy_id, 0x01);
	if (bmsr == 0xFFFF)
		goto eth_is_link_up_exit;
	if ((bmsr &= 0x0004) != 0x0004)
		goto eth_is_link_up_exit;

	bmcr = eth_mii_read(dev, phy_id, 0x00);
	if (bmcr == 0xFFFF)
		goto eth_is_link_up_exit;
	if ((bmcr & 0x1000) == 0x1000) {
#if (SUPPORT_GMII == 1)
		lpagb = eth_mii_read(dev, phy_id, 0x0a);
		if (lpagb == 0xFFFF) {
			lpagb = 0;
			goto eth_is_link_up_mii;
		}
		adv = eth_mii_read(dev, phy_id, 0x09);
		if (adv == 0xFFFF) {
			lpagb = 0;
			goto eth_is_link_up_mii;
		}
		lpagb &= adv << 2;
eth_is_link_up_mii:
#endif

		lpa = eth_mii_read(dev, phy_id, 0x05);
		if (lpa == 0xFFFF)
			goto eth_is_link_up_exit;
		adv = eth_mii_read(dev, phy_id, 0x04);
		if (adv == 0xFFFF)
			goto eth_is_link_up_exit;
		lpa &= adv;

		adv = eth_mii_read(dev, phy_id, 0x01);
		if (adv == 0xFFFF)
			goto eth_is_link_up_exit;
		if ((adv & 0x04) == 0)
			goto eth_is_link_up_exit;

		dev->link_speed = SPEED_10;
		if (lpagb & (0x0800 | 0x0400)) {
			dev->link_speed = SPEED_1000;
			if (lpagb & 0x0800)
				dev->link_duplex = DUPLEX_FULL;
		} else if (lpa & (0x0100 | 0x0080)) {
			dev->link_speed = SPEED_100;
			if (lpa & 0x0100)
				dev->link_duplex = DUPLEX_FULL;
		} else {
			if (lpa & 0x0040)
				dev->link_duplex = DUPLEX_FULL;
		}
	} else {
		adv = (bmcr & 0x2000) >> 13;
		adv |= (bmcr & 0x0040) >> 5;
		switch(adv) {
		case 1:
			dev->link_speed = SPEED_100;
			break;
		case 2:
			dev->link_speed = SPEED_1000;
			break;
		case 0:
		default:
			dev->link_speed = SPEED_10;
			break;
		}
		if (bmcr & 0x0100)
			dev->link_duplex = DUPLEX_FULL;
	}

	if (dev->link_speed) {
		eth_mac_cfg = ETH_MAC_CFG_RE | ETH_MAC_CFG_TE;
		if (dev->link_speed < SPEED_1000)
			eth_mac_cfg |= ETH_MAC_CFG_PS;
		if (dev->link_duplex == DUPLEX_FULL)
			eth_mac_cfg |= ETH_MAC_CFG_DM;
		writel(dev->regbase + ETH_MAC_CFG_OFFSET, eth_mac_cfg);
	}

eth_is_link_up_exit:
	return dev->link_speed;
}

static int eth_reset(struct bld_eth_dev_s *dev)
{
	eth_stop_rx_tx(dev);
	eth_reset_dma(dev);

	return 0;
}

/**
 * Initialize the ethernet controller.
 */
void eth_init(bld_net_if_t *nif, bld_eth_dev_t *dev, u8 *hwaddr)
{
	int i;

	vic_disable(dev->irq);
	eth_reset_dma(dev);

	for (i = 0; i < BOARD_ETH_TX_FRAMES; i++) {
		dev->txd[i].status = 0;
		dev->txd[i].length = ETH_TDES1_IC | ETH_TDES1_LS |
			ETH_TDES1_FS | ETH_TDES1_TCH;
		dev->txd[i].buffer1 = (u32) dev->tx[i].buf;
		dev->txd[i].buffer2 = (u32) &dev->txd[(i + 1) % BOARD_ETH_TX_FRAMES];
	}

	for (i = 0; i < BOARD_ETH_RX_FRAMES; i++) {
		dev->rxd[i].status = ETH_RDES0_OWN;
		dev->rxd[i].length = ETH_RDES1_RCH |
			ETH_RDES1_RBS1x(BOARD_ETH_FRAMES_SIZE);
		dev->rxd[i].buffer1 = (u32) dev->rx[i].buf;
		dev->rxd[i].buffer2 = (u32) &dev->rxd[(i + 1) % BOARD_ETH_RX_FRAMES];
	}

	_clean_flush_d_cache();

	dev->get_tx_frame_buf = eth_get_tx_frame_buf;
	dev->send_frame = eth_send_frame;
	dev->is_link_up = eth_is_link_up;
	dev->reset = eth_reset;
	dev->nif = nif;

	writel(dev->regbase + ETH_MAC_MAC0_HI_OFFSET,
		(hwaddr[5] << 8) | hwaddr[4]);
	writel(dev->regbase + ETH_MAC_MAC0_LO_OFFSET,
		(hwaddr[3] << 24) | (hwaddr[2] << 16) |
		(hwaddr[1] <<  8) | hwaddr[0]);

	writel(dev->regbase + ETH_DMA_TX_DESC_LIST_OFFSET, (u32)dev->txd);
	writel(dev->regbase + ETH_DMA_RX_DESC_LIST_OFFSET, (u32)dev->rxd);

	writel(dev->regbase + ETH_MAC_FRAME_FILTER_OFFSET, 0x00000000);
	writel(dev->regbase + ETH_DMA_INTEN_OFFSET,
		ETH_DMA_INTEN_NIE | ETH_DMA_INTEN_AIE |
		ETH_DMA_INTEN_FBE | ETH_DMA_INTEN_RWE | ETH_DMA_INTEN_RUE |
		ETH_DMA_INTEN_RIE | ETH_DMA_INTEN_UNE | ETH_DMA_INTEN_OVE |
		ETH_DMA_INTEN_TJE | ETH_DMA_INTEN_TUE | ETH_DMA_INTEN_TIE);
	writel(dev->regbase + ETH_DMA_BUS_MODE_OFFSET, ETH_DMA_BUS_MODE_FB |
		ETH_DMA_BUS_MODE_PBL_16 | ETH_DMA_BUS_MODE_DA_RX);
	writel(dev->regbase + ETH_DMA_OPMODE_OFFSET, ETH_DMA_OPMODE_TTC_256 |
		ETH_DMA_OPMODE_RTC_64 | ETH_DMA_OPMODE_SR | ETH_DMA_OPMODE_ST);

	disable_interrupts();
	vic_set_type(dev->irq, VIRQ_LEVEL_HIGH);
	G_eth_dev = dev;
	writel(dev->regbase + ETH_MAC_CFG_OFFSET, ETH_MAC_CFG_RE |
		ETH_MAC_CFG_TE | ETH_MAC_CFG_DM | ETH_MAC_CFG_PS);
	writel(dev->regbase + ETH_DMA_STATUS_OFFSET,
		readl(dev->regbase + ETH_DMA_STATUS_OFFSET));
	vic_enable(dev->irq);
	enable_interrupts();

	writel(dev->regbase + ETH_DMA_RX_POLL_DMD_OFFSET, 0x1);
}

void eth_filter_source_address(struct bld_eth_dev_s *dev, u8 *hwaddr)
{
	writel(dev->regbase + ETH_MAC_MAC1_HI_OFFSET,
		(0x3 << 30) |
		(hwaddr[5] << 8) | hwaddr[4]);
	writel(dev->regbase + ETH_MAC_MAC1_LO_OFFSET,
		(hwaddr[3] << 24) | (hwaddr[2] << 16) |
		(hwaddr[1] <<  8) | hwaddr[0]);
	writel(dev->regbase + ETH_MAC_FRAME_FILTER_OFFSET,
		ETH_MAC_FRAME_FILTER_SAF);
}

void eth_pass_source_address(struct bld_eth_dev_s *dev)
{
	writel(dev->regbase + ETH_MAC_MAC1_HI_OFFSET, 0x0000ffff);
	writel(dev->regbase + ETH_MAC_MAC1_LO_OFFSET, 0xffffffff);
	writel(dev->regbase + ETH_MAC_FRAME_FILTER_OFFSET, 0x00000000);
}

#endif  /* ETH_INSTANCES == 1 */

