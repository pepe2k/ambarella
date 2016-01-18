/*
 * kernel/private/drivers/ambarella/vin/sbrg/sbrg_core.c
 *
 * Author: Haowei Lo <hwlo@ambarella.com>
 *
 * Copyright (C) 2004-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <sbrg/amba_sbrg.h>
#include "sbrg_pri.h"

static LIST_HEAD(sbrg_adapters);
static DEFINE_MUTEX(sbrg_adapter_mutex);
static DEFINE_IDR(amba_sbrg_adapter_idr);

static const char amba_sbrg_name[] = "sbrg";

/* ========================================================================== */
int amba_sbrg_add_adapter(struct __amba_sbrg_adapter *adap)
{
	int					retval = 0;
	int					id;

	sbrg_dbg("amba_sbrg_add_adapter\n");

	if (!adap) {
		retval = -EINVAL;
		goto amba_sbrg_add_adapter_exit;
	}

	if (adap->id & ~MAX_ID_MASK) {
		retval = -EINVAL;
		goto amba_sbrg_add_adapter_exit;
	}
	mutex_init(&adap->cmd_lock);
	if (idr_pre_get(&amba_sbrg_adapter_idr, GFP_KERNEL) == 0) {
		retval = -ENOMEM;
		goto amba_sbrg_add_adapter_exit;
	}

	mutex_lock(&sbrg_adapter_mutex);
	retval = idr_get_new_above(&amba_sbrg_adapter_idr, adap, adap->id, &id);
	if (retval == 0 && id != adap->id) {
		retval = -EBUSY;
		idr_remove(&amba_sbrg_adapter_idr, id);
	}
	if (retval == 0) {
		list_add_tail(&adap->list, &sbrg_adapters);
	} else {
		sbrg_err("amba_sbrg_add_adapter %s-%d fail with %d!\n",
			adap->name, adap->id, retval);
	}

	snprintf(adap->name, sizeof(adap->name),
		"%s%d", amba_sbrg_name, adap->id);
	mutex_unlock(&sbrg_adapter_mutex);

amba_sbrg_add_adapter_exit:
	return retval;
}
EXPORT_SYMBOL(amba_sbrg_add_adapter);

int amba_sbrg_del_adapter(struct __amba_sbrg_adapter *adap)
{
	int					retval = 0;

	sbrg_dbg("amba_sbrg_del_adapter\n");

	if (!adap) {
		retval = -EINVAL;
		goto amba_sbrg_del_adapter_exit;
	}

	mutex_lock(&sbrg_adapter_mutex);
	list_del(&adap->list);
	idr_remove(&amba_sbrg_adapter_idr, adap->id);
	mutex_unlock(&sbrg_adapter_mutex);

amba_sbrg_del_adapter_exit:
	return retval;
}
EXPORT_SYMBOL(amba_sbrg_del_adapter);

struct __amba_sbrg_adapter *amba_sbrg_get_adapter(int id)
{
	struct __amba_sbrg_adapter		*adap = NULL;

	mutex_lock(&sbrg_adapter_mutex);
	adap = (struct __amba_sbrg_adapter *)idr_find(&amba_sbrg_adapter_idr, id);
	mutex_unlock(&sbrg_adapter_mutex);

	return adap;
}
EXPORT_SYMBOL(amba_sbrg_get_adapter);

int amba_sbrg_adapter_cmd(int adapid, int cmd, void *args)
{
	int					retval = 0;
	struct __amba_sbrg_adapter		*adap;

	sbrg_dbg("amba_sbrg_adapter_cmd %d %d 0x%x\n", adapid, cmd, (u32) args);

	adap = amba_sbrg_get_adapter(adapid);
	if (!adap) {
		retval = -EINVAL;
		goto amba_sbrg_adapter_cmd_exit;
	}
	mutex_lock(&adap->cmd_lock);
	if (likely(adap->docmd)) {

		retval = adap->docmd(adap, cmd, args);
		if (retval) {
			sbrg_err("%s-%d docmd[%d] return with %d!\n",
				adap->name, adap->id, cmd, retval);
			goto amba_sbrg_adapter_cmd_exit_cmdlock;
		}
	} else {
		sbrg_err("%s-%d should support docmd!\n", adap->name, adap->id);
		retval = -EINVAL;
	}

amba_sbrg_adapter_cmd_exit_cmdlock:
	mutex_unlock(&adap->cmd_lock);

amba_sbrg_adapter_cmd_exit:
	return retval;
}
EXPORT_SYMBOL(amba_sbrg_adapter_cmd);
