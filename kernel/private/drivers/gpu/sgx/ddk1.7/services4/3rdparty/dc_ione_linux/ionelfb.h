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

#ifndef __IONELFB_H__
#define __IONELFB_H__

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

typedef void *       IONELFB_HANDLE;

typedef bool IONELFB_BOOL, *IONELFB_PBOOL;
#define	IONELFB_FALSE false
#define IONELFB_TRUE true

typedef	atomic_t	IONELFB_ATOMIC_BOOL;

typedef atomic_t	IONELFB_ATOMIC_INT;

typedef struct IONELFB_BUFFER_TAG
{
	struct IONELFB_BUFFER_TAG	*psNext;
	struct IONELFB_DEVINFO_TAG	*psDevInfo;

	struct work_struct sWork;


	unsigned long		     	ulYOffset;



	IMG_SYS_PHYADDR              	sSysAddr;
	IMG_CPU_VIRTADDR             	sCPUVAddr;
	PVRSRV_SYNC_DATA            	*psSyncData;

	IONELFB_HANDLE      		hCmdComplete;
	unsigned long    		ulSwapInterval;
} IONELFB_BUFFER;

#ifdef CONFIG_SGX_QUEUE_EACH_FRAME
typedef enum IONELFB_BUFFER_STATE_TAG
{
	IONELFB_BUFFER_IDLE,
	IONELFB_BUFFER_PENDING,
	IONELFB_BUFFER_VSYNCED,
} IONELFB_BUFFER_STATE;

typedef struct IONELFB_VSYNC_BUFFER_TAG
{
	IONELFB_BUFFER			buffer;
	IONELFB_BUFFER_STATE		state;
} IONELFB_VSYNC_BUFFER;
#endif

typedef struct IONELFB_SWAPCHAIN_TAG
{

	unsigned int			uiSwapChainID;


	unsigned long       		ulBufferCount;


	IONELFB_BUFFER     		*psBuffer;


	struct workqueue_struct   	*psWorkQueue;


	IONELFB_BOOL			bNotVSynced;


	int				iBlankEvents;


	unsigned int            	uiFBDevID;

	int				irq_no;
	wait_queue_head_t		vsync_wq;
	spinlock_t			wait_num_lock;
	int				wait_num;
	unsigned int			irq_interval;
	unsigned int			timeout;

#ifdef CONFIG_SGX_QUEUE_EACH_FRAME
	struct workqueue_struct 	*refresh_workqueue;
	struct work_struct		refresh_worker;
	IONELFB_VSYNC_BUFFER		*pvsync_buffers;
	int				enqueue_index;
	int				dequeue_index;
#endif
} IONELFB_SWAPCHAIN;

typedef struct IONELFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;
	unsigned long       ulPhysicalWidthmm;
	unsigned long       ulPhysicalHeightmm;



	IMG_SYS_PHYADDR     sSysAddr;
	IMG_CPU_VIRTADDR    sCPUVAddr;


	PVRSRV_PIXEL_FORMAT ePixelFormat;
}IONELFB_FBINFO;

typedef struct IONELFB_DEVINFO_TAG
{

	unsigned int            uiFBDevID;


	unsigned int            uiPVRDevID;


	struct mutex		sCreateSwapChainMutex;


	IONELFB_BUFFER          sSystemBuffer;


	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;


	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;


	IONELFB_FBINFO          sFBInfo;


	IONELFB_SWAPCHAIN      *psSwapChain;


	unsigned int		uiSwapChainID;


	IONELFB_ATOMIC_BOOL     sFlushCommands;


	struct fb_info         *psLINFBInfo;


	struct notifier_block   sLINNotifBlock;





	IMG_DEV_VIRTADDR	sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;


	DISPLAY_FORMAT          sDisplayFormat;


	DISPLAY_DIMS            sDisplayDim;


	IONELFB_ATOMIC_BOOL	sBlanked;


	IONELFB_ATOMIC_INT	sBlankEvents;

#ifdef CONFIG_HAS_EARLYSUSPEND

	IONELFB_ATOMIC_BOOL	sEarlySuspendFlag;

	struct early_suspend    sEarlySuspend;
#endif

#if defined(SUPPORT_DRI_DRM)
	IONELFB_ATOMIC_BOOL     sLeaveVT;
#endif

}  IONELFB_DEVINFO;

#define	IONELFB_PAGE_SIZE 4096

#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR IONE Linux Display Driver"
#define	DRVNAME	"ionelfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

