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
#include <plat/ambcache.h>
#include <plat/atag.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "fb2.h"
#include "stream_texture.h"

#define FB2_COMMAND_COUNT		1
#define	FB2_VSYNC_SETTLE_COUNT		5
#define	FB2_MAX_NUM_DEVICES		FB_MAX
#if (FB2_MAX_NUM_DEVICES > FB_MAX)
#error "FB2_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

#define RANGE_TO_PAGES(range)		(((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

static FB2_DEVINFO *gapsDevInfo[FB2_MAX_NUM_DEVICES];

static PFN_DC_GET_PVRJTABLE gpfnGetPVRJTable = NULL;

static inline unsigned long RoundUpToMultiple(unsigned long x, unsigned long y)
{
	unsigned long div = x / y;
	unsigned long rem = x % y;

	return (div + ((rem == 0) ? 0 : 1)) * y;
}

static unsigned long GCD(unsigned long x, unsigned long y)
{
	while (y != 0)
	{
		unsigned long r = x % y;
		x = y;
		y = r;
	}

	return x;
}

static unsigned long LCM(unsigned long x, unsigned long y)
{
	unsigned long gcd = GCD(x, y);

	return (gcd == 0) ? 0 : ((x / gcd) * y);
}

unsigned FB2MaxFBDevIDPlusOne(void)
{
	return FB2_MAX_NUM_DEVICES;
}

