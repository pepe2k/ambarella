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

#include <asm/atomic.h>

#if defined(SUPPORT_DRI_DRM)
#include <drm/drmP.h>
#else
#include <linux/module.h>
#endif

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/mutex.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define PVR_IONEFB3_NEEDS_PLAT_VRFB_H
#endif

#if defined(PVR_IONEFB_NEEDS_PLAT_VRFB_H)
# include <plat/vrfb.h>
#else
# if defined(PVR_IONEFB3_NEEDS_MACH_VRFB_H)
#  include <mach/vrfb.h>
# endif
#endif

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif

#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "ionelfb.h"
#include "pvrmodule.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define IONE_DSS_MANAGER(man, dev) struct ione_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_device *drv = (dev)
#define IONE_DSS_MANAGER(man, dev) struct ione_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif

#define SGX_VSYNC0_IRQ		VOUT_IRQ
#define SGX_VSYNC1_IRQ		ORC_VOUT0_IRQ

static irqreturn_t sgx_vsync_isr(int irqno, void *dev_id)
{
	IONELFB_SWAPCHAIN *psSwapChain = (IONELFB_SWAPCHAIN *)dev_id;
	unsigned long flags;

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	if (psSwapChain->wait_num) {
		psSwapChain->wait_num = 0;
		psSwapChain->irq_interval = 0;
	} else {
		psSwapChain->irq_interval++;
	}
	psSwapChain->timeout = 0;
	spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	wake_up_interruptible(&psSwapChain->vsync_wq);

	return IRQ_HANDLED;
}

