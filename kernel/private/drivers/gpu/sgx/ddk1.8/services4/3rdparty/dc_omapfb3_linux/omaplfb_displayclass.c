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
#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "ionelfb.h"

#if defined(CONFIG_DSSCOMP)

#if !defined(CONFIG_ION_IONE)
#error CONFIG_DSSCOMP support requires CONFIG_ION_IONE
#endif

#include <linux/ion.h>
#include <linux/ione_ion.h>

extern struct ion_client *gpsIONClient;

#include <mach/tiler.h>
#include <video/dsscomp.h>
#include <plat/dsscomp.h>

#endif 

#define IONELFB_COMMAND_COUNT		1

#define	IONELFB_VSYNC_SETTLE_COUNT	5

#define	IONELFB_MAX_NUM_DEVICES		FB_MAX
#if (IONELFB_MAX_NUM_DEVICES > FB_MAX)
#error "IONELFB_MAX_NUM_DEVICES must not be greater than FB_MAX"
#endif

static IONELFB_DEVINFO *gapsDevInfo[IONELFB_MAX_NUM_DEVICES];

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

unsigned IONELFBMaxFBDevIDPlusOne(void)
{
	return IONELFB_MAX_NUM_DEVICES;
}

IONELFB_DEVINFO *IONELFBGetDevInfoPtr(unsigned uiFBDevID)
{
	WARN_ON(uiFBDevID >= IONELFBMaxFBDevIDPlusOne());

	if (uiFBDevID >= IONELFB_MAX_NUM_DEVICES)
	{
		return NULL;
	}

	return gapsDevInfo[uiFBDevID];
}

static inline void IONELFBSetDevInfoPtr(unsigned uiFBDevID, IONELFB_DEVINFO *psDevInfo)
{
	WARN_ON(uiFBDevID >= IONELFB_MAX_NUM_DEVICES);

	if (uiFBDevID < IONELFB_MAX_NUM_DEVICES)
	{
		gapsDevInfo[uiFBDevID] = psDevInfo;
	}
}

static inline IONELFB_BOOL SwapChainHasChanged(IONELFB_DEVINFO *psDevInfo, IONELFB_SWAPCHAIN *psSwapChain)
{
	return (psDevInfo->psSwapChain != psSwapChain) ||
		(psDevInfo->uiSwapChainID != psSwapChain->uiSwapChainID);
}

static inline IONELFB_BOOL DontWaitForVSync(IONELFB_DEVINFO *psDevInfo)
{
	IONELFB_BOOL bDontWait;

	bDontWait = IONELFBAtomicBoolRead(&psDevInfo->sBlanked) ||
			IONELFBAtomicBoolRead(&psDevInfo->sFlushCommands);

#if defined(CONFIG_HAS_EARLYSUSPEND)
	bDontWait = bDontWait || IONELFBAtomicBoolRead(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	bDontWait = bDontWait || IONELFBAtomicBoolRead(&psDevInfo->sLeaveVT);
#endif
	return bDontWait;
}

static IMG_VOID SetDCState(IMG_HANDLE hDevice, IMG_UINT32 ui32State)
{
	IONELFB_DEVINFO *psDevInfo = (IONELFB_DEVINFO *)hDevice;

	switch (ui32State)
	{
		case DC_STATE_FLUSH_COMMANDS:
			IONELFBAtomicBoolSet(&psDevInfo->sFlushCommands, IONELFB_TRUE);
			break;
		case DC_STATE_NO_FLUSH_COMMANDS:
			IONELFBAtomicBoolSet(&psDevInfo->sFlushCommands, IONELFB_FALSE);
			break;
		default:
			break;
	}
}

static PVRSRV_ERROR OpenDCDevice(IMG_UINT32 uiPVRDevID,
                                 IMG_HANDLE *phDevice,
                                 PVRSRV_SYNC_DATA* psSystemBufferSyncData)
{
	IONELFB_DEVINFO *psDevInfo;
	IONELFB_ERROR eError;
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;

	for (i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		psDevInfo = IONELFBGetDevInfoPtr(i);
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
	
	eError = IONELFBUnblankDisplay(psDevInfo);
	if (eError != IONELFB_OK)
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: IONELFBUnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError));
		return PVRSRV_ERROR_UNBLANK_DISPLAY_FAILED;
	}

	
	*phDevice = (IMG_HANDLE)psDevInfo;
	
	return PVRSRV_OK;
}

static PVRSRV_ERROR CloseDCDevice(IMG_HANDLE hDevice)
{
#if defined(SUPPORT_DRI_DRM)
	IONELFB_DEVINFO *psDevInfo = (IONELFB_DEVINFO *)hDevice;

	IONELFBAtomicBoolSet(&psDevInfo->sLeaveVT, IONELFB_FALSE);
	(void) IONELFBUnblankDisplay(psDevInfo);
#else
	UNREFERENCED_PARAMETER(hDevice);
#endif
	return PVRSRV_OK;
}

