/*
 * kernel/private/drivers/ambarella/vin/vin_core.c
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

#include "vin_pri.h"

#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <plat/highres_timer.h>

static LIST_HEAD(vin_adapters);
static LIST_HEAD(vin_sources);
static DEFINE_MUTEX(vin_adapter_mutex);
static DEFINE_MUTEX(vin_source_mutex);
static DEFINE_IDR(amba_vin_adapter_idr);
static DEFINE_IDR(amba_vin_source_idr);

static const char amba_vin_name[] = "vin";

/* ========================================================================== */
int amba_vin_add_adapter(struct __amba_vin_adapter *adap)
{
	int					retval = 0;
	int					id;

	vin_dbg("amba_vin_add_adapter\n");

	if (!adap) {
		retval = -EINVAL;
		goto amba_vin_add_adapter_exit;
	}
	mutex_init(&adap->cmd_lock);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	if (idr_pre_get(&amba_vin_adapter_idr, GFP_KERNEL) == 0) {
		retval = -ENOMEM;
		goto amba_vin_add_adapter_exit;
	}

	mutex_lock(&vin_adapter_mutex);
	retval = idr_get_new_above(&amba_vin_adapter_idr, adap, adap->id, &id);
#else
	mutex_lock(&vin_adapter_mutex);
	id = idr_alloc_cyclic(&amba_vin_adapter_idr, adap,
		adap->id, 0, GFP_KERNEL);
	if (id < 0) {
		retval = -ENOMEM;
	} else {
		retval = 0;
	}
#endif
	if (id != adap->id) {
		retval = -EBUSY;
		idr_remove(&amba_vin_adapter_idr, id);
	}
	if (retval == 0) {
		list_add_tail(&adap->list, &vin_adapters);
	} else {
		vin_err("amba_vin_add_adapter %s-%d fail with %d!\n",
			adap->name, adap->id, retval);
	}
	adap->total_source_num = 0;
	adap->active_source_id = 0;
	adap->pvsync_irq = NULL;
	adap->pvin_irq = NULL;
	adap->pidsp_last_pixel_irq = NULL;
	adap->pidsp_irq = NULL;
	adap->pgpio_irq = NULL;
	highres_timer_init(&adap->vsync_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	spin_lock_init(&adap->lock);

	adap->vsync_mode = AMBA_VIN_VSYNC_IRQ_NUM;
	adap->vsync_delay = 0;
	snprintf(adap->name, sizeof(adap->name),
		"%s%d", amba_vin_name, adap->id);
	mutex_unlock(&vin_adapter_mutex);

amba_vin_add_adapter_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_add_adapter);

int amba_vin_del_adapter(struct __amba_vin_adapter *adap)
{
	int					retval = 0;
	struct list_head			*item;
	struct __amba_vin_source		*src;

	vin_dbg("amba_vin_del_adapter\n");

	if (!adap) {
		retval = -EINVAL;
		goto amba_vin_del_adapter_exit;
	}

	mutex_lock(&vin_source_mutex);
	list_for_each(item, &vin_sources) {
		src = list_entry(item, struct __amba_vin_source, list);
		if (src->adapid == adap->id) {
			retval = -EBUSY;
			break;
		}
	}
	mutex_unlock(&vin_source_mutex);
	if (retval)
		goto amba_vin_del_adapter_exit;

	mutex_lock(&vin_adapter_mutex);
	list_del(&adap->list);
	idr_remove(&amba_vin_adapter_idr, adap->id);
	mutex_unlock(&vin_adapter_mutex);

amba_vin_del_adapter_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_del_adapter);

struct __amba_vin_adapter *__amba_vin_get_adapter(int id)
{
	struct __amba_vin_adapter		*adap = NULL;

	vin_dbg("__amba_vin_get_adapter %d\n", id);

	mutex_lock(&vin_adapter_mutex);
	adap = (struct __amba_vin_adapter *)idr_find(&amba_vin_adapter_idr, id);
	mutex_unlock(&vin_adapter_mutex);

	return adap;
}

int amba_vin_add_source(struct __amba_vin_source *src)
{
	int					retval = 0;
	int					id;
	struct __amba_vin_adapter		*adap;

	vin_dbg("amba_vin_add_source\n");

	if (!src) {
		retval = -EINVAL;
		goto amba_vin_add_source_exit;
	}

	adap = __amba_vin_get_adapter(src->adapid);
	if (!adap) {
		vin_warn("Can't find adap[%d] for %s, try default adap!\n",
			src->adapid, src->name);
		src->adapid = AMBA_VIN_ADAPTER_STARTING_ID;
		adap = __amba_vin_get_adapter(src->adapid);
		if (!adap) {
			vin_warn("Can't find default adap for %s!\n",
				src->name);
			retval = -ENODEV;
			goto amba_vin_add_source_exit;
		}
	}
	mutex_init(&src->cmd_lock);

	retval = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_REGISTER_SOURCE, src);
	if (retval != 0) {
		retval = -EIO;
		goto amba_vin_add_source_exit;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
	if (idr_pre_get(&amba_vin_source_idr, GFP_KERNEL) == 0) {
		retval = -ENOMEM;
		goto amba_vin_add_source_exit;
	}

	mutex_lock(&vin_source_mutex);
	retval = idr_get_new_above(&amba_vin_source_idr, src,
		AMBA_VIN_SOURCE_STARTING_ID, &id);
#else
	mutex_lock(&vin_source_mutex);
	id = idr_alloc_cyclic(&amba_vin_source_idr, src,
		AMBA_VIN_SOURCE_STARTING_ID, 0, GFP_KERNEL);
	if (id < 0) {
		retval = -ENOMEM;
	} else {
		retval = 0;
	}
#endif
	if (retval == 0) {
		src->id = id;
		list_add_tail(&src->list, &vin_sources);
	} else {
		vin_err("amba_vin_add_source %d:%s-%d fail with %d!\n",
			src->adapid, src->name, src->id, retval);
		retval = amba_vin_adapter_cmd(src->adapid,
			AMBA_VIN_ADAP_UNREGISTER_SOURCE, src);
	}
	mutex_unlock(&vin_source_mutex);

amba_vin_add_source_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_add_source);

struct __amba_vin_source *__amba_vin_get_source(int id)
{
	struct __amba_vin_source		*src = NULL;

	vin_dbg("__amba_vin_get_source %d\n", id);

	mutex_lock(&vin_source_mutex);
	src = (struct __amba_vin_source *) idr_find(&amba_vin_source_idr, id);
	mutex_unlock(&vin_source_mutex);

	return src;
}

int amba_vin_del_source(struct __amba_vin_source *src)
{
	int					retval = 0;

	vin_dbg("amba_vin_del_source\n");

	if (!src) {
		retval = -EINVAL;
		goto amba_vin_del_source_exit;
	}

	if (__amba_vin_get_source(src->id) == NULL) {
		retval = -ENXIO;
		goto amba_vin_del_source_exit;
	}

	retval = amba_vin_adapter_cmd(src->adapid,
		AMBA_VIN_ADAP_UNREGISTER_SOURCE, src);
	if (retval) {
		retval = -EBUSY;
		goto amba_vin_del_source_exit;
	}

	mutex_lock(&vin_source_mutex);
	list_del(&src->list);
	idr_remove(&amba_vin_source_idr, src->id);
	mutex_unlock(&vin_source_mutex);

amba_vin_del_source_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_del_source);

int amba_vin_adapter_cmd(int adapid, enum amba_vin_adap_cmd cmd, void *args)
{
	int					retval = 0;
	struct __amba_vin_adapter		*adap;

	vin_dbg("amba_vin_adapter_cmd %d %d 0x%x\n", adapid, cmd, (u32) args);

	adap = __amba_vin_get_adapter(adapid);
	if (!adap) {
		retval = -EINVAL;
		goto amba_vin_adapter_cmd_exit;
	}

	mutex_lock(&adap->cmd_lock);
	if (likely(adap->docmd)) {
		retval = adap->docmd(adap, cmd, args);
		if (retval) {
			vin_err("%s-%d docmd[%d] return with %d!\n",
				adap->name, adap->id, cmd, retval);
			goto amba_vin_adapter_cmd_exit_cmdlock;
		}
	} else {
		vin_err("%s-%d should support docmd!\n", adap->name, adap->id);
		retval = -EINVAL;
	}

amba_vin_adapter_cmd_exit_cmdlock:
	mutex_unlock(&adap->cmd_lock);

amba_vin_adapter_cmd_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_adapter_cmd);

int amba_vin_source_cmd(int srcid, int chid,
	enum amba_vin_src_cmd cmd, void *args)
{
	int					retval = 0;
	struct __amba_vin_source		*src;

	vin_dbg("amba_vin_source_cmd %d %d %d 0x%x\n",
		srcid, chid, cmd, (int) args);

	src = __amba_vin_get_source(srcid);
	if (!src) {
		retval = -EINVAL;
		goto amba_vin_source_kernel_cmd_exit;
	}

	mutex_lock(&src->cmd_lock);
	if (likely(src->docmd)) {
		if ((chid >= 0) && (chid != src->active_channel_id)) {
			retval = src->docmd(src,
				AMBA_VIN_SRC_SELECT_CHANNEL, &chid);
			if (retval) {
				vin_err("%s-%d docmd[%d] return with %d!\n",
					src->name, src->id, cmd, retval);
				goto amba_vin_source_kernel_cmd_exit_cmdlock;
			}
			src->active_channel_id = chid;
		}

		retval = src->docmd(src, cmd, args);
		if (retval) {
			vin_err("%s-%d docmd[%d] return with %d!\n",
				src->name, src->id, cmd, retval);
			goto amba_vin_source_kernel_cmd_exit_cmdlock;
		}
	} else {
		vin_err("%s-%d should support docmd!\n", src->name, src->id);
		retval = -EINVAL;
	}

amba_vin_source_kernel_cmd_exit_cmdlock:
	mutex_unlock(&src->cmd_lock);

amba_vin_source_kernel_cmd_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_source_cmd);

int amba_vin_pm(u32 pmval)
{
	int					retval = 0;
	struct list_head			*item;
	struct __amba_vin_source		*psrc;
	struct __amba_vin_adapter		*padap;
	u32					src_cmd;
	u32					adap_cmd;

	switch (pmval) {
	case AMBA_EVENT_PRE_CPUFREQ:
	case AMBA_EVENT_PRE_PM:
	case AMBA_EVENT_PRE_TOSS:
		src_cmd = AMBA_VIN_SRC_SUSPEND;
		adap_cmd = AMBA_VIN_ADAP_SUSPEND;
		break;

	case AMBA_EVENT_POST_CPUFREQ:
	case AMBA_EVENT_POST_PM:
	case AMBA_EVENT_POST_TOSS:
		src_cmd = AMBA_VIN_SRC_RESUME;
		adap_cmd = AMBA_VIN_ADAP_RESUME;
		break;

	case AMBA_EVENT_CHECK_CPUFREQ:
	case AMBA_EVENT_CHECK_PM:
	case AMBA_EVENT_CHECK_TOSS:

	case AMBA_EVENT_TAKEOVER_DSP:
	case AMBA_EVENT_PRE_TAKEOVER_DSP:
	case AMBA_EVENT_POST_TAKEOVER_DSP:
	case AMBA_EVENT_GIVEUP_DSP:
	case AMBA_EVENT_PRE_GIVEUP_DSP:
	case AMBA_EVENT_POST_GIVEUP_DSP:
	case AMBA_EVENT_POST_VIN_LOSS:
		goto amba_vin_pm_exit;
		break;

	default:
		vin_err("%s: unknown event 0x%x\n", __func__, pmval);
		retval = -EINVAL;
		goto amba_vin_pm_exit;
		break;
	}

	mutex_lock(&vin_source_mutex);
	list_for_each(item, &vin_sources) {
		psrc = list_entry(item, struct __amba_vin_source, list);
		mutex_lock(&psrc->cmd_lock);
		retval = psrc->docmd(psrc, src_cmd, NULL);
		if (retval) {
			vin_err("%s-%d %d return %d!\n",
				psrc->name, psrc->id, src_cmd, retval);
		}
		mutex_unlock(&psrc->cmd_lock);
	}
	mutex_unlock(&vin_source_mutex);
	mutex_lock(&vin_adapter_mutex);
	list_for_each(item, &vin_adapters) {
		padap = list_entry(item, struct __amba_vin_adapter, list);
		mutex_lock(&padap->cmd_lock);
		retval = padap->docmd(padap, adap_cmd, NULL);
		if (retval) {
			vin_err("%s-%d %d return %d!\n",
				padap->name, padap->id, adap_cmd, retval);
		}
		mutex_unlock(&padap->cmd_lock);
	}
	mutex_unlock(&vin_adapter_mutex);

amba_vin_pm_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vin_pm);

