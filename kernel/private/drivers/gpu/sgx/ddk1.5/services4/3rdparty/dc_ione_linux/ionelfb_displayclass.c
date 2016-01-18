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

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/spinlock.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "ionelfb.h"

static void *gpvAnchor;

static int fb_idx = 0;

#define IONELFB_COMMAND_COUNT		1

static PFN_DC_GET_PVRJTABLE pfnGetPVRJTable = 0;

static IONELFB_DEVINFO * GetAnchorPtr(void)
{
	return (IONELFB_DEVINFO *)gpvAnchor;
}

static void SetAnchorPtr(IONELFB_DEVINFO *psDevInfo)
{
	gpvAnchor = (void*)psDevInfo;
}


static void FlushInternalVSyncQueue(IONELFB_SWAPCHAIN *psSwapChain)
{
	IONELFB_VSYNC_FLIP_ITEM *psFlipItem;
	unsigned long            ulMaxIndex;
	unsigned long            i;


	psFlipItem = &psSwapChain->psVSyncFlips[psSwapChain->ulRemoveIndex];
	ulMaxIndex = psSwapChain->ulBufferCount - 1;

	for(i = 0; i < psSwapChain->ulBufferCount; i++)
	{
		if (psFlipItem->bValid == IONE_FALSE)
		{
			continue;
		}

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": FlushInternalVSyncQueue: Flushing swap buffer (index %lu)\n", psSwapChain->ulRemoveIndex));

		if(psFlipItem->bFlipped == IONE_FALSE)
		{

			IONELFBFlip(psSwapChain, (unsigned long)psFlipItem->sSysAddr);
		}

		if(psFlipItem->bCmdCompleted == IONE_FALSE)
		{
			DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX ": FlushInternalVSyncQueue: Calling command complete for swap buffer (index %lu)\n", psSwapChain->ulRemoveIndex));

			psSwapChain->psPVRJTable->pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_TRUE);
		}


		psSwapChain->ulRemoveIndex++;

		if(psSwapChain->ulRemoveIndex > ulMaxIndex)
		{
			psSwapChain->ulRemoveIndex = 0;
		}


		psFlipItem->bFlipped = IONE_FALSE;
		psFlipItem->bCmdCompleted = IONE_FALSE;
		psFlipItem->bValid = IONE_FALSE;


		psFlipItem = &psSwapChain->psVSyncFlips[psSwapChain->ulRemoveIndex];
	}

	psSwapChain->ulInsertIndex = 0;
	psSwapChain->ulRemoveIndex = 0;
}

static void SetFlushStateInternalNoLock(IONELFB_DEVINFO* psDevInfo,
                                        IONE_BOOL bFlushState)
{
	IONELFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;

	if (psSwapChain == NULL)
	{
		return;
	}

	if (bFlushState)
	{
		if (psSwapChain->ulSetFlushStateRefCount == 0)
		{
			IONELFBDisableVSyncInterrupt(psSwapChain);
			psSwapChain->bFlushCommands = IONE_TRUE;
			FlushInternalVSyncQueue(psSwapChain);
		}
		psSwapChain->ulSetFlushStateRefCount++;
	}
	else
	{
		if (psSwapChain->ulSetFlushStateRefCount != 0)
		{
			psSwapChain->ulSetFlushStateRefCount--;
			if (psSwapChain->ulSetFlushStateRefCount == 0)
			{
				psSwapChain->bFlushCommands = IONE_FALSE;
				IONELFBEnableVSyncInterrupt(psSwapChain);
			}
		}
	}
}

static IMG_VOID SetFlushStateInternal(IONELFB_DEVINFO* psDevInfo,
                                      IONE_BOOL bFlushState)
{
	unsigned long ulLockFlags;

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);

	SetFlushStateInternalNoLock(psDevInfo, bFlushState);

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
}

