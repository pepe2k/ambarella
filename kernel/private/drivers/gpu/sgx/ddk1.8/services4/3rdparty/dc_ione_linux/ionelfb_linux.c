/**********************************************************************
 *
 * Copyright (C) Imagination Technologies Ltd. All rights reserved.
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

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
#ifndef AUTOCONF_INCLUDED
#include <linux/config.h>
#endif
#endif

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
//#include <linux/ionefb.h>
#include <linux/mutex.h>

#if defined(PVR_IONELFB_DRM_FB)
#include <plat/display.h>
#include <linux/ione_gpu.h>
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define PVR_IONEFB_NEEDS_PLAT_VRFB_H
#endif

#if defined(PVR_IONEFB_NEEDS_PLAT_VRFB_H)
//#include <plat/vrfb.h>
#else
#if defined(PVR_IONEFB_NEEDS_MACH_VRFB_H)
#include <mach/vrfb.h>
#endif
#endif

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif
//#include <ionefb/ionefb.h>
#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif
#endif

#if defined(CONFIG_DSSCOMP)
#include <mach/tiler.h>
#include <video/dsscomp.h>
#include <plat/dsscomp.h>
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "ione.h"
#include "pvrmodule.h"
#if defined(SUPPORT_DRI_DRM)
#include "pvr_drm.h"
#include "3rdparty_dc_drm_shared.h"
#endif

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if !defined(PVR_IONELFB_DRM_FB)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define IONE_DSS_MANAGER(man, dev) struct ione_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_device *drv = (dev)
#define IONE_DSS_MANAGER(man, dev) struct ione_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif
#endif

enum ione_dss_update_mode {
	IONE_DSS_UPDATE_AUTO,
	IONE_DSS_UPDATE_MANUAL,
	IONE_DSS_UPDATE_DISABLED,
};

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
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))

	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else

	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

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
	destroy_workqueue(psSwapChain->psWorkQueue);
}

void IONELFBFlip(IONELFB_DEVINFO *psDevInfo, IONELFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	IONELFB_CONSOLE_LOCK();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

#if defined(CONFIG_DSSCOMP)
	{

		struct fb_fix_screeninfo sFBFix = psDevInfo->psLINFBInfo->fix;
		struct dsscomp_setup_dispc_data d =
		{
			.num_ovls = 1,
			.num_mgrs = 1,
			.mgrs[0].alpha_blending = 1,
			.ovls[0] =
			{
				.cfg =
				{
					.win.w = sFBVar.xres,
					.win.h = sFBVar.yres,
					.crop.x = sFBVar.xoffset,
					.crop.y = sFBVar.yoffset,
					.crop.w = sFBVar.xres,
					.crop.h = sFBVar.yres,
					.width = sFBVar.xres_virtual,
					.height = sFBVar.yres_virtual,
					.stride = sFBFix.line_length,
					.enabled = 1,
					.global_alpha = 255,
				},
			},
		};


		struct tiler_pa_info *pas[] = { NULL };

		d.ovls[0].ba = sFBFix.smem_start;
		ionefb_mode_to_dss_mode(&sFBVar, &d.ovls[0].cfg.color_mode);

		res = dsscomp_gralloc_queue(&d, pas, true, NULL, NULL);
	}
#else

#if !defined(PVR_IONELFB_DONT_USE_FB_PAN_DISPLAY)

	if (sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
#endif
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
#if !defined(PVR_IONELFB_DONT_USE_FB_PAN_DISPLAY)
	else
	{
		res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
#endif
#endif

	IONELFB_CONSOLE_UNLOCK();
}

/*#if !defined(PVR_IONELFB_DRM_FB) || defined(DEBUG)
static IONELFB_BOOL IONELFBValidateDSSUpdateMode(enum ione_dss_update_mode eMode)
{
	switch (eMode)
	{
		case IONE_DSS_UPDATE_AUTO:
		case IONE_DSS_UPDATE_MANUAL:
		case IONE_DSS_UPDATE_DISABLED:
			return IONELFB_TRUE;
		default:
			break;
	}

	return IONELFB_FALSE;
}

static IONELFB_UPDATE_MODE IONELFBFromDSSUpdateMode(enum ione_dss_update_mode eMode)
{
	switch (eMode)
	{
		case IONE_DSS_UPDATE_AUTO:
			return IONELFB_UPDATE_MODE_AUTO;
		case IONE_DSS_UPDATE_MANUAL:
			return IONELFB_UPDATE_MODE_MANUAL;
		case IONE_DSS_UPDATE_DISABLED:
			return IONELFB_UPDATE_MODE_DISABLED;
		default:
			break;
	}

	return IONELFB_UPDATE_MODE_UNDEFINED;
}
#endif*/