FB2_DEVINFO *FB2GetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= FB2MaxFBDevIDPlusOne());

	if (uiFBDevID >= FB2_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

static inline void FB2SetDevInfoPtr(unsigned uiFBDevID, FB2_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= FB2_MAX_NUM_DEVICES);

	if (uiFBDevID < FB2_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline FB2_BOOL SwapChainHasChanged(FB2_DEVINFO *psDevInfo, FB2_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

static inline FB2_BOOL DontWaitForVSync(FB2_DEVINFO *psDevInfo)
{
	FB2_BOOL bDontWait;

	bDontWait = FB2AtomicBoolRead(&psDevInfo->sBlanked) ||
			FB2AtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || FB2AtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif

	return bDontWait;
}

static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	FB2_DEVINFO *psDevInfo = (FB2_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			FB2AtomicBoolSet(&psDevInfo->sFlushCommands, FB2_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			FB2AtomicBoolSet(&psDevInfo->sFlushCommands, FB2_FALSE);
			break;
		default:
			break;
	}
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	FB2_DEVINFO *psDevInfo;
	FB2_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne = 3;
	unsigned i;

	for (i = 2; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = FB2GetDevInfoPtr(i);
		if (psDevInfo != NULL && psDevInfo->uiPVRDevID == uiPVRDevID)
		{
			break;
		}
	}
	if (i == uiMaxFBDevIDPlusOne)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: PVR Device %u not found\n", __FUNCTION__, uiPVRDevID));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}


	psDevInfo->sSystemBuffer.psSyncData = psSystemBufferSyncData;

	eError = FB2UnblankDisplay(psDevInfo);
	if (eError != FB2_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: FB2UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
	}


	*phDevice = (IMG_HANDLE)psDevInfo;

	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(hDevice);

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	FB2_DEVINFO	*psDevInfo;

	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;

	*pui32NumFormats = 1;

	if(psFormat)
	{
		psFormat[0] = psDevInfo->sDisplayFormat;
	}

	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCDims(IMG_HANDLE hDevice,
                               DISPLAY_FORMAT *psFormat,
                               IMG_UINT32 *pui32NumDims,
                               DISPLAY_DIMS *psDim)
{
	FB2_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;

	*pui32NumDims = 1;


	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}

	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	FB2_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	FB2_DEVINFO	*psDevInfo;

	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;

	*psDCInfo = psDevInfo->sDisplayInfo;

	return PVRSRV_OK;
}

static PVRSRV_ERROR GetDCBufferAddr(IMG_HANDLE        hDevice,
                                    IMG_HANDLE        hBuffer,
                                    IMG_SYS_PHYADDR   **ppsSysAddr,
                                    IMG_UINT32        *pui32ByteSize,
                                    IMG_VOID          **ppvCpuVAddr,
                                    IMG_HANDLE        *phOSMapInfo,
                                    IMG_BOOL          *pbIsContiguous,
	                                IMG_UINT32		  *pui32TilingStride)
{
	FB2_DEVINFO	*psDevInfo;
	FB2_BUFFER *psSystemBuffer;

	UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if(!hBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!ppsSysAddr)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	if (!pui32ByteSize)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;

	psSystemBuffer = (FB2_BUFFER *)hBuffer;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	*ppsSysAddr	= psSystemBuffer->psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	*ppsSysAddr = &psSystemBuffer->sSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	*ppsSysAddr	= psSystemBuffer->psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	*ppsSysAddr	= psSystemBuffer->psSysAddr;
#endif


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
#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
		*pbIsContiguous = IMG_FALSE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
		*pbIsContiguous = IMG_TRUE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
		*pbIsContiguous = IMG_FALSE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
		*pbIsContiguous = IMG_FALSE;
#endif
	}

	return PVRSRV_OK;
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
	FB2_DEVINFO	*psDevInfo;
	FB2_SWAPCHAIN *psSwapChain;
	FB2_BUFFER *psBuffer;
	IMG_UINT32 i;
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32BuffersToSkip;

	UNREFERENCED_PARAMETER(ui32OEMFlags);


	if(!hDevice
	|| !psDstSurfAttrib
	|| !psSrcSurfAttrib
	|| !ppsSyncData
	|| !phSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;


	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	FB2CreateSwapChainLock(psDevInfo);


	if(psDevInfo->psSwapChain != NULL)
	{
		eError = PVRSRV_ERROR_FLIP_CHAIN_EXISTS;
		goto ExitUnLock;
	}


	if(ui32BufferCount > psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}

	if ((psDevInfo->sFBInfo.ulRoundedBufferSize * (unsigned long)ui32BufferCount) > psDevInfo->sFBInfo.ulFBSize)
	{
		eError = PVRSRV_ERROR_TOOMANYBUFFERS;
		goto ExitUnLock;
	}


	ui32BuffersToSkip = psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers - ui32BufferCount;


	if(psDstSurfAttrib->pixelformat != psDevInfo->sDisplayFormat.pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psDevInfo->sDisplayDim.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psDevInfo->sDisplayDim.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psDevInfo->sDisplayDim.ui32Height)
	{

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	if(psDstSurfAttrib->pixelformat != psSrcSurfAttrib->pixelformat
	|| psDstSurfAttrib->sDims.ui32ByteStride != psSrcSurfAttrib->sDims.ui32ByteStride
	|| psDstSurfAttrib->sDims.ui32Width != psSrcSurfAttrib->sDims.ui32Width
	|| psDstSurfAttrib->sDims.ui32Height != psSrcSurfAttrib->sDims.ui32Height)
	{

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}


	UNREFERENCED_PARAMETER(ui32Flags);

#if defined(PVR_FB2_UPDATE_MODE)
	if (!FB2SetUpdateMode(psDevInfo, PVR_FB2_UPDATE_MODE))
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set frame buffer update mode %d\n", __FUNCTION__, psDevInfo->uiFBDevID, PVR_FB2_UPDATE_MODE);
	}
#endif

	psSwapChain = (FB2_SWAPCHAIN*)FB2AllocKernelMem(sizeof(FB2_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	psBuffer = (FB2_BUFFER*)FB2AllocKernelMem(sizeof(FB2_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->bNotVSynced = FB2_TRUE;
	psSwapChain->uiFBDevID = psDevInfo->uiFBDevID;


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

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
		psBuffer[i].psSysAddr = psDevInfo->sFBInfo.psSysAddr + ui32BufferOffset / PAGE_SIZE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
		psBuffer[i].psSysAddr = psDevInfo->sFBInfo.psSysAddr + ui32BufferOffset / PAGE_SIZE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
		psBuffer[i].psSysAddr = psDevInfo->sFBInfo.psSysAddr + ui32BufferOffset / PAGE_SIZE;
#endif
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
		psBuffer[i].psDevInfo = psDevInfo;

		FB2InitBufferForSwap(&psBuffer[i]);
	}

	if (FB2CreateSwapQueue(psSwapChain) != FB2_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (FB2EnableLFBEventNotification(psDevInfo)!= FB2_OK)
	{
		eError = PVRSRV_ERROR_UNABLE_TO_ENABLE_EVENT;
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't enable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
		goto ErrorDestroySwapQueue;
	}

	psDevInfo->uiSwapChainID++;
	if (psDevInfo->uiSwapChainID == 0)
	{
		psDevInfo->uiSwapChainID++;
	}

	psSwapChain->uiSwapChainID = psDevInfo->uiSwapChainID;

	psDevInfo->psSwapChain = psSwapChain;

	*pui32SwapChainID = psDevInfo->uiSwapChainID;

	*phSwapChain = (IMG_HANDLE)psSwapChain;

	eError = PVRSRV_OK;
	goto ExitUnLock;

ErrorDestroySwapQueue:
	FB2DestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	FB2FreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	FB2FreeKernelMem(psSwapChain);
ExitUnLock:
	FB2CreateSwapChainUnLock(psDevInfo);
	return eError;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	FB2_DEVINFO	*psDevInfo;
	FB2_SWAPCHAIN *psSwapChain;
	FB2_ERROR eError;


	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;
	psSwapChain = (FB2_SWAPCHAIN*)hSwapChain;

	FB2CreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}


	FB2DestroySwapQueue(psSwapChain);

	eError = FB2DisableLFBEventNotification(psDevInfo);
	if (eError != FB2_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}


	FB2FreeKernelMem(psSwapChain->psBuffer);
	FB2FreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	FB2Flip(psDevInfo, &psDevInfo->sSystemBuffer);
	(void) FB2CheckModeAndSync(psDevInfo);

	eError = PVRSRV_OK;

ExitUnLock:
	FB2CreateSwapChainUnLock(psDevInfo);

	return eError;
}

static PVRSRV_ERROR SetDCDstRect(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain,
	IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);



	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcRect(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_RECT *psRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(psRect);



	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCDstColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);



	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR SetDCSrcColourKey(IMG_HANDLE hDevice,
                                      IMG_HANDLE hSwapChain,
                                      IMG_UINT32 ui32CKColour)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);
	UNREFERENCED_PARAMETER(ui32CKColour);



	return PVRSRV_ERROR_NOT_SUPPORTED;
}

static PVRSRV_ERROR GetDCBuffers(IMG_HANDLE hDevice,
                                 IMG_HANDLE hSwapChain,
                                 IMG_UINT32 *pui32BufferCount,
                                 IMG_HANDLE *phBuffer)
{
	FB2_DEVINFO   *psDevInfo;
	FB2_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;


	if(!hDevice
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (FB2_DEVINFO*)hDevice;
	psSwapChain = (FB2_SWAPCHAIN*)hSwapChain;

	FB2CreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto Exit;
	}


	*pui32BufferCount = (IMG_UINT32)psSwapChain->ulBufferCount;


	for(i=0; i<psSwapChain->ulBufferCount; i++)
	{
		phBuffer[i] = (IMG_HANDLE)&psSwapChain->psBuffer[i];
	}

	eError = PVRSRV_OK;

Exit:
	FB2CreateSwapChainUnLock(psDevInfo);

	return eError;
}

static PVRSRV_ERROR SwapToDCBuffer(IMG_HANDLE hDevice,
                                   IMG_HANDLE hBuffer,
                                   IMG_UINT32 ui32SwapInterval,
                                   IMG_HANDLE hPrivateTag,
                                   IMG_UINT32 ui32ClipRectCount,
                                   IMG_RECT *psClipRect)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hBuffer);
	UNREFERENCED_PARAMETER(ui32SwapInterval);
	UNREFERENCED_PARAMETER(hPrivateTag);
	UNREFERENCED_PARAMETER(ui32ClipRectCount);
	UNREFERENCED_PARAMETER(psClipRect);



	return PVRSRV_OK;
}

static PVRSRV_ERROR SwapToDCSystem(IMG_HANDLE hDevice,
                                   IMG_HANDLE hSwapChain)
{
	UNREFERENCED_PARAMETER(hDevice);
	UNREFERENCED_PARAMETER(hSwapChain);


	return PVRSRV_OK;
}

static FB2_BOOL WaitForVSyncSettle(FB2_DEVINFO *psDevInfo)
{
		unsigned i;
		for(i = 0; i < FB2_VSYNC_SETTLE_COUNT; i++)
		{
			if (DontWaitForVSync(psDevInfo) || !FB2WaitForVSync(psDevInfo))
			{
				return FB2_FALSE;
			}
		}

		return FB2_TRUE;
}

void FB2SwapHandler(FB2_BUFFER *psBuffer)
{
	FB2_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	FB2_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	FB2_BOOL bPreviouslyNotVSynced;

	FB2Flip(psDevInfo, psBuffer);

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = FB2_TRUE;


	if (!DontWaitForVSync(psDevInfo))
	{
		FB2_UPDATE_MODE eMode = FB2GetUpdateMode(psDevInfo);
		int iBlankEvents = FB2AtomicIntRead(&psDevInfo->sBlankEvents);

		switch(eMode)
		{
			case FB2_UPDATE_MODE_AUTO:
				psSwapChain->bNotVSynced = FB2_FALSE;

				if (bPreviouslyNotVSynced || psSwapChain->iBlankEvents != iBlankEvents)
				{
					psSwapChain->iBlankEvents = iBlankEvents;
					psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
				} else if (psBuffer->ulSwapInterval != 0)
				{
					psSwapChain->bNotVSynced = !FB2WaitForVSync(psDevInfo);
				}
				break;
#if defined(PVR_FB2_MANUAL_UPDATE_SYNC_IN_SWAP)
			case FB2_UPDATE_MODE_MANUAL:
				if (psBuffer->ulSwapInterval != 0)
				{
					(void) FB2ManualSync(psDevInfo);
				}
				break;
#endif
			default:
				break;
		}
	}

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	FB2_DEVINFO *psDevInfo;
	FB2_BUFFER *psBuffer;
	FB2_SWAPCHAIN *psSwapChain;


	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}


	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL || sizeof(DISPLAYCLASS_FLIP_COMMAND) != ui32DataSize)
	{
		return IMG_FALSE;
	}


	psDevInfo = (FB2_DEVINFO*)psFlipCmd->hExtDevice;
	psBuffer = (FB2_BUFFER*)psFlipCmd->hExtBuffer;
	psSwapChain = (FB2_SWAPCHAIN*) psFlipCmd->hExtSwapChain;

	FB2CreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{

		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (FB2_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = (unsigned long)psFlipCmd->ui32SwapInterval;

		FB2QueueBufferForSwap(psSwapChain, psBuffer);
	}

	FB2CreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}


static FB2_ERROR FB2InitFBDev(FB2_DEVINFO *psDevInfo, STREAM_TEXTURE_FB2INFO *fb2_info)
{
	FB2_FBINFO		*psPVRFBInfo = &psDevInfo->sFBInfo;
	FB2_ERROR		eError = FB2_ERROR_GENERIC;
	unsigned long		FBStride, FBSize;
	unsigned long		ulLCM;
	IMG_CPU_VIRTADDR	LinAddr;
#if defined(CONFIG_SGX_STREAMING_BUFFER_KMALLOC)
	IMG_SYS_PHYADDR		PhysAddr;
#else
	IMG_SYS_PHYADDR		*PhysAddr;
#endif

	if (fb2_info->format == PVRSRV_PIXEL_FORMAT_RGB565) {
		FBStride = 2 * fb2_info->width;
	} else {
		FBStride = 4 * fb2_info->width;
	}
	FBSize	= fb2_info->size;
	ulLCM	= LCM(FBStride, FB2_PAGE_SIZE);

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
{
	unsigned long		ulPages;
	unsigned long		ulPage, ulBuffer, ulOffset;

	ulPages = RANGE_TO_PAGES(fb2_info->size);
	PhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!PhysAddr)
	{
		printk("Error: %s %d\n", __func__, __LINE__);
		return FB2_FALSE;
	}

	ulPage = 0;
	for (ulBuffer = 0; ulBuffer < fb2_info->n_buffer; ulBuffer++) {
		for (ulOffset = 0; ulOffset < fb2_info->size / fb2_info->n_buffer; ulOffset += PAGE_SIZE) {
			PhysAddr[ulPage++].uiAddr = (unsigned long)fb2_info->buffer[ulBuffer] + ulOffset;
		}
	}

	LinAddr	= NULL;
}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	eError = BCAllocContigMemory(FBSize, NULL, &LinAddr, &PhysAddr);
	if (eError) {
		printk("Error: %s %d\n", __func__, __LINE__);
		return FB2_FALSE;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	eError = BCAllocDiscontigMemory(FBSize, NULL, &LinAddr, &PhysAddr);
	if (eError) {
		printk("Error: %s %d\n", __func__, __LINE__);
		return FB2_FALSE;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	eError = BCAllocATTMemory(FBSize, NULL, &LinAddr, &PhysAddr);
	if (eError) {
		printk("Error: %s %d\n", __func__, __LINE__);
		return FB2_FALSE;
	}
#endif

#if defined(CONFIG_SGX_STREAMING_BUFFER_KMALLOC)
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 physical address: 0x%lx\n", PhysAddr.uiAddr));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 virtual address: 0x%lx\n", LinAddr));
#endif
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 size: %lu\n", FBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 virtual width: %lu\n", fb2_info->width));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 virtual height: %lu\n", fb2_info->height * 5));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 width: %lu\n", fb2_info->width));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 height: %lu\n", fb2_info->height));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Framebuffer2 stride: %lu\n", FBStride));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": LCM of stride and page size: %lu\n", ulLCM));


	psPVRFBInfo->ulWidth			= fb2_info->width;
	psPVRFBInfo->ulHeight			= fb2_info->height;
	psPVRFBInfo->ulByteStride		= FBStride;
	psPVRFBInfo->ulFBSize			= FBSize;
	psPVRFBInfo->ulBufferSize		= psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;
	psPVRFBInfo->ulRoundedBufferSize	= RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);
	psPVRFBInfo->ePixelFormat		= fb2_info->format;
	psDevInfo->sFBInfo.ulPhysicalWidthmm	= 90;
	psDevInfo->sFBInfo.ulPhysicalHeightmm	= 54;