static void SetFlushStateExternal(IONELFB_DEVINFO* psDevInfo,
                                  IONE_BOOL bFlushState)
{
	unsigned long ulLockFlags;

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);


	if (psDevInfo->bFlushCommands != bFlushState)
	{
		psDevInfo->bFlushCommands = bFlushState;
		SetFlushStateInternalNoLock(psDevInfo, bFlushState);
	}

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
}

static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	IONELFB_DEVINFO *psDevInfo = (IONELFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			SetFlushStateExternal(psDevInfo, IONE_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			SetFlushStateExternal(psDevInfo, IONE_FALSE);
			break;
		default:
			break;
	}

	return;
}

static int FrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	IONELFB_DEVINFO *psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	IONE_BOOL bBlanked;


	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	psDevInfo = GetAnchorPtr();
	psSwapChain = psDevInfo->psSwapChain;

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? IONE_TRUE: IONE_FALSE;

	if (bBlanked != psSwapChain->bBlanked)
	{
		psSwapChain->bBlanked = bBlanked;

		if (bBlanked)
		{

			SetFlushStateInternal(psDevInfo, IONE_TRUE);
		}
		else
		{

			SetFlushStateInternal(psDevInfo, IONE_FALSE);
		}
	}

	return 0;
}


static IONE_ERROR UnblankDisplay(IONELFB_DEVINFO *psDevInfo)
{
	int res;

	acquire_console_sem();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	release_console_sem();
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": fb_blank failed (%d)", res);
		return (IONE_ERROR_GENERIC);
	}

	return (IONE_OK);
}

static IONE_ERROR EnableLFBEventNotification(IONELFB_DEVINFO *psDevInfo)
{
	int                res;
	IONELFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	IONE_ERROR         eError;


	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = FrameBufferEvents;

	psSwapChain->bBlanked = IONE_FALSE;

	res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": fb_register_client failed (%d)", res);

		return (IONE_ERROR_GENERIC);
	}

	eError = UnblankDisplay(psDevInfo);
	if (eError != IONE_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": UnblankDisplay failed (%d)", eError));
		return eError;
	}

	return (IONE_OK);
}

static IONE_ERROR DisableLFBEventNotification(IONELFB_DEVINFO *psDevInfo)
{
	int res;


	res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": fb_unregister_client failed (%d)", res);
		return (IONE_ERROR_GENERIC);
	}

	return (IONE_OK);
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 ui32DeviceID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	IONELFB_DEVINFO *psDevInfo;
	IONE_ERROR eError;

	UNREFERENCED_PARAMETER(ui32DeviceID);

	psDevInfo = GetAnchorPtr();


	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;

	eError = UnblankDisplay(psDevInfo);
	if (eError != IONE_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": UnblankDisplay failed (%d)", eError));
		return (PVRSRV_ERROR_GENERIC);
	}


	*phDevice = (IMG_HANDLE)psDevInfo;

	return (PVRSRV_OK);
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

	return (PVRSRV_OK);
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	IONELFB_DEVINFO	*psDevInfo;

	if(!hDevice || !pui32NumFormats)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*pui32NumFormats = 1;

	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return (PVRSRV_OK);
}

static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice,
                               DISPLAY_FORMAT *psFormat,
                               IMG_UINT32 *pui32NumDims,
                               DISPLAY_DIMS *psDim)
{
	IONELFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;


	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	IONELFB_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return (PVRSRV_OK);
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	IONELFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return (PVRSRV_OK);
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer,
                                    IMG_SYS_PHYADDR   **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID          **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous)
{
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_BUFFER *psSystemBuffer;

	if(!hDevice)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}
	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	if(!hBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}
	psSystemBuffer = (IONELFB_BUFFER *)hBuffer;

	if (!ppsSysAddr)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	if (!pui32ByteSize)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
		*pbIsContiguous = IMG_TRUE;
	}

	return (PVRSRV_OK);
}

