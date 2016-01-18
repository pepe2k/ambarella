/*
 * kernel/private/drivers/ambarella/vin/vin_irq.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>
#include "vin_pri.h"

u32 vsync_irq_flag = 0;
EXPORT_SYMBOL(vsync_irq_flag);
wait_queue_head_t vsync_irq_wait;
EXPORT_SYMBOL(vsync_irq_wait);

/* ==========================================================================*/
static const char amba_vin_vsync_name[] = "vsync";
static const char amba_vin_vin_name[] = "vin";
static const char amba_vin_idsp_last_pixel_name[] = "idsp_last_pixel";
static const char amba_vin_idsp_name[] = "idsp";
static const char amba_vin_gpio_name[] = "gpio";
static const char amba_vin_idsp_sof_name[] = "idsp_sof";

static const char *amba_vin_irq_name[AMBA_VIN_VSYNC_IRQ_NUM] = {
	amba_vin_vsync_name,
	amba_vin_vin_name,
	amba_vin_idsp_last_pixel_name,
	amba_vin_idsp_name,
	amba_vin_gpio_name,
	amba_vin_idsp_sof_name,
};


/* ==========================================================================*/
static irqreturn_t amba_vin_irq_irq(int irqno, void *dev_id)
{
	struct __amba_vin_irq_info		*pirqinfo;

	pirqinfo = (struct __amba_vin_irq_info *)dev_id;

	if (irqno == IDSP_SENSOR_VSYNC_IRQ) {
		vsync_irq_flag++;
		wake_up(&vsync_irq_wait);
	}

	if (pirqinfo->callback){
		pirqinfo->callback(pirqinfo->callback_data, pirqinfo->counter);
#if (CHIP_REV == S2)
		if(irqno == IDSP_PIP_LAST_PIXEL_IRQ) {// special for S2
			amba_writel(APB_BASE + 0x118000, 0x8000);
			amba_writel(APB_BASE + 0x107ca0, 0x1008);
		}
#endif
	} else {
		pirqinfo->counter++;
		atomic_set(&pirqinfo->proc_hinfo.sync_proc_flag, 0xFFFFFFFF);
		wake_up_all(&pirqinfo->proc_hinfo.sync_proc_head);
	}

	return IRQ_HANDLED;
}

static int amba_vin_irq_read(char *start, void *data)
{
	struct __amba_vin_irq_info		*pirqinfo;

	pirqinfo = (struct __amba_vin_irq_info *)data;

	return sprintf(start, "%08x", pirqinfo->counter);
}

static const struct file_operations amba_vin_irq_fops = {
	.open = ambsync_proc_open,
	.read = ambsync_proc_read,
	.release = ambsync_proc_release,
	.write = ambsync_proc_write,
};

/* ==========================================================================*/
int amba_vin_irq_add(struct __amba_vin_adapter *adap, unsigned int irq,
	unsigned long flags, u32 mode, struct __amba_vin_irq_info *pirqinfo, u8 use_proc)
{
	int					retval = 0;

	if (mode >= AMBA_VIN_VSYNC_IRQ_NUM) {
		vin_err("%s unknown mode %d!\n", __func__, mode);
		retval = -EINVAL;
		goto amba_vin_irq_add_exit;
	}

	pirqinfo->irq = irq;
	pirqinfo->flags = flags;
	snprintf(pirqinfo->name, sizeof(pirqinfo->name),
		"%s_%s", adap->name, amba_vin_irq_name[mode]);

	pirqinfo->counter = 0;
	ambsync_proc_hinit(&pirqinfo->proc_hinfo);
	pirqinfo->proc_hinfo.sync_read_proc = amba_vin_irq_read;
	pirqinfo->proc_hinfo.sync_read_data = pirqinfo;

	pirqinfo->callback_data = NULL;
	//pirqinfo->callback = NULL;

	if ((irq >= VIC_IRQ(0)) && (irq < VIC_IRQ(NR_IRQS))) {
		retval = request_irq(irq, amba_vin_irq_irq, flags,
			pirqinfo->name, pirqinfo);
		if (retval) {
			vin_err("%s can't request IRQ(%d)!\n", __func__, irq);
			goto amba_vin_irq_add_exit;
		}
	}

	if(use_proc) {
		pirqinfo->proc_file = proc_create_data(pirqinfo->name, S_IRUGO,
			get_ambarella_proc_dir(), &amba_vin_irq_fops,
			&pirqinfo->proc_hinfo);
		if (pirqinfo->proc_file == NULL) {
			vin_err("%s can't register %s!\n", __func__,
				amba_vin_irq_name[mode]);
			retval = -EINVAL;
			goto amba_vin_irq_add_freeirq;
		}
	}

	goto amba_vin_irq_add_exit;

amba_vin_irq_add_freeirq:
	pirqinfo->irq = -1;
	if ((irq >= VIC_IRQ(0)) && (irq < VIC_IRQ(NR_IRQS))) {
		free_irq(irq, pirqinfo);
	}

amba_vin_irq_add_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_irq_add);

void amba_vin_irq_remove(struct __amba_vin_irq_info *pirqinfo)
{
	if (pirqinfo) {
		if ((pirqinfo->irq >= VIC_IRQ(0)) &&
			(pirqinfo->irq < VIC_IRQ(NR_IRQS))) {
			free_irq(pirqinfo->irq, pirqinfo);
		}
		if (pirqinfo->proc_file)
			remove_proc_entry(pirqinfo->name,
				get_ambarella_proc_dir());
	}
}
EXPORT_SYMBOL(amba_vin_irq_remove);