static PVRSRV_ERROR EnumDCFormats(IMG_HANDLE hDevice,
                                  IMG_UINT32 *pui32NumFormats,
                                  DISPLAY_FORMAT *psFormat)
{
	IONELFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !pui32NumFormats)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	
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
	IONELFB_DEVINFO	*psDevInfo;

	if(!hDevice || !psFormat || !pui32NumDims)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*pui32NumDims = 1;

	
	if(psDim)
	{
		psDim[0] = psDevInfo->sDisplayDim;
	}
	
	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCSystemBuffer(IMG_HANDLE hDevice, IMG_HANDLE *phBuffer)
{
	IONELFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	*phBuffer = (IMG_HANDLE)&psDevInfo->sSystemBuffer;

	return PVRSRV_OK;
}


static PVRSRV_ERROR GetDCInfo(IMG_HANDLE hDevice, DISPLAY_INFO *psDCInfo)
{
	IONELFB_DEVINFO	*psDevInfo;
	
	if(!hDevice || !psDCInfo)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

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
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_BUFFER *psSystemBuffer;

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

	psDevInfo = (IONELFB_DEVINFO*)hDevice;

	psSystemBuffer = (IONELFB_BUFFER *)hBuffer;

	*ppsSysAddr = &psSystemBuffer->sSysAddr;

	*pui32ByteSize = (IMG_UINT32)psDevInfo->sFBInfo.ulBufferSize;

	if (ppvCpuVAddr)
	{
#if defined(CONFIG_DSSCOMP)
		*ppvCpuVAddr = psDevInfo->sFBInfo.bIs2D ? NULL : psSystemBuffer->sCPUVAddr;
#else
		*ppvCpuVAddr = psSystemBuffer->sCPUVAddr;
#endif
	}

	if (phOSMapInfo)
	{
		*phOSMapInfo = (IMG_HANDLE)0;
	}

	if (pbIsContiguous)
	{
#if defined(CONFIG_DSSCOMP)
		*pbIsContiguous = !psDevInfo->sFBInfo.bIs2D;
#else
		*pbIsContiguous = IMG_TRUE;
#endif
	}

#if defined(CONFIG_DSSCOMP)
	if (psDevInfo->sFBInfo.bIs2D)
	{
		int i = (psSystemBuffer->sSysAddr.uiAddr - psDevInfo->sFBInfo.psPageList->uiAddr) >> PAGE_SHIFT;
		*ppsSysAddr = psDevInfo->sFBInfo.psPageList + psDevInfo->sFBInfo.ulHeight * i;
	}
#endif

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
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	IONELFB_BUFFER *psBuffer;
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

	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	
	
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChains == 0)
	{
		return PVRSRV_ERROR_NOT_SUPPORTED;
	}

	IONELFBCreateSwapChainLock(psDevInfo);

	
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
	
#if defined(PVR_IONEFB3_UPDATE_MODE)
	if (!IONELFBSetUpdateMode(psDevInfo, PVR_IONEFB3_UPDATE_MODE))
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't set frame buffer update mode %d\n", __FUNCTION__, psDevInfo->uiFBDevID, PVR_IONEFB3_UPDATE_MODE);
	}
#endif
	
	psSwapChain = (IONELFB_SWAPCHAIN*)IONELFBAllocKernelMem(sizeof(IONELFB_SWAPCHAIN));
	if(!psSwapChain)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ExitUnLock;
	}

	psBuffer = (IONELFB_BUFFER*)IONELFBAllocKernelMem(sizeof(IONELFB_BUFFER) * ui32BufferCount);
	if(!psBuffer)
	{
		eError = PVRSRV_ERROR_OUT_OF_MEMORY;
		goto ErrorFreeSwapChain;
	}

	psSwapChain->ulBufferCount = (unsigned long)ui32BufferCount;
	psSwapChain->psBuffer = psBuffer;
	psSwapChain->bNotVSynced = IONELFB_TRUE;
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

#if defined(CONFIG_DSSCOMP)
		if (psDevInfo->sFBInfo.bIs2D)
		{
			ui32BufferOffset = 0;
		}
#endif 

		psBuffer[i].psSyncData = ppsSyncData[i];

		psBuffer[i].sSysAddr.uiAddr = psDevInfo->sFBInfo.sSysAddr.uiAddr + ui32BufferOffset;
		psBuffer[i].sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr + ui32BufferOffset;
		psBuffer[i].ulYOffset = ui32BufferOffset / psDevInfo->sFBInfo.ulByteStride;
		psBuffer[i].psDevInfo = psDevInfo;

#if defined(CONFIG_DSSCOMP)
		if (psDevInfo->sFBInfo.bIs2D)
		{
			psBuffer[i].sSysAddr.uiAddr += ui32SwapBuffer *
				ALIGN((IMG_UINT32)psDevInfo->sFBInfo.ulWidth * psDevInfo->sFBInfo.uiBytesPerPixel, PAGE_SIZE);
		}