/*static IONELFB_BOOL IONELFBValidateUpdateMode(IONELFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case IONELFB_UPDATE_MODE_AUTO:
		case IONELFB_UPDATE_MODE_MANUAL:
		case IONELFB_UPDATE_MODE_DISABLED:
			return IONELFB_TRUE;
		default:
			break;
	}

	return IONELFB_FALSE;
}

static enum ione_dss_update_mode IONELFBToDSSUpdateMode(IONELFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case IONELFB_UPDATE_MODE_AUTO:
			return IONE_DSS_UPDATE_AUTO;
		case IONELFB_UPDATE_MODE_MANUAL:
			return IONE_DSS_UPDATE_MANUAL;
		case IONELFB_UPDATE_MODE_DISABLED:
			return IONE_DSS_UPDATE_DISABLED;
		default:
			break;
	}

	return -1;
}*/

#if defined(DEBUG)
static const char *IONELFBUpdateModeToString(IONELFB_UPDATE_MODE eMode)
{
	switch(eMode)
	{
		case IONELFB_UPDATE_MODE_AUTO:
			return "Auto Update Mode";
		case IONELFB_UPDATE_MODE_MANUAL:
			return "Manual Update Mode";
		case IONELFB_UPDATE_MODE_DISABLED:
			return "Update Mode Disabled";
		case IONELFB_UPDATE_MODE_UNDEFINED:
			return "Update Mode Undefined";
		default:
			break;
	}

	return "Unknown Update Mode";
}

/*static const char *IONELFBDSSUpdateModeToString(enum ione_dss_update_mode eMode)
{
	if (!IONELFBValidateDSSUpdateMode(eMode))
	{
		return "Unknown Update Mode";
	}

	return IONELFBUpdateModeToString(IONELFBFromDSSUpdateMode(eMode));
}*/

void IONELFBPrintInfo(IONELFB_DEVINFO *psDevInfo)
{
#if defined(PVR_IONELFB_DRM_FB)
	struct drm_connector *psConnector;
	unsigned uConnectors;
	unsigned uConnector;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: DRM framebuffer\n", psDevInfo->uiFBDevID));

	for (psConnector = NULL, uConnectors = 0;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		uConnectors++;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Number of screens (DRM connectors): %u\n", psDevInfo->uiFBDevID, uConnectors));

	if (uConnectors == 0)
	{
		return;
	}

	for (psConnector = NULL, uConnector = 0;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; uConnector++)
	{
		enum ione_dss_update_mode eMode = ione_connector_get_update_mode(psConnector);

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: Screen %u: %s (%d)\n", psDevInfo->uiFBDevID, uConnector, IONELFBDSSUpdateModeToString(eMode), (int)eMode));

	}
#else
	IONELFB_UPDATE_MODE eMode = IONELFBGetUpdateMode(psDevInfo);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: non-DRM framebuffer\n", psDevInfo->uiFBDevID));

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": Device %u: %s\n", psDevInfo->uiFBDevID, IONELFBUpdateModeToString(eMode)));
#endif
}
#endif

