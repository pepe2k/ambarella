/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_arm11.c
 *
 * History:
 *    2013/01/05 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static eis_ipc_info_t	*pinfo = NULL;

#include "eis_algo.c"

void gyro_callback(gyro_data_t *data, void *arg)
{
	eis_handler(data);
       printk(KERN_DEBUG "eis_cortex: Gyro_callback\n");
}

static irqreturn_t amba_eis_vin_isr(int irq, void *dev_data)
{

	do_gettimeofday(&vcap_time);
       printk(KERN_DEBUG "eis_cortex: amba_eis_vin_isr, time %ld\n", vcap_time.tv_sec * 1000000L + vcap_time.tv_usec);
	return IRQ_HANDLED;
}

static int __init amba_eis_init(void)
{
	int			ret = 0;
	eis_sensor_ipc_t	*p_sensor;
	iav_eis_info_ex_t	iav_eis_info;

	pinfo = (eis_ipc_info_t *)kzalloc(sizeof(eis_ipc_info_t), GFP_KERNEL);
	if (!pinfo) {
		printk("%s: Fail to allocate eis info!\n", __func__);
		ret = -ENOMEM;
		goto amba_eis_init_exit;
	} else {
		pinfo->share_data = (eis_share_data_t *)EIS_IPC_DATA_VIRT_ADDR;
	}

	pinfo->vin_irq = EIS_VIN_IRQ;
	ret = request_irq(pinfo->vin_irq, amba_eis_vin_isr,
		IRQF_TRIGGER_RISING | IRQF_SHARED, "EIS", pinfo);
	if (ret) {
		printk("%s: Fail to request eis vin irq!\n", __func__);
		pinfo->vin_irq = 0;
		goto amba_eis_init_exit;
	}

	pinfo->dev_id = MKDEV(EIS_MAJOR, EIS_MINOR);
	ret = register_chrdev_region(pinfo->dev_id, 1, "eis");
	if (ret) {
		goto amba_eis_init_exit;
	}

	cdev_init(&pinfo->char_dev, NULL);
	pinfo->char_dev.owner = THIS_MODULE;
	ret = cdev_add(&pinfo->char_dev, pinfo->dev_id, 1);
	if (ret) {
		unregister_chrdev_region(pinfo->dev_id, 1);
		goto amba_eis_init_exit;
	}

	memset(pinfo->share_data, 0, sizeof(*pinfo->share_data));
	p_sensor = &pinfo->share_data->sensor_ipc;
	p_sensor->cortex_pingpong	= EIS_BUF_NA;
	p_sensor->arm11_pingpong	= EIS_BUF_NA;

	eis_algo_init();

	vcap_time.tv_usec	= 0;
	vcap_time.tv_sec	= 0;
	gyro_register_eis_callback(gyro_callback, pinfo);


	iav_eis_info.eis_update_addr	= (u32)(&pinfo->share_data->enhance_turbo_buf.update_info);
	iav_eis_info.cmd_read_delay	= 0;
	set_eis_info_ex(&iav_eis_info);

amba_eis_init_exit:
	if (ret && pinfo) {
		if (pinfo->vin_irq) {
			free_irq(pinfo->vin_irq, pinfo);
		}
		kfree(pinfo);
		pinfo = NULL;
	}
	return ret;
}

static void __exit amba_eis_exit(void)
{
	if (pinfo) {
		gyro_unregister_eis_callback();

		if (pinfo->vin_irq) {
			free_irq(pinfo->vin_irq, pinfo);
		}
		cdev_del(&pinfo->char_dev);
		unregister_chrdev_region(pinfo->dev_id, 1);
		pinfo = NULL;
	}
}

module_init(amba_eis_init);
module_exit(amba_eis_exit);

MODULE_DESCRIPTION("Ambarella EIS IPC ARM11 Driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");
