/*
 * drivers/mmc/host/ambarella_sd.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/scatterlist.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>

#include <asm/dma.h>

#include <mach/hardware.h>
#include <plat/sd.h>
#include <plat/event.h>

/* ==========================================================================*/
#define CONFIG_SD_AMBARELLA_TIMEOUT_VAL		(0xe)
#define CONFIG_SD_AMBARELLA_WAIT_TIMEOUT	(HZ / 100)
#define CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT	(100000)
#define CONFIG_SD_AMBARELLA_DEFAULT_CLOCK	(24000000)
#define CONFIG_SD_AMBARELLA_MAX_TIMEOUT		(10 * HZ)
#define CONFIG_SD_AMBARELLA_VSW_PRE_SPEC	(5)
#define CONFIG_SD_AMBARELLA_VSW_POST_SPEC	(1)
#define CONFIG_SD_AMBARELLA_VSW_WAIT_LIMIT	(1000)

#undef CONFIG_SD_AMBARELLA_DEBUG
#undef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE

#define ambsd_printk(level, phcinfo, format, arg...)	\
	printk(level "%s.%u: " format,			\
	dev_name(((struct ambarella_sd_controller_info *)phcinfo->pinfo)->dev),\
	phcinfo->slot_id, ## arg)

#define ambsd_err(phcinfo, format, arg...)		\
	ambsd_printk(KERN_ERR, phcinfo, format, ## arg)
#define ambsd_warn(phcinfo, format, arg...)		\
	ambsd_printk(KERN_WARNING, phcinfo, format, ## arg)
#define ambsd_info(phcinfo, format, arg...)		\
	ambsd_printk(KERN_INFO, phcinfo, format, ## arg)
#define ambsd_rtdbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)

#ifdef CONFIG_SD_AMBARELLA_DEBUG
#define ambsd_dbg(phcinfo, format, arg...)		\
	ambsd_printk(KERN_DEBUG, phcinfo, format, ## arg)
#else
#define ambsd_dbg(phcinfo, format, arg...)		\
	({ if (0) ambsd_printk(KERN_DEBUG, phcinfo, format, ##arg); 0; })
#endif

/* ==========================================================================*/
enum ambarella_sd_state {
	AMBA_SD_STATE_IDLE,
	AMBA_SD_STATE_CMD,
	AMBA_SD_STATE_DATA,
	AMBA_SD_STATE_RESET,
	AMBA_SD_STATE_ERR
};

struct ambarella_sd_mmc_info {
	struct mmc_host			*mmc;
	struct mmc_request		*mrq;

	wait_queue_head_t		wait;

	enum ambarella_sd_state		state;

	struct scatterlist		*sg;
	u32				sg_len;
	u32				wait_tmo;
	u16				blk_sz;
	u16				blk_cnt;
	u32				arg_reg;
	u16				xfr_reg;
	u16				cmd_reg;
	u16				sta_reg;
	u8				tmo;
	u8				use_adma;
	u32				sta_counter;

	char				*buf_vaddress;
	dma_addr_t			buf_paddress;
	u32				dma_address;
	u32				dma_size;

	void				(*pre_dma)(void *data);
	void				(*post_dma)(void *data);

	struct ambarella_sd_slot	*plat_info;
	u32				slot_id;
	void				*pinfo;
	u32				valid;

	struct notifier_block		system_event;
	struct semaphore		system_event_sem;
};

struct ambarella_sd_controller_info {
	unsigned char __iomem 		*regbase;
	struct device			*dev;
	unsigned int			irq;
	u32				dma_fix;
	u32				clk_limit;
	u32				reset_error;

	u32				max_blk_sz;
	struct kmem_cache		*buf_cache;

	struct ambarella_sd_controller	*pcontroller;
	struct ambarella_sd_mmc_info	*pslotinfo[AMBA_SD_MAX_SLOT_NUM];
	struct mmc_ios			controller_ios;
};

/* ==========================================================================*/
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
static void ambarella_sd_show_info(struct ambarella_sd_mmc_info *pslotinfo)
{
	ambsd_dbg(pslotinfo, "Enter %s\n", __func__);
	ambsd_dbg(pslotinfo, "sg = 0x%x.\n", (u32)pslotinfo->sg);
	ambsd_dbg(pslotinfo, "sg_len = 0x%x.\n", pslotinfo->sg_len);
	ambsd_dbg(pslotinfo, "tmo = 0x%x.\n", pslotinfo->tmo);
	ambsd_dbg(pslotinfo, "blk_sz = 0x%x.\n", pslotinfo->blk_sz);
	ambsd_dbg(pslotinfo, "blk_cnt = 0x%x.\n", pslotinfo->blk_cnt);
	ambsd_dbg(pslotinfo, "arg_reg = 0x%x.\n", pslotinfo->arg_reg);
	ambsd_dbg(pslotinfo, "xfr_reg = 0x%x.\n", pslotinfo->xfr_reg);
	ambsd_dbg(pslotinfo, "cmd_reg = 0x%x.\n", pslotinfo->cmd_reg);
	ambsd_dbg(pslotinfo, "buf_vaddress = 0x%x.\n",
		(u32)pslotinfo->buf_vaddress);
	ambsd_dbg(pslotinfo, "buf_paddress = 0x%x.\n", pslotinfo->buf_paddress);
	ambsd_dbg(pslotinfo, "dma_address = 0x%x.\n", pslotinfo->dma_address);
	ambsd_dbg(pslotinfo, "dma_size = 0x%x.\n", pslotinfo->dma_size);
	ambsd_dbg(pslotinfo, "pre_dma = 0x%x.\n", (u32)pslotinfo->pre_dma);
	ambsd_dbg(pslotinfo, "post_dma = 0x%x.\n", (u32)pslotinfo->post_dma);
	ambsd_dbg(pslotinfo, "SD: state = 0x%x.\n", pslotinfo->state);
	ambsd_dbg(pslotinfo, "Exit %s\n", __func__);
}
#endif

static u32 ambarella_sd_check_dma_boundary(
	struct ambarella_sd_mmc_info *pslotinfo,
	u32 address, u32 size, u32 max_size)
{
	u32 start_512kb;
	u32 end_512kb;

	start_512kb = (address) & (~(max_size - 1));
	end_512kb = (address + size - 1) & (~(max_size - 1));

	if (start_512kb != end_512kb) {
		return 0;
	}

	return 1;
}

static u32 ambarella_sd_dma_mask_to_size(u32 mask)
{
	u32 size;

	switch (mask) {
	case SD_BLK_SZ_512KB:
		size = 0x80000;
		break;
	case SD_BLK_SZ_256KB:
		size = 0x40000;
		break;
	case SD_BLK_SZ_128KB:
		size = 0x20000;
		break;
	case SD_BLK_SZ_64KB:
		size = 0x10000;
		break;
	case SD_BLK_SZ_32KB:
		size = 0x8000;
		break;
	case SD_BLK_SZ_16KB:
		size = 0x4000;
		break;
	case SD_BLK_SZ_8KB:
		size = 0x2000;
		break;
	case SD_BLK_SZ_4KB:
		size = 0x1000;
		break;
	default:
		size = 0;
		BUG_ON(1);
		break;
	}

	return size;
}

static void ambarella_sd_pre_sg_to_dma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;
	int i;
	u32 offset;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	for (i = 0, offset = 0; i < pslotinfo->sg_len; i++) {
		memcpy(pslotinfo->buf_vaddress + offset,
			sg_virt(&pslotinfo->sg[i]),
			pslotinfo->sg[i].length);
		offset += pslotinfo->sg[i].length;
	}
	BUG_ON(offset != pslotinfo->dma_size);
	dma_sync_single_for_device(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_pre_sg_to_adma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;
	int i;
	u32 offset;
	u32 dma_len;
	u32 remain_size;
	u32 current_addr;
	u32 word_num, byte_num;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_len = dma_map_sg(pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_TO_DEVICE);
	for (i = 0, offset = 0; i < dma_len; i++) {
		remain_size = sg_dma_len(&pslotinfo->sg[i]);
		current_addr = sg_dma_address(&pslotinfo->sg[i]);
		if (pinfo->dma_fix) {
			current_addr |= pinfo->dma_fix;
		}
		if (unlikely(current_addr & 0x3)) {
			ambsd_err(pslotinfo,
				"Please disable AMBA_SD_PRIVATE_CAPS_ADMA\n");
			BUG_ON(1);
		}
		while (unlikely(remain_size > SD_ADMA_TBL_LINE_MAX_LEN)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			offset += SD_ADMA_TBL_LINE_SIZE;
			current_addr += SD_ADMA_TBL_LINE_MAX_LEN;
			remain_size -= SD_ADMA_TBL_LINE_MAX_LEN;
		}
		word_num = remain_size >> 2;
		byte_num = remain_size - (word_num << 2);
		if (word_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(word_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			current_addr += (word_num << 2);
			if (byte_num) {
				offset += SD_ADMA_TBL_LINE_SIZE;
			}
		}
		if (byte_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				byte_num << 16;
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
		}
		if (unlikely(i == dma_len - 1)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				SD_ADMA_TBL_ATTR_END;
		}
		offset += SD_ADMA_TBL_LINE_SIZE;
	}
	dma_sync_single_for_device(pinfo->dev, pslotinfo->buf_paddress,
		offset, DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_post_sg_to_dma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_sync_single_for_cpu(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_TO_DEVICE);
}

static void ambarella_sd_post_sg_to_adma(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_sync_single_for_cpu(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_FROM_DEVICE);
	dma_unmap_sg(pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_TO_DEVICE);
}

static void ambarella_sd_pre_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_sync_single_for_device(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_FROM_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_pre_adma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;
	int i;
	u32 dma_len;
	u32 offset;
	u32 remain_size;
	u32 current_addr;
	u32 word_num, byte_num;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_len = dma_map_sg(pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_FROM_DEVICE);
	for (i = 0, offset = 0; i < dma_len; i++) {
		remain_size = sg_dma_len(&pslotinfo->sg[i]);
		current_addr = sg_dma_address(&pslotinfo->sg[i]);
		if (pinfo->dma_fix) {
			current_addr |= pinfo->dma_fix;
		}
		if (unlikely(current_addr & 0x3)) {
			ambsd_err(pslotinfo,
				"Please disable AMBA_SD_PRIVATE_CAPS_ADMA\n");
			BUG_ON(1);
		}
		while (unlikely(remain_size > SD_ADMA_TBL_LINE_MAX_LEN)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			offset += SD_ADMA_TBL_LINE_SIZE;
			current_addr += SD_ADMA_TBL_LINE_MAX_LEN;
			remain_size -= SD_ADMA_TBL_LINE_MAX_LEN;
		}
		word_num = remain_size >> 2;
		byte_num = remain_size - (word_num << 2);
		if (word_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_WORD |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(word_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
			current_addr += (word_num << 2);
			if (byte_num) {
				offset += SD_ADMA_TBL_LINE_SIZE;
			}
		}
		if (byte_num) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) =
				(SD_ADMA_TBL_ATTR_TRAN |
				SD_ADMA_TBL_ATTR_VALID);
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				(byte_num << 16);
			*(u32 *)(pslotinfo->buf_vaddress + offset + 4) =
				current_addr;
		}
		if (unlikely(i == dma_len - 1)) {
			*(u32 *)(pslotinfo->buf_vaddress + offset) |=
				SD_ADMA_TBL_ATTR_END;
		}
		offset += SD_ADMA_TBL_LINE_SIZE;
	}
	dma_sync_single_for_device(pinfo->dev, pslotinfo->buf_paddress,
		offset, DMA_TO_DEVICE);
	pslotinfo->dma_address = pslotinfo->buf_paddress;
	pslotinfo->blk_sz |= SD_BLK_SZ_512KB;
}

static void ambarella_sd_post_dma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;
	int i;
	u32 offset;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_sync_single_for_cpu(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_FROM_DEVICE);
	for (i = 0, offset = 0; i < pslotinfo->sg_len; i++) {
		memcpy(sg_virt(&pslotinfo->sg[i]),
			pslotinfo->buf_vaddress + offset,
			pslotinfo->sg[i].length);
		offset += pslotinfo->sg[i].length;
	}
	BUG_ON(offset != pslotinfo->dma_size);
}

static void ambarella_sd_post_adma_to_sg(void *data)
{
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)data;
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	dma_sync_single_for_cpu(pinfo->dev, pslotinfo->buf_paddress,
		pslotinfo->dma_size, DMA_FROM_DEVICE);
	dma_unmap_sg(pinfo->dev, pslotinfo->sg,
		pslotinfo->sg_len, DMA_FROM_DEVICE);
}

static void ambarella_sd_request_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	down(&pslotinfo->system_event_sem);

	if (pslotinfo->plat_info->request)
		pslotinfo->plat_info->request();
}

static void ambarella_sd_release_bus(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	if (pslotinfo->plat_info->release)
		pslotinfo->plat_info->release();

	up(&pslotinfo->system_event_sem);
}

static void ambarella_sd_enable_int(struct mmc_host *mmc, u32 mask)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	pslotinfo->plat_info->set_int(mask, 1);
}

static void ambarella_sd_disable_int(struct mmc_host *mmc, u32 mask)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	pslotinfo->plat_info->set_int(mask, 0);
}

static void ambarella_sd_set_iclk(struct mmc_host *mmc, u16 clk_div)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	clk_div <<= 8;
	clk_div |= SD_CLK_ICLK_EN;
	amba_writew(pinfo->regbase + SD_CLK_OFFSET, clk_div);
	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_ICLK_STABLE)
			break;
		if ((clkreg & ~SD_CLK_ICLK_STABLE) != clk_div) {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET, clk_div);
			udelay(1);
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo,
				"Wait SD_CLK_ICLK_STABLE = %d @ 0x%x\n",
				counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_clear_clken(struct mmc_host *mmc)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_EN) {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET,
				(clkreg & ~SD_CLK_EN));
			udelay(1);
		} else {
			break;
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "%s(%d @ 0x%x)\n",
				__func__, counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_set_clken(struct mmc_host *mmc)
{
	u16 clkreg;
	u32 counter = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	while (1) {
		clkreg = amba_readw(pinfo->regbase + SD_CLK_OFFSET);
		if (clkreg & SD_CLK_EN) {
			break;
		} else {
			amba_writew(pinfo->regbase + SD_CLK_OFFSET,
				(clkreg | SD_CLK_EN));
			udelay(1);
		}
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "%s(%d @ 0x%x)\n",
				__func__, counter, clkreg);
			break;
		}
	}
}