#endif 

		IONELFBInitBufferForSwap(&psBuffer[i]);
	}

	if (IONELFBCreateSwapQueue(psSwapChain) != IONELFB_OK)
	{ 
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Failed to create workqueue\n", __FUNCTION__, psDevInfo->uiFBDevID);
		eError = PVRSRV_ERROR_UNABLE_TO_INSTALL_ISR;
		goto ErrorFreeBuffers;
	}

	if (IONELFBEnableLFBEventNotification(psDevInfo)!= IONELFB_OK)
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
	IONELFBDestroySwapQueue(psSwapChain);
ErrorFreeBuffers:
	IONELFBFreeKernelMem(psBuffer);
ErrorFreeSwapChain:
	IONELFBFreeKernelMem(psSwapChain);
ExitUnLock:
	IONELFBCreateSwapChainUnLock(psDevInfo);
	return eError;
}

static PVRSRV_ERROR DestroyDCSwapChain(IMG_HANDLE hDevice,
	IMG_HANDLE hSwapChain)
{
	IONELFB_DEVINFO	*psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	IONELFB_ERROR eError;

	
	if(!hDevice || !hSwapChain)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	psSwapChain = (IONELFB_SWAPCHAIN*)hSwapChain;

	IONELFBCreateSwapChainLock(psDevInfo);

	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: Swap chain mismatch\n", __FUNCTION__, psDevInfo->uiFBDevID);

		eError = PVRSRV_ERROR_INVALID_PARAMS;
		goto ExitUnLock;
	}

	
	IONELFBDestroySwapQueue(psSwapChain);

	eError = IONELFBDisableLFBEventNotification(psDevInfo);
	if (eError != IONELFB_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Couldn't disable framebuffer event notification\n", __FUNCTION__, psDevInfo->uiFBDevID);
	}

	
	IONELFBFreeKernelMem(psSwapChain->psBuffer);
	IONELFBFreeKernelMem(psSwapChain);

	psDevInfo->psSwapChain = NULL;

	IONELFBFlip(psDevInfo, &psDevInfo->sSystemBuffer);
	(void) IONELFBCheckModeAndSync(psDevInfo);

	eError = PVRSRV_OK;

ExitUnLock:
	IONELFBCreateSwapChainUnLock(psDevInfo);

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
	IONELFB_DEVINFO   *psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain;
	PVRSRV_ERROR eError;
	unsigned i;
	
	
	if(!hDevice 
	|| !hSwapChain
	|| !pui32BufferCount
	|| !phBuffer)
	{
		return PVRSRV_ERROR_INVALID_PARAMS;
	}
	
	psDevInfo = (IONELFB_DEVINFO*)hDevice;
	psSwapChain = (IONELFB_SWAPCHAIN*)hSwapChain;

	IONELFBCreateSwapChainLock(psDevInfo);

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
	IONELFBCreateSwapChainUnLock(psDevInfo);

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

static IONELFB_BOOL WaitForVSyncSettle(IONELFB_DEVINFO *psDevInfo)
{
		unsigned i;
		for(i = 0; i < IONELFB_VSYNC_SETTLE_COUNT; i++)
		{
			if (DontWaitForVSync(psDevInfo) || !IONELFBWaitForVSync(psDevInfo))
			{
				return IONELFB_FALSE;
			}
		}

		return IONELFB_TRUE;
}

void IONELFBSwapHandler(IONELFB_BUFFER *psBuffer)
{
	IONELFB_DEVINFO *psDevInfo = psBuffer->psDevInfo;
	IONELFB_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	IONELFB_BOOL bPreviouslyNotVSynced;

#if defined(SUPPORT_DRI_DRM)
	if (!IONELFBAtomicBoolRead(&psDevInfo->sLeaveVT))
#endif
	{
		IONELFBFlip(psDevInfo, psBuffer);
	}

	bPreviouslyNotVSynced = psSwapChain->bNotVSynced;
	psSwapChain->bNotVSynced = IONELFB_TRUE;


	if (!DontWaitForVSync(psDevInfo))
	{
		IONELFB_UPDATE_MODE eMode = IONELFBGetUpdateMode(psDevInfo);
		int iBlankEvents = IONELFBAtomicIntRead(&psDevInfo->sBlankEvents);

		switch(eMode)
		{
			case IONELFB_UPDATE_MODE_AUTO:
				psSwapChain->bNotVSynced = IONELFB_FALSE;

				if (bPreviouslyNotVSynced || psSwapChain->iBlankEvents != iBlankEvents)
				{
					psSwapChain->iBlankEvents = iBlankEvents;
					psSwapChain->bNotVSynced = !WaitForVSyncSettle(psDevInfo);
				} else if (psBuffer->ulSwapInterval != 0)
				{
					psSwapChain->bNotVSynced = !IONELFBWaitForVSync(psDevInfo);
				}
				break;
#if defined(PVR_IONEFB3_MANUAL_UPDATE_SYNC_IN_SWAP)
			case IONELFB_UPDATE_MODE_MANUAL:
				if (psBuffer->ulSwapInterval != 0)
				{
					(void) IONELFBManualSync(psDevInfo);
				}
				break;
#endif
			default:
				break;
		}
	}

	psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete((IMG_HANDLE)psBuffer->hCmdComplete, IMG_TRUE);
}