static PVRSRV_ERROR CreateDCSwapChain(IMG_HANDLE hDevice,
                                      IMG_UINT32 ui32Flags,
                                      DISPLAY_SURF_ATTRIBUTES *psDstSurfAttrib,
                                      DISPLAY_SURF_ATTRIBUTES *psSrcSurfAttrib,
                                      IMG_UINT32 ui32BufferCount,
                                      PVRSRV_SYNC_DATA **ppsSyncData,
                                      IMG_UINT32 ui32OEMFlags,
                                      IMG_HANDLE *phSwapChain,
                                      IMG_UINT32 *pui32SwapChainID)
{
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	IONELFB_BUFFER *psBuffer;
	IONELFB_VSYNC_FLIP_ITEM *psVSyncFlips;
	IMG_UINT32 i;
	PVRSRV_ERROR eError = PVRSRV_ERROR_GENERIC;
	unsigned long ulLockFlags;
	IMG_UINT32 ui32BuffersToSkip;

	UNREFERENCED_PARAMETER(ui32OEMFlags);
	UNREFERENCED_PARAMETER(pui32SwapChainID);


	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;


	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return (PVRSRV_ERROR_NOT_SUPPORTED);
	}


	if(psDevInfo->psSwapChain != NULL)
	{
		return (PVRSRV_ERROR_FLIP_CHAIN_EXISTS);
	}


	if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		return (PVRSRV_ERROR_TOOMANYBUFFERS);
	}

	if ((psDevInfo->sFBInfo.ulRoundedBufferSize * (unsigned long)ui32BufferCount) > psDevInfo->sFBInfo.ulFBSize)
	{
		return (PVRSRV_ERROR_TOOMANYBUFFERS);
	}


	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;


	if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
	{

		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{

		return (PVRSRV_ERROR_INVALID_PARAMS);
	}


	UNREFERENCED_PARAMETER(ui32Flags);


	psSwapChain = (IONELFB_SWAPCHAIN*)IONELFBAllocKernelMem(sizeof(IONELFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		return (PVRSRV_ERROR_OUT_OF_MEMORY);
	}

	psBuffer = (IONELFB_BUFFER*)IONELFBAllocKernelMem(sizeof(IONELFB_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psVSyncFlips = (IONELFB_VSYNC_FLIP_ITEM *)IONELFBAllocKernelMem(sizeof(IONELFB_VSYNC_FLIP_ITEM) * ui32BufferCount);
	if (!psVSyncFlips)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeBuffers;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->psVSyncFlips = psVSyncFlips;
	psSwapChain->ulInsertIndex = 0;
	psSwapChain->ulRemoveIndex = 0;
	psSwapChain->psPVRJTable = &psDevInfo->sPVRJTable;
	psSwapChain->psSwapChainLock = &psDevInfo->sSwapChainLock;


	for(i=0; i<ui32BufferCount-1; i++)
	{
		psBuffer[i].psNext = &psBuffer[i+1];
	}

	psBuffer[i].psNext = &psBuffer[0];


	for(i=0; i<ui32BufferCount; i++)
	{
		IMG_UINT32 ui32SwapBuffer = i + ui32BuffersToSkip;
		IMG_UINT32 ui32BufferOffset = ui32SwapBuffer * (IMG_UINT32)psDevInfo->sFBInfo.ulRoundedBufferSize;

		psBuffer[i].psSyncData = ppsSyncData[i];

		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
	}


	for(i=0; i<ui32BufferCount; i++)
	{
		psVSyncFlips[i].bValid = IONE_FALSE;
		psVSyncFlips[i].bFlipped = IONE_FALSE;
		psVSyncFlips[i].bCmdCompleted = IONE_FALSE;
	}

	IONELFBEnableDisplayRegisterAccess();


	//psSwapChain->pvRegs = ioremap(psDevInfo->psLINFBInfo->fix.mmio_start, psDevInfo->psLINFBInfo->fix.mmio_len);
	psSwapChain->pvRegs = psDevInfo->psLINFBInfo->screen_base;
	if (psSwapChain->pvRegs == NULL)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": Couldn't map registers needed for flipping\n");
		goto ErrorDisableDisplayRegisters;
	}

	if (IONELFBInstallVSyncISR(psSwapChain) != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": ISR handler failed to register\n");
		goto ErrorUnmapRegisters;
	}

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);


	psDevInfo->psSwapChain = psSwapChain;


	psSwapChain->bFlushCommands = psDevInfo->bFlushCommands;

	if (psSwapChain->bFlushCommands)
	{
		psSwapChain->ulSetFlushStateRefCount = 1;
	}
	else
	{
		psSwapChain->ulSetFlushStateRefCount = 0;
		IONELFBEnableVSyncInterrupt(psSwapChain);
	}

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);

	if (EnableLFBEventNotification(psDevInfo)!= IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": Couldn't enable framebuffer event notification\n");
		goto ErrorUninstallVSyncInterrupt;
	}


	*phSwapChain = (IMG_HANDLE)psSwapChain;

	return (PVRSRV_OK);