static void ambarella_sd_reset_all(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u32 nis_flag = 0;
	u32 eis_flag = 0;
	u32 counter = 0;
	u8 reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	ambarella_sd_disable_int(mmc, 0xFFFFFFFF);
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, 0xFFFF, 0xFFFF);
	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_ALL);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_ALL))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_ALL....\n");
			break;
		}
	}

	ambarella_sd_set_iclk(mmc, 0x0000);
	amba_writeb(pinfo->regbase + SD_TMO_OFFSET,
		CONFIG_SD_AMBARELLA_TIMEOUT_VAL);

	nis_flag = SD_NISEN_REMOVAL	|
		SD_NISEN_INSERT		|
		SD_NISEN_DMA		|
		SD_NISEN_BLOCK_GAP	|
		SD_NISEN_XFR_DONE	|
		SD_NISEN_CMD_DONE;
	eis_flag = SD_EISEN_ACMD12_ERR	|
		SD_EISEN_CURRENT_ERR	|
		SD_EISEN_DATA_BIT_ERR	|
		SD_EISEN_DATA_CRC_ERR	|
		SD_EISEN_DATA_TMOUT_ERR	|
		SD_EISEN_CMD_IDX_ERR	|
		SD_EISEN_CMD_BIT_ERR	|
		SD_EISEN_CMD_CRC_ERR	|
		SD_EISEN_CMD_TMOUT_ERR;

	if (pslotinfo->use_adma == 1) {
		eis_flag |= SD_EISEN_ADMA_ERR;
	} else {
		eis_flag &= ~SD_EISEN_ADMA_ERR;
	}

	ambarella_sd_enable_int(mmc, (eis_flag << 16) | nis_flag);

	pslotinfo->state = AMBA_SD_STATE_RESET;
	pinfo->reset_error = 0;

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static void ambarella_sd_reset_cmd_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u32 counter = 0;
	u8 reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_CMD);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_CMD))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_CMD...\n");
			pinfo->reset_error = 1;
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static void ambarella_sd_reset_data_line(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u32 counter = 0;
	u8 reset_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambsd_dbg(pslotinfo, "Enter %s with state %u\n",
		__func__, pslotinfo->state);

	amba_writeb(pinfo->regbase + SD_RESET_OFFSET, SD_RESET_DAT);
	while (1) {
		reset_reg = amba_readb(pinfo->regbase + SD_RESET_OFFSET);
		if (!(reset_reg & SD_RESET_DAT))
			break;
		counter++;
		if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
			ambsd_warn(pslotinfo, "Wait SD_RESET_DAT...\n");
			pinfo->reset_error = 1;
			break;
		}
	}

	ambsd_dbg(pslotinfo, "Exit %s with counter %u\n", __func__, counter);
}