static IMG_BOOL ProcessFlipV1(IMG_HANDLE hCmdCookie,
							  IONELFB_DEVINFO *psDevInfo,
							  IONELFB_SWAPCHAIN *psSwapChain,
							  IONELFB_BUFFER *psBuffer,
							  unsigned long ulSwapInterval)
{
	IONELFBCreateSwapChainLock(psDevInfo);

	
	if (SwapChainHasChanged(psDevInfo, psSwapChain))
	{
		DEBUG_PRINTK((KERN_WARNING DRIVER_PREFIX
			": %s: Device %u (PVR Device ID %u): The swap chain has been destroyed\n",
			__FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	}
	else
	{
		psBuffer->hCmdComplete = (IONELFB_HANDLE)hCmdCookie;
		psBuffer->ulSwapInterval = ulSwapInterval;
#if defined(CONFIG_DSSCOMP)
		if (is_tiler_addr(psBuffer->sSysAddr.uiAddr))
		{
			IMG_UINT32 w = psBuffer->psDevInfo->sDisplayDim.ui32Width;
			IMG_UINT32 h = psBuffer->psDevInfo->sDisplayDim.ui32Height;
			struct dsscomp_setup_dispc_data comp = {
				.num_mgrs = 1,
				.mgrs[0].alpha_blending = 1,
				.num_ovls = 1,
				.ovls[0].cfg =
				{
					.width = w,
					.win.w = w,
					.crop.w = w,
					.height = h,
					.win.h = h,
					.crop.h = h,
					.stride = psBuffer->psDevInfo->sDisplayDim.ui32ByteStride,
					.color_mode = IONE_DSS_COLOR_ARGB32,
					.enabled = 1,
					.global_alpha = 255,
				},
				.mode = DSSCOMP_SETUP_DISPLAY,
			};
			struct tiler_pa_info *pas[1] = { NULL };
			comp.ovls[0].ba = (u32) psBuffer->sSysAddr.uiAddr;
			dsscomp_gralloc_queue(&comp, pas, true,
								  (void *) psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete,
								  (void *) psBuffer->hCmdComplete);
		}
		else
#endif 
		{
			IONELFBQueueBufferForSwap(psSwapChain, psBuffer);
		}
	}

	IONELFBCreateSwapChainUnLock(psDevInfo);

	return IMG_TRUE;
}

#if defined(CONFIG_DSSCOMP)

static IMG_BOOL ProcessFlipV2(IMG_HANDLE hCmdCookie,
							  IONELFB_DEVINFO *psDevInfo,
							  PDC_MEM_INFO *ppsMemInfos,
							  IMG_UINT32 ui32NumMemInfos,
							  struct dsscomp_setup_dispc_data *psDssData,
							  IMG_UINT32 ui32DssDataLength)
{
	struct tiler_pa_info *apsTilerPAs[5];
	IMG_UINT32 i, k;

	if(ui32DssDataLength != sizeof(*psDssData))
	{
		WARN(1, "invalid size of private data (%d vs %d)",
			 ui32DssDataLength, sizeof(*psDssData));
		return IMG_FALSE;
	}

	if(psDssData->num_ovls == 0 || ui32NumMemInfos == 0)
	{
		WARN(1, "must have at least one layer");
		return IMG_FALSE;
	}

	for(i = k = 0; i < ui32NumMemInfos && k < ARRAY_SIZE(apsTilerPAs); i++, k++)
	{
		struct tiler_pa_info *psTilerInfo;
		IMG_CPU_VIRTADDR virtAddr;
		IMG_CPU_PHYADDR phyAddr;
		IMG_UINT32 ui32NumPages;
		IMG_SIZE_T uByteSize;
		int j;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetByteSize(ppsMemInfos[i], &uByteSize);
		ui32NumPages = (uByteSize + PAGE_SIZE - 1) >> PAGE_SHIFT;

		apsTilerPAs[k] = NULL;

		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);

		
		if(psDssData->ovls[k].cfg.color_mode == IONE_DSS_COLOR_NV12)
		{
			
			BUG_ON(i + 1 >= ui32NumMemInfos);
			psDssData->ovls[k].ba = (u32)phyAddr.uiAddr;

			i++;
			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], 0, &phyAddr);
			psDssData->ovls[k].uv = (u32)phyAddr.uiAddr;

			continue;
		}

		
		if(is_tiler_addr((u32)phyAddr.uiAddr))
		{
			psDssData->ovls[k].ba = (u32)phyAddr.uiAddr;
			continue;
		}

		psTilerInfo = kzalloc(sizeof(*psTilerInfo), GFP_KERNEL);
		if(!psTilerInfo)
		{
			continue;
		}

		psTilerInfo->mem = kzalloc(sizeof(*psTilerInfo->mem) * ui32NumPages, GFP_KERNEL);
		if(!psTilerInfo->mem)
		{
			kfree(psTilerInfo);
			continue;
		}

		psTilerInfo->num_pg = ui32NumPages;
		psTilerInfo->memtype = TILER_MEM_USING;

		for(j = 0; j < ui32NumPages; j++)
		{
			psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuPAddr(ppsMemInfos[i], j << PAGE_SHIFT, &phyAddr);
			psTilerInfo->mem[j] = (u32)phyAddr.uiAddr;
		}

		
		psDevInfo->sPVRJTable.pfnPVRSRVDCMemInfoGetCpuVAddr(ppsMemInfos[i], &virtAddr);
		psDssData->ovls[k].ba = (u32)virtAddr;
		apsTilerPAs[k] = psTilerInfo;
	}

	
	for(i = k; i < psDssData->num_ovls && i < ARRAY_SIZE(apsTilerPAs); i++)
	{
		unsigned int ix = psDssData->ovls[i].ba;
		if(ix >= ARRAY_SIZE(apsTilerPAs))
		{
			WARN(1, "Invalid clone layer (%u); skipping all cloned layers", ix);
			psDssData->num_ovls = k;
			break;
		}
		apsTilerPAs[i] = apsTilerPAs[ix];
		psDssData->ovls[i].ba = psDssData->ovls[ix].ba;
		psDssData->ovls[i].uv = psDssData->ovls[ix].uv;
	}

	dsscomp_gralloc_queue(psDssData, apsTilerPAs, false,
						  (void *)psDevInfo->sPVRJTable.pfnPVRSRVCmdComplete,
						  (void *)hCmdCookie);

	for(i = 0; i < k; i++)
	{
		tiler_pa_free(apsTilerPAs[i]);
	}

	return IMG_TRUE;
}