IONELFB_UPDATE_MODE IONELFBGetUpdateMode(IONELFB_DEVINFO *psDevInfo)
{
#if defined(PVR_IONELFB_DRM_FB)
	struct drm_connector *psConnector;
	IONELFB_UPDATE_MODE eMode = IONELFB_UPDATE_MODE_UNDEFINED;


	for (psConnector = NULL;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		switch(ione_connector_get_update_mode(psConnector))
		{
			case IONE_DSS_UPDATE_MANUAL:
				eMode = IONELFB_UPDATE_MODE_MANUAL;
				break;
			case IONE_DSS_UPDATE_DISABLED:
				if (eMode == IONELFB_UPDATE_MODE_UNDEFINED)
				{
					eMode = IONELFB_UPDATE_MODE_DISABLED;
				}
				break;
			case IONE_DSS_UPDATE_AUTO:

			default:

				if (eMode != IONELFB_UPDATE_MODE_MANUAL)
				{
					eMode = IONELFB_UPDATE_MODE_AUTO;
				}
				break;
		}
	}

	return eMode;
#else
	/*struct ione_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	IONE_DSS_DRIVER(psDSSDrv, psDSSDev);

	enum ione_dss_update_mode eMode;

	if (psDSSDrv == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No DSS device\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return IONELFB_UPDATE_MODE_UNDEFINED;
	}

	if (psDSSDrv->get_update_mode == NULL)
	{
		if (strcmp(psDSSDev->name, "hdmi") == 0)
		{
			return IONELFB_UPDATE_MODE_AUTO;
		}
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No get_update_mode function\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return IONELFB_UPDATE_MODE_UNDEFINED;
	}

	eMode = psDSSDrv->get_update_mode(psDSSDev);
	if (!IONELFBValidateDSSUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
	}

	return IONELFBFromDSSUpdateMode(eMode);*/

	return IONELFB_UPDATE_MODE_AUTO;
#endif
}

IONELFB_BOOL IONELFBSetUpdateMode(IONELFB_DEVINFO *psDevInfo, IONELFB_UPDATE_MODE eMode)
{
#if defined(PVR_IONELFB_DRM_FB)
	struct drm_connector *psConnector;
	enum ione_dss_update_mode eDSSMode;
	IONELFB_BOOL bSuccess = IONELFB_FALSE;
	IONELFB_BOOL bFailure = IONELFB_FALSE;

	if (!IONELFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return IONELFB_FALSE;
	}
	eDSSMode = IONELFBToDSSUpdateMode(eMode);

	for (psConnector = NULL;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		int iRes = ione_connector_set_update_mode(psConnector, eDSSMode);
		IONELFB_BOOL bRes = (iRes == 0);


		bSuccess |= bRes;
		bFailure |= !bRes;
	}

	if (!bFailure)
	{
		if (!bSuccess)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: No screens\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}

		return IONELFB_TRUE;
	}

	if (!bSuccess)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for any screen\n", __FUNCTION__, psDevInfo->uiFBDevID, IONELFBUpdateModeToString(eMode)));
		return IONELFB_FALSE;
	}

	if (eMode == IONELFB_UPDATE_MODE_AUTO)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set %s for all screens\n", __FUNCTION__, psDevInfo->uiFBDevID, IONELFBUpdateModeToString(eMode)));
		return IONELFB_FALSE;
	}

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: %s set for some screens\n", __FUNCTION__, psDevInfo->uiFBDevID, IONELFBUpdateModeToString(eMode)));

	return IONELFB_TRUE;
#else
	/*struct ione_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	IONE_DSS_DRIVER(psDSSDrv, psDSSDev);
	enum ione_dss_update_mode eDSSMode;
	int res;

	if (psDSSDrv == NULL || psDSSDrv->set_update_mode == NULL)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Can't set update mode\n", __FUNCTION__, psDevInfo->uiFBDevID));
		return IONELFB_FALSE;
	}

	if (!IONELFBValidateUpdateMode(eMode))
	{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Unknown update mode (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, (int)eMode));
			return IONELFB_FALSE;
	}
	eDSSMode = IONELFBToDSSUpdateMode(eMode);

	res = psDSSDrv->set_update_mode(psDSSDev, eDSSMode);
	if (res != 0)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: set_update_mode (%s) failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, IONELFBDSSUpdateModeToString(eDSSMode), res));
	}

	return (res == 0);*/

	return 0;
