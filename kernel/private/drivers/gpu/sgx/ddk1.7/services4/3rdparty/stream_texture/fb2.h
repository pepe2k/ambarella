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

#ifndef __FB2_H__
#define __FB2_H__

#include <linux/version.h>

#include <asm/atomic.h>

#include <linux/kernel.h>
#include <linux/console.h>
#include <linux/fb.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/mutex.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define unref__ __attribute__ ((unused))

typedef void *       FB2_HANDLE;

typedef bool FB2_BOOL, *FB2_PBOOL;
#define	FB2_FALSE false
#define FB2_TRUE true

typedef	atomic_t	FB2_ATOMIC_BOOL;

typedef atomic_t	FB2_ATOMIC_INT;

typedef struct FB2_BUFFER_TAG
{
	struct FB2_BUFFER_TAG	*psNext;
	struct FB2_DEVINFO_TAG	*psDevInfo;
	struct work_struct	sWork;
	unsigned long		ulYOffset;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	IMG_SYS_PHYADDR		*psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	IMG_SYS_PHYADDR		sSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	IMG_SYS_PHYADDR		*psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	IMG_SYS_PHYADDR		*psSysAddr;
#endif

	IMG_CPU_VIRTADDR	sCPUVAddr;
	PVRSRV_SYNC_DATA	*psSyncData;

	FB2_HANDLE		hCmdComplete;
	unsigned long		ulSwapInterval;
} FB2_BUFFER;

typedef struct FB2_SWAPCHAIN_TAG
{

	unsigned int			uiSwapChainID;
	unsigned long			ulBufferCount;
	FB2_BUFFER     			*psBuffer;
	struct workqueue_struct		*psWorkQueue;
	FB2_BOOL			bNotVSynced;
	int				iBlankEvents;
	unsigned int			uiFBDevID;

	int				irq_no;
	wait_queue_head_t		vsync_wq;
	spinlock_t			wait_num_lock;
	int				wait_num;
	unsigned int			irq_interval;
	unsigned int			timeout;
} FB2_SWAPCHAIN;

typedef struct FB2_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;
	unsigned long       ulPhysicalWidthmm;
	unsigned long       ulPhysicalHeightmm;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	IMG_SYS_PHYADDR		*psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	IMG_SYS_PHYADDR		sSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	IMG_SYS_PHYADDR		*psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	IMG_SYS_PHYADDR		*psSysAddr;
#endif
	IMG_CPU_VIRTADDR    sCPUVAddr;


	PVRSRV_PIXEL_FORMAT ePixelFormat;
}FB2_FBINFO;

typedef struct FB2_DEVINFO_TAG
{

	unsigned int            uiFBDevID;


	unsigned int            uiPVRDevID;


	struct mutex		sCreateSwapChainMutex;


	FB2_BUFFER          sSystemBuffer;


	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;


	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;


	FB2_FBINFO          sFBInfo;


	FB2_SWAPCHAIN      *psSwapChain;


	unsigned int		uiSwapChainID;


	FB2_ATOMIC_BOOL     sFlushCommands;


	struct fb_info         *psLINFBInfo;


	struct notifier_block   sLINNotifBlock;





	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;


	DISPLAY_FORMAT          sDisplayFormat;


	DISPLAY_DIMS            sDisplayDim;


	FB2_ATOMIC_BOOL	sBlanked;


	FB2_ATOMIC_INT	sBlankEvents;

#ifdef CONFIG_HAS_EARLYSUSPEND

	FB2_ATOMIC_BOOL	sEarlySuspendFlag;

	struct early_suspend    sEarlySuspend;
#endif

#if defined(SUPPORT_DRI_DRM)
	FB2_ATOMIC_BOOL     sLeaveVT;
#endif

}  FB2_DEVINFO;

#define	FB2_PAGE_SIZE 4096

#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR IONE Linux Display Driver"
#define	DRVNAME	"ionelfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