#endif 

static IMG_BOOL ProcessFlip(IMG_HANDLE  hCmdCookie,
                            IMG_UINT32  ui32DataSize,
                            IMG_VOID   *pvData)
{
	DISPLAYCLASS_FLIP_COMMAND *psFlipCmd;
	IONELFB_DEVINFO *psDevInfo;

	
	if(!hCmdCookie || !pvData)
	{
		return IMG_FALSE;
	}

	
	psFlipCmd = (DISPLAYCLASS_FLIP_COMMAND*)pvData;

	if (psFlipCmd == IMG_NULL)
	{
		return IMG_FALSE;
	}

	psDevInfo = (IONELFB_DEVINFO*)psFlipCmd->hExtDevice;

	if(psFlipCmd->hExtBuffer)
	{
		return ProcessFlipV1(hCmdCookie,
							 psDevInfo,
							 psFlipCmd->hExtSwapChain,
							 psFlipCmd->hExtBuffer,
							 psFlipCmd->ui32SwapInterval);
	}
	else
	{
#if defined(CONFIG_DSSCOMP)
		DISPLAYCLASS_FLIP_COMMAND2 *psFlipCmd2;
		psFlipCmd2 = (DISPLAYCLASS_FLIP_COMMAND2 *)pvData;
		return ProcessFlipV2(hCmdCookie,
							 psDevInfo,
							 psFlipCmd2->ppsMemInfos,
							 psFlipCmd2->ui32NumMemInfos,
							 psFlipCmd2->pvPrivData,
							 psFlipCmd2->ui32PrivDataLength);
#else
		BUG();
#endif
	}
}

static IONELFB_ERROR IONELFBInitFBDev(IONELFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo;
	struct module *psLINFBOwner;
	IONELFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
	IONELFB_ERROR eError = IONELFB_ERROR_GENERIC;
	unsigned long FBSize;
	unsigned long ulLCM;
	unsigned uiFBDevID = psDevInfo->uiFBDevID;

	IONELFB_CONSOLE_LOCK();

	psLINFBInfo = registered_fb[uiFBDevID];
	if (psLINFBInfo == NULL)
	{
		eError = IONELFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	FBSize = (psLINFBInfo->screen_size) != 0 ?
					psLINFBInfo->screen_size :
					psLINFBInfo->fix.smem_len;

	
	if (FBSize == 0 || psLINFBInfo->fix.line_length == 0)
	{
		eError = IONELFB_ERROR_INVALID_DEVICE;
		goto ErrorRelSem;
	}

	psLINFBOwner = psLINFBInfo->fbops->owner;
	if (!try_module_get(psLINFBOwner))
	{
		printk(KERN_INFO DRIVER_PREFIX
			": %s: Device %u: Couldn't get framebuffer module\n", __FUNCTION__, uiFBDevID);

		goto ErrorRelSem;
	}

	if (psLINFBInfo->fbops->fb_open != NULL)
	{
		int res;

		res = psLINFBInfo->fbops->fb_open(psLINFBInfo, 0);
		if (res != 0)
		{
			printk(KERN_INFO DRIVER_PREFIX
				" %s: Device %u: Couldn't open framebuffer(%d)\n", __FUNCTION__, uiFBDevID, res);

			goto ErrorModPut;
		}
	}

	psDevInfo->psLINFBInfo = psLINFBInfo;

	ulLCM = LCM(psLINFBInfo->fix.line_length, IONELFB_PAGE_SIZE);

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer physical address: 0x%lx\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.smem_start));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual address: 0x%lx\n",
			psDevInfo->uiFBDevID, (unsigned long)psLINFBInfo->screen_base));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer size: %lu\n",
			psDevInfo->uiFBDevID, FBSize));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer virtual height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres_virtual));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer width: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.xres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer height: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->var.yres));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: Framebuffer stride: %u\n",
			psDevInfo->uiFBDevID, psLINFBInfo->fix.line_length));
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
			": Device %u: LCM of stride and page size: %lu\n",
			psDevInfo->uiFBDevID, ulLCM));

	
	IONELFBPrintInfo(psDevInfo);