static inline void ambarella_sd_data_done(
	struct ambarella_sd_mmc_info *pslotinfo, u16 nis, u16 eis)
{
	struct mmc_data *data;

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) == SD_CMD_RSP_48BUSY)) {
		if (eis) {
			pslotinfo->state = AMBA_SD_STATE_ERR;
		} else {
			pslotinfo->state = AMBA_SD_STATE_IDLE;
		}
		wake_up(&pslotinfo->wait);
		return;
	}
	if (pslotinfo->mrq == NULL) {
		ambsd_dbg(pslotinfo, "%s: mrq is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}
	if (pslotinfo->mrq->data == NULL) {
		ambsd_dbg(pslotinfo, "%s: data is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}

	data = pslotinfo->mrq->data;
	if (eis) {
		if (eis & SD_EIS_DATA_BIT_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_CRC_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_ADMA_ERR) {
			data->error = -EILSEQ;
		} else if (eis & SD_EIS_DATA_TMOUT_ERR) {
			data->error = -ETIMEDOUT;
		} else {
			data->error = -EIO;
		}
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		ambsd_err(pslotinfo, "%s: CMD[%u] get eis[0x%x]\n", __func__,
			pslotinfo->mrq->cmd->opcode, eis);
#endif
		pslotinfo->state = AMBA_SD_STATE_ERR;
		wake_up(&pslotinfo->wait);
		return;
	} else {
		data->bytes_xfered = pslotinfo->dma_size;
	}

	pslotinfo->state = AMBA_SD_STATE_IDLE;
	wake_up(&pslotinfo->wait);
}

static inline void ambarella_sd_cmd_done(
	struct ambarella_sd_mmc_info *pslotinfo, u16 nis, u16 eis)
{
	struct mmc_command *cmd;
	u32 rsp0, rsp1, rsp2, rsp3;
	struct ambarella_sd_controller_info *pinfo;
	u16 ac12es;

	if (pslotinfo->mrq == NULL) {
		ambsd_dbg(pslotinfo, "%s: mrq is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}
	if (pslotinfo->mrq->cmd == NULL) {
		ambsd_dbg(pslotinfo, "%s: cmd is NULL, nis[0x%x] eis[0x%x]\n",
			__func__, nis, eis);
		return;
	}

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	cmd = pslotinfo->mrq->cmd;
	if (eis) {
		if (eis & SD_EIS_CMD_BIT_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_CRC_ERR) {
			cmd->error = -EILSEQ;
		} else if (eis & SD_EIS_CMD_TMOUT_ERR) {
			cmd->error = -ETIMEDOUT;
		} else if (eis & SD_EIS_ACMD12_ERR) {
			ac12es = amba_readl(pinfo->regbase + SD_AC12ES_OFFSET);
			if (ac12es & SD_AC12ES_TMOUT_ERROR) {
				cmd->error = -ETIMEDOUT;
			} else if (eis & SD_AC12ES_CRC_ERROR) {
				cmd->error = -EILSEQ;
			} else {
				cmd->error = -EIO;
			}

			if (pslotinfo->mrq->stop) {
				pslotinfo->mrq->stop->error = cmd->error;
			} else {
				ambsd_err(pslotinfo, "%s NULL stop 0x%x %u\n",
					__func__, ac12es, cmd->error);
			}
		} else {
			cmd->error = -EIO;
		}
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		ambsd_err(pslotinfo, "%s: CMD[%u] get eis[0x%x]\n", __func__,
			pslotinfo->mrq->cmd->opcode, eis);
#endif
		pslotinfo->state = AMBA_SD_STATE_ERR;
		wake_up(&pslotinfo->wait);
		return;
	}

	if (cmd->flags & MMC_RSP_136) {
		rsp0 = amba_readl(pinfo->regbase + SD_RSP0_OFFSET);
		rsp1 = amba_readl(pinfo->regbase + SD_RSP1_OFFSET);
		rsp2 = amba_readl(pinfo->regbase + SD_RSP2_OFFSET);
		rsp3 = amba_readl(pinfo->regbase + SD_RSP3_OFFSET);
		cmd->resp[0] = ((rsp3 << 8) | (rsp2 >> 24));
		cmd->resp[1] = ((rsp2 << 8) | (rsp1 >> 24));
		cmd->resp[2] = ((rsp1 << 8) | (rsp0 >> 24));
		cmd->resp[3] = (rsp0 << 8);
	} else {
		cmd->resp[0] = amba_readl(pinfo->regbase + SD_RSP0_OFFSET);
	}

	if ((pslotinfo->state == AMBA_SD_STATE_CMD) &&
		((pslotinfo->cmd_reg & 0x3) != SD_CMD_RSP_48BUSY)) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
		wake_up(&pslotinfo->wait);
	}
}

static irqreturn_t ambarella_sd_irq(int irq, void *devid)
{
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo = NULL;
	u16 nis;
	u16 eis;
	u32 enabled = 0;
	u32 i;

	pinfo = (struct ambarella_sd_controller_info *)devid;

	/* Read the interrupt registers */
	amba_read2w(pinfo->regbase + SD_NIS_OFFSET, &nis, &eis);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		enabled = pslotinfo->plat_info->check_owner();
		if (enabled)
			break;
	}

	/* Clear interrupt */
	amba_write2w(pinfo->regbase + SD_NIS_OFFSET, nis, eis);
	if (enabled) {
		ambsd_dbg(pslotinfo, "%s nis = 0x%x, eis = 0x%x & %u\n",
			__func__, nis, eis, pslotinfo->state);
	} else {
		ambsd_dbg(pslotinfo, "%s[false] nis = 0x%x, eis = 0x%x\n",
			__func__, nis, eis);
		if (eis) {
			ambsd_err(pslotinfo,
				"FIO_CTR_REG = 0x%x, FIO_DMACTR_REG = 0x%x\n",
				amba_readl(FIO_CTR_REG),
				amba_readl(FIO_DMACTR_REG));
			ambarella_sd_reset_all(pslotinfo->mmc);
		}
		goto ambarella_sd_irq_exit;
	}

	if (nis & SD_NIS_CARD) {
		ambsd_dbg(pslotinfo, "SD_NIS_CARD\n");
		mmc_signal_sdio_irq(pslotinfo->mmc);
	}

	if (!ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd) &&
		(pslotinfo->plat_info->fixed_cd == -1)) {
		if (nis & SD_NIS_REMOVAL) {
			ambsd_dbg(pslotinfo, "SD_NIS_REMOVAL\n");
			mmc_detect_change(pslotinfo->mmc,
				pslotinfo->plat_info->cd_delay);
		} else if (nis & SD_NIS_INSERT) {
			ambsd_dbg(pslotinfo, "SD_NIS_INSERT\n");
			mmc_detect_change(pslotinfo->mmc,
				pslotinfo->plat_info->cd_delay);
		}
	}

	if (eis) {
		if (eis & (SD_EIS_CMD_TMOUT_ERR | SD_EIS_CMD_CRC_ERR |
			SD_EIS_CMD_BIT_ERR | SD_EIS_CMD_IDX_ERR |
			SD_EIS_ACMD12_ERR)) {
			ambarella_sd_reset_cmd_line(pslotinfo->mmc);
		}
		if (eis & (SD_EIS_DATA_TMOUT_ERR | SD_EIS_DATA_CRC_ERR)) {
			ambarella_sd_reset_data_line(pslotinfo->mmc);
		}
		if (eis & (SD_EIS_DATA_BIT_ERR | SD_EIS_CURRENT_ERR)) {
			ambarella_sd_reset_all(pslotinfo->mmc);
		}
		if (pslotinfo->state == AMBA_SD_STATE_CMD) {
			ambarella_sd_cmd_done(pslotinfo, nis, eis);
		} else if (pslotinfo->state == AMBA_SD_STATE_DATA) {
			ambarella_sd_data_done(pslotinfo, nis, eis);
		}
	} else {
		if (nis & SD_NIS_CMD_DONE) {
			ambarella_sd_cmd_done(pslotinfo, nis, eis);
		}
		if (nis & SD_NIS_XFR_DONE) {
			ambarella_sd_data_done(pslotinfo, nis, eis);
		}
#if 0
		if (nis & SD_NIS_DMA) {
			amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
			amba_readl(pinfo->regbase + SD_DMA_ADDR_OFFSET));
		}
#endif
	}

ambarella_sd_irq_exit:
	return IRQ_HANDLED;
}

static int ambarella_sd_gpio_cd_check_val(
	struct ambarella_sd_mmc_info *pslotinfo)
{
	u32 val = -1;

	if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd)) {
		ambarella_gpio_config(pslotinfo->plat_info->gpio_cd.irq_gpio,
			GPIO_FUNC_SW_INPUT);
		val = ambarella_gpio_get(
			pslotinfo->plat_info->gpio_cd.irq_gpio);
		ambarella_gpio_config(pslotinfo->plat_info->gpio_cd.irq_gpio,
			pslotinfo->plat_info->gpio_cd.irq_gpio_mode);
		val = (val == pslotinfo->plat_info->gpio_cd.irq_gpio_val) ?
			1 : 0;
		ambsd_dbg(pslotinfo, "%s:%u\n", (val == 1) ?
			"card insert" : "card eject",
			pslotinfo->plat_info->cd_delay);
	}

	return val;
}

static irqreturn_t ambarella_sd_gpio_cd_irq(int irq, void *devid)
{
	struct ambarella_sd_mmc_info *pslotinfo;

	pslotinfo = (struct ambarella_sd_mmc_info *)devid;
	if (pslotinfo->valid &&
		(ambarella_sd_gpio_cd_check_val(pslotinfo) != -1)) {
		mmc_detect_change(pslotinfo->mmc,
			pslotinfo->plat_info->cd_delay);
	}

	return IRQ_HANDLED;
}

static void ambarella_sd_set_clk(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u16 clk_div = 0x0000;
	u32 sd_clk;
	u32 desired_clk;
	u32 actual_clk;
	u32 bneed_div = 1;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_clear_clken(mmc);
	if (ios->clock == 0) {
		pinfo->pcontroller->active_clock = 0;
	} else {
		desired_clk = ios->clock;
		if (desired_clk > pinfo->clk_limit)
			desired_clk = pinfo->clk_limit;

		if (pinfo->pcontroller->support_pll_scaler) {
		if (desired_clk < 10000000) {
			/* Below 10Mhz, divide by sd controller */
			pinfo->pcontroller->set_pll(pinfo->clk_limit);
		} else {
			sd_clk = desired_clk;
			do {
				pinfo->pcontroller->set_pll(sd_clk);
				actual_clk = pinfo->pcontroller->get_pll();
				sd_clk -= 1000000;
			} while (actual_clk > desired_clk);
			bneed_div = 0;
		}
		}

		if (bneed_div) {
			sd_clk = pinfo->pcontroller->get_pll();
			for (clk_div = 0x0; clk_div <= 0x80;) {
				if (clk_div == 0)
					actual_clk = sd_clk;
				else
					actual_clk = sd_clk / (clk_div << 1);

				if (actual_clk <= desired_clk)
					break;

				if (clk_div >= 0x80)
					break;

				if (clk_div == 0x0)
					clk_div = 0x1;
				else
					clk_div <<= 1;
			}
		}
		ambsd_dbg(pslotinfo, "sd_pll = %u.\n",
			pinfo->pcontroller->get_pll());
		ambsd_dbg(pslotinfo, "desired_clk = %u.\n", desired_clk);
		ambsd_dbg(pslotinfo, "actual_clk = %u.\n", actual_clk);
		ambsd_dbg(pslotinfo, "clk_div = %u.\n", clk_div);
		pinfo->pcontroller->active_clock = actual_clk;
		ambarella_sd_set_iclk(mmc, clk_div);
		ambarella_sd_set_clken(mmc);
	}
}