typedef enum _IONELFB_ERROR_
{
	IONELFB_OK                             =  0,
	IONELFB_ERROR_GENERIC                  =  1,
	IONELFB_ERROR_OUT_OF_MEMORY            =  2,
	IONELFB_ERROR_TOO_FEW_BUFFERS          =  3,
	IONELFB_ERROR_INVALID_PARAMS           =  4,
	IONELFB_ERROR_INIT_FAILURE             =  5,
	IONELFB_ERROR_CANT_REGISTER_CALLBACK   =  6,
	IONELFB_ERROR_INVALID_DEVICE           =  7,
	IONELFB_ERROR_DEVICE_REGISTER_FAILED   =  8,
	IONELFB_ERROR_SET_UPDATE_MODE_FAILED   =  9
} IONELFB_ERROR;

typedef enum _IONELFB_UPDATE_MODE_
{
	IONELFB_UPDATE_MODE_UNDEFINED			= 0,
	IONELFB_UPDATE_MODE_MANUAL			= 1,
	IONELFB_UPDATE_MODE_AUTO			= 2,
	IONELFB_UPDATE_MODE_DISABLED			= 3
} IONELFB_UPDATE_MODE;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

IONELFB_ERROR IONELFBInit(void);
IONELFB_ERROR IONELFBDeInit(void);

IONELFB_DEVINFO *IONELFBGetDevInfoPtr(unsigned uiFBDevID);
unsigned IONELFBMaxFBDevIDPlusOne(void);
void *IONELFBAllocKernelMem(unsigned long ulSize);
void IONELFBFreeKernelMem(void *pvMem);
IONELFB_ERROR IONELFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
IONELFB_ERROR IONELFBCreateSwapQueue (IONELFB_SWAPCHAIN *psSwapChain);
void IONELFBDestroySwapQueue(IONELFB_SWAPCHAIN *psSwapChain);
void IONELFBInitBufferForSwap(IONELFB_BUFFER *psBuffer);
void IONELFBSwapHandler(IONELFB_BUFFER *psBuffer);
void IONELFBQueueBufferForSwap(IONELFB_SWAPCHAIN *psSwapChain, IONELFB_BUFFER *psBuffer);
void IONELFBFlip(IONELFB_DEVINFO *psDevInfo, IONELFB_BUFFER *psBuffer);
IONELFB_UPDATE_MODE IONELFBGetUpdateMode(IONELFB_DEVINFO *psDevInfo);
IONELFB_BOOL IONELFBSetUpdateMode(IONELFB_DEVINFO *psDevInfo, IONELFB_UPDATE_MODE eMode);
IONELFB_BOOL IONELFBWaitForVSync(IONELFB_DEVINFO *psDevInfo);
IONELFB_BOOL IONELFBManualSync(IONELFB_DEVINFO *psDevInfo);
IONELFB_BOOL IONELFBCheckModeAndSync(IONELFB_DEVINFO *psDevInfo);
IONELFB_ERROR IONELFBUnblankDisplay(IONELFB_DEVINFO *psDevInfo);
IONELFB_ERROR IONELFBEnableLFBEventNotification(IONELFB_DEVINFO *psDevInfo);
IONELFB_ERROR IONELFBDisableLFBEventNotification(IONELFB_DEVINFO *psDevInfo);
void IONELFBCreateSwapChainLockInit(IONELFB_DEVINFO *psDevInfo);
void IONELFBCreateSwapChainLockDeInit(IONELFB_DEVINFO *psDevInfo);
void IONELFBCreateSwapChainLock(IONELFB_DEVINFO *psDevInfo);
void IONELFBCreateSwapChainUnLock(IONELFB_DEVINFO *psDevInfo);
void IONELFBAtomicBoolInit(IONELFB_ATOMIC_BOOL *psAtomic, IONELFB_BOOL bVal);
void IONELFBAtomicBoolDeInit(IONELFB_ATOMIC_BOOL *psAtomic);
void IONELFBAtomicBoolSet(IONELFB_ATOMIC_BOOL *psAtomic, IONELFB_BOOL bVal);
IONELFB_BOOL IONELFBAtomicBoolRead(IONELFB_ATOMIC_BOOL *psAtomic);
void IONELFBAtomicIntInit(IONELFB_ATOMIC_INT *psAtomic, int iVal);
void IONELFBAtomicIntDeInit(IONELFB_ATOMIC_INT *psAtomic);
void IONELFBAtomicIntSet(IONELFB_ATOMIC_INT *psAtomic, int iVal);
int IONELFBAtomicIntRead(IONELFB_ATOMIC_INT *psAtomic);
void IONELFBAtomicIntInc(IONELFB_ATOMIC_INT *psAtomic);

#endif

