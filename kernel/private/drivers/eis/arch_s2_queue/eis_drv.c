/*
 * kernel/private/drivers/eis/arch_s2_ipc/eis_drv.c
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
#include <amba_common.h>
#include <iav_common.h>
#include <iav_encode_drv.h>
#include <amba_eis.h>
#include "eis_priv.h"
#include "eis_queue.c"


amba_eis_controller_t    g_eis_controller = {
    .vin_irq    = 0,
    .ring_buffer_full = 0,
    .wait_count = 0,
};



static const char * amba_eis_name = "amb_eis";
static int amba_eis_major = EIS_MAJOR;
static int amba_eis_minor = EIS_MINOR;
static struct cdev amba_eis_cdev;


DEFINE_MUTEX(amba_eis_mutex);
void amba_eis_lock(void)
{
	mutex_lock(&amba_eis_mutex);
}

void amba_eis_unlock(void)
{
	mutex_unlock(&amba_eis_mutex);
}


static long amba_eis_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval;
	amba_eis_lock();

	switch (cmd) {
                case AMBA_IOC_EIS_GET_INFO:
                    rval = amba_eis_get_info(filp->private_data, (struct amba_eis_info_s *)arg);
                    break;
                case AMBA_IOC_EIS_START_STAT:
                    rval = amba_eis_start_stat(filp->private_data);
                    break;
                case AMBA_IOC_EIS_STOP_STAT:
                    rval =  amba_eis_stop_stat(filp->private_data);
                    break;
                case AMBA_IOC_EIS_GET_STAT:
                    rval = amba_eis_get_stat(filp->private_data,  (struct amba_eis_stat_s *) arg);
                    break;
		default:
			rval = -ENOIOCTLCMD;
			break;
	}

	amba_eis_unlock();
	return rval;
}

static int amba_eis_open(struct inode *inode, struct file *filp)
{
	amba_eis_context_t *context = kzalloc(sizeof(amba_eis_context_t), GFP_KERNEL);
	if (context == NULL) {
		return -ENOMEM;
	}
	context->file = filp;
	context->mutex = &amba_eis_mutex;           //share mutex for protection
       context->controller = &g_eis_controller;     //share global eis controller
	filp->private_data = context;

	return 0;
}

static int amba_eis_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}



static struct file_operations amba_eis_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_eis_ioctl,
	.open = amba_eis_open,
	.release = amba_eis_release
};

static int __init amba_eis_create_dev(int *major, int minor, int numdev,
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


static int __init amba_eis_init(void)
{

        //create character device
    	amba_eis_create_dev(&amba_eis_major,
		amba_eis_minor, 1, amba_eis_name,
		&amba_eis_cdev, &amba_eis_fops);

        if (eis_init(&g_eis_controller) < 0) {
            printk(KERN_DEBUG "amba_eis_init failed \n");
            return -1;
        }

        return 0;
}

static void __exit amba_eis_exit(void)
{
       dev_t dev_id;

       eis_exit(&g_eis_controller);


	cdev_del(&amba_eis_cdev);
	dev_id = MKDEV(amba_eis_major, amba_eis_minor);
	unregister_chrdev_region(dev_id, 1);
}




module_init(amba_eis_init);
module_exit(amba_eis_exit);

MODULE_DESCRIPTION("Ambarella EIS IPC Driver with Gyro Queue");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