static void ambarella_sd_set_pwr(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (ios->power_mode == MMC_POWER_OFF) {
		ambarella_sd_reset_all(pslotinfo->mmc);
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET, SD_PWR_OFF);
		ambarella_set_gpio_output_can_sleep(
			&pslotinfo->plat_info->ext_reset, 1, 1);
		ambarella_set_gpio_output_can_sleep(
			&pslotinfo->plat_info->ext_power, 0, 1);
		if (pslotinfo->plat_info->set_vdd) {
			pslotinfo->plat_info->set_vdd(0);
		}
	} else if (ios->power_mode == MMC_POWER_UP) {
		if (pslotinfo->plat_info->set_vdd) {
			pslotinfo->plat_info->set_vdd(3300);
		}
		ambarella_set_gpio_output_can_sleep(
			&pslotinfo->plat_info->ext_power, 1, 1);
		ambarella_set_gpio_output_can_sleep(
			&pslotinfo->plat_info->ext_reset, 0, 1);
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_3_3V));
	} else if (ios->power_mode == MMC_POWER_ON) {
		switch (1 << ios->vdd) {
		case MMC_VDD_165_195:
		case MMC_VDD_32_33:
		case MMC_VDD_33_34:
			break;
		default:
			ambsd_err(pslotinfo, "%s Wrong voltage[%u]!\n",
				__func__, ios->vdd);
			break;
		}
	}
	msleep(pinfo->pcontroller->pwr_delay);
	ambsd_dbg(pslotinfo, "pwr = 0x%x.\n",
		amba_readb(pinfo->regbase + SD_PWR_OFFSET));
}

static void ambarella_sd_set_bus(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u8 hostr = 0;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	hostr = amba_readb(pinfo->regbase + SD_HOST_OFFSET);
	if (ios->bus_width == MMC_BUS_WIDTH_8) {
		hostr |= SD_HOST_8BIT;
		hostr &= ~(SD_HOST_4BIT);
	} else if (ios->bus_width == MMC_BUS_WIDTH_4) {
		hostr &= ~(SD_HOST_8BIT);
		hostr |= SD_HOST_4BIT;
	} else if (ios->bus_width == MMC_BUS_WIDTH_1) {
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	} else {
		ambsd_err(pslotinfo, "Unknown bus_width[%u], assume 1bit.\n",
			ios->bus_width);
		hostr &= ~(SD_HOST_8BIT);
		hostr &= ~(SD_HOST_4BIT);
	}
	hostr &= ~SD_HOST_HIGH_SPEED;
	switch (ios->timing) {
	case MMC_TIMING_LEGACY:
	case MMC_TIMING_MMC_HS:
	case MMC_TIMING_SD_HS:
	case MMC_TIMING_UHS_SDR12:
	case MMC_TIMING_UHS_SDR25:
	case MMC_TIMING_UHS_SDR50:
	case MMC_TIMING_UHS_SDR104:
		amba_clrbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		break;
	case MMC_TIMING_UHS_DDR50:
		hostr |= SD_HOST_HIGH_SPEED;
		amba_setbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		break;
	default:
		amba_clrbitsl(pinfo->regbase + SD_XC_CTR_OFFSET,
			SD_XC_CTR_DDR_EN);
		ambsd_err(pslotinfo, "Unknown timing[%d], assume legacy.\n",
			ios->timing);
		break;
	}
	if (pslotinfo->plat_info->set_bus_timing) {
		pslotinfo->plat_info->set_bus_timing(ios->timing);
	}
	amba_writeb(pinfo->regbase + SD_HOST_OFFSET, hostr);

	ambsd_dbg(pslotinfo, "hostr = 0x%x.\n", hostr);
}

static void ambarella_sd_check_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if ((pinfo->controller_ios.power_mode != ios->power_mode) ||
		(pinfo->controller_ios.vdd != ios->vdd) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_pwr(mmc, ios);
		pinfo->controller_ios.power_mode = ios->power_mode;
		pinfo->controller_ios.vdd = ios->vdd;
	}

	if ((pinfo->controller_ios.clock != ios->clock) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_clk(mmc, ios);
		pinfo->controller_ios.clock = ios->clock;
	}

	if ((pinfo->controller_ios.bus_width != ios->bus_width) ||
		(pinfo->controller_ios.timing != ios->timing) ||
		(pslotinfo->state == AMBA_SD_STATE_RESET)) {
		ambarella_sd_set_bus(mmc, ios);
		pinfo->controller_ios.bus_width = ios->bus_width;
		pinfo->controller_ios.timing = ios->timing;
	}

	if (pslotinfo->state == AMBA_SD_STATE_RESET) {
		pslotinfo->state = AMBA_SD_STATE_IDLE;
	}
}

static u32 ambarella_sd_check_cd(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	int cdpin;
	u32 valid_cd = 0;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	if (pslotinfo->plat_info->fixed_cd == 1) {
		valid_cd = 1;
	} else if (pslotinfo->plat_info->fixed_cd == 0) {
		valid_cd = 0;
	} else {
		cdpin = ambarella_sd_gpio_cd_check_val(pslotinfo);
		if (cdpin == 1) {
			valid_cd = 1;
		} else if (cdpin == -1) {
			if (amba_tstbitsl(pinfo->regbase + SD_STA_OFFSET,
				SD_STA_CARD_INSERTED)) {
				valid_cd = 1;
			}
		}
	}

	return valid_cd;
}

static inline void ambarella_sd_prepare_tmo(
	struct ambarella_sd_mmc_info *pslotinfo,
	struct mmc_data *pmmcdata)
{
	struct ambarella_sd_controller_info *pinfo;
#if defined(CONFIG_SD_AMBARELLA_CALIB_TMO)
	u32 actual_cycle_ns;
	u32 desired_tmo;
	u32 actual_tmo;
	u32 actual_hz;
#endif

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
#if defined(CONFIG_SD_AMBARELLA_CALIB_TMO)
	if (pinfo->pcontroller->active_clock >= HZ) {
		actual_hz = pinfo->pcontroller->active_clock / HZ;
		actual_cycle_ns = ((1000000000 / HZ) / actual_hz);
		desired_tmo = pmmcdata->timeout_ns / actual_cycle_ns;
		desired_tmo += pmmcdata->timeout_clks;
		for (pslotinfo->tmo = 0; pslotinfo->tmo < 0xe;
			pslotinfo->tmo++) {
			actual_tmo = (0x1 << (13 + pslotinfo->tmo));
			if (actual_tmo >= desired_tmo)
				break;
		}
		pslotinfo->wait_tmo = (actual_tmo + actual_hz - 1);
		pslotinfo->wait_tmo /= actual_hz;
	} else {
		pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
		pslotinfo->wait_tmo = pinfo->pcontroller->wait_tmo;
	}
	if ((pslotinfo->wait_tmo > 0) && (pslotinfo->wait_tmo <
		CONFIG_SD_AMBARELLA_MAX_TIMEOUT)) {
		pslotinfo->sta_counter = CONFIG_SD_AMBARELLA_MAX_TIMEOUT;
		pslotinfo->sta_counter /= pslotinfo->wait_tmo;
	} else {
		pslotinfo->sta_counter = 1;
		pslotinfo->wait_tmo = (1 * HZ);
	}
#else
	pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
	pslotinfo->wait_tmo = pinfo->pcontroller->wait_tmo;
	if ((pslotinfo->wait_tmo > 0) && (pslotinfo->wait_tmo <
		CONFIG_SD_AMBARELLA_MAX_TIMEOUT)) {
		pslotinfo->sta_counter = CONFIG_SD_AMBARELLA_MAX_TIMEOUT;
		pslotinfo->sta_counter /= pslotinfo->wait_tmo;
	} else {
		pslotinfo->sta_counter = 1;
		pslotinfo->wait_tmo = (1 * HZ);
	}
#endif
	ambsd_dbg(pslotinfo, "timeout_ns = %u, timeout_clks = %u @ %uHz, "
		"wait_tmo = %u, tmo = %u, sta_counter = %u.\n",
		pmmcdata->timeout_ns, pmmcdata->timeout_clks,
		pinfo->pcontroller->active_clock, pslotinfo->wait_tmo,
		pslotinfo->tmo, pslotinfo->sta_counter);
}