#if defined(CONFIG_DSSCOMP)
	{
		
		int n = FBSize / RoundUpToMultiple(psLINFBInfo->fix.line_length * psLINFBInfo->var.yres, ulLCM);
		int res;
		int i, x, y, w;
		ion_phys_addr_t phys;
		size_t size;
		struct tiler_view_t view;

		struct ione_ion_tiler_alloc_data sAllocData =
		{
			
			
			.w = ALIGN(psLINFBInfo->var.xres, PAGE_SIZE / (psLINFBInfo->var.bits_per_pixel / 8)),
			.h = psLINFBInfo->var.yres,
			.fmt = psLINFBInfo->var.bits_per_pixel == 16 ? TILER_PIXEL_FMT_16BIT : TILER_PIXEL_FMT_32BIT,
			.flags = 0,
		};

		printk(KERN_DEBUG DRIVER_PREFIX
			   " %s: Device %u: Requesting %d TILER 2D framebuffers\n",
			   __FUNCTION__, uiFBDevID, n);

		
		if (n > 3)
			n = 3;

		sAllocData.w *= n;

		psPVRFBInfo->uiBytesPerPixel = psLINFBInfo->var.bits_per_pixel >> 3;
		psPVRFBInfo->bIs2D = IONELFB_TRUE;

		res = ione_ion_tiler_alloc(gpsIONClient, &sAllocData);
		psPVRFBInfo->psIONHandle = sAllocData.handle;
		if (res < 0)
		{
			printk(KERN_ERR DRIVER_PREFIX
				   " %s: Device %u: Could not allocate 2D framebuffer(%d)\n",
				   __FUNCTION__, uiFBDevID, res);
			goto ErrorModPut;
		}

		psLINFBInfo->fix.smem_start = ion_phys(gpsIONClient, sAllocData.handle, &phys, &size);

		psPVRFBInfo->sSysAddr.uiAddr = phys;
		psPVRFBInfo->sCPUVAddr = 0;
		psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
		psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;

		psPVRFBInfo->ulByteStride = PAGE_ALIGN(psPVRFBInfo->ulWidth * psPVRFBInfo->uiBytesPerPixel);
		w = psPVRFBInfo->ulByteStride >> PAGE_SHIFT;
 
		
		psPVRFBInfo->ulFBSize = sAllocData.h * n * psPVRFBInfo->ulByteStride;
		psPVRFBInfo->psPageList = kzalloc(w * n * psPVRFBInfo->ulHeight * sizeof(*psPVRFBInfo->psPageList), GFP_KERNEL);
		if (!psPVRFBInfo->psPageList)
		{
			printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Could not allocate page list\n", __FUNCTION__, psDevInfo->uiFBDevID);
			ion_free(gpsIONClient, sAllocData.handle);
			goto ErrorModPut;
		}

		tilview_create(&view, phys, psDevInfo->sFBInfo.ulWidth, psDevInfo->sFBInfo.ulHeight);
		for(i = 0; i < n; i++)
		{
			for(y = 0; y < psDevInfo->sFBInfo.ulHeight; y++)
			{
				for(x = 0; x < w; x++)
				{
					psPVRFBInfo->psPageList[i * psDevInfo->sFBInfo.ulHeight * w + y * w + x].uiAddr =
					phys + view.v_inc * y + ((x + i * w) << PAGE_SHIFT);
				}
			}
		}
	}
#else 
	
	psPVRFBInfo->sSysAddr.uiAddr = psLINFBInfo->fix.smem_start;
	psPVRFBInfo->sCPUVAddr = psLINFBInfo->screen_base;

	psPVRFBInfo->ulWidth = psLINFBInfo->var.xres;
	psPVRFBInfo->ulHeight = psLINFBInfo->var.yres;
	psPVRFBInfo->ulByteStride =  psLINFBInfo->fix.line_length;
	psPVRFBInfo->ulFBSize = FBSize;
