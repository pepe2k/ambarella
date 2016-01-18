/**********************************************************************
 *
 * Copyright(c) 2008 Imagination Technologies Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful but, except
 * as otherwise stated in writing, without any warranty; without even the
 * implied warranty of merchantability or fitness for a particular purpose.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * Contact Information:
 * Imagination Technologies Ltd. <gpl-support@imgtec.com>
 * Home Park Estate, Kings Langley, Herts, WD4 8LZ, UK
 *
 ******************************************************************************/

#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif

#include <linux/version.h>
#include <linux/module.h>

#include <linux/pci.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

#if defined(LDM_PLATFORM)
#include <linux/platform_device.h>
#endif

#include <asm/io.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "ionelfb.h"
#include "pvrmodule.h"

MODULE_SUPPORTED_DEVICE(DEVNAME);

static int isr_enabled = 0;
static spinlock_t isr_lock;

#define unref__ __attribute__ ((unused))

void *IONELFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void IONELFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}


IONE_ERROR IONELFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (IONE_ERROR_INVALID_PARAMS);
	}


	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (IONE_OK);
}

#if defined(SYS_USING_INTERRUPTS)

static irqreturn_t IONELFBVSyncISR(int irqno, void *arg)
{
	IONELFB_SWAPCHAIN *psSwapChain= (IONELFB_SWAPCHAIN *)arg;
	spin_lock(&isr_lock);
	if (isr_enabled)
		(void) IONELFBVSyncIHandler(psSwapChain);
	spin_unlock(&isr_lock);

	return IRQ_HANDLED;
}

static inline int IONELFBRegisterVSyncISR(IONELFB_SWAPCHAIN *psSwapChain)
{
	return request_irq(IONE_VSYNC_IRQ, IONELFBVSyncISR,
		IRQF_TRIGGER_RISING | IRQF_SHARED, "sgx vsync", psSwapChain);
}

static inline int IONELFBUnregisterVSyncISR(IONELFB_SWAPCHAIN *psSwapChain)
{
	free_irq(IONE_VSYNC_IRQ, psSwapChain);

	return 0;
}

#endif

void IONELFBEnableVSyncInterrupt(IONELFB_SWAPCHAIN *psSwapChain)
{
	spin_lock(&isr_lock);
	isr_enabled = 1;
	spin_unlock(&isr_lock);
}

void IONELFBDisableVSyncInterrupt(IONELFB_SWAPCHAIN *psSwapChain)
{
	spin_lock(&isr_lock);
	isr_enabled = 0;
	spin_unlock(&isr_lock);
}

IONE_ERROR IONELFBInstallVSyncISR(IONELFB_SWAPCHAIN *psSwapChain)
{
#if defined(SYS_USING_INTERRUPTS)
	IONELFBDisableVSyncInterrupt(psSwapChain);

	if (IONELFBRegisterVSyncISR(psSwapChain))
	{
		printk(KERN_INFO DRIVER_PREFIX ": IONELFBInstallVSyncISR: Request IONELCD IRQ failed\n");
		return (IONE_ERROR_INIT_FAILURE);
	}

#endif
	return (IONE_OK);
}


IONE_ERROR IONELFBUninstallVSyncISR (IONELFB_SWAPCHAIN *psSwapChain)
{
#if defined(SYS_USING_INTERRUPTS)
	IONELFBDisableVSyncInterrupt(psSwapChain);

	IONELFBUnregisterVSyncISR(psSwapChain);

#endif
	return (IONE_OK);
}

void IONELFBEnableDisplayRegisterAccess(void)
{
}

void IONELFBDisableDisplayRegisterAccess(void)
{
}

extern int amba_gpu_update_osd(unsigned long physicalAddr);

void IONELFBFlip(IONELFB_SWAPCHAIN *psSwapChain, unsigned long aPhyAddr)
{
	amba_gpu_update_osd(aPhyAddr);
}

#if defined(LDM_PLATFORM)

static IONE_BOOL bDeviceSuspended;

static void IONELFBCommonSuspend(void)
{
	if (bDeviceSuspended)
	{
		return;
	}

	IONELFBDriverSuspend();

	bDeviceSuspended = IONE_TRUE;
}

static int IONELFBDriverSuspend_Entry(struct platform_device unref__ *pDevice, pm_message_t unref__ state)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": IONELFBDriverSuspend_Entry\n"));

	IONELFBCommonSuspend();

	return 0;
}

static int IONELFBDriverResume_Entry(struct platform_device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": IONELFBDriverResume_Entry\n"));

	IONELFBDriverResume();

	bDeviceSuspended = IONE_FALSE;

	return 0;
}

static IMG_VOID IONELFBDriverShutdown_Entry(struct platform_device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": IONELFBDriverShutdown_Entry\n"));

	IONELFBCommonSuspend();
}

static struct platform_driver ionelfb_driver = {
	.driver = {
		.name		= DRVNAME,
	},
	.suspend	= IONELFBDriverSuspend_Entry,
	.resume		= IONELFBDriverResume_Entry,
	.shutdown	= IONELFBDriverShutdown_Entry,
};

#if defined(MODULE)

static void IONELFBDeviceRelease_Entry(struct device unref__ *pDevice)
{
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": IONELFBDriverRelease_Entry\n"));

	IONELFBCommonSuspend();
}

static struct platform_device ionelfb_device = {
	.name			= DEVNAME,
	.id				= -1,
	.dev 			= {
		.release		= IONELFBDeviceRelease_Entry
	}
};

#endif

#endif

static int __init IONELFB_Init(void)
{
#if defined(LDM_PLATFORM)
	int error;
#endif

	spin_lock_init(&isr_lock);

	if(IONELFBInit() != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": IONELFB_Init: IONELFBInit failed\n");
		return -ENODEV;
	}

#if defined(LDM_PLATFORM)
	if ((error = platform_driver_register(&ionelfb_driver)) != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": IONELFB_Init: Unable to register platform driver (%d)\n", error);

		goto ExitDeinit;
	}

#if defined(MODULE)
	if ((error = platform_device_register(&ionelfb_device)) != 0)
	{
		platform_driver_unregister(&ionelfb_driver);

		printk(KERN_WARNING DRIVER_PREFIX ": IONELFB_Init: Unable to register platform device (%d)\n", error);

		goto ExitDeinit;
	}
#endif

#endif

	return 0;

#if defined(LDM_PLATFORM)
ExitDeinit:
	if(IONELFBDeinit() != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": IONELFB_Init: IONELFBDeinit failed\n");
	}

	return -ENODEV;
#endif
}

static IMG_VOID __exit IONELFB_Cleanup(IMG_VOID)
{
#if defined (LDM_PLATFORM)
#if defined (MODULE)
	platform_device_unregister(&ionelfb_device);
#endif
	platform_driver_unregister(&ionelfb_driver);
#endif

	if(IONELFBDeinit() != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": IONELFB_Cleanup: IONELFBDeinit failed\n");
	}
}

module_init(IONELFB_Init);
module_exit(IONELFB_Cleanup);