static inline void ambarella_sd_pre_cmd(
	struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	pslotinfo->state = AMBA_SD_STATE_CMD;
	pslotinfo->sg_len = 0;
	pslotinfo->sg = NULL;
	pslotinfo->blk_sz = 0;
	pslotinfo->blk_cnt = 0;
	pslotinfo->arg_reg = 0;
	pslotinfo->cmd_reg = 0;
	pslotinfo->sta_reg = 0;
	pslotinfo->tmo = CONFIG_SD_AMBARELLA_TIMEOUT_VAL;
	pslotinfo->wait_tmo = (1 * HZ);
	pslotinfo->xfr_reg = 0;
	pslotinfo->dma_address = 0;
	pslotinfo->dma_size = 0;

	if (pslotinfo->mrq->stop) {
		if (likely(pslotinfo->mrq->stop->opcode ==
			MMC_STOP_TRANSMISSION)) {
			pslotinfo->xfr_reg = SD_XFR_AC12_EN;
		} else {
			ambsd_err(pslotinfo, "%s strange stop cmd%u\n",
				__func__, pslotinfo->mrq->stop->opcode);
		}
	}

	if (!(pslotinfo->mrq->cmd->flags & MMC_RSP_PRESENT))
		pslotinfo->cmd_reg = SD_CMD_RSP_NONE;
	else if (pslotinfo->mrq->cmd->flags & MMC_RSP_136)
		pslotinfo->cmd_reg = SD_CMD_RSP_136;
	else if (pslotinfo->mrq->cmd->flags & MMC_RSP_BUSY)
		pslotinfo->cmd_reg = SD_CMD_RSP_48BUSY;
	else
		pslotinfo->cmd_reg = SD_CMD_RSP_48;
	if (pslotinfo->mrq->cmd->flags & MMC_RSP_CRC)
		pslotinfo->cmd_reg |= SD_CMD_CHKCRC;
	if (pslotinfo->mrq->cmd->flags & MMC_RSP_OPCODE)
		pslotinfo->cmd_reg |= SD_CMD_CHKIDX;
	pslotinfo->cmd_reg |= SD_CMD_IDX(pslotinfo->mrq->cmd->opcode);
	pslotinfo->arg_reg = pslotinfo->mrq->cmd->arg;

	if (pslotinfo->mrq->data) {
		pslotinfo->state = AMBA_SD_STATE_DATA;
		ambarella_sd_prepare_tmo(pslotinfo, pslotinfo->mrq->data);
		pslotinfo->blk_sz = (pslotinfo->mrq->data->blksz & 0xFFF);
		pslotinfo->dma_size = pslotinfo->mrq->data->blksz *
			pslotinfo->mrq->data->blocks;
		pslotinfo->sg_len = pslotinfo->mrq->data->sg_len;
		pslotinfo->sg = pslotinfo->mrq->data->sg;
		pslotinfo->xfr_reg |= SD_XFR_DMA_EN;
		pslotinfo->cmd_reg |= SD_CMD_DATA;
		pslotinfo->blk_cnt = pslotinfo->mrq->data->blocks;
		if (pslotinfo->blk_cnt > 1) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg |= SD_XFR_BLKCNT_EN;
		}
		if (pslotinfo->mrq->data->flags & MMC_DATA_STREAM) {
			pslotinfo->xfr_reg |= SD_XFR_MUL_SEL;
			pslotinfo->xfr_reg &= ~SD_XFR_BLKCNT_EN;
		}
		if (pslotinfo->mrq->data->flags & MMC_DATA_WRITE) {
			pslotinfo->xfr_reg &= ~SD_XFR_CTH_SEL;
			pslotinfo->sta_reg = (SD_STA_WRITE_XFR_ACTIVE |
				SD_STA_DAT_ACTIVE);
			if (pslotinfo->use_adma == 1) {
				pslotinfo->pre_dma = &ambarella_sd_pre_sg_to_adma;
				pslotinfo->post_dma = &ambarella_sd_post_sg_to_adma;
			} else {
				pslotinfo->pre_dma = &ambarella_sd_pre_sg_to_dma;
				pslotinfo->post_dma = &ambarella_sd_post_sg_to_dma;
			}
		} else {
			pslotinfo->xfr_reg |= SD_XFR_CTH_SEL;
			pslotinfo->sta_reg = (SD_STA_READ_XFR_ACTIVE |
				SD_STA_DAT_ACTIVE);
			if (pslotinfo->use_adma == 1) {
				pslotinfo->pre_dma = &ambarella_sd_pre_adma_to_sg;
				pslotinfo->post_dma = &ambarella_sd_post_adma_to_sg;
			} else {
				pslotinfo->pre_dma = &ambarella_sd_pre_dma_to_sg;
				pslotinfo->post_dma = &ambarella_sd_post_dma_to_sg;
			}
		}
		pslotinfo->pre_dma(pslotinfo);
		if (pinfo->dma_fix) {
			pslotinfo->dma_address |= pinfo->dma_fix;
		}
	}
}

static inline void ambarella_sd_send_cmd(
	struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo;
	u32 valid_request = 0;
	u32 counter = 0;
	u32 sta_reg;
	u32 tmpreg;
	long timeout;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(pslotinfo->mmc);

	valid_request = ambarella_sd_check_cd(pslotinfo->mmc);
	ambsd_dbg(pslotinfo, "cmd = %u valid_request = %u.\n",
		pslotinfo->mrq->cmd->opcode, valid_request);
	if (!valid_request) {
		pslotinfo->mrq->cmd->error = -ENOMEDIUM;
		pslotinfo->state = AMBA_SD_STATE_ERR;
		goto ambarella_sd_send_cmd_exit;
	}

	ambarella_sd_check_ios(pslotinfo->mmc, &pslotinfo->mmc->ios);
	if (pslotinfo->mrq->data) {
		while (1) {
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((sta_reg & SD_STA_CMD_INHIBIT_DAT) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_STA_CMD_INHIBIT_DAT...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				pinfo->reset_error = 1;
				goto ambarella_sd_send_cmd_exit;
			}
		}

		amba_writeb(pinfo->regbase + SD_TMO_OFFSET, pslotinfo->tmo);
		if (pslotinfo->use_adma == 1) {
			amba_setbitsb(pinfo->regbase + SD_HOST_OFFSET,
				SD_HOST_ADMA);
			amba_writel(pinfo->regbase + SD_ADMA_ADDR_OFFSET,
				pslotinfo->dma_address);
		} else {
			amba_clrbitsb(pinfo->regbase + SD_HOST_OFFSET,
				SD_HOST_ADMA);
			amba_writel(pinfo->regbase + SD_DMA_ADDR_OFFSET,
				pslotinfo->dma_address);
		}
		amba_write2w(pinfo->regbase + SD_BLK_SZ_OFFSET,
			pslotinfo->blk_sz, pslotinfo->blk_cnt);
		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			pslotinfo->xfr_reg, pslotinfo->cmd_reg);
	} else {
		while (1) {
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((sta_reg & SD_STA_CMD_INHIBIT_CMD) == 0) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_WAIT_COUNTER_LIMIT) {
				ambsd_warn(pslotinfo,
					"Wait SD_STA_CMD_INHIBIT_CMD...\n");
				pslotinfo->state = AMBA_SD_STATE_ERR;
				pinfo->reset_error = 1;
				goto ambarella_sd_send_cmd_exit;
			}
		}

		amba_writel(pinfo->regbase + SD_ARG_OFFSET, pslotinfo->arg_reg);
		amba_write2w(pinfo->regbase + SD_XFR_OFFSET,
			0x00, pslotinfo->cmd_reg);
	}

ambarella_sd_send_cmd_exit:
	if (pslotinfo->state == AMBA_SD_STATE_CMD) {
		timeout = wait_event_timeout(pslotinfo->wait,
			(pslotinfo->state != AMBA_SD_STATE_CMD),
			pslotinfo->wait_tmo);
		if (pslotinfo->state == AMBA_SD_STATE_CMD) {
			ambsd_err(pslotinfo,
				"cmd%u %u@[%ld:%u], sta=0x%04x\n",
				pslotinfo->mrq->cmd->opcode,
				pslotinfo->state, timeout, pslotinfo->wait_tmo,
				amba_readl(pinfo->regbase + SD_STA_OFFSET));
			pslotinfo->mrq->cmd->error = -ETIMEDOUT;
		}
	} else if (pslotinfo->state == AMBA_SD_STATE_DATA) {
		do {
			timeout = wait_event_timeout(pslotinfo->wait,
				(pslotinfo->state != AMBA_SD_STATE_DATA),
				pslotinfo->wait_tmo);
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			if ((pslotinfo->state == AMBA_SD_STATE_DATA) &&
				(sta_reg & pslotinfo->sta_reg)) {
				ambsd_rtdbg(pslotinfo, "data%u %u@"
					"[%ld:%u:%u:%u], sta=0x%04x:0x%04x\n",
					pslotinfo->mrq->cmd->opcode,
					pslotinfo->state, timeout,
					pslotinfo->wait_tmo,
					pslotinfo->mrq->data->timeout_ns,
					pslotinfo->mrq->data->timeout_clks,
					sta_reg, pslotinfo->sta_reg);
				ambsd_rtdbg(pslotinfo,
					"DMA %u in %u sg [0x%08x:0x%08x]\n",
					pslotinfo->dma_size,
					pslotinfo->sg_len,
					pslotinfo->dma_address,
					pslotinfo->dma_size);
				tmpreg = amba_readw(pinfo->regbase +
					SD_BLK_CNT_OFFSET);
				if (tmpreg) {
					ambsd_rtdbg(pslotinfo,
						"SD_DMA_ADDR_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_DMA_ADDR_OFFSET));
					amba_writel((pinfo->regbase +
						SD_DMA_ADDR_OFFSET),
						amba_readl(pinfo->regbase +
						SD_DMA_ADDR_OFFSET));
				} else {
					ambsd_rtdbg(pslotinfo,
						"SD_DATA_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_DATA_OFFSET));
					ambsd_rtdbg(pslotinfo,
						"SD_STA_OFFSET[0x%08X]\n",
						amba_readl(pinfo->regbase +
						SD_STA_OFFSET));
				}
			} else {
				break;
			}
		} while (pslotinfo->sta_counter--);
		if (pslotinfo->state == AMBA_SD_STATE_DATA) {
			ambsd_err(pslotinfo,
				"data%u %u@%u, sta=0x%04x:0x%04x\n",
				pslotinfo->mrq->cmd->opcode,
				pslotinfo->state,
				pslotinfo->wait_tmo,
				sta_reg, pslotinfo->sta_reg);
			pslotinfo->mrq->data->error = -ETIMEDOUT;
		}
	}
}

static inline void ambarella_sd_post_cmd(
	struct ambarella_sd_mmc_info *pslotinfo)
{
	struct ambarella_sd_controller_info *pinfo;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;
	if (pslotinfo->state == AMBA_SD_STATE_IDLE) {
		ambarella_sd_release_bus(pslotinfo->mmc);
		if (pslotinfo->mrq->data) {
			pslotinfo->post_dma(pslotinfo);
		}
	} else {
#ifdef CONFIG_SD_AMBARELLA_DEBUG_VERBOSE
		u32 counter = 0;

		ambsd_err(pslotinfo, "CMD%u retries[%u] state[%u].\n",
			pslotinfo->mrq->cmd->opcode,
			pslotinfo->mrq->cmd->retries,
			pslotinfo->state);
		for (counter = 0; counter < 0x100; counter += 4) {
			ambsd_err(pslotinfo, "0x%04x: 0x%08x\n",
			counter, amba_readl(pinfo->regbase + counter));
		}
		ambarella_sd_show_info(pslotinfo);
#endif
		if (pinfo->reset_error) {
			ambarella_sd_reset_all(pslotinfo->mmc);
		}
		ambarella_sd_release_bus(pslotinfo->mmc);
	}
}

static void ambarella_sd_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);

	pslotinfo->mrq = mrq;
	ambarella_sd_pre_cmd(pslotinfo);
	ambarella_sd_send_cmd(pslotinfo);
	ambarella_sd_post_cmd(pslotinfo);
	pslotinfo->mrq = NULL;
	mmc_request_done(mmc, mrq);
}