#endif
}

IONELFB_BOOL IONELFBWaitForVSync(IONELFB_DEVINFO *psDevInfo)
{
#if defined(PVR_IONELFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL;)
	{
		(void) ione_encoder_wait_for_vsync(psConnector->encoder);
	}

	return IONELFB_TRUE;
#else
	/*struct ione_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	IONE_DSS_MANAGER(psDSSMan, psDSSDev);

	if (psDSSMan != NULL && WAIT_FOR_VSYNC(psDSSMan) != NULL)
	{
		int res = WAIT_FOR_VSYNC(psDSSMan)(psDSSMan);
		if (res != 0)
		{
			DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Wait for vsync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res));
			return IONELFB_FALSE;
		}
	}*/

	return IONELFB_TRUE;
#endif
}

IONELFB_BOOL IONELFBManualSync(IONELFB_DEVINFO *psDevInfo)
{
#if defined(PVR_IONELFB_DRM_FB)
	struct drm_connector *psConnector;

	for (psConnector = NULL;
		(psConnector = ione_fbdev_get_next_connector(psDevInfo->psLINFBInfo, psConnector)) != NULL; )
	{

		if (ione_connector_sync(psConnector) != 0)
		{
			(void) ione_encoder_wait_for_vsync(psConnector->encoder);
		}
	}

	return IONELFB_TRUE;
#else
	/*struct ione_dss_device *psDSSDev = fb2display(psDevInfo->psLINFBInfo);
	IONE_DSS_DRIVER(psDSSDrv, psDSSDev);

	if (psDSSDrv != NULL && psDSSDrv->sync != NULL)
	{
		int res = psDSSDrv->sync(psDSSDev);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Sync failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
			return IONELFB_FALSE;
		}
	}*/

	return IONELFB_TRUE;
#endif
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

#if 0
	if (psDevInfo != NULL)
	{
		if (bBlanked)
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
		else
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unblank event received\n", __FUNCTION__, psDevInfo->uiFBDevID));
		}
	}
	else
	{
		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": %s: Device %u: Blank/Unblank event for unknown framebuffer\n", __FUNCTION__, psFBInfo->node));
	}
#endif

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

	IONELFB_CONSOLE_LOCK();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	IONELFB_CONSOLE_UNLOCK();
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (IONELFB_ERROR_GENERIC);
	}

	return (IONELFB_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void IONELFBBlankDisplay(IONELFB_DEVINFO *psDevInfo)
{
	IONELFB_CONSOLE_LOCK();
	fb_blank(psDevInfo->psLINFBInfo, 1);
	IONELFB_CONSOLE_UNLOCK();
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
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (IONELFB_ERROR_GENERIC);
	}

	eError = IONELFBUnblankDisplay(psDevInfo);
	if (eError != IONELFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = IONELFBEarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = IONELFBEarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
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
		printk(KERN_ERR DRIVER_PREFIX
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

	printk(KERN_ERR DRIVER_PREFIX
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
				printk(KERN_WARNING DRIVER_PREFIX ": %s: PVR Device %u: Display %s\n",
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

			IONELFB_CONSOLE_LOCK();
			ret = fb_blank(psDevInfo->psLINFBInfo, iFBMode);
			IONELFB_CONSOLE_UNLOCK();

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
		printk(KERN_ERR DRIVER_PREFIX ": %s: IONELFBInit failed\n", __FUNCTION__);
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
		printk(KERN_ERR DRIVER_PREFIX ": %s: IONELFBDeInit failed\n", __FUNCTION__);
	}
}

#if !defined(SUPPORT_DRI_DRM)
late_initcall(IONELFB_Init);
module_exit(IONELFB_Cleanup);
#endif
