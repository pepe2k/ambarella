/*
 * kernel/private/drivers/ambarella/debug/debug_core.c
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <amba_common.h>
#include <amba_debug.h>
#include <amba_vin.h>

#define AMBA_DEBUG_NAME			"ambad"
#define AMBA_DEBUG_DEV_NUM		1
#define AMBA_DEBUG_DEV_MAJOR		AMBA_DEV_MAJOR
#define AMBA_DEBUG_DEV_MINOR		(AMBA_DEV_MINOR_PUBLIC_END + 8)
#define DDR_START			DEFAULT_MEM_START
#define DDR_SIZE			(0x40000000)

static struct cdev amba_debug_cdev;
static dev_t amba_debug_dev_id =
	MKDEV(AMBA_DEBUG_DEV_MAJOR, AMBA_DEBUG_DEV_MINOR);
DEFINE_MUTEX(amba_debug_mutex);

static int amba_debug_vin_srcid = 0;
static struct amba_debug_mem_info mem_info;

typedef int (*pamba_vin_source_cmd)(int srcid, int chid,
	enum amba_vin_src_cmd cmd, void *args);

static long amba_debug_ioctl(struct file *filp,
	unsigned int cmd, unsigned long args)
{
	int				errorCode = 0;
	pamba_vin_source_cmd		pvincmd = NULL;

	mutex_lock(&amba_debug_mutex);

	switch (cmd) {
	case AMBA_DEBUG_IOC_GET_DEBUG_FLAG:
		errorCode = put_user(ambarella_debug_level, (u32 __user *)args);
		break;

	case AMBA_DEBUG_IOC_SET_DEBUG_FLAG:
		errorCode = get_user(ambarella_debug_level, (u32 __user *)args);
		break;

	case AMBA_DEBUG_IOC_GET_MEM_INFO:
		errorCode = copy_to_user(
			(struct amba_debug_mem_info __user *)args,
			&mem_info, sizeof(mem_info)) ? -EFAULT : 0;
		break;

	case AMBA_DEBUG_IOC_VIN_SET_SRC_ID:
		amba_debug_vin_srcid = (int)args;
		break;

	case AMBA_DEBUG_IOC_VIN_GET_DEV_ID:
	{
		u32				dev_id;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
		pvincmd = (pamba_vin_source_cmd)
			ambarella_debug_lookup_name("amba_vin_source_cmd");
#else
		pvincmd = (pamba_vin_source_cmd)amba_vin_source_cmd;
#endif
		if (pvincmd) {
			errorCode = pvincmd(amba_debug_vin_srcid, -1,
				AMBA_VIN_SRC_TEST_GET_DEV_ID, &dev_id);
		} else {
			printk("Can't find amba_vin_source_cmd\n");
			errorCode = -EFAULT;
		}
		if (errorCode)
			break;

		errorCode = put_user(dev_id, (u32 __user *)args);
	}
		break;

	case AMBA_DEBUG_IOC_VIN_GET_REG_DATA:
	{
		struct amba_vin_test_reg_data	reg_data;

		errorCode = copy_from_user(&reg_data,
			(struct amba_vin_test_reg_data __user *)args,
			sizeof(reg_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
		pvincmd = (pamba_vin_source_cmd)
			ambarella_debug_lookup_name("amba_vin_source_cmd");
#else
		pvincmd = (pamba_vin_source_cmd)amba_vin_source_cmd;
#endif
		if (pvincmd) {
			errorCode = pvincmd(amba_debug_vin_srcid, -1,
				AMBA_VIN_SRC_TEST_GET_REG_DATA, &reg_data);
		} else {
			printk("Can't find amba_vin_source_cmd\n");
			errorCode = -EFAULT;
		}
		if (errorCode)
			break;

		errorCode = copy_to_user(
			(struct amba_vin_test_reg_data __user *)args,
			&reg_data, sizeof(reg_data)) ? -EFAULT : 0;
	}
		break;

	case AMBA_DEBUG_IOC_VIN_SET_REG_DATA:
	{
		struct amba_vin_test_reg_data	reg_data;

		errorCode = copy_from_user(&reg_data,
			(struct amba_vin_test_reg_data __user *)args,
			sizeof(reg_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
		pvincmd = (pamba_vin_source_cmd)
			ambarella_debug_lookup_name("amba_vin_source_cmd");
#else
		pvincmd = (pamba_vin_source_cmd)amba_vin_source_cmd;
#endif
		if (pvincmd) {
			errorCode = pvincmd(amba_debug_vin_srcid, -1,
				AMBA_VIN_SRC_TEST_SET_REG_DATA, &reg_data);
		} else {
			printk("Can't find amba_vin_source_cmd\n");
			errorCode = -EFAULT;
		}
	}
		break;

	case AMBA_DEBUG_IOC_GET_GPIO:
	{
		struct amba_vin_test_gpio	gpio_data;

		errorCode = copy_from_user(&gpio_data,
			(struct amba_vin_test_gpio __user *)args,
			sizeof(gpio_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

		ambarella_gpio_config(gpio_data.id, GPIO_FUNC_SW_INPUT);
		gpio_data.data = ambarella_gpio_get(gpio_data.id);
		errorCode = copy_to_user(
			(struct amba_vin_test_gpio __user *)args,
			&gpio_data, sizeof(gpio_data)) ? -EFAULT : 0;
	}
		break;

	case AMBA_DEBUG_IOC_SET_GPIO:
	{
		struct amba_vin_test_gpio	gpio_data;

		errorCode = copy_from_user(&gpio_data,
			(struct amba_vin_test_gpio __user *)args,
			sizeof(gpio_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

		ambarella_gpio_config(gpio_data.id, GPIO_FUNC_SW_OUTPUT);
		ambarella_gpio_set(gpio_data.id, gpio_data.data);
	}
		break;

	case AMBA_DEBUG_IOC_VIN_GET_SBRG_REG_DATA:
	{
		struct amba_vin_test_reg_data	reg_data;

		errorCode = copy_from_user(&reg_data,
			(struct amba_vin_test_reg_data __user *)args,
			sizeof(reg_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
		pvincmd = (pamba_vin_source_cmd)
			ambarella_debug_lookup_name("amba_vin_source_cmd");
#else
		pvincmd = (pamba_vin_source_cmd)amba_vin_source_cmd;
#endif
		if (pvincmd) {
			errorCode = pvincmd(amba_debug_vin_srcid, -1,
				AMBA_VIN_SRC_TEST_GET_SBRG_REG_DATA, &reg_data);
		} else {
			printk("Can't find amba_vin_source_cmd\n");
			errorCode = -EFAULT;
		}
		if (errorCode)
			break;

		errorCode = copy_to_user(
			(struct amba_vin_test_reg_data __user *)args,
			&reg_data, sizeof(reg_data)) ? -EFAULT : 0;
	}
		break;

	case AMBA_DEBUG_IOC_VIN_SET_SBRG_REG_DATA:
	{
		struct amba_vin_test_reg_data	reg_data;

		errorCode = copy_from_user(&reg_data,
			(struct amba_vin_test_reg_data __user *)args,
			sizeof(reg_data)) ? -EFAULT : 0;
		if (errorCode)
			break;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
		pvincmd = (pamba_vin_source_cmd)
			ambarella_debug_lookup_name("amba_vin_source_cmd");
#else
		pvincmd = (pamba_vin_source_cmd)amba_vin_source_cmd;
#endif
		if (pvincmd) {
			errorCode = pvincmd(amba_debug_vin_srcid, -1,
				AMBA_VIN_SRC_TEST_SET_SBRG_REG_DATA, &reg_data);
		} else {
			printk("Can't find amba_vin_source_cmd\n");
			errorCode = -EFAULT;
		}
	}
		break;

	default:
		errorCode = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&amba_debug_mutex);

	return errorCode;
}

static int amba_debug_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int		errorCode = 0;

	mutex_lock(&amba_debug_mutex);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,	vma->vm_pgoff,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			errorCode = -EINVAL;
	}

	mutex_unlock(&amba_debug_mutex);
	return errorCode;
}

static int amba_debug_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int amba_debug_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations amba_debug_fops = {
	.owner		= THIS_MODULE,
	.open		= amba_debug_open,
	.release	= amba_debug_release,
	.unlocked_ioctl	= amba_debug_ioctl,
	.mmap		= amba_debug_mmap,
};

static void __exit amba_debug_exit(void)
{
	cdev_del(&amba_debug_cdev);
	unregister_chrdev_region(amba_debug_dev_id, AMBA_DEBUG_DEV_NUM);
}

static int __init amba_debug_init(void)
{
	int			errorCode = 0;

	errorCode = register_chrdev_region(amba_debug_dev_id,
		AMBA_DEBUG_DEV_NUM, AMBA_DEBUG_NAME);
	if (errorCode) {
		printk(KERN_ERR "amba_debug_init failed to get dev region.\n");
		goto amba_debug_init_exit;
	}

	cdev_init(&amba_debug_cdev, &amba_debug_fops);
	amba_debug_cdev.owner = THIS_MODULE;
	errorCode = cdev_add(&amba_debug_cdev,
		amba_debug_dev_id, AMBA_DEBUG_DEV_NUM);
	if (errorCode) {
		printk(KERN_ERR "amba_debug_init cdev_add failed %d.\n",
			errorCode);
		goto amba_debug_init_exit;
	}

	memset(&mem_info, 0, sizeof(mem_info));
#ifdef AXI_PHYS_BASE
	if (AXI_PHYS_BASE) {
		//I1
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].exist	= 1;
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].start	= AXI_PHYS_BASE;
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].size	= AXI_SIZE;
	}
#else
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].exist	= 0;
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].start	= 0;
		mem_info.modules[AMBA_DEBUG_MODULE_AXI].size	= 0;
#endif
	mem_info.modules[AMBA_DEBUG_MODULE_AHB].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_AHB].start	= AHB_PHYS_BASE;
	mem_info.modules[AMBA_DEBUG_MODULE_AHB].size	= AHB_SIZE;

	mem_info.modules[AMBA_DEBUG_MODULE_APB].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_APB].start	= APB_PHYS_BASE;
	mem_info.modules[AMBA_DEBUG_MODULE_APB].size	= APB_SIZE;

	mem_info.modules[AMBA_DEBUG_MODULE_RAM].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_RAM].start	= DDR_START;
	mem_info.modules[AMBA_DEBUG_MODULE_RAM].size	= DDR_SIZE;

	mem_info.modules[AMBA_DEBUG_MODULE_VIN].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_VIN].start	= ambarella_virt_to_phys(VIN_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_VIN].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_VOUT].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_VOUT].start	= ambarella_virt_to_phys(VOUT_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_VOUT].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_VOUT2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_VOUT2].start	= ambarella_virt_to_phys(VOUT2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_VOUT2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_HDMI].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_HDMI].start	= ambarella_virt_to_phys(HDMI_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_HDMI].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_GPIO0].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO0].start	= ambarella_virt_to_phys(GPIO0_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO0].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_GPIO1].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO1].start	= ambarella_virt_to_phys(GPIO1_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO1].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_GPIO2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO2].start	= ambarella_virt_to_phys(GPIO2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_GPIO3].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO3].start	= ambarella_virt_to_phys(GPIO3_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO3].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_GPIO4].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO4].start	= ambarella_virt_to_phys(GPIO4_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_GPIO4].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SPI].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SPI].start	= ambarella_virt_to_phys(SPI_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SPI].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SPI2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SPI2].start	= ambarella_virt_to_phys(SPI2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SPI2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SPI3].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SPI3].start	= ambarella_virt_to_phys(SPI3_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SPI3].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SPI_SLAVE].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SPI_SLAVE].start	= ambarella_virt_to_phys(SPI_SLAVE_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SPI_SLAVE].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_IDC].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_IDC].start	= ambarella_virt_to_phys(IDC_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_IDC].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_IDC2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_IDC2].start	= ambarella_virt_to_phys(IDC2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_IDC2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_ETH].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_ETH].start	= ambarella_virt_to_phys(ETH_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_ETH].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_ETH2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_ETH2].start	= ambarella_virt_to_phys(ETH2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_ETH2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SD].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SD].start	= ambarella_virt_to_phys(SD_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SD].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SD2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SD2].start	= ambarella_virt_to_phys(SD2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SD2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_RCT].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_RCT].start	= ambarella_virt_to_phys(RCT_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_RCT].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_RTC].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_RTC].start	= ambarella_virt_to_phys(RTC_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_RTC].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_ADC].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_ADC].start	= ambarella_virt_to_phys(ADC_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_ADC].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_WDOG].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_WDOG].start	= ambarella_virt_to_phys(WDOG_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_WDOG].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_TIMER].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_TIMER].start	= ambarella_virt_to_phys(TIMER_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_TIMER].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_PWM].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_PWM].start	= ambarella_virt_to_phys(PWM_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_PWM].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_STEPPER].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_STEPPER].start	= ambarella_virt_to_phys(STEPPER_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_STEPPER].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_IR].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_IR].start	= ambarella_virt_to_phys(IR_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_IR].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_VIC].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_VIC].start	= ambarella_virt_to_phys(VIC_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_VIC].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_VIC2].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_VIC2].start	= ambarella_virt_to_phys(VIC2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_VIC2].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SENSOR].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SENSOR].start	= ambarella_virt_to_phys(SD2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SENSOR].size		= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_SBRG].exist		= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_SBRG].start		= ambarella_virt_to_phys(SD2_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_SBRG].size		= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	mem_info.modules[AMBA_DEBUG_MODULE_FDET].exist	= 1;
	mem_info.modules[AMBA_DEBUG_MODULE_FDET].start	= ambarella_virt_to_phys(FACE_DETECTION_BASE);
	mem_info.modules[AMBA_DEBUG_MODULE_FDET].size	= AMBA_DEBUG_MODULE_SIZE_UNKNOWN;

	printk(KERN_INFO "amba_debug_init %d:%d.\n",
		MAJOR(amba_debug_dev_id), MINOR(amba_debug_dev_id));

amba_debug_init_exit:
	return errorCode;
}

module_init(amba_debug_init);
module_exit(amba_debug_exit);

MODULE_DESCRIPTION("Ambarella kernel debug driver");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