static void ambarella_sd_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	ambarella_sd_request_bus(mmc);
	ambarella_sd_check_ios(mmc, ios);
	ambarella_sd_release_bus(mmc);
}

static int ambarella_sd_get_ro(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u32 wpspl;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);

	if (pslotinfo->plat_info->fixed_wp < 0) {
		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1) {
			wpspl = ambarella_get_gpio_input(
				&pslotinfo->plat_info->gpio_wp);
		} else {
			wpspl = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			wpspl &= SD_STA_WPS_PL;
		}
	} else {
		wpspl =	pslotinfo->plat_info->fixed_wp;
	}

	ambarella_sd_release_bus(mmc);

	ambsd_dbg(pslotinfo, "RO[%u].\n", wpspl);
	return wpspl ? 1 : 0;
}

static int ambarella_sd_get_cd(struct mmc_host *mmc)
{
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	u32 cdpin;

	ambarella_sd_request_bus(mmc);
	cdpin = ambarella_sd_check_cd(mmc);
	ambarella_sd_release_bus(mmc);

	ambsd_dbg(pslotinfo, "CD[%u].\n", cdpin);
	return cdpin ? 1 : 0;
}

static void ambarella_sd_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	if (enable)
		ambarella_sd_enable_int(mmc, SD_NISEN_CARD);
	else
		ambarella_sd_disable_int(mmc, SD_NISEN_CARD);
}

static int ambarella_sd_start_signal_voltage_switch(struct mmc_host *mmc,
	struct mmc_ios *ios)
{
	int retval = 0;
	struct ambarella_sd_mmc_info *pslotinfo = mmc_priv(mmc);
	struct ambarella_sd_controller_info *pinfo;
	u32 counter = 0;
	u32 sta_reg;

	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	ambarella_sd_request_bus(mmc);
	if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_330) {
#if 0	//Not necessary in current kernel
		if (pslotinfo->plat_info->set_vdd) {
			pslotinfo->plat_info->set_vdd(3300);
		}
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_3_3V));
#endif
	} else if (ios->signal_voltage == MMC_SIGNAL_VOLTAGE_180) {
		ambarella_sd_clear_clken(mmc);
		msleep(CONFIG_SD_AMBARELLA_VSW_PRE_SPEC);
		if (pslotinfo->plat_info->set_vdd) {
			pslotinfo->plat_info->set_vdd(1800);
		}
		amba_writeb(pinfo->regbase + SD_PWR_OFFSET,
			(SD_PWR_ON | SD_PWR_1_8V));
		ambarella_sd_set_clken(mmc);
		msleep(CONFIG_SD_AMBARELLA_VSW_POST_SPEC);
		while (1) {
			sta_reg = amba_readl(pinfo->regbase + SD_STA_OFFSET);
			ambsd_dbg(pslotinfo, "SD_STA_OFFSET = 0x%x.\n",
				sta_reg);
			ambsd_dbg(pslotinfo, "SD_CLK_OFFSET = 0x%x.\n",
				amba_readw(pinfo->regbase + SD_CLK_OFFSET));
			ambsd_dbg(pslotinfo, "SD_PWR_OFFSET = 0x%x.\n",
				amba_readb(pinfo->regbase + SD_PWR_OFFSET));
			if ((sta_reg & 0x1f00000) == 0x1f00000) {
				break;
			}
			counter++;
			if (counter > CONFIG_SD_AMBARELLA_VSW_WAIT_LIMIT) {
				retval = -1;
				ambsd_err(pslotinfo, "SD_STA_OFFSET = 0x%x.\n",
					sta_reg);
				break;
			}
			msleep(1);
		}
	}
	ambarella_sd_release_bus(mmc);

	return retval;
}

static const struct mmc_host_ops ambarella_sd_host_ops = {
	.request = ambarella_sd_request,
	.set_ios = ambarella_sd_ios,
	.get_ro = ambarella_sd_get_ro,
	.get_cd = ambarella_sd_get_cd,
	.enable_sdio_irq = ambarella_sd_enable_sdio_irq,
	.start_signal_voltage_switch = ambarella_sd_start_signal_voltage_switch,
};

static int ambarella_sd_system_event(struct notifier_block *nb,
	unsigned long val, void *data)
{
	int retval = NOTIFY_OK;
	struct ambarella_sd_mmc_info *pslotinfo;
	struct ambarella_sd_controller_info *pinfo;

	pslotinfo = container_of(nb, struct ambarella_sd_mmc_info,
		system_event);
	pinfo = (struct ambarella_sd_controller_info *)pslotinfo->pinfo;

	switch (val) {
	case AMBA_EVENT_PRE_CPUFREQ:
		pr_debug("%s[%u]: Pre Change\n", __func__, pslotinfo->slot_id);
		down(&pslotinfo->system_event_sem);
		break;

	case AMBA_EVENT_POST_CPUFREQ:
		pr_debug("%s[%u]: Post Change\n", __func__, pslotinfo->slot_id);
		ambarella_sd_set_clk(pslotinfo->mmc, &pinfo->controller_ios);
		up(&pslotinfo->system_event_sem);
		break;

	default:
		break;
	}

	return retval;
}