#endif 

	psPVRFBInfo->ulBufferSize = psPVRFBInfo->ulHeight * psPVRFBInfo->ulByteStride;

	
	psPVRFBInfo->ulRoundedBufferSize = RoundUpToMultiple(psPVRFBInfo->ulBufferSize, ulLCM);

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
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
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
			printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
		}
	}	
	else
	{
		printk(KERN_INFO DRIVER_PREFIX ": %s: Device %u: Unknown FB format\n", __FUNCTION__, uiFBDevID);
	}

	psDevInfo->sFBInfo.ulPhysicalWidthmm =
		((int)psLINFBInfo->var.width  > 0) ? psLINFBInfo->var.width  : 90;

	psDevInfo->sFBInfo.ulPhysicalHeightmm =
		((int)psLINFBInfo->var.height > 0) ? psLINFBInfo->var.height : 54;

	
	psDevInfo->sFBInfo.sSysAddr.uiAddr = psPVRFBInfo->sSysAddr.uiAddr;
	psDevInfo->sFBInfo.sCPUVAddr = psPVRFBInfo->sCPUVAddr;

	eError = IONELFB_OK;
	goto ErrorRelSem;

ErrorModPut:
	module_put(psLINFBOwner);
ErrorRelSem:
	IONELFB_CONSOLE_UNLOCK();

	return eError;
}

static void IONELFBDeInitFBDev(IONELFB_DEVINFO *psDevInfo)
{
	struct fb_info *psLINFBInfo = psDevInfo->psLINFBInfo;
	struct module *psLINFBOwner;

	IONELFB_CONSOLE_LOCK();

#if defined(CONFIG_DSSCOMP)
	{
		IONELFB_FBINFO *psPVRFBInfo = &psDevInfo->sFBInfo;
		kfree(psPVRFBInfo->psPageList);
		if (psPVRFBInfo->psIONHandle)
		{
			ion_free(gpsIONClient, psPVRFBInfo->psIONHandle);
		}
	}
#endif 

	psLINFBOwner = psLINFBInfo->fbops->owner;

	if (psLINFBInfo->fbops->fb_release != NULL) 
	{
		(void) psLINFBInfo->fbops->fb_release(psLINFBInfo, 0);
	}

	module_put(psLINFBOwner);

	IONELFB_CONSOLE_UNLOCK();
}

static IONELFB_DEVINFO *IONELFBInitDev(unsigned uiFBDevID)
{
	PFN_CMD_PROC	 	pfnCmdProcList[IONELFB_COMMAND_COUNT];
	IMG_UINT32		aui32SyncCountList[IONELFB_COMMAND_COUNT][2];
	IONELFB_DEVINFO		*psDevInfo = NULL;

	
	psDevInfo = (IONELFB_DEVINFO *)IONELFBAllocKernelMem(sizeof(IONELFB_DEVINFO));

	if(psDevInfo == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't allocate device information structure\n", __FUNCTION__, uiFBDevID);

		goto ErrorExit;
	}

	
	memset(psDevInfo, 0, sizeof(IONELFB_DEVINFO));

	psDevInfo->uiFBDevID = uiFBDevID;

	
	if(!(*gpfnGetPVRJTable)(&psDevInfo->sPVRJTable))
	{
		goto ErrorFreeDevInfo;
	}

	
	if(IONELFBInitFBDev(psDevInfo) != IONELFB_OK)
	{
		
		goto ErrorFreeDevInfo;
	}

	psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers = (IMG_UINT32)(psDevInfo->sFBInfo.ulFBSize / psDevInfo->sFBInfo.ulRoundedBufferSize);
	if (psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers != 0)
	{
		psDevInfo->sDisplayInfo.ui32MaxSwapChains = 1;
		psDevInfo->sDisplayInfo.ui32MaxSwapInterval = 1;
	}

	psDevInfo->sDisplayInfo.ui32PhysicalWidthmm = psDevInfo->sFBInfo.ulPhysicalWidthmm;
	psDevInfo->sDisplayInfo.ui32PhysicalHeightmm = psDevInfo->sFBInfo.ulPhysicalHeightmm;

	strncpy(psDevInfo->sDisplayInfo.szDisplayName, DISPLAY_DEVICE_NAME, MAX_DISPLAY_NAME_SIZE);

	psDevInfo->sDisplayFormat.pixelformat = psDevInfo->sFBInfo.ePixelFormat;
	psDevInfo->sDisplayDim.ui32Width      = (IMG_UINT32)psDevInfo->sFBInfo.ulWidth;
	psDevInfo->sDisplayDim.ui32Height     = (IMG_UINT32)psDevInfo->sFBInfo.ulHeight;
	psDevInfo->sDisplayDim.ui32ByteStride = (IMG_UINT32)psDevInfo->sFBInfo.ulByteStride;

	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: Maximum number of swap chain buffers: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->sDisplayInfo.ui32MaxSwapChainBuffers));

	
	psDevInfo->sSystemBuffer.sSysAddr = psDevInfo->sFBInfo.sSysAddr;
	psDevInfo->sSystemBuffer.sCPUVAddr = psDevInfo->sFBInfo.sCPUVAddr;
	psDevInfo->sSystemBuffer.psDevInfo = psDevInfo;

	IONELFBInitBufferForSwap(&psDevInfo->sSystemBuffer);

	

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

	
	if(psDevInfo->sPVRJTable.pfnPVRSRVRegisterDCDevice(
		&psDevInfo->sDCJTable,
		&psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Services device registration failed\n", __FUNCTION__, uiFBDevID);

		goto ErrorDeInitFBDev;
	}
	DEBUG_PRINTK((KERN_INFO DRIVER_PREFIX
		": Device %u: PVR Device ID: %u\n",
		psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID));
	
	
	pfnCmdProcList[DC_FLIP_COMMAND] = ProcessFlip;

	
	aui32SyncCountList[DC_FLIP_COMMAND][0] = 0; 
	aui32SyncCountList[DC_FLIP_COMMAND][1] = 10; 

	



	if (psDevInfo->sPVRJTable.pfnPVRSRVRegisterCmdProcList(psDevInfo->uiPVRDevID,
															&pfnCmdProcList[0],
															aui32SyncCountList,
															IONELFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: Couldn't register command processing functions with PVR Services\n", __FUNCTION__, uiFBDevID);
		goto ErrorUnregisterDevice;
	}

	IONELFBCreateSwapChainLockInit(psDevInfo);

	IONELFBAtomicBoolInit(&psDevInfo->sBlanked, IONELFB_FALSE);
	IONELFBAtomicIntInit(&psDevInfo->sBlankEvents, 0);
	IONELFBAtomicBoolInit(&psDevInfo->sFlushCommands, IONELFB_FALSE);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	IONELFBAtomicBoolInit(&psDevInfo->sEarlySuspendFlag, IONELFB_FALSE);
#endif
#if defined(SUPPORT_DRI_DRM)
	IONELFBAtomicBoolInit(&psDevInfo->sLeaveVT, IONELFB_FALSE);
#endif
	return psDevInfo;

ErrorUnregisterDevice:
	(void)psDevInfo->sPVRJTable.pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID);
ErrorDeInitFBDev:
	IONELFBDeInitFBDev(psDevInfo);
ErrorFreeDevInfo:
	IONELFBFreeKernelMem(psDevInfo);
ErrorExit:
	return NULL;
}