ErrorUninstallVSyncInterrupt:
	if(IONELFBUninstallVSyncISR(psSwapChain) != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": Couldn't uninstall VSync ISR\n");
	}
ErrorUnmapRegisters:
	//iounmap(psSwapChain->pvRegs);
ErrorDisableDisplayRegisters:
	IONELFBDisableDisplayRegisterAccess();
	IONELFBFreeKernelMem(psVSyncFlips);
ErrorFreeBuffers:
	IONELFBFreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	IONELFBFreeKernelMem(psSwapChain);

	return eError;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	unsigned long ulLockFlags;
	IONE_ERROR eError;


	if(!hDevice || !hSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	psSwapChain = (IONELFB_SWAPCHAIN*)hSwapChain;
	if (psSwapChain != psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	eError = DisableLFBEventNotification(psDevInfo);
	if (eError != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": Couldn't disable framebuffer event notification\n");
	}

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);

	IONELFBDisableVSyncInterrupt(psSwapChain);


	FlushInternalVSyncQueue(psSwapChain);


	IONELFBFlip(psSwapChain, (unsigned long)psDevInfo->sFBInfo.sSysAddr.uiAddr);


	psDevInfo->psSwapChain = NULL;

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);

	if(IONELFBUninstallVSyncISR(psSwapChain) != IONE_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": Couldn't uninstall VSync ISR\n");
		return (PVRSRV_ERROR_GENERIC);
	}


	//iounmap(psSwapChain->pvRegs);

	IONELFBDisableDisplayRegisterAccess();


	IONELFBFreeKernelMem(psSwapChain->psVSyncFlips);
	IONELFBFreeKernelMem(psSwapChain->psBuffer);
	IONELFBFreeKernelMem(psSwapChain);

	return (PVRSRV_OK);
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain,
	IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);



	return (PVRSRV_ERROR_NOT_SUPPORTED);
}

static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);



	return (PVRSRV_ERROR_NOT_SUPPORTED);
}

static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);



	return (PVRSRV_ERROR_NOT_SUPPORTED);
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);



	return (PVRSRV_ERROR_NOT_SUPPORTED);
}

static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	IONELFB_DEVINFO   *psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	unsigned long      i;


	if(!hDevice
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	psSwapChain = (IONELFB_SWAPCHAIN*)hSwapChain;
	if (psSwapChain != psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}


	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;


	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}

	return (PVRSRV_OK);
}

static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,
                                   IMG_HANDLE hBuffer,
                                   IMG_UINT32 ui32SwapInterval,
                                   IMG_HANDLE hPrivateTag,
                                   IMG_UINT32 ui32ClipRectCount,
                                   IMG_RECT *psClipRect)
{
	IONELFB_DEVINFO *psDevInfo;

	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(psClipRect);

	if(!hDevice
	|| !hBuffer
	|| (ui32ClipRectCount != 0))
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;


	return (PVRSRV_OK);
}