/* ==========================================================================*/
static void amba_vin_vsync_update_proc(struct __amba_vin_irq_info *pirqinfo)
{
	if (pirqinfo) {
		pirqinfo->counter++;
		atomic_set(&pirqinfo->proc_hinfo.sync_proc_flag, 0xFFFFFFFF);
		wake_up_all(&pirqinfo->proc_hinfo.sync_proc_head);
	}
}

static void amba_vin_vsync_callback(void *callback_data, u32 counter)
{
	struct __amba_vin_adapter		*adap;
	ktime_t					ktime;

	adap = (struct __amba_vin_adapter *)callback_data;
#ifdef CONFIG_VIN_FPS_STAT
	amba_vin_vsync_update_fps_stat();
#endif

	if (adap->vsync_delay) {
			ktime = ktime_set(0, adap->vsync_delay);  //nS
			highres_timer_start(&adap->vsync_timer, ktime, HRTIMER_MODE_REL);
	} else {
		amba_vin_vsync_update_proc(adap->pvsync_irq);
	}
}

static enum hrtimer_restart amba_vin_vsync_timer(struct hrtimer *timer)
{
	struct __amba_vin_adapter		*adap;
	unsigned long				flags;

	adap = (struct __amba_vin_adapter *)container_of(timer,
		struct __amba_vin_adapter, vsync_timer);

	spin_lock_irqsave(&adap->lock, flags);
	amba_vin_vsync_update_proc(adap->pvsync_irq);
	spin_unlock_irqrestore(&adap->lock, flags);

	return HRTIMER_NORESTART;
}

int amba_vin_vsync_bind(struct __amba_vin_adapter *adap,
	struct amba_vin_irq_fix *pirq_fix)
{
	int					retval = 0;
	struct __amba_vin_irq_info		*pirqinfo = NULL;

	if (adap->vsync_mode != AMBA_VIN_VSYNC_IRQ_NUM) {
		vin_err("%s still active %d!\n", __func__, adap->vsync_mode);
		retval = -EINVAL;
		goto amba_vin_irq_bind_exit;
	}

	if (pirq_fix == NULL) {
		vin_err("%s NULL pirq_fix!\n", __func__);
		retval = -EINVAL;
		goto amba_vin_irq_bind_exit;
	}

	if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_VSYNC) {
		vin_warn("%s: No need bind to vsync itself!\n", __func__);
	} else if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_VIN) {
		pirqinfo = adap->pvin_irq;
	} else if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_IDSP_LAST_PIXEL) {
		pirqinfo = adap->pidsp_last_pixel_irq;
	} else if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_IDSP) {
		pirqinfo = adap->pidsp_irq;
	} else if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_GPIO) {
		pirqinfo = adap->pgpio_irq;
	} else if (pirq_fix->mode == AMBA_VIN_VSYNC_IRQ_IDSP_SOF) {
		pirqinfo = adap->pidsp_sof_irq;
	} else {
		vin_err("%s unknown mode %d!\n", __func__, pirq_fix->mode);
		retval = -EINVAL;
		goto amba_vin_irq_bind_exit;
	}

	if (pirqinfo) {
		adap->vsync_timer.function = amba_vin_vsync_timer;
		adap->vsync_mode = pirq_fix->mode;
		adap->vsync_delay = pirq_fix->delay;
		if ((pirqinfo->irq >= VIC_IRQ(0)) &&
			(pirqinfo->irq < VIC_IRQ(NR_IRQS))) {
			synchronize_irq(pirqinfo->irq);
		}
		pirqinfo->callback_data = adap;
		pirqinfo->callback = amba_vin_vsync_callback;
		vin_info("%s to %s with %dns delay!\n",
			__func__, amba_vin_irq_name[adap->vsync_mode],
			adap->vsync_delay);
	} else {
		vin_warn("%s: Skip NULL pirqinfo!\n", __func__);
	}

amba_vin_irq_bind_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_vsync_bind);

void amba_vin_vsync_unbind(struct __amba_vin_adapter *adap)
{
	struct __amba_vin_irq_info		*pirqinfo = NULL;

	if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_VSYNC) {
		vin_warn("%s: No need unbind from vsync itself!\n", __func__);
	} else if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_VIN) {
		pirqinfo = adap->pvin_irq;
	} else if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_IDSP_LAST_PIXEL) {
		pirqinfo = adap->pidsp_last_pixel_irq;
	} else if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_IDSP) {
		pirqinfo = adap->pidsp_irq;
	} else if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_GPIO) {
		pirqinfo = adap->pgpio_irq;
	} else if (adap->vsync_mode == AMBA_VIN_VSYNC_IRQ_IDSP_SOF) {
		pirqinfo = adap->pidsp_sof_irq;
	} else {
		vin_err("%s unknown vsync_mode %d!\n",
			__func__, adap->vsync_mode);
	}

	if (pirqinfo) {
		highres_timer_cancel(&adap->vsync_timer);
		if ((pirqinfo->irq >= VIC_IRQ(0)) &&
			(pirqinfo->irq < VIC_IRQ(NR_IRQS))) {
			synchronize_irq(pirqinfo->irq);
		}
		pirqinfo->callback = NULL;
		pirqinfo->callback_data = NULL;
		adap->vsync_mode = AMBA_VIN_VSYNC_IRQ_NUM;
		adap->vsync_delay = 0;
	}
}
EXPORT_SYMBOL(amba_vin_vsync_unbind);