#if defined(CONFIG_SGX_STREAMING_BUFFER_KMALLOC)
	psPVRFBInfo->sSysAddr.uiAddr		= PhysAddr.uiAddr;
	psPVRFBInfo->sCPUVAddr			= LinAddr;
	psDevInfo->sFBInfo.sSysAddr.uiAddr	= psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr		= psPVRFBInfo->sCPUVAddr;
#else
	psPVRFBInfo->psSysAddr			= PhysAddr;
	psPVRFBInfo->sCPUVAddr			= LinAddr;
	psDevInfo->sFBInfo.psSysAddr		= psPVRFBInfo->psSysAddr;
	psDevInfo->sFBInfo.sCPUVAddr		= psPVRFBInfo->sCPUVAddr;
#endif

	eError = FB2_OK;
	goto ErrorRelSem;

ErrorRelSem:
	return eError;
}

static void FB2DeInitFBDev(FB2_DEVINFO *psDevInfo)
{
#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	kfree(psDevInfo->sFBInfo.psSysAddr);
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	BCFreeContigMemory(psDevInfo->sFBInfo.ulFBSize, NULL, &psDevInfo->sFBInfo.sCPUVAddr, psDevInfo->sFBInfo.sSysAddr);
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	BCFreeDiscontigMemory(psDevInfo->sFBInfo.ulFBSize, NULL, &psDevInfo->sFBInfo.sCPUVAddr, psDevInfo->sFBInfo.psSysAddr);
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	BCFreeATTMemory(psDevInfo->sFBInfo.ulFBSize, NULL, &psDevInfo->sFBInfo.sCPUVAddr, psDevInfo->sFBInfo.psSysAddr);
#endif
}