static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE hDevice,
                                   IMG_HANDLE hSwapChain)
{
	IONELFB_DEVINFO   *psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	unsigned long      ulLockFlags;

	if(!hDevice || !hSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	psSwapChain = (IONELFB_SWAPCHAIN*)hSwapChain;
	if (psSwapChain != psDevInfo->psSwapChain)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);


	FlushInternalVSyncQueue(psSwapChain);


	IONELFBFlip(psSwapChain, (unsigned long)psDevInfo->sFBInfo.sSysAddr.uiAddr);

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);

	return (PVRSRV_OK);
}

IONE_BOOL IONELFBVSyncIHandler(IONELFB_SWAPCHAIN *psSwapChain)
{
	IONE_BOOL bStatus = IONE_FALSE;
	IONELFB_VSYNC_FLIP_ITEM *psFlipItem;
	unsigned long ulMaxIndex;
	unsigned long ulLockFlags;

	psFlipItem = &psSwapChain->psVSyncFlips[psSwapChain->ulRemoveIndex];
	ulMaxIndex = psSwapChain->ulBufferCount - 1;

	spin_lock_irqsave(psSwapChain->psSwapChainLock, ulLockFlags);


	if (psSwapChain->bFlushCommands)
	{
		goto ExitUnlock;
	}

	while(psFlipItem->bValid)
	{

		if(psFlipItem->bFlipped)
		{

			if(!psFlipItem->bCmdCompleted)
			{

				psSwapChain->psPVRJTable->pfnPVRSRVCmdComplete((IMG_HANDLE)psFlipItem->hCmdComplete, IMG_TRUE);


				psFlipItem->bCmdCompleted = IONE_TRUE;
			}


			psFlipItem->ulSwapInterval--;


			if(psFlipItem->ulSwapInterval == 0)
			{

				psSwapChain->ulRemoveIndex++;

				if(psSwapChain->ulRemoveIndex > ulMaxIndex)
				{
					psSwapChain->ulRemoveIndex = 0;
				}


				psFlipItem->bCmdCompleted = IONE_FALSE;
				psFlipItem->bFlipped = IONE_FALSE;


				psFlipItem->bValid = IONE_FALSE;
			}
			else
			{

				break;
			}
		}
		else
		{

			IONELFBFlip(psSwapChain, (unsigned long)psFlipItem->sSysAddr);


			psFlipItem->bFlipped = IONE_TRUE;


			break;
		}


		psFlipItem = &psSwapChain->psVSyncFlips[psSwapChain->ulRemoveIndex];
	}

ExitUnlock:
	spin_unlock_irqrestore(psSwapChain->psSwapChainLock, ulLockFlags);

	return bStatus;
}

static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	IONELFB_DEVINFO *psDevInfo;
	IONELFB_BUFFER *psBuffer;
	IONELFB_SWAPCHAIN *psSwapChain;
#if defined(SYS_USING_INTERRUPTS)
	IONELFB_VSYNC_FLIP_ITEM* psFlipItem;
#endif
	unsigned long ulLockFlags;


	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}


	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL || sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
	{
		return IMG_FALSE;
	}


	psDevInfo = (IONELFB_DEVINFO*)psFlipCmd->hExtDevice;

	psBuffer = (IONELFB_BUFFER*)psFlipCmd->hExtBuffer;
	psSwapChain = (IONELFB_SWAPCHAIN*) psFlipCmd->hExtSwapChain;

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);


	if (psDevInfo->bDeviceSuspended)
	{
		psSwapChain->psPVRJTable->pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);
		goto ExitTrueUnlock;
	}