typedef enum _FB2_ERROR_
{
	FB2_OK                             =  0,
	FB2_ERROR_GENERIC                  =  1,
	FB2_ERROR_OUT_OF_MEMORY            =  2,
	FB2_ERROR_TOO_FEW_BUFFERS          =  3,
	FB2_ERROR_INVALID_PARAMS           =  4,
	FB2_ERROR_INIT_FAILURE             =  5,
	FB2_ERROR_CANT_REGISTER_CALLBACK   =  6,
	FB2_ERROR_INVALID_DEVICE           =  7,
	FB2_ERROR_DEVICE_REGISTER_FAILED   =  8,
	FB2_ERROR_SET_UPDATE_MODE_FAILED   =  9
} FB2_ERROR;

typedef enum _FB2_UPDATE_MODE_
{
	FB2_UPDATE_MODE_UNDEFINED			= 0,
	FB2_UPDATE_MODE_MANUAL			= 1,
	FB2_UPDATE_MODE_AUTO			= 2,
	FB2_UPDATE_MODE_DISABLED			= 3
} FB2_UPDATE_MODE;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

FB2_ERROR FB2Init(STREAM_TEXTURE_FB2INFO *);
FB2_ERROR FB2DeInit(void);

FB2_DEVINFO *FB2GetDevInfoPtr(unsigned uiFBDevID);
unsigned FB2MaxFBDevIDPlusOne(void);
void *FB2AllocKernelMem(unsigned long ulSize);
void FB2FreeKernelMem(void *pvMem);
FB2_ERROR FB2GetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
FB2_ERROR FB2CreateSwapQueue (FB2_SWAPCHAIN *psSwapChain);
void FB2DestroySwapQueue(FB2_SWAPCHAIN *psSwapChain);
void FB2InitBufferForSwap(FB2_BUFFER *psBuffer);
void FB2SwapHandler(FB2_BUFFER *psBuffer);
void FB2QueueBufferForSwap(FB2_SWAPCHAIN *psSwapChain, FB2_BUFFER *psBuffer);
void FB2Flip(FB2_DEVINFO *psDevInfo, FB2_BUFFER *psBuffer);
FB2_UPDATE_MODE FB2GetUpdateMode(FB2_DEVINFO *psDevInfo);
FB2_BOOL FB2SetUpdateMode(FB2_DEVINFO *psDevInfo, FB2_UPDATE_MODE eMode);
FB2_BOOL FB2WaitForVSync(FB2_DEVINFO *psDevInfo);
FB2_BOOL FB2ManualSync(FB2_DEVINFO *psDevInfo);
FB2_BOOL FB2CheckModeAndSync(FB2_DEVINFO *psDevInfo);
FB2_ERROR FB2UnblankDisplay(FB2_DEVINFO *psDevInfo);
FB2_ERROR FB2EnableLFBEventNotification(FB2_DEVINFO *psDevInfo);
FB2_ERROR FB2DisableLFBEventNotification(FB2_DEVINFO *psDevInfo);
void FB2CreateSwapChainLockInit(FB2_DEVINFO *psDevInfo);
void FB2CreateSwapChainLockDeInit(FB2_DEVINFO *psDevInfo);
void FB2CreateSwapChainLock(FB2_DEVINFO *psDevInfo);
void FB2CreateSwapChainUnLock(FB2_DEVINFO *psDevInfo);
void FB2AtomicBoolInit(FB2_ATOMIC_BOOL *psAtomic, FB2_BOOL bVal);
void FB2AtomicBoolDeInit(FB2_ATOMIC_BOOL *psAtomic);
void FB2AtomicBoolSet(FB2_ATOMIC_BOOL *psAtomic, FB2_BOOL bVal);
FB2_BOOL FB2AtomicBoolRead(FB2_ATOMIC_BOOL *psAtomic);
void FB2AtomicIntInit(FB2_ATOMIC_INT *psAtomic, int iVal);
void FB2AtomicIntDeInit(FB2_ATOMIC_INT *psAtomic);
void FB2AtomicIntSet(FB2_ATOMIC_INT *psAtomic, int iVal);
int FB2AtomicIntRead(FB2_ATOMIC_INT *psAtomic);
void FB2AtomicIntInc(FB2_ATOMIC_INT *psAtomic);

#endif