IONELFB_ERROR IONELFBInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;
	unsigned uiDevicesFound = 0;

	if(IONELFBGetLibFuncAddr ("PVRGetDisplayClassJTable", &gpfnGetPVRJTable) != IONELFB_OK)
	{
		return IONELFB_ERROR_INIT_FAILURE;
	}

	
	for(i = uiMaxFBDevIDPlusOne; i-- != 0;)
	{
		IONELFB_DEVINFO *psDevInfo = IONELFBInitDev(i);

		if (psDevInfo != NULL)
		{
			
			IONELFBSetDevInfoPtr(psDevInfo->uiFBDevID, psDevInfo);
			uiDevicesFound++;
		}
	}

	return (uiDevicesFound != 0) ? IONELFB_OK : IONELFB_ERROR_INIT_FAILURE;
}

static IONELFB_BOOL IONELFBDeInitDev(IONELFB_DEVINFO *psDevInfo)
{
	PVRSRV_DC_DISP2SRV_KMJTABLE *psPVRJTable = &psDevInfo->sPVRJTable;

	IONELFBCreateSwapChainLockDeInit(psDevInfo);

	IONELFBAtomicBoolDeInit(&psDevInfo->sBlanked);
	IONELFBAtomicIntDeInit(&psDevInfo->sBlankEvents);
	IONELFBAtomicBoolDeInit(&psDevInfo->sFlushCommands);
#if defined(CONFIG_HAS_EARLYSUSPEND)
	IONELFBAtomicBoolDeInit(&psDevInfo->sEarlySuspendFlag);
#endif
#if defined(SUPPORT_DRI_DRM)
	IONELFBAtomicBoolDeInit(&psDevInfo->sLeaveVT);
#endif
	psPVRJTable = &psDevInfo->sPVRJTable;

	if (psPVRJTable->pfnPVRSRVRemoveCmdProcList (psDevInfo->uiPVRDevID, IONELFB_COMMAND_COUNT) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't unregister command processing functions\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return IONELFB_FALSE;
	}

	
	if (psPVRJTable->pfnPVRSRVRemoveDCDevice(psDevInfo->uiPVRDevID) != PVRSRV_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: PVR Device %u: Couldn't remove device from PVR Services\n", __FUNCTION__, psDevInfo->uiFBDevID, psDevInfo->uiPVRDevID);
		return IONELFB_FALSE;
	}
	
	IONELFBDeInitFBDev(psDevInfo);

	IONELFBSetDevInfoPtr(psDevInfo->uiFBDevID, NULL);

	
	IONELFBFreeKernelMem(psDevInfo);

	return IONELFB_TRUE;
}

IONELFB_ERROR IONELFBDeInit(void)
{
	unsigned uiMaxFBDevIDPlusOne = IONELFBMaxFBDevIDPlusOne();
	unsigned i;
	IONELFB_BOOL bError = IONELFB_FALSE;

	for(i = 0; i < uiMaxFBDevIDPlusOne; i++)
	{
		IONELFB_DEVINFO *psDevInfo = IONELFBGetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			bError |= !IONELFBDeInitDev(psDevInfo);
		}
	}

	return (bError) ? IONELFB_ERROR_INIT_FAILURE : IONELFB_OK;
}