#if defined(SYS_USING_INTERRUPTS)

	if(psFlipCmd->ui32SwapInterval == 0 || psSwapChain->bFlushCommands == IONE_TRUE)
	{
#endif

		IONELFBFlip(psSwapChain, (unsigned long)psBuffer->sSysAddr.uiAddr);


		psSwapChain->psPVRJTable->pfnPVRSRVCmdComplete(hCmdCookie, IMG_TRUE);

#if defined(SYS_USING_INTERRUPTS)
		goto ExitTrueUnlock;
	}

	psFlipItem = &psSwapChain->psVSyncFlips[psSwapChain->ulInsertIndex];


	if(psFlipItem->bValid == IONE_FALSE)
	{
		unsigned long ulMaxIndex = psSwapChain->ulBufferCount - 1;

		if(psSwapChain->ulInsertIndex == psSwapChain->ulRemoveIndex)
		{

			IONELFBFlip(psSwapChain, (unsigned long)psBuffer->sSysAddr.uiAddr);

			psFlipItem->bFlipped = IONE_TRUE;
		}
		else
		{
			psFlipItem->bFlipped = IONE_FALSE;
		}

		psFlipItem->hCmdComplete = (IONE_HANDLE)hCmdCookie;
		psFlipItem->ulSwapInterval = (unsigned long)psFlipCmd->ui32SwapInterval;
		psFlipItem->sSysAddr = &psBuffer->sSysAddr;
		psFlipItem->bValid = IONE_TRUE;

		psSwapChain->ulInsertIndex++;
		if(psSwapChain->ulInsertIndex > ulMaxIndex)
		{
			psSwapChain->ulInsertIndex = 0;
		}

		goto ExitTrueUnlock;
	}

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
	return IMG_FALSE;
#endif

ExitTrueUnlock:
	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
	return IMG_TRUE;
}


static IONE_ERROR InitDev(IONELFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	IONELFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	IONE_ERROR eError = IONE_ERROR_GENERIC;
	unsigned long FBSize;

	acquire_console_sem();

	if (fb_idx < 0 || fb_idx >= num_registered_fb)
	{
		eError = IONE_ERROR_INVALID_DEVICE;
		goto errRelSem;
	}

	psLINFBInfo = registered_fb[fb_idx];

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		printk(KERN_INFO DRIVER_PREFIX
			": Couldn't get framebuffer module\n");

		goto errRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(psLINFBInfo, 0);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX
				": Couldn't open framebuffer: %d\n", res);

			goto errModPut;
		}
	}

	psDevInfo->psLINFBInfo = psLINFBInfo;

	FBSize = psLINFBInfo->fix.smem_len;
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer physical address: 0x%lx\n",
			psLINFBInfo->fix.smem_start));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer virtual address: 0x%lx\n",
			(unsigned long)psLINFBInfo->screen_base));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer size: %lu\n",
			FBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer virtual width: %u\n",
			psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer virtual height: %u\n",
			psLINFBInfo->var.yres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer width: %u\n",
			psLINFBInfo->var.xres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer height: %u\n",
			psLINFBInfo->var.yres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer stride: %u\n",
			psLINFBInfo->fix.line_length));


	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
	psPVRFBInfo->ulFBSize = FBSize;
	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;

	psPVRFBInfo->ulRoundedBufferSize = IONELFB_PAGE_ROUNDUP(psPVRFBInfo->ulBufferSize);

	if(psLINFBInfo->var.bits_per_pixel == 16)
	{
		if((psLINFBInfo->var.red.length == 5) &&
			(psLINFBInfo->var.green.length == 6) &&
			(psLINFBInfo->var.blue.length == 5) &&
			(psLINFBInfo->var.red.offset == 11) &&
			(psLINFBInfo->var.green.offset == 5) &&
			(psLINFBInfo->var.blue.offset == 0) &&
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_RGB565;
		}
		else
		{
			printk("Unknown FB format\n");
		}
	}
	else if(psLINFBInfo->var.bits_per_pixel == 32)
	{
		if((psLINFBInfo->var.red.length == 8) &&
			(psLINFBInfo->var.green.length == 8) &&
			(psLINFBInfo->var.blue.length == 8) &&
			(psLINFBInfo->var.red.offset == 16) &&
			(psLINFBInfo->var.green.offset == 8) &&
			(psLINFBInfo->var.blue.offset == 0) &&
			(psLINFBInfo->var.red.msb_right == 0))
		{
			psPVRFBInfo->ePixelFormat = PVRSRV_PIXEL_FORMAT_ARGB8888;
		}
		else
		{
			printk("Unknown FB format\n");
		}
	}
	else
	{
		printk("Unknown FB format\n");
	}


	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = IONE_OK;
	goto errRelSem;

