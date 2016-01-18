/*
 * dsplog_drv.c
 *
 * History:
 * 	2013/09/30 - [Louis Sun] create new dsplog driver
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <amba_common.h>
#include "utils.h"
#include "dsplog_priv.h"
#include "dsplog_drv.h"

/* Now separate dsplog driver from dsp driver.
    DSP driver is kernel mode only driver used by IAV driver.
    DSP log driver supports DSP logging and it is not called by dsp driver.
    DSP log driver is optional and only used for debugging
   */


MODULE_AUTHOR("Louis Sun <lysun@ambarella.com>");
MODULE_DESCRIPTION("Ambarella dsplog driver");
MODULE_LICENSE("Proprietary");


amba_dsplog_controller_t    g_dsplog_controller;


static int amba_dsplog_major = AMBA_DEV_MAJOR;
static int amba_dsplog_minor = (AMBA_DEV_MINOR_PUBLIC_END + 9);
static const char * amba_dsplog_name = "dsplog";
static struct cdev amba_dsplog_cdev;




static ssize_t amba_dsplog_read(struct file *filp, char __user *buffer, size_t count, loff_t *offp)
{
	return dsplog_read(buffer, count);
	return -1;
}



DEFINE_MUTEX(amba_dsplog_mutex);

void dsplog_lock(void)
{
	mutex_lock(&amba_dsplog_mutex);
}

void dsplog_unlock(void)
{
	mutex_unlock(&amba_dsplog_mutex);
}


static long amba_dsplog_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	dsplog_lock();
      switch (cmd) {
              case AMBA_IOC_DSPLOG_START_CAPUTRE:
                rval = dsplog_start_cap();
                break;

             case AMBA_IOC_DSPLOG_STOP_CAPTURE:
                rval = dsplog_stop_cap();
                break;

            case AMBA_IOC_DSPLOG_SET_LOG_LEVEL:
                rval = dsplog_set_level((int)arg);
                break;

            case AMBA_IOC_DSPLOG_GET_LOG_LEVEL:
                rval = dsplog_get_level((int *) arg);
                break;

            case AMBA_IOC_DSPLOG_PARSE_LOG:
                rval = dsplog_parse((int)arg);
                break;

		default:
			rval = -ENOIOCTLCMD;
			break;
	}

	dsplog_unlock();
	return rval;
}

static int amba_dsplog_open(struct inode *inode, struct file *filp)
{
	amba_dsplog_context_t *context = kzalloc(sizeof(amba_dsplog_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_dsplog_mutex;
	context->controller = &g_dsplog_controller;
	filp->private_data = context;

	return 0;
}

static int amba_dsplog_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static struct file_operations amba_dsplog_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_dsplog_ioctl,
	.open = amba_dsplog_open,
	.release = amba_dsplog_release,
	.read = amba_dsplog_read,
};

static int __init amba_dsplog_create_dev(int *major, int minor, int numdev,
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
		dsp_printk("failed to get dev region for %s.\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		dsp_printk("cdev_add failed for %s, error = %d.\n", name, rval);
		unregister_chrdev_region(dev_id, 1);
		return rval;
	}

	dsp_printk("%s dev init done, dev_id = %d:%d.\n",
		name, MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static int __init amba_dsplog_init(void)
{
	if ( amba_dsplog_create_dev(&amba_dsplog_major,
		amba_dsplog_minor, 1, amba_dsplog_name,
		&amba_dsplog_cdev, &amba_dsplog_fops) < 0) {
            return -1;
        }

       return dsplog_init();
}


static void __exit amba_dsplog_exit(void)
{

	dev_t dev_id;
	dsplog_deinit(&g_dsplog_controller);
	cdev_del(&amba_dsplog_cdev);
	dev_id = MKDEV(amba_dsplog_major, amba_dsplog_minor);
	unregister_chrdev_region(dev_id, 1);
}

module_init(amba_dsplog_init);
module_exit(amba_dsplog_exit);