void *IONELFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void IONELFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void IONELFBCreateSwapChainLockInit(IONELFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void IONELFBCreateSwapChainLockDeInit(IONELFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void IONELFBCreateSwapChainLock(IONELFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void IONELFBCreateSwapChainUnLock(IONELFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void IONELFBAtomicBoolInit(IONELFB_ATOMIC_BOOL *psAtomic, IONELFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void IONELFBAtomicBoolDeInit(IONELFB_ATOMIC_BOOL *psAtomic)
{
}

void IONELFBAtomicBoolSet(IONELFB_ATOMIC_BOOL *psAtomic, IONELFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

IONELFB_BOOL IONELFBAtomicBoolRead(IONELFB_ATOMIC_BOOL *psAtomic)
{
	return (IONELFB_BOOL)atomic_read(psAtomic);
}

void IONELFBAtomicIntInit(IONELFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void IONELFBAtomicIntDeInit(IONELFB_ATOMIC_INT *psAtomic)
{
}

void IONELFBAtomicIntSet(IONELFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int IONELFBAtomicIntRead(IONELFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void IONELFBAtomicIntInc(IONELFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

IONELFB_ERROR IONELFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (IONELFB_ERROR_INVALID_PARAMS);
	}


	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (IONELFB_OK);
}

void IONELFBQueueBufferForSwap(IONELFB_SWAPCHAIN *psSwapChain, IONELFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

static void WorkQueueHandler(struct work_struct *psWork)
{
	IONELFB_BUFFER *psBuffer = container_of(psWork, IONELFB_BUFFER, sWork);

	IONELFBSwapHandler(psBuffer);
}

IONELFB_ERROR IONELFBCreateSwapQueue(IONELFB_SWAPCHAIN *psSwapChain)
{
	int	errorCode;

	psSwapChain->psWorkQueue = create_singlethread_workqueue(DEVNAME);
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: create_singlethreaded_workqueue failed\n", __FUNCTION__, psSwapChain->uiFBDevID);
		return (IONELFB_ERROR_INIT_FAILURE);
	}

	init_waitqueue_head(&psSwapChain->vsync_wq);
	spin_lock_init(&psSwapChain->wait_num_lock);
	psSwapChain->wait_num = 0;
	psSwapChain->irq_interval = 0;
	psSwapChain->timeout = 0;
	if (psSwapChain->uiFBDevID == 0) {
		psSwapChain->irq_no = SGX_VSYNC0_IRQ;
		errorCode = request_irq(psSwapChain->irq_no, sgx_vsync_isr,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "sgx fb0", psSwapChain);
	} else {
		psSwapChain->irq_no = SGX_VSYNC1_IRQ;
		errorCode = request_irq(psSwapChain->irq_no, sgx_vsync_isr,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "sgx fb1", psSwapChain);
	}
	if (errorCode) {
		printk(KERN_WARNING DRIVER_PREFIX "Request SGX Vsync IRQ %d Failed!\n", psSwapChain->irq_no);
		return (IONELFB_ERROR_INIT_FAILURE);
	}

	return (IONELFB_OK);
}

void IONELFBInitBufferForSwap(IONELFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

void IONELFBDestroySwapQueue(IONELFB_SWAPCHAIN *psSwapChain)
{
	free_irq(psSwapChain->irq_no, psSwapChain);
	destroy_workqueue(psSwapChain->psWorkQueue);
}

void IONELFBFlip(IONELFB_DEVINFO *psDevInfo, IONELFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;


	if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
	else
	{
		res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
}

IONELFB_UPDATE_MODE IONELFBGetUpdateMode(IONELFB_DEVINFO *psDevInfo)
{
	return IONELFB_UPDATE_MODE_AUTO;
}

IONELFB_BOOL IONELFBSetUpdateMode(IONELFB_DEVINFO *psDevInfo, IONELFB_UPDATE_MODE eMode)
{
	return 0;
}

#ifdef CONFIG_SGX_SYNCHRONIZE_NONE
IONELFB_BOOL IONELFBWaitForVSync(IONELFB_DEVINFO *psDevInfo)
{
	return IONELFB_TRUE;
}
#endif

#ifdef CONFIG_SGX_SYNCHRONIZE_NEXT_VSYNC
IONELFB_BOOL IONELFBWaitForVSync(IONELFB_DEVINFO *psDevInfo)
{
	IONELFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	unsigned long flags;

	/* Vout is not running, do not wait */
	if (psSwapChain->timeout) {
		return IONELFB_TRUE;
	}

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	psSwapChain->wait_num = 1;
	spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	if (!wait_event_interruptible_timeout(psSwapChain->vsync_wq, psSwapChain->wait_num == 0, HZ / 10)) {
		spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
		psSwapChain->timeout = 1;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
		printk("Wait for vout%d irq timeout, it might die!\n", psSwapChain->uiFBDevID);
	}

	return IONELFB_TRUE;
}
#endif

#ifdef CONFIG_SGX_SYNCHRONIZE_INTERVAL
IONELFB_BOOL IONELFBWaitForVSync(IONELFB_DEVINFO *psDevInfo)
{
	IONELFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	unsigned long flags;

	/* Vout is not running, do not wait */
	if (psSwapChain->timeout) {
		return IONELFB_TRUE;
	}

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	if (psSwapChain->irq_interval) {
		psSwapChain->irq_interval = 0;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	} else {
		psSwapChain->wait_num = 1;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
		if (!wait_event_interruptible_timeout(psSwapChain->vsync_wq, psSwapChain->wait_num == 0, HZ / 10)) {
			spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
			psSwapChain->timeout = 1;
			spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
			printk("Wait for vout%d irq timeout, it might die!\n", psSwapChain->uiFBDevID);
		}
	}

	return IONELFB_TRUE;
}
#endif

IONELFB_BOOL IONELFBManualSync(IONELFB_DEVINFO *psDevInfo)
{
	return IONELFB_TRUE;
}

IONELFB_BOOL IONELFBCheckModeAndSync(IONELFB_DEVINFO *psDevInfo)
{
	IONELFB_UPDATE_MODE eMode = IONELFBGetUpdateMode(psDevInfo);

	switch(eMode)
	{
		case IONELFB_UPDATE_MODE_AUTO:
		case IONELFB_UPDATE_MODE_MANUAL:
			return IONELFBManualSync(psDevInfo);
		default:
			break;
	}

	return IONELFB_TRUE;
}

static int IONELFBFrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	IONELFB_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	IONELFB_BOOL bBlanked;


	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? IONELFB_TRUE: IONELFB_FALSE;

	psDevInfo = IONELFBGetDevInfoPtr(psFBInfo->node);

	if (psDevInfo != NULL)
	{
		IONELFBAtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		IONELFBAtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

IONELFB_ERROR IONELFBUnblankDisplay(IONELFB_DEVINFO *psDevInfo)
{
	int res;

	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (IONELFB_ERROR_GENERIC);
	}

	return (IONELFB_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void IONELFBBlankDisplay(IONELFB_DEVINFO *psDevInfo)
{
	fb_blank(psDevInfo->psLINFBInfo, 1);
}

static void IONELFBEarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		IONELFB_DEVINFO *psDevInfo = IONELFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			IONELFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, IONELFB_TRUE);
			IONELFBBlankDisplay(psDevInfo);
		}
	}
}

static void IONELFBEarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		IONELFB_DEVINFO *psDevInfo = IONELFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			IONELFBUnblankDisplay(psDevInfo);
			IONELFBAtomicBoolSet(&psDevInfo->sEarlySuspendFlag, IONELFB_FALSE);
		}
	}
}

#endif

IONELFB_ERROR IONELFBEnableLFBEventNotification(IONELFB_DEVINFO *psDevInfo)
{
	int                res;
	IONELFB_ERROR         eError;


	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = IONELFBFrameBufferEvents;

	IONELFBAtomicBoolSet(&psDevInfo->sBlanked, IONELFB_FALSE);
	IONELFBAtomicIntSet(&psDevInfo->sBlankEvents, 0);

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (IONELFB_ERROR_GENERIC);
	}

	eError = IONELFBUnblankDisplay(psDevInfo);
	if (eError != IONELFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = IONELFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = IONELFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (IONELFB_OK);
}

IONELFB_ERROR IONELFBDisableLFBEventNotification(IONELFB_DEVINFO *psDevInfo)
{
	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif


	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (IONELFB_ERROR_GENERIC);
	}

	IONELFBAtomicBoolSet(&psDevInfo->sBlanked, IONELFB_FALSE);

	return (IONELFB_OK);
}

#if defined(SUPPORT_DRI_DRM) && defined(PVR_DISPLAY_CONTROLLER_DRM_IOCTL)
static IONELFB_DEVINFO *IONELFBPVRDevIDToDevInfo(unsigned uiPVRDevID)
{
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i=0; i < uiMaxFBDevIDPlusOne; i++)
	{
		IONELFB_DEVINFO *psDevInfo = IONELFBGetDevInfoPtr(i);

		if (psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			return psDevInfo;
		}
	}

	printk(KERN_WARNING DRIVER_PREFIX
		": %s: PVR Device %u: Couldn't find device\n", __FUNCTION__, uiPVRDevID);

	return NULL;
}

int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Ioctl)(struct drm_device unref__ *dev, void *arg, struct drm_file unref__ *pFile)
{
	uint32_t *puiArgs;
	uint32_t uiCmd;
	unsigned uiPVRDevID;
	int ret = 0;
	IONELFB_DEVINFO *psDevInfo;

	if (arg == NULL)
	{
		return -EFAULT;
	}

	puiArgs = (uint32_t *)arg;
	uiCmd = puiArgs[PVR_DRM_DISP_ARG_CMD];
	uiPVRDevID = puiArgs[PVR_DRM_DISP_ARG_DEV];

	psDevInfo = IONELFBPVRDevIDToDevInfo(uiPVRDevID);
	if (psDevInfo == NULL)
	{
		return -EINVAL;
	}


	switch (uiCmd)
	{
		case PVR_DRM_DISP_CMD_LEAVE_VT:
		case PVR_DRM_DISP_CMD_ENTER_VT:
		{
			IONELFB_BOOL bLeaveVT = (uiCmd == PVR_DRM_DISP_CMD_LEAVE_VT);
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: %s\n",
				__FUNCTION__, uiPVRDevID,
				bLeaveVT ? "Leave VT" : "Enter VT"));

			IONELFBCreateSwapChainLock(psDevInfo);

			IONELFBAtomicBoolSet(&psDevInfo->sLeaveVT, bLeaveVT);
			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);

				if (bLeaveVT)
				{
					IONELFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
					(void) IONELFBCheckModeAndSync(psDevInfo);
				}
			}

			IONELFBCreateSwapChainUnLock(psDevInfo);
			(void) IONELFBUnblankDisplay(psDevInfo);
			break;
		}
		case PVR_DRM_DISP_CMD_ON:
		case PVR_DRM_DISP_CMD_STANDBY:
		case PVR_DRM_DISP_CMD_SUSPEND:
		case PVR_DRM_DISP_CMD_OFF:
		{
			int iFBMode;
#if defined(DEBUG)
			{
				const char *pszMode;
				switch(uiCmd)
				{
					case PVR_DRM_DISP_CMD_ON:
						pszMode = "On";
						break;
					case PVR_DRM_DISP_CMD_STANDBY:
						pszMode = "Standby";
						break;
					case PVR_DRM_DISP_CMD_SUSPEND:
						pszMode = "Suspend";
						break;
					case PVR_DRM_DISP_CMD_OFF:
						pszMode = "Off";
						break;
					default:
						pszMode = "(Unknown Mode)";
						break;
				}
				printk (KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: Display %s\n",
				__FUNCTION__, uiPVRDevID, pszMode);
			}
#endif
			switch(uiCmd)
			{
				case PVR_DRM_DISP_CMD_ON:
					iFBMode = FB_BLANK_UNBLANK;
					break;
				case PVR_DRM_DISP_CMD_STANDBY:
					iFBMode = FB_BLANK_HSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_SUSPEND:
					iFBMode = FB_BLANK_VSYNC_SUSPEND;
					break;
				case PVR_DRM_DISP_CMD_OFF:
					iFBMode = FB_BLANK_POWERDOWN;
					break;
				default:
					return -EINVAL;
			}

			IONELFBCreateSwapChainLock(psDevInfo);

			if (psDevInfo->psSwapChain != NULL)
			{
				flush_workqueue(psDevInfo->psSwapChain->psWorkQueue);
			}

			ret = fb_blank(psDevInfo->psLINFBInfo, iFBMode);

			IONELFBCreateSwapChainUnLock(psDevInfo);

			break;
		}
		default:
		{
			ret = -EINVAL;
			break;
		}
	}

	return ret;
}
#endif

#if defined(SUPPORT_DRI_DRM)
int PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Init)(struct drm_device unref__ *dev)
#else
static int __init IONELFB_Init(void)
#endif
{

	if(IONELFBInit() != IONELFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: IONELFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;

}

#if defined(SUPPORT_DRI_DRM)
void PVR_DRM_MAKENAME(DISPLAY_CONTROLLER, _Cleanup)(struct drm_device unref__ *dev)
#else
static void __exit IONELFB_Cleanup(void)
#endif
{
	if(IONELFBDeInit() != IONELFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: IONELFBDeInit failed\n", __FUNCTION__);
	}
}

#if !defined(SUPPORT_DRI_DRM)
late_initcall(IONELFB_Init);
module_exit(IONELFB_Cleanup);
#endif