errModPut:
	module_put(psLINFBOwner);
errRelSem:
	release_console_sem();
	return eError;
}

static void DeInitDev(IONELFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	acquire_console_sem();

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL)
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	release_console_sem();
}

IONE_ERROR IONELFBInit(void)
{
	IONELFB_DEVINFO		*psDevInfo;

	psDevInfo = GetAnchorPtr();

	if (psDevInfo == NULL)
	{
		PFN_CMD_PROC	 		pfnCmdProcList[IONELFB_COMMAND_COUNT];
		IMG_UINT32				aui32SyncCountList[IONELFB_COMMAND_COUNT][2];

		psDevInfo = (IONELFB_DEVINFO *)IONELFBAllocKernelMem(sizeof(IONELFB_DEVINFO));

		if(!psDevInfo)
		{
			return (IONE_ERROR_OUT_OF_MEMORY);
		}


		memset(psDevInfo, 0, sizeof(IONELFB_DEVINFO));


		SetAnchorPtr((void*)psDevInfo);


		psDevInfo->ulRefCount = 0;


		if(InitDev(psDevInfo) != IONE_OK)
		{
			return (IONE_ERROR_INIT_FAILURE);
		}

		if(IONELFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &pfnGetPVRJTable) != IONE_OK)
		{
			return (IONE_ERROR_INIT_FAILURE);
		}


		if(!(*pfnGetPVRJTable)(&psDevInfo->sPVRJTable))
		{
			return (IONE_ERROR_INIT_FAILURE);
		}


		spin_lock_init(&psDevInfo->sSwapChainLock);

		psDevInfo->psSwapChain = 0;
		psDevInfo->bFlushCommands = IONE_FALSE;
		psDevInfo->bDeviceSuspended = IONE_FALSE;

		psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
		if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers == 0)
		{
			psDevInfo->sDisplayInfo.ui32MaxSwapChains = 0;
			psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 0;
		}
		else
		{
			psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
			psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 3;
		}
		psDevInfo->sDisplayInfo.ui32MinSwapInterval = 0;

		strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

		psDevInfo->sDisplayFormat.pixelformat = psDevInfo->sFBInfo.ePixelFormat;
		psDevInfo->sDisplayDim.ui32Width      = (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
		psDevInfo->sDisplayDim.ui32Height     = (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
		psDevInfo->sDisplayDim.ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;

		DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Maximum number of swap chain buffers: %lu\n",
			psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));


		psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
		psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
		psDevInfo->sSystemBuffer.ulBufferSize = psDevInfo->sFBInfo.ulRoundedBufferSize;



		psDevInfo->sDCJTable.ui32TableSize = sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
		psDevInfo->sDCJTable.pfnOpenDCDevice = OpenDCDevice;
		psDevInfo->sDCJTable.pfnCloseDCDevice = CloseDCDevice;
		psDevInfo->sDCJTable.pfnEnumDCFormats = EnumDCFormats;
		psDevInfo->sDCJTable.pfnEnumDCDims = EnumDCDims;
		psDevInfo->sDCJTable.pfnGetDCSystemBuffer = GetDCSystemBuffer;
		psDevInfo->sDCJTable.pfnGetDCInfo = GetDCInfo;
		psDevInfo->sDCJTable.pfnGetBufferAddr = GetDCBufferAddr;
		psDevInfo->sDCJTable.pfnCreateDCSwapChain = CreateDCSwapChain;
		psDevInfo->sDCJTable.pfnDestroyDCSwapChain = DestroyDCSwapChain;
		psDevInfo->sDCJTable.pfnSetDCDstRect = SetDCDstRect;
		psDevInfo->sDCJTable.pfnSetDCSrcRect = SetDCSrcRect;
		psDevInfo->sDCJTable.pfnSetDCDstColourKey = SetDCDstColourKey;
		psDevInfo->sDCJTable.pfnSetDCSrcColourKey = SetDCSrcColourKey;
		psDevInfo->sDCJTable.pfnGetDCBuffers = GetDCBuffers;
		psDevInfo->sDCJTable.pfnSwapToDCBuffer = SwapToDCBuffer;
		psDevInfo->sDCJTable.pfnSwapToDCSystem = SwapToDCSystem;
		psDevInfo->sDCJTable.pfnSetDCState = SetDCState;


		if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice (
			&psDevInfo->sDCJTable,
			&psDevInfo->ulDeviceID ) != PVRSRV_OK)
		{
			return (IONE_ERROR_DEVICE_REGISTER_FAILED);
		}


		pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;


		aui32SyncCountList[DC_FLIP_COMMAND][0] = 0;
		aui32SyncCountList[DC_FLIP_COMMAND][1] = 2;





		if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList (psDevInfo->ulDeviceID,
																&pfnCmdProcList[0],
																aui32SyncCountList,
																IONELFB_COMMAND_COUNT) != PVRSRV_OK)
		{
			printk(KERN_WARNING DRIVER_PREFIX ": Can't register callback\n");
			return (IONE_ERROR_CANT_REGISTER_CALLBACK);
		}

	}


	psDevInfo->ulRefCount++;


	return (IONE_OK);

	}