/* ==========================================================================*/
static int ambarella_sd_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo = NULL;
	struct resource *irq;
	struct resource *mem;
	struct mmc_host *mmc;
	u32 hc_cap = 0;
	u32 i;
	u32 clock_min;
	u32 hc_timeout_clk = 0;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get SD/MMC mem resource failed!\n");
		retval = -ENXIO;
		goto ambarella_sd_probe_na;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (irq == NULL) {
		dev_err(&pdev->dev, "Get SD/MMC irq resource failed!\n");
		retval = -ENXIO;
		goto ambarella_sd_probe_na;
	}

	pinfo = kzalloc(sizeof(struct ambarella_sd_controller_info),
		GFP_KERNEL);
	if (pinfo == NULL) {
		dev_err(&pdev->dev, "Out of memory!\n");
		retval = -ENOMEM;
		goto ambarella_sd_probe_na;
	}
	pinfo->regbase = (unsigned char __iomem *)mem->start;
	pinfo->dev = &pdev->dev;
	pinfo->irq = irq->start;
	pinfo->pcontroller =
		(struct ambarella_sd_controller *)pdev->dev.platform_data;
	if ((pinfo->pcontroller == NULL) ||
		(pinfo->pcontroller->get_pll == NULL) ||
		(pinfo->pcontroller->set_pll == NULL)) {
		dev_err(&pdev->dev, "Need SD/MMC controller info!\n");
		retval = -EPERM;
		goto ambarella_sd_probe_free_pinfo;
	}

	pinfo->dma_fix = pinfo->pcontroller->dma_fix;
	if (pinfo->pcontroller->wait_tmo < CONFIG_SD_AMBARELLA_WAIT_TIMEOUT) {
		dev_notice(&pdev->dev, "Change wait timeout from %u to %u.\n",
			pinfo->pcontroller->wait_tmo,
			CONFIG_SD_AMBARELLA_WAIT_TIMEOUT);
		pinfo->pcontroller->wait_tmo =
			CONFIG_SD_AMBARELLA_WAIT_TIMEOUT;
	}

	pinfo->pcontroller->set_pll(pinfo->pcontroller->max_clock ?
		pinfo->pcontroller->max_clock :
		CONFIG_SD_AMBARELLA_DEFAULT_CLOCK);
	pinfo->clk_limit = pinfo->pcontroller->get_pll();
	clock_min = pinfo->clk_limit >> 8;
	if (pinfo->clk_limit < clock_min) {
		pinfo->clk_limit = clock_min;
		dev_err(&pdev->dev, "Wrong clk_limit %uHz vs %uHz!\n",
			pinfo->clk_limit, clock_min);
		retval = -EPERM;
		goto ambarella_sd_probe_free_pinfo;
	}
	if (pinfo->pcontroller->max_clock != pinfo->clk_limit) {
		pinfo->pcontroller->max_clock = pinfo->clk_limit;
	}
	pinfo->max_blk_sz = ambarella_sd_dma_mask_to_size(
		pinfo->pcontroller->max_blk_mask);
	pinfo->buf_cache = kmem_cache_create(dev_name(&pdev->dev),
		pinfo->max_blk_sz, pinfo->max_blk_sz, 0, NULL);
	if (!pinfo->buf_cache) {
		dev_err(&pdev->dev, "Can't alloc DMA Cache");
		retval = -ENOMEM;
		goto ambarella_sd_probe_free_pinfo;
	}

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		mmc = mmc_alloc_host(sizeof(struct ambarella_sd_mmc_info),
			&pdev->dev);
		if (!mmc) {
			dev_err(&pdev->dev, "Failed to allocate mmc host"
				" for slot%u!\n", i);
			retval = -ENOMEM;
			goto ambarella_sd_probe_free_host;
		}
		mmc->ops = &ambarella_sd_host_ops;

		pinfo->pslotinfo[i] = pslotinfo = mmc_priv(mmc);
		pslotinfo->mmc = mmc;
		init_waitqueue_head(&pslotinfo->wait);
		pslotinfo->state = AMBA_SD_STATE_ERR;
		pslotinfo->plat_info = &(pinfo->pcontroller->slot[i]);
		if ((pslotinfo->plat_info == NULL) ||
			(pslotinfo->plat_info->check_owner == NULL) ||
			(pslotinfo->plat_info->set_int == NULL)) {
			dev_err(&pdev->dev, "Need SD/MMC slot info!\n");
			retval = -EPERM;
			goto ambarella_sd_probe_free_host;
		}
		pslotinfo->slot_id = i;
		pslotinfo->pinfo = pinfo;
		sema_init(&pslotinfo->system_event_sem, 1);

		ambarella_sd_request_bus(mmc);

		ambarella_sd_reset_all(mmc);

		hc_cap = amba_readl(pinfo->regbase + SD_CAP_OFFSET);
		hc_timeout_clk = SD_CAP_TOCLK_FREQ(hc_cap);
		if (hc_cap & SD_CAP_TOCLK_MHZ) {
			dev_dbg(&pdev->dev,
				"Timeout Clock Frequency: %uMHz.\n",
				SD_CAP_TOCLK_FREQ(hc_cap));
			hc_timeout_clk *= 1000;
		} else {
			dev_dbg(&pdev->dev,
				"Timeout Clock Frequency: %uKHz.\n",
				SD_CAP_TOCLK_FREQ(hc_cap));
		}

		mmc->f_min = clock_min;
		mmc->f_max = pinfo->clk_limit;
		dev_dbg(&pdev->dev,
			"SD Clock: base[%uMHz], min[%uHz], max[%uHz].\n",
			SD_CAP_BASE_FREQ(hc_cap),
			mmc->f_min,
			mmc->f_max);
		if (pslotinfo->plat_info->private_caps &
			AMBA_SD_PRIVATE_CAPS_DTO_BY_SDCLK) {
			hc_timeout_clk = mmc->f_max / 1000;
		}
		if (hc_timeout_clk == 0) {
			hc_timeout_clk = 24000;
		}
		mmc->max_discard_to = (1 << 27) / hc_timeout_clk;

		if (hc_cap & SD_CAP_MAX_2KB_BLK)
			mmc->max_blk_size = 2048;
		else if (hc_cap & SD_CAP_MAX_1KB_BLK)
			mmc->max_blk_size = 1024;
		else if (hc_cap & SD_CAP_MAX_512B_BLK)
			mmc->max_blk_size = 512;
		dev_dbg(&pdev->dev,
			"SD max_blk_size: %u.\n",
			mmc->max_blk_size);

		mmc->caps = pslotinfo->plat_info->default_caps;
		if (pinfo->clk_limit > 25000000) {
			mmc->caps |= MMC_CAP_SD_HIGHSPEED;
			mmc->caps |= MMC_CAP_MMC_HIGHSPEED;
		}
		if (hc_cap & SD_CAP_DMA) {
			dev_dbg(&pdev->dev, "HW support DMA!\n");
		} else {
			ambsd_err(pslotinfo, "HW do not support DMA!\n");
			retval = -ENODEV;
			goto ambarella_sd_probe_free_host;
		}
		pslotinfo->use_adma = 0;
		if (hc_cap & SD_CAP_ADMA_SUPPORT) {
			dev_dbg(&pdev->dev, "HW support ADMA!\n");
			if (pslotinfo->plat_info->private_caps &
				AMBA_SD_PRIVATE_CAPS_ADMA) {
				pslotinfo->use_adma = 1;
			}
		}
		if (hc_cap & SD_CAP_SUS_RES) {
			dev_dbg(&pdev->dev, "HW support Suspend/Resume!\n");
		} else {
			dev_dbg(&pdev->dev,
				"HW do not support Suspend/Resume!\n");
		}

		mmc->ocr_avail = 0;
		if ((hc_cap & SD_CAP_VOL_1_8V) &&
			(pslotinfo->plat_info->private_caps &
			AMBA_SD_PRIVATE_CAPS_VDD_18)) {
			mmc->ocr_avail |= MMC_VDD_165_195;
			if (hc_cap & SD_CAP_HIGH_SPEED) {
				mmc->caps |= MMC_CAP_1_8V_DDR;
			}
			mmc->caps |= MMC_CAP_UHS_SDR12;
			if (pinfo->clk_limit > 25000000) {
				mmc->caps |= MMC_CAP_UHS_SDR25;
				if (pslotinfo->plat_info->private_caps &
					AMBA_SD_PRIVATE_CAPS_DDR) {
					mmc->caps |= MMC_CAP_UHS_DDR50;
				}
			}
			if (pinfo->clk_limit > 50000000) {
				mmc->caps |= MMC_CAP_UHS_SDR50;
			}
			if (pinfo->clk_limit > 100000000) {
				mmc->caps |= MMC_CAP_UHS_SDR104;
			}
		}
		if (hc_cap & SD_CAP_VOL_3_3V) {
			mmc->ocr_avail |= MMC_VDD_32_33 | MMC_VDD_33_34;
		}
		if (mmc->ocr_avail == 0) {
			ambsd_err(pslotinfo,
				"Hardware report wrong voltages[0x%x].!\n",
				hc_cap);
			retval = -ENODEV;
			goto ambarella_sd_probe_free_host;
		}

		if (hc_cap & SD_CAP_INTMODE) {
			dev_dbg(&pdev->dev, "HW support Interrupt mode!\n");
		} else {
			ambsd_err(pslotinfo,
				"HW do not support Interrupt mode!\n");
			retval = -ENODEV;
			goto ambarella_sd_probe_free_host;
		}

		mmc->max_blk_count = 0xFFFF;
		mmc->max_seg_size = pinfo->max_blk_sz;
		mmc->max_segs = mmc->max_seg_size / PAGE_SIZE;
		mmc->max_req_size = min(mmc->max_seg_size,
			mmc->max_blk_size * mmc->max_blk_count);
		pslotinfo->buf_vaddress = kmem_cache_alloc(
			pinfo->buf_cache, GFP_KERNEL);
		if (!pslotinfo->buf_vaddress) {
			ambsd_err(pslotinfo, "Can't alloc DMA memory");
			retval = -ENOMEM;
			goto ambarella_sd_probe_free_host;
		}
		pslotinfo->buf_paddress = dma_map_single(
			pinfo->dev, pslotinfo->buf_vaddress,
			mmc->max_req_size, DMA_BIDIRECTIONAL);
		if (ambarella_sd_check_dma_boundary(pslotinfo,
			pslotinfo->buf_paddress, mmc->max_req_size,
			pinfo->max_blk_sz) == 0) {
			ambsd_err(pslotinfo, "DMA boundary err!\n");
			retval = -ENOMEM;
			goto ambarella_sd_probe_free_host;
		}
		dev_notice(&pdev->dev, "Slot%u use bounce buffer["
			"0x%p<->0x%08x]\n", pslotinfo->slot_id,
			pslotinfo->buf_vaddress,
			pslotinfo->buf_paddress);

		pslotinfo->plat_info->active_caps = mmc->caps;
		dev_dbg(&pdev->dev, "SD caps: 0x%x.\n", mmc->caps);
		dev_dbg(&pdev->dev, "SD ocr: 0x%x.\n", mmc->ocr_avail);
		dev_notice(&pdev->dev, "Slot%u req_size=0x%08X, "
			"segs=%u, seg_size=0x%08X\n",
			pslotinfo->slot_id, mmc->max_req_size,
			mmc->max_segs, mmc->max_seg_size);
		if (pslotinfo->use_adma == 1) {
			dev_notice(&pdev->dev, "Slot%u use ADMA\n",
				pslotinfo->slot_id);
		} else {
			dev_notice(&pdev->dev, "Slot%u use DMA\n",
				pslotinfo->slot_id);
		}

		if (pslotinfo->plat_info->ext_power.gpio_id != -1) {
			retval = gpio_request(
				pslotinfo->plat_info->ext_power.gpio_id,
				pdev->name);
			if (retval < 0) {
				ambsd_err(pslotinfo, "Can't get Power GPIO%d\n",
				pslotinfo->plat_info->ext_power.gpio_id);
				pslotinfo->plat_info->ext_power.gpio_id = -1;
			}
		}

		if (pslotinfo->plat_info->ext_reset.gpio_id != -1) {
			retval = gpio_request(
				pslotinfo->plat_info->ext_reset.gpio_id,
				pdev->name);
			if (retval < 0) {
				ambsd_err(pslotinfo, "Can't get Reset GPIO%d\n",
				pslotinfo->plat_info->ext_reset.gpio_id);
				pslotinfo->plat_info->ext_reset.gpio_id = -1;
			}
		}

		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1) {
			retval = gpio_request(
				pslotinfo->plat_info->gpio_wp.gpio_id,
				pdev->name);
			if (retval < 0) {
				ambsd_err(pslotinfo, "Can't get WP GPIO%d\n",
				pslotinfo->plat_info->gpio_wp.gpio_id);
				pslotinfo->plat_info->gpio_wp.gpio_id = -1;
			}
		}

		ambarella_sd_release_bus(mmc);
	}

	retval = request_irq(pinfo->irq, ambarella_sd_irq,
		IRQF_SHARED | IRQF_TRIGGER_HIGH,
		dev_name(&pdev->dev), pinfo);
	if (retval) {
		dev_err(&pdev->dev, "Can't Request IRQ%u!\n", pinfo->irq);
		goto ambarella_sd_probe_free_host;
	}

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		pslotinfo->system_event.notifier_call =
			ambarella_sd_system_event;
		ambarella_register_event_notifier(&pslotinfo->system_event);

		if (ambarella_is_valid_gpio_irq(
			&pslotinfo->plat_info->gpio_cd)) {
			retval = gpio_request(
				pslotinfo->plat_info->gpio_cd.irq_gpio,
				pdev->name);
			if (retval < 0) {
				ambsd_err(pslotinfo, "Can't get CD GPIO%d\n",
				pslotinfo->plat_info->gpio_cd.irq_gpio);
				pslotinfo->plat_info->gpio_cd.irq_gpio = -1;
				continue;
			}

			ambarella_sd_gpio_cd_check_val(pslotinfo);
			retval = request_irq(
				pslotinfo->plat_info->gpio_cd.irq_line,
				ambarella_sd_gpio_cd_irq,
				pslotinfo->plat_info->gpio_cd.irq_type,
				dev_name(&pdev->dev), pslotinfo);
			if (retval) {
				ambsd_err(pslotinfo,
				"Can't Request GPIO(%d) CD IRQ(%d)!\n",
				pslotinfo->plat_info->gpio_cd.irq_gpio,
				pslotinfo->plat_info->gpio_cd.irq_line);
			}
		}
	}

	platform_set_drvdata(pdev, pinfo);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		pslotinfo->plat_info->pmmc_host = pslotinfo->mmc;
		pslotinfo->valid = 1;
		retval = mmc_add_host(pslotinfo->mmc);
		if (retval) {
			ambsd_err(pslotinfo, "Can't add mmc host!\n");
		}
	}

	dev_notice(&pdev->dev,
		"Ambarella SD/MMC[%d] has %u slots @ %uHz, [0x%08x:0x%08x]\n",
		pdev->id, pinfo->pcontroller->num_slots,
		pinfo->clk_limit, hc_cap, pinfo->dma_fix);
	retval = 0;
	goto ambarella_sd_probe_na;

