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

extern IMG_BOOL PVRGetDisplayClassJTable(PVRSRV_DC_DISP2SRV_KMJTABLE *psJTable);

#define IONE_VSYNC_IRQ          VOUT_IRQ
#define IONE_VSYNC0_IRQ		VOUT_IRQ
#define IONE_VSYNC1_IRQ		ORC_VOUT0_IRQ

typedef void *       IONE_HANDLE;

typedef enum tag_ione_bool
{
	IONE_FALSE = 0,
	IONE_TRUE  = 1,
} IONE_BOOL, *IONE_PBOOL;

typedef struct IONELFB_BUFFER_TAG
{
	unsigned long                ulBufferSize;




	IMG_SYS_PHYADDR              sSysAddr;
	IMG_CPU_VIRTADDR             sCPUVAddr;
	PVRSRV_SYNC_DATA            *psSyncData;

	struct IONELFB_BUFFER_TAG	*psNext;
} IONELFB_BUFFER;

typedef struct IONELFB_VSYNC_FLIP_ITEM_TAG
{



	IONE_HANDLE      hCmdComplete;

	unsigned long    ulSwapInterval;

	IONE_BOOL        bValid;

	IONE_BOOL        bFlipped;

	IONE_BOOL        bCmdCompleted;





	IMG_SYS_PHYADDR* sSysAddr;
} IONELFB_VSYNC_FLIP_ITEM;

typedef struct PVRPDP_SWAPCHAIN_TAG
{

	unsigned long       ulBufferCount;

	IONELFB_BUFFER     *psBuffer;

	IONELFB_VSYNC_FLIP_ITEM	*psVSyncFlips;


	unsigned long       ulInsertIndex;


	unsigned long       ulRemoveIndex;


	void *pvRegs;


	PVRSRV_DC_DISP2SRV_KMJTABLE	*psPVRJTable;


	IONE_BOOL           bFlushCommands;


	unsigned long       ulSetFlushStateRefCount;


	IONE_BOOL           bBlanked;


	spinlock_t         *psSwapChainLock;
} IONELFB_SWAPCHAIN;

typedef struct IONELFB_FBINFO_TAG
{
	unsigned long       ulFBSize;
	unsigned long       ulBufferSize;
	unsigned long       ulRoundedBufferSize;
	unsigned long       ulWidth;
	unsigned long       ulHeight;
	unsigned long       ulByteStride;



	IMG_SYS_PHYADDR     sSysAddr;
	IMG_CPU_VIRTADDR    sCPUVAddr;


	PVRSRV_PIXEL_FORMAT ePixelFormat;
}IONELFB_FBINFO;

typedef struct IONELFB_DEVINFO_TAG
{
	unsigned long           ulDeviceID;


	IONELFB_BUFFER          sSystemBuffer;


	PVRSRV_DC_DISP2SRV_KMJTABLE	sPVRJTable;


	PVRSRV_DC_SRV2DISP_KMJTABLE	sDCJTable;


	IONELFB_FBINFO          sFBInfo;


	unsigned long           ulRefCount;


	IONELFB_SWAPCHAIN      *psSwapChain;


	IONE_BOOL               bFlushCommands;


	struct fb_info         *psLINFBInfo;


	struct notifier_block   sLINNotifBlock;


	IONE_BOOL               bDeviceSuspended;


	spinlock_t             sSwapChainLock;





	IMG_DEV_VIRTADDR		sDisplayDevVAddr;

	DISPLAY_INFO            sDisplayInfo;


	DISPLAY_FORMAT          sDisplayFormat;


	DISPLAY_DIMS            sDisplayDim;

}  IONELFB_DEVINFO;

#define	IONELFB_PAGE_SIZE 4096
#define	IONELFB_PAGE_MASK (IONELFB_PAGE_SIZE - 1)
#define	IONELFB_PAGE_TRUNC (~IONELFB_PAGE_MASK)

#define	IONELFB_PAGE_ROUNDUP(x) (((x) + IONELFB_PAGE_MASK) & IONELFB_PAGE_TRUNC)

#ifdef	DEBUG
#define	DEBUG_PRINTK(x) printk x
#else
#define	DEBUG_PRINTK(x)
#endif

#define DISPLAY_DEVICE_NAME "PowerVR IONE Linux Display Driver"
#define	DRVNAME	"ionelfb"
#define	DEVNAME	DRVNAME
#define	DRIVER_PREFIX DRVNAME

typedef enum _IONE_ERROR_
{
	IONE_OK                             =  0,
	IONE_ERROR_GENERIC                  =  1,
	IONE_ERROR_OUT_OF_MEMORY            =  2,
	IONE_ERROR_TOO_FEW_BUFFERS          =  3,
	IONE_ERROR_INVALID_PARAMS           =  4,
	IONE_ERROR_INIT_FAILURE             =  5,
	IONE_ERROR_CANT_REGISTER_CALLBACK   =  6,
	IONE_ERROR_INVALID_DEVICE           =  7,
	IONE_ERROR_DEVICE_REGISTER_FAILED   =  8
} IONE_ERROR;


#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

IONE_ERROR IONELFBInit(void);
IONE_ERROR IONELFBDeinit(void);

#ifdef	LDM_PLATFORM
void IONELFBDriverSuspend(void);
void IONELFBDriverResume(void);
#endif

void *IONELFBAllocKernelMem(unsigned long ulSize);
void IONELFBFreeKernelMem(void *pvMem);
IONE_ERROR IONELFBGetLibFuncAddr(char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable);
IONE_ERROR IONELFBInstallVSyncISR (IONELFB_SWAPCHAIN *psSwapChain);
IONE_ERROR IONELFBUninstallVSyncISR(IONELFB_SWAPCHAIN *psSwapChain);
IONE_BOOL IONELFBVSyncIHandler(IONELFB_SWAPCHAIN *psSwapChain);
void IONELFBEnableVSyncInterrupt(IONELFB_SWAPCHAIN *psSwapChain);
void IONELFBDisableVSyncInterrupt(IONELFB_SWAPCHAIN *psSwapChain);
void IONELFBEnableDisplayRegisterAccess(void);
void IONELFBDisableDisplayRegisterAccess(void);
void IONELFBFlip(IONELFB_SWAPCHAIN *psSwapChain, unsigned long aPhyAddr);

#endif