IONE_ERROR IONELFBDeinit(void)
{
	IONELFB_DEVINFO *psDevInfo, *psDevFirst;

	psDevFirst = GetAnchorPtr();
	psDevInfo = psDevFirst;


	if (psDevInfo == NULL)
	{
		return (IONE_ERROR_GENERIC);
	}


	psDevInfo->ulRefCount--;

	if (psDevInfo->ulRefCount == 0)
	{

		PVRSRV_DC_DISP2SRV_KMJTABLE	*psJTable = &psDevInfo->sPVRJTable;

		if (psDevInfo->sPVRJTable.pfnPVRSRVRemoveCmdProcList (psDevInfo->ulDeviceID, IONELFB_COMMAND_COUNT) != PVRSRV_OK)
		{
			return (IONE_ERROR_GENERIC);
		}


		if (psJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->ulDeviceID) != PVRSRV_OK)
		{
			return (IONE_ERROR_GENERIC);
		}

		DeInitDev(psDevInfo);


		IONELFBFreeKernelMem(psDevInfo);
	}


	SetAnchorPtr(NULL);


	return (IONE_OK);
}


#if defined(LDM_PLATFORM)
void IONELFBDriverSuspend(void)
{
	IONELFB_DEVINFO *psDevInfo = GetAnchorPtr();
	unsigned long    ulLockFlags;

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);

	if (psDevInfo->bDeviceSuspended)
	{
		goto ExitUnlock;
	}
	psDevInfo->bDeviceSuspended = IONE_TRUE;


	SetFlushStateInternalNoLock(psDevInfo, IONE_TRUE);

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);


	if (psDevInfo->psSwapChain != NULL)
	{
		IONELFBDisableDisplayRegisterAccess();
	}

	return;

ExitUnlock:
	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
}

void IONELFBDriverResume(void)
{
	IONELFB_DEVINFO *psDevInfo = GetAnchorPtr();
	unsigned long    ulLockFlags;

	if (psDevInfo->bDeviceSuspended == IONE_FALSE)
	{
		return;
	}

	if (psDevInfo->psSwapChain != NULL)
	{
		IONELFBEnableDisplayRegisterAccess();
	}

	spin_lock_irqsave(&psDevInfo->sSwapChainLock, ulLockFlags);


	SetFlushStateInternalNoLock(psDevInfo, IONE_FALSE);

	psDevInfo->bDeviceSuspended = IONE_FALSE;

	spin_unlock_irqrestore(&psDevInfo->sSwapChainLock, ulLockFlags);
}
#endif

