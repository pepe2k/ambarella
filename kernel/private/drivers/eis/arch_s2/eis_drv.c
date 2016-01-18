/*
 * eis_drv.c
 *
 * History:
 *	2012/10/18 - [Park Luo] created file
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

#include "ambas_vin.h"
#include "iav_common.h"
#include "iav_encode_drv.h"
#include "utils.h"
#include "eis_drv.h"
#include "eis_timer.h"
//#include "../../gyro/gyro.h"


#define	USE_HRTIMER_FOR_EIS		(0)

MODULE_AUTHOR("Park Luo<clluo@ambarella.com>");
MODULE_DESCRIPTION("Ambarella eis driver");
MODULE_LICENSE("Proprietary");

static const char *eis_name = "eis";
static int eis_major = 248;
static int eis_minor = 10;
static struct cdev eis_cdev;

DEFINE_MUTEX(eis_mutex);

#if USE_HRTIMER_FOR_EIS
static struct semaphore G_eis_routine_wait;
static struct amba_vin_eis_info	vin_eis_info;
static s32	Dly_ms_test = 0;
#endif

static struct eis_hrtimer_s G_eis_hrtimer;
static int G_eis_timer_started = 0;

struct timeval vcap_time;
static spinlock_t G_eis_lock;
static iav_eis_info_ex_t	iav_eis_info;
static u32 *eis_enhance_turbo_buf;
static u32 pp_buf_size = 1<<12;
static eis_coeff_info_t* _eis_coeff[2];
static eis_update_info_t* _eis_update;


extern int mn34041pl_get_eis_info_ex(struct amba_vin_eis_info *eis_info);

static inline unsigned long __eis_spin_lock(void)
{
	unsigned long flags = 0;
	//spin_lock_irqsave(&G_eis_lock, flags);
	spin_lock(&G_eis_lock);
	return flags;
}
#define eis_irq_save(_flags)	\
	do {	_flags = __eis_spin_lock();	} while (0)

static inline void eis_irq_restore(unsigned long flags)
{
	//spin_unlock_irqrestore(&G_eis_lock, flags);
	spin_unlock(&G_eis_lock);
}

static inline void eis_lock(void)
{
	mutex_lock(&eis_mutex);
}

static inline void eis_unlock(void)
{
	mutex_unlock(&eis_mutex);
}

static int stop_eis_hrtimer(void)
{
	if (G_eis_timer_started) {
		eis_hrtimer_deinit(&G_eis_hrtimer);
	}
	return 0;
}

#if USE_HRTIMER_FOR_EIS
static int start_eis_hrtimer(void)
{
	// Init HR timer for EIS routine task
	if (eis_hrtimer_init(DEFAULT_HRTIMER_PERIOD_US, &G_eis_hrtimer) < 0) {
		eis_error("Failed to init HR timer for EIS!\n");
		return -EIO;
	} else {
		eis_hrtimer_start(&G_eis_hrtimer);
	}
	G_eis_timer_started = 1;
	return 0;
}

static int eis_calc_cmd_dly(void)
{
	s32 Dly_ms;

	if (Dly_ms_test == 0) {
		// minus 2 for safety margin to configure iDSP section 2 cmd.
		Dly_ms = (u32)(vin_eis_info.vb_lines + vin_eis_info.cap_start_y) *
			vin_eis_info.row_time / 1000000 - 2;
		if (Dly_ms < 0)
			Dly_ms = 0;
	} else {
		Dly_ms = Dly_ms_test;
	}

	return Dly_ms;
}

// Please add eis computation task in this function
static int eis_auto_test(void)
{
	if (mn34041pl_get_eis_info_ex(&vin_eis_info) == 0) {
		eis_printk("VIN ready\n");
	}

	return 0;
}

static int eis_task(void)
{
	eis_auto_test();
	return 0;
}

static int eis_routine_task(void * arg)
{
	daemonize("eis_routine_guard");

	while (1) {
		if (down_interruptible(&G_eis_routine_wait) != 0) {
			eis_error("Failed to get");
			continue;
		}
		eis_task();
	}

	return 0;
}

static int eis_timer_task(void * arg)
{
	daemonize("eis_timer_guard\n");

	while (1) {
		eis_hrtimer_wait_next(&G_eis_hrtimer);
		up(&G_eis_routine_wait);
	}

	return 0;
}
#endif

static int eis_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int eis_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int eis_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int errorCode = 0;
	unsigned long	pfn;

	eis_lock();
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	pfn = virt_to_phys(eis_enhance_turbo_buf) >> PAGE_SHIFT;
	if (remap_pfn_range(vma, vma->vm_start, pfn,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			errorCode = -EINVAL;
	}
	eis_unlock();

	eis_printk("vm start 0x%lx end 0x%lx\n", vma->vm_start, vma->vm_end);
	return errorCode;
}

static long eis_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rval = 0;
	eis_buf_info_t buf_info;
	port_value_t pv;

	eis_lock();
	switch (cmd) {
	case EIS_IOC_GET_BUF_INFO:
		buf_info.buf_size = pp_buf_size;
		buf_info.block0_addr = VIRT_TO_DSP(_eis_coeff[0]);
		buf_info.block1_addr = VIRT_TO_DSP(_eis_coeff[1]);
		if (copy_to_user((eis_buf_info_t* __user)arg, &buf_info, sizeof(eis_buf_info_t))) {
			rval = -EFAULT;
		}
		break;

	case EIS_IOC_DUMP_BUF:
		if (copy_to_user((u8* __user)arg, eis_enhance_turbo_buf, pp_buf_size)) {
			rval = -EFAULT;
		}
		break;

	case EIS_IOC_UPDATE_BUF:
		invalidate_d_cache((void*)eis_enhance_turbo_buf, pp_buf_size);
		break;

        case EIS_IOC_CLEAN_BUF:
		clean_d_cache((void*)eis_enhance_turbo_buf, pp_buf_size);
		break;

	case EIS_IOC_RW_PORT:
		if (copy_from_user(&pv, (port_value_t* __user)arg, sizeof(pv))){
			rval = -EFAULT;
			break;
		}

		if (pv.rw) {// 1 write
			amba_writel(get_ambarella_apb_virt()+pv.addr, pv.value);
		} else {
			pv.value = amba_readl(get_ambarella_apb_virt()+pv.addr);
			if (copy_to_user((port_value_t* __user)arg, &pv, sizeof(pv))) {
				rval = -EFAULT;
			}
		}
		break;

	case EIS_IOC_GET_VIN:
		if (copy_to_user((struct timeval* __user)arg, &vcap_time, sizeof(vcap_time))) {
			rval = -EFAULT;
		}
		break;

	default:
		eis_error("Unknown EIS ioctl [%x].\n", cmd);
		break;
	}
	eis_unlock();

	return rval;
}

static struct file_operations eis_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = eis_ioctl,
	.mmap = eis_mmap,
	.open = eis_open,
	.release = eis_release
};

static void __exit __eis_exit(void)
{
	dev_t eis_dev_id;
	eis_dev_id = MKDEV(eis_major, eis_minor);

	kfree(eis_enhance_turbo_buf);
	eis_enhance_turbo_buf = NULL;
	_eis_coeff[0] = _eis_coeff[1] = NULL;
	_eis_update = NULL;

	cdev_del(&eis_cdev);
	unregister_chrdev_region(eis_dev_id, 1);

	stop_eis_hrtimer();

	eis_printk("eis exit\n");
}

static int __init eis_create_dev(int *major, int minor, int numdev,
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
		eis_error("failed to get dev region for %s\n", name);
		return rval;
	}

	cdev_init(cdev, fops);
	cdev->owner = THIS_MODULE;
	rval = cdev_add(cdev, dev_id, 1);
	if (rval) {
		eis_error("cdev_add failed for %s, error = %d\n", name, rval);
		return rval;
	}

	eis_printk("%s dev init done, dev_id = %d:%d\n", name,
		MAJOR(dev_id), MINOR(dev_id));
	return 0;
}

static irqreturn_t test_irq(int irqno, void *dev_id)
{
	do_gettimeofday(&vcap_time);
	return IRQ_HANDLED;
}

static int __init __eis_init(void)
{
	int rval = 0;
	int dev = 1;

	eis_printk("Kernel eis_init\n");

	eis_enhance_turbo_buf = kzalloc(pp_buf_size, GFP_KERNEL);
	if (eis_enhance_turbo_buf == NULL)
		goto Fail;

	_eis_coeff[0] = (eis_coeff_info_t*)eis_enhance_turbo_buf;
	_eis_coeff[1] = (eis_coeff_info_t*)(eis_enhance_turbo_buf +
		sizeof(eis_coeff_info_t)/sizeof(u32));
	_eis_update = (eis_update_info_t*)(eis_enhance_turbo_buf +
		(sizeof(eis_coeff_info_t)/sizeof(u32) << 1));
	iav_eis_info.eis_update_addr = (u32)_eis_update;
	iav_eis_info.cmd_read_delay = 0;//eis_calc_cmd_dly();
	set_eis_info_ex(&iav_eis_info);

	eis_printk("Size [%d], kernel addr: coeff [0x%p] [0x%p] update [0x%p]\n",
		pp_buf_size, _eis_coeff[0], _eis_coeff[1], _eis_update);
	eis_printk("DSP addr 0x%x = 0x%x = 0x%x\n", VIRT_TO_DSP(_eis_coeff[0]),
		VIRT_TO_DSP(_eis_coeff[1]), VIRT_TO_DSP(_eis_update));
	if ((rval = eis_create_dev(&eis_major, eis_minor, 1,
		eis_name, &eis_cdev, &eis_fops)) < 0) {
		eis_error("Failed to create EIS dev!\n");
		goto Fail;
	}

	rval = request_irq(IDSP_SENSOR_VSYNC_IRQ, test_irq,
		//IRQF_TRIGGER_FALLING | IRQF_SHARED, "test_irq", &dev);
		IRQF_TRIGGER_RISING | IRQF_SHARED, "test_irq", &dev);
	if (rval) {
		eis_printk("Can't request IDSP_SENSOR_VSYNC_IRQ, rval = %d\n", rval);
		goto Fail;
	} else {
		eis_printk("Request IDSP_SENSOR_VSYNC_IRQ SUCCESS, rval = %d\n", rval);
	}

#if USE_HRTIMER_FOR_EIS
	if ((rval = start_eis_hrtimer()) < 0) {
		eis_error("Failed to create EIS timer!\n");
		goto Fail;
	}

	/* Create kernel thread to run eis routine task */
	sema_init(&G_eis_routine_wait, 0);
	kernel_thread(eis_timer_task, NULL, 0);
	kernel_thread(eis_routine_task, NULL, 0);

	mpu6000_get_info(&gyro_info);
#endif

	return 0;

Fail:
	__eis_exit();
	return rval;
}

module_init(__eis_init);
module_exit(__eis_exit);


