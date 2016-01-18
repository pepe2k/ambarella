/*
 * kernel/private/drivers/p_iris/p_iris.c
 *
 * History:
 *	2012/06/29 - [Louis Sun]
 *
 * Copyright (C) 2004-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_p_iris.h>
#include "p_iris_priv.h"


static const char * amba_p_iris_name = "amb_iris";
static int amba_p_iris_major = AMBA_DEV_MAJOR;
static int amba_p_iris_minor = (AMBA_DEV_MINOR_PUBLIC_START + 17);
static struct cdev amba_p_iris_cdev;

//static u8 * amba_p_iris_buffer = NULL;

amba_p_iris_controller_t amba_p_iris_controller = {
	.cfg = {
				.gpio_id = { -1, -1 },
				.gpio_val = 0,
				.timer_period = 10,
				.min_mechanical = 0,
				.max_mechanical = 100,
				.min_optical = 0,
				.max_optical = 100,
			},
	.pos = P_IRIS_DEFAULT_POS,
	.state = P_IRIS_WORK_STATE_NOT_INIT,
	.steps_to_move = 0,
};



DEFINE_MUTEX(amba_p_iris_mutex);

void amba_p_iris_lock(void)
{
	mutex_lock(&amba_p_iris_mutex);
}

void amba_p_iris_unlock(void)
{
	mutex_unlock(&amba_p_iris_mutex);
}

static long amba_p_iris_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	amba_p_iris_lock();

	switch (cmd) {
		case AMBA_IOC_P_IRIS_INIT_CFG:
			rval = amba_p_iris_init_config(filp->private_data, (struct amba_p_iris_cfg_s *)arg);
			break;
		case AMBA_IOC_P_IRIS_GET_CFG:
			rval = amba_p_iris_get_config(filp->private_data, (struct amba_p_iris_cfg_s *)arg);
			break;
		case AMBA_IOC_P_IRIS_RESET:
			rval = amba_p_iris_reset(filp->private_data, (int)arg);
			break;
		case AMBA_IOC_P_IRIS_MOVE_STEPS:
			rval = amba_p_iris_move_steps(filp->private_data, (int)arg);
			break;
		case AMBA_IOC_P_IRIS_GET_POS:
			rval = amba_p_iris_get_position(filp->private_data, (int *)arg);
			break;
		default:
			rval = -ENOIOCTLCMD;
			break;
	}

	amba_p_iris_unlock();
	return rval;
}

static int amba_p_iris_open(struct inode *inode, struct file *filp)
{
	amba_p_iris_context_t *context = kzalloc(sizeof(amba_p_iris_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_p_iris_mutex;
	//context->buffer = amba_p_iris_buffer;
	context->controller = &amba_p_iris_controller;

	filp->private_data = context;

	return 0;
}

static int amba_p_iris_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static struct file_operations amba_p_iris_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_p_iris_ioctl,
	.open = amba_p_iris_open,
	.release = amba_p_iris_release
};

static int __init amba_p_iris_create_dev(int *major, int minor, int numdev,
	const char *name, struct cdev *cdev, struct file_operations *fops)
{
	dev_t dev_id;
	int rval;

	if (*major) {
		dev_id = MKDEV(*major, minor);
		rval = register_chrdev_region(dev_id, numdev, name);
	} else {
		rval = alloc_chrdev_region(&dev_id, 0, numdev, name);
		*major = MAJOR(dev_id);
	}

	if (rval) {
		printk(KERN_DEBUG "failed to get dev  region for %s.\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		printk(KERN_DEBUG "cdev_add failed for %s, error = %d.\n", name, rval);
		unregister_chrdev_region(dev_id, 1);
		return rval;
	}

	printk(KERN_DEBUG "%s dev init done, dev_id = %d:%d.\n",
		name, MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static int __init amba_p_iris_init(void)
{
/*	amba_p_iris_buffer = (void *)__get_free_page(GFP_KERNEL);
	if (amba_p_iris_buffer == NULL) {
		return -ENOMEM;
	}
*/
	return amba_p_iris_create_dev(&amba_p_iris_major,
		amba_p_iris_minor, 1, amba_p_iris_name,
		&amba_p_iris_cdev, &amba_p_iris_fops);
}


static void __exit amba_p_iris_exit(void)
{

	dev_t dev_id;

	amba_p_iris_deinit(&amba_p_iris_controller);

	cdev_del(&amba_p_iris_cdev);

	dev_id = MKDEV(amba_p_iris_major, amba_p_iris_minor);
	unregister_chrdev_region(dev_id, 1);


}

module_init(amba_p_iris_init);
module_exit(amba_p_iris_exit);

MODULE_DESCRIPTION("Ambarella P-Iris driver for simple GPIO based drive");
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Louis Sun, <lysun@ambarella.com>");
MODULE_ALIAS("p-iris-driver");