static FB2_DEVINFO *FB2InitDev(STREAM_TEXTURE_FB2INFO *fb2_info)
{
	PFN_CMD_PROC	 	pfnCmdProcList[FB2_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[FB2_COMMAND_COUNT][2];
	FB2_DEVINFO		*psDevInfo = NULL;

	psDevInfo = (FB2_DEVINFO *)FB2AllocKernelMem(sizeof(FB2_DEVINFO));
	if(psDevInfo == NULL) {
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Couldn't allocate device information structure\n", __FUNCTION__);
		goto ErrorExit;
	}
	memset(psDevInfo, 0, sizeof(FB2_DEVINFO));
	psDevInfo->uiFBDevID = 2;

	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable)) {
		goto ErrorFreeDevInfo;
	}

	if(FB2InitFBDev(psDevInfo, fb2_info) != FB2_OK)
	{
		goto ErrorFreeDevInfo;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers != 0) {
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
	}

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm	= psDevInfo->sFBInfo.ulPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm	= psDevInfo->sFBInfo.ulPhysicalHeightmm;
	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);
	psDevInfo->sDisplayFormat.pixelformat		= psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->sDisplayDim.ui32Width		= (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
	psDevInfo->sDisplayDim.ui32Height		= (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
	psDevInfo->sDisplayDim.ui32ByteStride		= (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: Maximum number of swap chain buffers: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	psDevInfo->sSystemBuffer.psSysAddr		= psDevInfo->sFBInfo.psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	psDevInfo->sSystemBuffer.sSysAddr		= psDevInfo->sFBInfo.sSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	psDevInfo->sSystemBuffer.psSysAddr		= psDevInfo->sFBInfo.psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	psDevInfo->sSystemBuffer.psSysAddr		= psDevInfo->sFBInfo.psSysAddr;
#endif
	psDevInfo->sSystemBuffer.sCPUVAddr		= psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo		= psDevInfo;
	FB2InitBufferForSwap(&psDevInfo->sSystemBuffer);

	psDevInfo->sDCJTable.ui32TableSize		= sizeof(PVRSRV_DC_SRV2DISP_KMJTABLE);
	psDevInfo->sDCJTable.pfnOpenDCDevice		= OpenDCDevice;
	psDevInfo->sDCJTable.pfnCloseDCDevice		= CloseDCDevice;
	psDevInfo->sDCJTable.pfnEnumDCFormats		= EnumDCFormats;
	psDevInfo->sDCJTable.pfnEnumDCDims		= EnumDCDims;
	psDevInfo->sDCJTable.pfnGetDCSystemBuffer	= GetDCSystemBuffer;
	psDevInfo->sDCJTable.pfnGetDCInfo		= GetDCInfo;
	psDevInfo->sDCJTable.pfnGetBufferAddr		= GetDCBufferAddr;
	psDevInfo->sDCJTable.pfnCreateDCSwapChain	= CreateDCSwapChain;
	psDevInfo->sDCJTable.pfnDestroyDCSwapChain	= DestroyDCSwapChain;
	psDevInfo->sDCJTable.pfnSetDCDstRect		= SetDCDstRect;
	psDevInfo->sDCJTable.pfnSetDCSrcRect		= SetDCSrcRect;
	psDevInfo->sDCJTable.pfnSetDCDstColourKey	= SetDCDstColourKey;
	psDevInfo->sDCJTable.pfnSetDCSrcColourKey	= SetDCSrcColourKey;
	psDevInfo->sDCJTable.pfnGetDCBuffers		= GetDCBuffers;
	psDevInfo->sDCJTable.pfnSwapToDCBuffer		= SwapToDCBuffer;
	psDevInfo->sDCJTable.pfnSwapToDCSystem		= SwapToDCSystem;
	psDevInfo->sDCJTable.pfnSetDCState		= SetDCState;
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
		&psDevInfo->sDCJTable, &psDevInfo->uiPVRDevID) != PVRSRV_OK) {
		printk(KERN_ERR DRIVER_PREFIX
			"%s: PVR Services device registration failed\n", __FUNCTION__);
		goto ErrorDeInitFBDev;
	}
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		"%s: Device: PVR Device ID: %u\n",
		__FUNCTION__, psDevInfo->uiPVRDevID));


	pfnCmdProcList[DC_FLIP_COMMAND]			= ProcessFlip;
	aui32SyncCountList[DC_FLIP_COMMAND][0]		= 0;
	aui32SyncCountList[DC_FLIP_COMMAND][1]		= 2;
	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
		&pfnCmdProcList[0], aui32SyncCountList,	FB2_COMMAND_COUNT) != PVRSRV_OK) {
		printk(KERN_ERR DRIVER_PREFIX
			"%s: Couldn't register command processing functions with PVR Services\n", __FUNCTION__);
		goto ErrorUnregisterDevice;
	}

	FB2CreateSwapChainLockInit(psDevInfo);

	FB2AtomicBoolInit(&psDevInfo->sBlanked, FB2_FALSE);
	FB2AtomicIntInit(&psDevInfo->sBlankEvents, 0);
	FB2AtomicBoolInit(&psDevInfo->sFlushCommands, FB2_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	FB2AtomicBoolInit(&psDevInfo->sEarlySuspendFlag, FB2_FALSE);
#endif

	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	FB2DeInitFBDev(psDevInfo);
ErrorFreeDevInfo:
	FB2FreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

FB2_ERROR FB2Init(STREAM_TEXTURE_FB2INFO *fb2_info)
{
	FB2_DEVINFO *psDevInfo;

	if(FB2GetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != FB2_OK)
	{
		return FB2_ERROR_INIT_FAILURE;
	}

	psDevInfo = FB2InitDev(fb2_info);
	if (psDevInfo != NULL) {
		FB2SetDevInfoPtr(2, psDevInfo);
		return FB2_OK;
	} else {
		return FB2_ERROR_INIT_FAILURE;
	}
}

static FB2_BOOL FB2DeInitDev(FB2_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	FB2CreateSwapChainLockDeInit(psDevInfo);
	FB2AtomicBoolDeInit(&psDevInfo->sBlanked);
	FB2AtomicIntDeInit(&psDevInfo->sBlankEvents);
	FB2AtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	FB2AtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif

	psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, FB2_COMMAND_COUNT) != PVRSRV_OK) {
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return FB2_FALSE;
	}

	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK) {
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return FB2_FALSE;
	}

	FB2DeInitFBDev(psDevInfo);
	FB2SetDevInfoPtr(psDevInfo->uiFBDevID, NULL);
	FB2FreeKernelMem(psDevInfo);

	return FB2_TRUE;
}

FB2_ERROR FB2DeInit(void)
{
	FB2_BOOL bError = FB2_FALSE;
	FB2_DEVINFO *psDevInfo;

	psDevInfo = FB2GetDevInfoPtr(2);
	if (psDevInfo != NULL) {
		bError |= !FB2DeInitDev(psDevInfo);
	}

	return (bError) ? FB2_ERROR_INIT_FAILURE : FB2_OK;
}