ambarella_sd_probe_free_host:
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->plat_info->gpio_wp.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->gpio_wp.gpio_id);
		if (pslotinfo->plat_info->ext_power.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->ext_power.gpio_id);
		if (pslotinfo->plat_info->ext_reset.gpio_id != -1)
			gpio_free(pslotinfo->plat_info->ext_reset.gpio_id);
		if (pslotinfo->buf_paddress) {
			dma_unmap_single(pinfo->dev, pslotinfo->buf_paddress,
				pslotinfo->mmc->max_req_size,
				DMA_BIDIRECTIONAL);
			pslotinfo->buf_paddress = (dma_addr_t)NULL;
		}
		if (pslotinfo->buf_vaddress) {
			kmem_cache_free(pinfo->buf_cache,
				pslotinfo->buf_vaddress);
			pslotinfo->buf_vaddress = NULL;
		}
		if (pslotinfo->mmc) {
			mmc_free_host(pslotinfo->mmc);
		}
	}
	if (pinfo->buf_cache) {
		kmem_cache_destroy(pinfo->buf_cache);
		pinfo->buf_cache = NULL;
	}

ambarella_sd_probe_free_pinfo:
	kfree(pinfo);

ambarella_sd_probe_na:
	return retval;
}

static int ambarella_sd_remove(struct platform_device *pdev)
{
	int retval = 0;
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;
	u32 i;

	pinfo = platform_get_drvdata(pdev);

	if (pinfo) {
		free_irq(pinfo->irq, pinfo);

		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			pslotinfo->plat_info->pmmc_host = NULL;
			ambarella_unregister_event_notifier(
				&pslotinfo->system_event);
			if (ambarella_is_valid_gpio_irq(
				&pslotinfo->plat_info->gpio_cd)) {
				free_irq(pslotinfo->plat_info->gpio_cd.irq_line,
					pslotinfo);
				gpio_free(
					pslotinfo->plat_info->gpio_cd.irq_gpio);
			}
			if (pslotinfo->mmc) {
				mmc_remove_host(pslotinfo->mmc);
			}
		}

		for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
			pslotinfo = pinfo->pslotinfo[i];
			if (pslotinfo->plat_info->ext_power.gpio_id != -1) {
				gpio_free(
				pslotinfo->plat_info->ext_power.gpio_id);
			}
			if (pslotinfo->plat_info->ext_reset.gpio_id != -1) {
				gpio_free(
				pslotinfo->plat_info->ext_reset.gpio_id);
			}
			if (pslotinfo->plat_info->gpio_wp.gpio_id != -1) {
				gpio_free(
				pslotinfo->plat_info->gpio_wp.gpio_id);
			}
			if (pslotinfo->buf_paddress) {
				dma_unmap_single(pinfo->dev,
					pslotinfo->buf_paddress,
					pslotinfo->mmc->max_req_size,
					DMA_BIDIRECTIONAL);
				pslotinfo->buf_paddress = (dma_addr_t)NULL;
			}
			if (pslotinfo->buf_vaddress) {
				kmem_cache_free(pinfo->buf_cache,
					pslotinfo->buf_vaddress);
				pslotinfo->buf_vaddress = NULL;
			}
			if (pslotinfo->mmc) {
				mmc_free_host(pslotinfo->mmc);
			}
		}
		if (pinfo->buf_cache) {
			kmem_cache_destroy(pinfo->buf_cache);
			pinfo->buf_cache = NULL;
		}
		kfree(pinfo);
	}

	dev_notice(&pdev->dev,
		"Remove Ambarella Media Processor SD/MMC Host Controller.\n");

	return retval;
}

#ifdef CONFIG_AMBARELLA_EMMC_BOOT
static void ambarella_sd_shutdown(struct platform_device *pdev)
{
	struct ambarella_sd_controller_info	*pinfo;
	struct mmc_host				*host;
	struct mmc_command			cmd;

	pinfo = platform_get_drvdata(pdev);

	if (pdev->id == 0 && pinfo) {
		if ((system_state == SYSTEM_RESTART) ||
			(system_state == SYSTEM_HALT)) {
			host = pinfo->pslotinfo[0]->mmc;

			if (mmc_try_claim_host(host)) {
				cmd.opcode	= 0;
				cmd.arg		= 0xf0f0f0f0;
				cmd.flags	= MMC_RSP_NONE;

				mmc_wait_for_cmd(host, &cmd, 0);
			} else {
				dev_err(&pdev->dev, "Unable to claim host!\n");
			}
		}

		dev_info(&pdev->dev, "%s @ %d.\n", __func__, system_state);
	}
}
#endif

#ifdef CONFIG_PM
static int ambarella_sd_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int retval = 0;
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;
	u32 i;

	pinfo = platform_get_drvdata(pdev);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			retval = mmc_suspend_host(pslotinfo->mmc);
			if (retval) {
				ambsd_err(pslotinfo,
				"mmc_suspend_host[%d] failed[%d]!\n",
				i, retval);
			}
		}
	}

	disable_irq(pinfo->irq);
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd))
			disable_irq(pslotinfo->plat_info->gpio_cd.irq_line);
	}

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, retval, state.event);
	return retval;
}

static int ambarella_sd_resume(struct platform_device *pdev)
{
	int retval = 0;
	struct ambarella_sd_controller_info *pinfo;
	struct ambarella_sd_mmc_info *pslotinfo;
	u32 i;

	pinfo = platform_get_drvdata(pdev);
	pinfo->pcontroller->set_pll(pinfo->clk_limit);
	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		ambarella_sd_reset_all(pslotinfo->mmc);
		if (ambarella_is_valid_gpio_irq(&pslotinfo->plat_info->gpio_cd))
			enable_irq(pslotinfo->plat_info->gpio_cd.irq_line);
	}
	enable_irq(pinfo->irq);

	for (i = 0; i < pinfo->pcontroller->num_slots; i++) {
		pslotinfo = pinfo->pslotinfo[i];
		if (pslotinfo->mmc) {
			retval = mmc_resume_host(pslotinfo->mmc);
			if (retval) {
				ambsd_err(pslotinfo,
				"mmc_resume_host[%d] failed[%d]!\n",
				i, retval);
			}
		}
	}

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, retval);

	return retval;
}
#endif

static struct platform_driver ambarella_sd_driver = {
	.probe		= ambarella_sd_probe,
	.remove		= ambarella_sd_remove,
#ifdef CONFIG_AMBARELLA_EMMC_BOOT
	.shutdown	= ambarella_sd_shutdown,
#endif
#ifdef CONFIG_PM
	.suspend	= ambarella_sd_suspend,
	.resume		= ambarella_sd_resume,
#endif
	.driver		= {
		.name	= "ambarella-sd",
		.owner	= THIS_MODULE,
	},
};

static int __init ambarella_sd_init(void)
{
	int retval = 0;

	retval = platform_driver_register(&ambarella_sd_driver);
	if (retval) {
		printk(KERN_ERR "%s: Register failed %d!\n",
			__func__, retval);
	}

	return retval;
}

static void __exit ambarella_sd_exit(void)
{
	platform_driver_unregister(&ambarella_sd_driver);
}

fs_initcall(ambarella_sd_init);
module_exit(ambarella_sd_exit);

MODULE_DESCRIPTION("Ambarella Media Processor SD/MMC Host Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("GPL");

