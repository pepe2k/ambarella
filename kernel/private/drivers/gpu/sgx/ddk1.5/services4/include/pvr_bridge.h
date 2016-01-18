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

#ifndef __PVR_BRIDGE_H__
#define __PVR_BRIDGE_H__

#if defined (__cplusplus)
extern "C" {
#endif

#include "servicesint.h"

#ifdef __linux__

		#include <linux/ioctl.h>

    #define PVRSRV_IOC_GID      'g'
    #define PVRSRV_IO(INDEX)    _IO(PVRSRV_IOC_GID, INDEX, PVRSRV_BRIDGE_PACKAGE)
    #define PVRSRV_IOW(INDEX)   _IOW(PVRSRV_IOC_GID, INDEX, PVRSRV_BRIDGE_PACKAGE)
    #define PVRSRV_IOR(INDEX)   _IOR(PVRSRV_IOC_GID, INDEX, PVRSRV_BRIDGE_PACKAGE)
    #define PVRSRV_IOWR(INDEX)  _IOWR(PVRSRV_IOC_GID, INDEX, PVRSRV_BRIDGE_PACKAGE)

#else

			#error Unknown platform: Cannot define ioctls

	#define PVRSRV_IO(INDEX)    (PVRSRV_IOC_GID + INDEX)
	#define PVRSRV_IOW(INDEX)   (PVRSRV_IOC_GID + INDEX)
	#define PVRSRV_IOR(INDEX)   (PVRSRV_IOC_GID + INDEX)
	#define PVRSRV_IOWR(INDEX)  (PVRSRV_IOC_GID + INDEX)

	#define PVRSRV_BRIDGE_BASE                  PVRSRV_IOC_GID
#endif


#define PVRSRV_BRIDGE_CORE_CMD_FIRST			0UL
#define PVRSRV_BRIDGE_ENUM_DEVICES				PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+0)
#define PVRSRV_BRIDGE_ACQUIRE_DEVICEINFO		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+1)
#define PVRSRV_BRIDGE_RELEASE_DEVICEINFO		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+2)
#define PVRSRV_BRIDGE_CREATE_DEVMEMCONTEXT		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+3)
#define PVRSRV_BRIDGE_DESTROY_DEVMEMCONTEXT		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+4)
#define PVRSRV_BRIDGE_GET_DEVMEM_HEAPINFO		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+5)
#define PVRSRV_BRIDGE_ALLOC_DEVICEMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+6)
#define PVRSRV_BRIDGE_FREE_DEVICEMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+7)
#define PVRSRV_BRIDGE_GETFREE_DEVICEMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+8)
#define PVRSRV_BRIDGE_CREATE_COMMANDQUEUE		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+9)
#define PVRSRV_BRIDGE_DESTROY_COMMANDQUEUE		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+10)
#define	PVRSRV_BRIDGE_MHANDLE_TO_MMAP_DATA           PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+11)
#define PVRSRV_BRIDGE_CONNECT_SERVICES			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+12)
#define PVRSRV_BRIDGE_DISCONNECT_SERVICES		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+13)
#define PVRSRV_BRIDGE_WRAP_DEVICE_MEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+14)
#define PVRSRV_BRIDGE_GET_DEVICEMEMINFO			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+15)
#define PVRSRV_BRIDGE_RESERVE_DEV_VIRTMEM		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+16)
#define PVRSRV_BRIDGE_FREE_DEV_VIRTMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+17)
#define PVRSRV_BRIDGE_MAP_EXT_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+18)
#define PVRSRV_BRIDGE_UNMAP_EXT_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+19)
#define PVRSRV_BRIDGE_MAP_DEV_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+20)
#define PVRSRV_BRIDGE_UNMAP_DEV_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+21)
#define PVRSRV_BRIDGE_MAP_DEVICECLASS_MEMORY	PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+22)
#define PVRSRV_BRIDGE_UNMAP_DEVICECLASS_MEMORY	PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+23)
#define PVRSRV_BRIDGE_MAP_MEM_INFO_TO_USER		PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+24)
#define PVRSRV_BRIDGE_UNMAP_MEM_INFO_FROM_USER	PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+25)
#define PVRSRV_BRIDGE_EXPORT_DEVICEMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+26)
#define PVRSRV_BRIDGE_RELEASE_MMAP_DATA			PVRSRV_IOWR(PVRSRV_BRIDGE_CORE_CMD_FIRST+27)
#define PVRSRV_BRIDGE_CORE_CMD_LAST				(PVRSRV_BRIDGE_CORE_CMD_FIRST+27)

#define PVRSRV_BRIDGE_SIM_CMD_FIRST				(PVRSRV_BRIDGE_CORE_CMD_LAST+1)
#define PVRSRV_BRIDGE_PROCESS_SIMISR_EVENT		PVRSRV_IOWR(PVRSRV_BRIDGE_SIM_CMD_FIRST+0)
#define PVRSRV_BRIDGE_REGISTER_SIM_PROCESS		PVRSRV_IOWR(PVRSRV_BRIDGE_SIM_CMD_FIRST+1)
#define PVRSRV_BRIDGE_UNREGISTER_SIM_PROCESS	PVRSRV_IOWR(PVRSRV_BRIDGE_SIM_CMD_FIRST+2)
#define PVRSRV_BRIDGE_SIM_CMD_LAST				(PVRSRV_BRIDGE_SIM_CMD_FIRST+2)

#define PVRSRV_BRIDGE_MAPPING_CMD_FIRST			(PVRSRV_BRIDGE_SIM_CMD_LAST+1)
#define PVRSRV_BRIDGE_MAPPHYSTOUSERSPACE		PVRSRV_IOWR(PVRSRV_BRIDGE_MAPPING_CMD_FIRST+0)
#define PVRSRV_BRIDGE_UNMAPPHYSTOUSERSPACE		PVRSRV_IOWR(PVRSRV_BRIDGE_MAPPING_CMD_FIRST+1)
#define PVRSRV_BRIDGE_GETPHYSTOUSERSPACEMAP		PVRSRV_IOWR(PVRSRV_BRIDGE_MAPPING_CMD_FIRST+2)
#define PVRSRV_BRIDGE_MAPPING_CMD_LAST			(PVRSRV_BRIDGE_MAPPING_CMD_FIRST+2)

#define PVRSRV_BRIDGE_STATS_CMD_FIRST			(PVRSRV_BRIDGE_MAPPING_CMD_LAST+1)
#define	PVRSRV_BRIDGE_GET_FB_STATS				PVRSRV_IOWR(PVRSRV_BRIDGE_STATS_CMD_FIRST+0)
#define PVRSRV_BRIDGE_STATS_CMD_LAST			(PVRSRV_BRIDGE_STATS_CMD_FIRST+0)

#define PVRSRV_BRIDGE_MISC_CMD_FIRST			(PVRSRV_BRIDGE_STATS_CMD_LAST+1)
#define PVRSRV_BRIDGE_GET_MISC_INFO				PVRSRV_IOWR(PVRSRV_BRIDGE_MISC_CMD_FIRST+0)
#define PVRSRV_BRIDGE_RELEASE_MISC_INFO			PVRSRV_IOWR(PVRSRV_BRIDGE_MISC_CMD_FIRST+1)
#define PVRSRV_BRIDGE_MISC_CMD_LAST				(PVRSRV_BRIDGE_MISC_CMD_FIRST+1)

#define PVRSRV_BRIDGE_OVERLAY_CMD_FIRST			(PVRSRV_BRIDGE_MISC_CMD_LAST+1)
#if defined (SUPPORT_OVERLAY_ROTATE_BLIT)
#define PVRSRV_BRIDGE_INIT_3D_OVL_BLT_RES		PVRSRV_IOWR(PVRSRV_BRIDGE_OVERLAY_CMD_FIRST+0)
#define PVRSRV_BRIDGE_DEINIT_3D_OVL_BLT_RES		PVRSRV_IOWR(PVRSRV_BRIDGE_OVERLAY_CMD_FIRST+1)
#endif
#define PVRSRV_BRIDGE_OVERLAY_CMD_LAST			(PVRSRV_BRIDGE_OVERLAY_CMD_FIRST+1)

#if defined(PDUMP)
#define PVRSRV_BRIDGE_PDUMP_CMD_FIRST			(PVRSRV_BRIDGE_OVERLAY_CMD_FIRST+1)
#define PVRSRV_BRIDGE_PDUMP_INIT			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+0)
#define PVRSRV_BRIDGE_PDUMP_MEMPOL			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+1)
#define PVRSRV_BRIDGE_PDUMP_DUMPMEM			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+2)
#define PVRSRV_BRIDGE_PDUMP_REG				PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+3)
#define PVRSRV_BRIDGE_PDUMP_REGPOL			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+4)
#define PVRSRV_BRIDGE_PDUMP_COMMENT			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+5)
#define PVRSRV_BRIDGE_PDUMP_SETFRAME			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+6)
#define PVRSRV_BRIDGE_PDUMP_ISCAPTURING			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+7)
#define PVRSRV_BRIDGE_PDUMP_DUMPBITMAP			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+8)
#define PVRSRV_BRIDGE_PDUMP_DUMPREADREG			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+9)
#define PVRSRV_BRIDGE_PDUMP_SYNCPOL			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+10)
#define PVRSRV_BRIDGE_PDUMP_DUMPSYNC			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+11)
#define PVRSRV_BRIDGE_PDUMP_MEMPAGES			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+12)
#define PVRSRV_BRIDGE_PDUMP_DRIVERINFO			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+13)
#define PVRSRV_BRIDGE_PDUMP_PDREG			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+14)
#define PVRSRV_BRIDGE_PDUMP_DUMPPDDEVPADDR		PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+15)
#define PVRSRV_BRIDGE_PDUMP_CYCLE_COUNT_REG_READ	PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+16)
#define PVRSRV_BRIDGE_PDUMP_STARTINITPHASE			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+17)
#define PVRSRV_BRIDGE_PDUMP_STOPINITPHASE			PVRSRV_IOWR(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+18)
#define PVRSRV_BRIDGE_PDUMP_CMD_LAST			(PVRSRV_BRIDGE_PDUMP_CMD_FIRST+18)
#else
#define PVRSRV_BRIDGE_PDUMP_CMD_LAST			PVRSRV_BRIDGE_OVERLAY_CMD_LAST
#endif

#define PVRSRV_BRIDGE_OEM_CMD_FIRST				(PVRSRV_BRIDGE_PDUMP_CMD_LAST+1)
#define PVRSRV_BRIDGE_GET_OEMJTABLE				PVRSRV_IOWR(PVRSRV_BRIDGE_OEM_CMD_FIRST+0)
#define PVRSRV_BRIDGE_OEM_CMD_LAST				(PVRSRV_BRIDGE_OEM_CMD_FIRST+0)

#define PVRSRV_BRIDGE_DEVCLASS_CMD_FIRST		(PVRSRV_BRIDGE_OEM_CMD_LAST+1)
#define PVRSRV_BRIDGE_ENUM_CLASS				PVRSRV_IOWR(PVRSRV_BRIDGE_DEVCLASS_CMD_FIRST+0)
#define PVRSRV_BRIDGE_DEVCLASS_CMD_LAST			(PVRSRV_BRIDGE_DEVCLASS_CMD_FIRST+0)

#define PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST		(PVRSRV_BRIDGE_DEVCLASS_CMD_LAST+1)
#define PVRSRV_BRIDGE_OPEN_DISPCLASS_DEVICE		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+0)
#define PVRSRV_BRIDGE_CLOSE_DISPCLASS_DEVICE	PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+1)
#define PVRSRV_BRIDGE_ENUM_DISPCLASS_FORMATS	PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+2)
#define PVRSRV_BRIDGE_ENUM_DISPCLASS_DIMS		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+3)
#define PVRSRV_BRIDGE_GET_DISPCLASS_SYSBUFFER	PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+4)
#define PVRSRV_BRIDGE_GET_DISPCLASS_INFO		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+5)
#define PVRSRV_BRIDGE_CREATE_DISPCLASS_SWAPCHAIN		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+6)
#define PVRSRV_BRIDGE_DESTROY_DISPCLASS_SWAPCHAIN		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+7)
#define PVRSRV_BRIDGE_SET_DISPCLASS_DSTRECT		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+8)
#define PVRSRV_BRIDGE_SET_DISPCLASS_SRCRECT		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+9)
#define PVRSRV_BRIDGE_SET_DISPCLASS_DSTCOLOURKEY		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+10)
#define PVRSRV_BRIDGE_SET_DISPCLASS_SRCCOLOURKEY		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+11)
#define PVRSRV_BRIDGE_GET_DISPCLASS_BUFFERS		PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+12)
#define PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_BUFFER	PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+13)
#define PVRSRV_BRIDGE_SWAP_DISPCLASS_TO_SYSTEM	PVRSRV_IOWR(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+14)
#define PVRSRV_BRIDGE_DISPCLASS_CMD_LAST		(PVRSRV_BRIDGE_DISPCLASS_CMD_FIRST+14)


#define PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST		(PVRSRV_BRIDGE_DISPCLASS_CMD_LAST+1)
#define PVRSRV_BRIDGE_OPEN_BUFFERCLASS_DEVICE	PVRSRV_IOWR(PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST+0)
#define PVRSRV_BRIDGE_CLOSE_BUFFERCLASS_DEVICE	PVRSRV_IOWR(PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST+1)
#define PVRSRV_BRIDGE_GET_BUFFERCLASS_INFO		PVRSRV_IOWR(PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST+2)
#define PVRSRV_BRIDGE_GET_BUFFERCLASS_BUFFER	PVRSRV_IOWR(PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST+3)
#define PVRSRV_BRIDGE_BUFCLASS_CMD_LAST			(PVRSRV_BRIDGE_BUFCLASS_CMD_FIRST+3)

#define PVRSRV_BRIDGE_WRAP_CMD_FIRST			(PVRSRV_BRIDGE_BUFCLASS_CMD_LAST+1)
#define PVRSRV_BRIDGE_WRAP_EXT_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_WRAP_CMD_FIRST+0)
#define PVRSRV_BRIDGE_UNWRAP_EXT_MEMORY			PVRSRV_IOWR(PVRSRV_BRIDGE_WRAP_CMD_FIRST+1)
#define PVRSRV_BRIDGE_WRAP_CMD_LAST				(PVRSRV_BRIDGE_WRAP_CMD_FIRST+1)

#define PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST		(PVRSRV_BRIDGE_WRAP_CMD_LAST+1)
#define PVRSRV_BRIDGE_ALLOC_SHARED_SYS_MEM		PVRSRV_IOWR(PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST+0)
#define PVRSRV_BRIDGE_FREE_SHARED_SYS_MEM		PVRSRV_IOWR(PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST+1)
#define PVRSRV_BRIDGE_MAP_MEMINFO_MEM			PVRSRV_IOWR(PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST+2)
#define PVRSRV_BRIDGE_UNMAP_MEMINFO_MEM			PVRSRV_IOWR(PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST+3)
#define PVRSRV_BRIDGE_SHAREDMEM_CMD_LAST		(PVRSRV_BRIDGE_SHAREDMEM_CMD_FIRST+3)

#define PVRSRV_BRIDGE_SERVICES4_TMP_CMD_FIRST	(PVRSRV_BRIDGE_SHAREDMEM_CMD_LAST+1)
#define PVRSRV_BRIDGE_GETMMU_PD_DEVPADDR        PVRSRV_IOWR(PVRSRV_BRIDGE_SERVICES4_TMP_CMD_FIRST+0)
#define PVRSRV_BRIDGE_SERVICES4_TMP_CMD_LAST	(PVRSRV_BRIDGE_SERVICES4_TMP_CMD_FIRST+0)

#define PVRSRV_BRIDGE_INITSRV_CMD_FIRST			(PVRSRV_BRIDGE_SERVICES4_TMP_CMD_LAST+1)
#define PVRSRV_BRIDGE_INITSRV_CONNECT			PVRSRV_IOWR(PVRSRV_BRIDGE_INITSRV_CMD_FIRST+0)
#define PVRSRV_BRIDGE_INITSRV_DISCONNECT		PVRSRV_IOWR(PVRSRV_BRIDGE_INITSRV_CMD_FIRST+1)
#define PVRSRV_BRIDGE_INITSRV_CMD_LAST			(PVRSRV_BRIDGE_INITSRV_CMD_FIRST+1)

#define PVRSRV_BRIDGE_EVENT_OBJECT_CMD_FIRST	(PVRSRV_BRIDGE_INITSRV_CMD_LAST+1)
#define PVRSRV_BRIDGE_EVENT_OBJECT_WAIT			PVRSRV_IOWR(PVRSRV_BRIDGE_EVENT_OBJECT_CMD_FIRST+0)
#define PVRSRV_BRIDGE_EVENT_OBJECT_OPEN			PVRSRV_IOWR(PVRSRV_BRIDGE_EVENT_OBJECT_CMD_FIRST+1)
#define PVRSRV_BRIDGE_EVENT_OBJECT_CLOSE		PVRSRV_IOWR(PVRSRV_BRIDGE_EVENT_OBJECT_CMD_FIRST+2)
#define PVRSRV_BRIDGE_EVENT_OBJECT_CMD_LAST		(PVRSRV_BRIDGE_EVENT_OBJECT_CMD_FIRST+2)

#define PVRSRV_BRIDGE_SYNC_OPS_CMD_FIRST		(PVRSRV_BRIDGE_EVENT_OBJECT_CMD_LAST+1)
#define PVRSRV_BRIDGE_MODIFY_PENDING_SYNC_OPS	PVRSRV_IOWR(PVRSRV_BRIDGE_SYNC_OPS_CMD_FIRST+0)
#define PVRSRV_BRIDGE_MODIFY_COMPLETE_SYNC_OPS	PVRSRV_IOWR(PVRSRV_BRIDGE_SYNC_OPS_CMD_FIRST+1)
#define PVRSRV_BRIDGE_SYNC_OPS_CMD_LAST			(PVRSRV_BRIDGE_SYNC_OPS_CMD_FIRST+1)

#define PVRSRV_BRIDGE_LAST_NON_DEVICE_CMD		(PVRSRV_BRIDGE_SYNC_OPS_CMD_LAST+1)


#define PVRSRV_KERNEL_MODE_CLIENT				1

typedef struct PVRSRV_BRIDGE_RETURN_TAG
{
	PVRSRV_ERROR eError;
	IMG_VOID *pvData;

}PVRSRV_BRIDGE_RETURN;


typedef struct PVRSRV_BRIDGE_PACKAGE_TAG
{
	IMG_UINT32				ui32BridgeID;
	IMG_UINT32				ui32Size;
	IMG_VOID				*pvParamIn;
	IMG_UINT32				ui32InBufferSize;
	IMG_VOID				*pvParamOut;
	IMG_UINT32				ui32OutBufferSize;

	IMG_HANDLE				hKernelServices;
}PVRSRV_BRIDGE_PACKAGE;





typedef struct PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_UINT32			uiDevIndex;
	PVRSRV_DEVICE_TYPE	eDeviceType;

} PVRSRV_BRIDGE_IN_ACQUIRE_DEVICEINFO;


typedef struct PVRSRV_BRIDGE_IN_ENUMCLASS_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	PVRSRV_DEVICE_CLASS sDeviceClass;
} PVRSRV_BRIDGE_IN_ENUMCLASS;


typedef struct PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_CLOSE_DISPCLASS_DEVICE;


typedef struct PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_FORMATS;


typedef struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_GET_DISPCLASS_SYSBUFFER;


typedef struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_GET_DISPCLASS_INFO;


typedef struct PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_CLOSE_BUFFERCLASS_DEVICE;


typedef struct PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
} PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_INFO;



typedef struct PVRSRV_BRIDGE_IN_RELEASE_DEVICEINFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;

} PVRSRV_BRIDGE_IN_RELEASE_DEVICEINFO;



typedef struct PVRSRV_BRIDGE_IN_FREE_CLASSDEVICEINFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	PVRSRV_DEVICE_CLASS DeviceClass;
	IMG_VOID*			pvDevInfo;

}PVRSRV_BRIDGE_IN_FREE_CLASSDEVICEINFO;



typedef struct PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_HANDLE 			hDevMemContext;

}PVRSRV_BRIDGE_IN_GET_DEVMEM_HEAPINFO;



typedef struct PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;

}PVRSRV_BRIDGE_IN_CREATE_DEVMEMCONTEXT;



typedef struct PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE 			hDevCookie;
	IMG_HANDLE 			hDevMemContext;

}PVRSRV_BRIDGE_IN_DESTROY_DEVMEMCONTEXT;



typedef struct PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_HANDLE			hDevMemHeap;
	IMG_UINT32			ui32Attribs;
	IMG_SIZE_T			ui32Size;
	IMG_SIZE_T			ui32Alignment;

}PVRSRV_BRIDGE_IN_ALLOCDEVICEMEM;


typedef struct PVRSRV_BRIDGE_IN_MAPMEMINFOTOUSER_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;

}PVRSRV_BRIDGE_IN_MAPMEMINFOTOUSER;


typedef struct PVRSRV_BRIDGE_IN_UNMAPMEMINFOFROMUSER_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	IMG_PVOID				 pvLinAddr;
	IMG_HANDLE				 hMappingInfo;

}PVRSRV_BRIDGE_IN_UNMAPMEMINFOFROMUSER;


typedef struct PVRSRV_BRIDGE_IN_FREEDEVICEMEM_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	IMG_HANDLE				hDevCookie;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;

}PVRSRV_BRIDGE_IN_FREEDEVICEMEM;


typedef struct PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	IMG_HANDLE				hDevCookie;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;

}PVRSRV_BRIDGE_IN_EXPORTDEVICEMEM;


typedef struct PVRSRV_BRIDGE_IN_GETFREEDEVICEMEM_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_UINT32			ui32Flags;

} PVRSRV_BRIDGE_IN_GETFREEDEVICEMEM;


typedef struct PVRSRV_BRIDGE_IN_CREATECOMMANDQUEUE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_SIZE_T			ui32QueueSize;

}PVRSRV_BRIDGE_IN_CREATECOMMANDQUEUE;



typedef struct PVRSRV_BRIDGE_IN_DESTROYCOMMANDQUEUE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	PVRSRV_QUEUE_INFO	*psQueueInfo;

}PVRSRV_BRIDGE_IN_DESTROYCOMMANDQUEUE;



typedef struct PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hMHandle;
} PVRSRV_BRIDGE_IN_MHANDLE_TO_MMAP_DATA;



typedef struct PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hMHandle;
} PVRSRV_BRIDGE_IN_RELEASE_MMAP_DATA;



typedef struct PVRSRV_BRIDGE_IN_RESERVE_DEV_VIRTMEM_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevMemHeap;
	IMG_DEV_VIRTADDR	*psDevVAddr;
	IMG_SIZE_T			ui32Size;
	IMG_SIZE_T			ui32Alignment;

}PVRSRV_BRIDGE_IN_RESERVE_DEV_VIRTMEM;


typedef struct PVRSRV_BRIDGE_OUT_CONNECT_SERVICES_TAG
{
	PVRSRV_ERROR 			eError;
	IMG_HANDLE		hKernelServices;
}PVRSRV_BRIDGE_OUT_CONNECT_SERVICES;


typedef struct PVRSRV_BRIDGE_OUT_RESERVE_DEV_VIRTMEM_TAG
{
	PVRSRV_ERROR 			eError;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;

}PVRSRV_BRIDGE_OUT_RESERVE_DEV_VIRTMEM;



typedef struct PVRSRV_BRIDGE_IN_FREE_DEV_VIRTMEM_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;

}PVRSRV_BRIDGE_IN_FREE_DEV_VIRTMEM;



typedef struct PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	IMG_HANDLE				hKernelMemInfo;
	IMG_HANDLE				hDstDevMemHeap;

}PVRSRV_BRIDGE_IN_MAP_DEV_MEMORY;



typedef struct PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY_TAG
{
	PVRSRV_ERROR			eError;
	PVRSRV_KERNEL_MEM_INFO	*psDstKernelMemInfo;
	PVRSRV_KERNEL_SYNC_INFO	*psDstKernelSyncInfo;
	PVRSRV_CLIENT_MEM_INFO	sDstClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sDstClientSyncInfo;

}PVRSRV_BRIDGE_OUT_MAP_DEV_MEMORY;



typedef struct PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY_TAG
{
	IMG_UINT32					ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO		*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO		sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO		sClientSyncInfo;

}PVRSRV_BRIDGE_IN_UNMAP_DEV_MEMORY;



typedef struct PVRSRV_BRIDGE_IN_MAP_EXT_MEMORY_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	IMG_SYS_PHYADDR			*psSysPAddr;
	IMG_UINT32				ui32Flags;

}PVRSRV_BRIDGE_IN_MAP_EXT_MEMORY;


typedef struct PVRSRV_BRIDGE_IN_UNMAP_EXT_MEMORY_TAG
{
	IMG_UINT32					ui32BridgeFlags;
	PVRSRV_CLIENT_MEM_INFO		sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO		sClientSyncInfo;
	IMG_UINT32					ui32Flags;

}PVRSRV_BRIDGE_IN_UNMAP_EXT_MEMORY;


typedef struct PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY_TAG
{
	IMG_UINT32					ui32BridgeFlags;
	IMG_HANDLE		hDeviceClassBuffer;
	IMG_HANDLE		hDevMemContext;

}PVRSRV_BRIDGE_IN_MAP_DEVICECLASS_MEMORY;



typedef struct PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY_TAG
{
	PVRSRV_ERROR			eError;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO sClientSyncInfo;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	IMG_HANDLE				hMappingInfo;

}PVRSRV_BRIDGE_OUT_MAP_DEVICECLASS_MEMORY;



typedef struct PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;

}PVRSRV_BRIDGE_IN_UNMAP_DEVICECLASS_MEMORY;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_MEMPOL_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	IMG_UINT32 ui32Offset;
	IMG_UINT32 ui32Value;
	IMG_UINT32 ui32Mask;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_MEMPOL;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	IMG_BOOL bIsRead;
	IMG_UINT32 ui32Value;
	IMG_UINT32 ui32Mask;

}PVRSRV_BRIDGE_IN_PDUMP_SYNCPOL;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_PVOID pvLinAddr;
	IMG_PVOID pvAltLinAddr;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	IMG_UINT32 ui32Offset;
	IMG_UINT32 ui32Bytes;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_DUMPMEM;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_PVOID pvAltLinAddr;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	IMG_UINT32 ui32Offset;
	IMG_UINT32 ui32Bytes;

}PVRSRV_BRIDGE_IN_PDUMP_DUMPSYNC;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_DUMPREG_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_HWREG sHWReg;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_DUMPREG;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_REGPOL_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_HWREG sHWReg;
	IMG_UINT32 ui32Mask;
	IMG_UINT32 ui32Flags;
}PVRSRV_BRIDGE_IN_PDUMP_REGPOL;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_DUMPPDREG_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_HWREG sHWReg;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_DUMPPDREG;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_MEMPAGES_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hKernelMemInfo;
	IMG_DEV_PHYADDR		*pPages;
	IMG_UINT32			ui32NumPages;
	IMG_DEV_VIRTADDR	sDevAddr;
	IMG_UINT32			ui32Start;
	IMG_UINT32			ui32Length;
	IMG_BOOL			bContinuous;

}PVRSRV_BRIDGE_IN_PDUMP_MEMPAGES;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_COMMENT_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_CHAR szComment[PVRSRV_PDUMP_MAX_COMMENT_SIZE];
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_COMMENT;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_SETFRAME_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_UINT32 ui32Frame;

}PVRSRV_BRIDGE_IN_PDUMP_SETFRAME;




typedef struct PVRSRV_BRIDGE_IN_PDUMP_BITMAP_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_CHAR szFileName[PVRSRV_PDUMP_MAX_FILENAME_SIZE];
	IMG_UINT32 ui32FileOffset;
	IMG_UINT32 ui32Width;
	IMG_UINT32 ui32Height;
	IMG_UINT32 ui32StrideInBytes;
	IMG_DEV_VIRTADDR sDevBaseAddr;
	IMG_UINT32 ui32Size;
	PDUMP_PIXEL_FORMAT ePixelFormat;
	PDUMP_MEM_FORMAT eMemFormat;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_BITMAP;



typedef struct PVRSRV_BRIDGE_IN_PDUMP_READREG_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_CHAR szFileName[PVRSRV_PDUMP_MAX_FILENAME_SIZE];
	IMG_UINT32 ui32FileOffset;
	IMG_UINT32 ui32Address;
	IMG_UINT32 ui32Size;
	IMG_UINT32 ui32Flags;

}PVRSRV_BRIDGE_IN_PDUMP_READREG;


typedef struct PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_CHAR szString[PVRSRV_PDUMP_MAX_COMMENT_SIZE];
	IMG_BOOL bContinuous;

}PVRSRV_BRIDGE_IN_PDUMP_DRIVERINFO;

typedef struct PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE hKernelMemInfo;
	IMG_UINT32 ui32Offset;
	IMG_DEV_PHYADDR sPDDevPAddr;
}PVRSRV_BRIDGE_IN_PDUMP_DUMPPDDEVPADDR;


typedef struct PVRSRV_BRIDGE_PDUM_IN_CYCLE_COUNT_REG_READ_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_UINT32 ui32RegOffset;
	IMG_BOOL bLastFrame;
}PVRSRV_BRIDGE_IN_PDUMP_CYCLE_COUNT_REG_READ;


typedef struct PVRSRV_BRIDGE_OUT_ENUMDEVICE_TAG
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32NumDevices;
	PVRSRV_DEVICE_IDENTIFIER asDeviceIdentifier[PVRSRV_MAX_DEVICES];

}PVRSRV_BRIDGE_OUT_ENUMDEVICE;



typedef struct PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO_TAG
{

	PVRSRV_ERROR		eError;
	IMG_HANDLE			hDevCookie;

} PVRSRV_BRIDGE_OUT_ACQUIRE_DEVICEINFO;



typedef struct PVRSRV_BRIDGE_OUT_ENUMCLASS_TAG
{
	PVRSRV_ERROR eError;
	IMG_UINT32 ui32NumDevices;
	IMG_UINT32 ui32DevID[PVRSRV_MAX_DEVICES];

}PVRSRV_BRIDGE_OUT_ENUMCLASS;



typedef struct PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE_TAG
{
	IMG_UINT32		ui32BridgeFlags;
	IMG_UINT32		ui32DeviceID;
	IMG_HANDLE		hDevCookie;

}PVRSRV_BRIDGE_IN_OPEN_DISPCLASS_DEVICE;


typedef struct PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE_TAG
{
	PVRSRV_ERROR	eError;
	IMG_HANDLE		hDeviceKM;

}PVRSRV_BRIDGE_OUT_OPEN_DISPCLASS_DEVICE;



typedef struct PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	IMG_HANDLE              hDevCookie;
	IMG_HANDLE				hDevMemContext;
	IMG_VOID				*pvLinAddr;
	IMG_SIZE_T              ui32ByteSize;
	IMG_SIZE_T              ui32PageOffset;
	IMG_BOOL                bPhysContig;
	IMG_UINT32				ui32NumPageTableEntries;
	IMG_SYS_PHYADDR         *psSysPAddr;
	IMG_UINT32				ui32Flags;

}PVRSRV_BRIDGE_IN_WRAP_EXT_MEMORY;


typedef struct PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY_TAG
{
	PVRSRV_ERROR	eError;
	PVRSRV_CLIENT_MEM_INFO  sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;

}PVRSRV_BRIDGE_OUT_WRAP_EXT_MEMORY;


typedef struct PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE hKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO sClientSyncInfo;

}PVRSRV_BRIDGE_IN_UNWRAP_EXT_MEMORY;


#define PVRSRV_MAX_DC_DISPLAY_FORMATS			10
#define PVRSRV_MAX_DC_DISPLAY_DIMENSIONS		10
#define PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS			4
#define PVRSRV_MAX_DC_CLIP_RECTS				32


typedef struct PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS_TAG
{
	PVRSRV_ERROR	eError;
	IMG_UINT32		ui32Count;
	DISPLAY_FORMAT	asFormat[PVRSRV_MAX_DC_DISPLAY_FORMATS];

}PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_FORMATS;



typedef struct PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS_TAG
{
	IMG_UINT32		ui32BridgeFlags;
	IMG_HANDLE		hDeviceKM;
	DISPLAY_FORMAT	sFormat;

}PVRSRV_BRIDGE_IN_ENUM_DISPCLASS_DIMS;



typedef struct PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS_TAG
{
	PVRSRV_ERROR	eError;
	IMG_UINT32		ui32Count;
	DISPLAY_DIMS	asDim[PVRSRV_MAX_DC_DISPLAY_DIMENSIONS];

}PVRSRV_BRIDGE_OUT_ENUM_DISPCLASS_DIMS;



typedef struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO_TAG
{
	PVRSRV_ERROR	eError;
	DISPLAY_INFO	sDisplayInfo;

}PVRSRV_BRIDGE_OUT_GET_DISPCLASS_INFO;



typedef struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER_TAG
{
	PVRSRV_ERROR	eError;
	IMG_HANDLE		hBuffer;

}PVRSRV_BRIDGE_OUT_GET_DISPCLASS_SYSBUFFER;



typedef struct PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	IMG_HANDLE				hDeviceKM;
	IMG_UINT32				ui32Flags;
	DISPLAY_SURF_ATTRIBUTES	sDstSurfAttrib;
	DISPLAY_SURF_ATTRIBUTES	sSrcSurfAttrib;
	IMG_UINT32				ui32BufferCount;
	IMG_UINT32				ui32OEMFlags;
	IMG_UINT32				ui32SwapChainID;

} PVRSRV_BRIDGE_IN_CREATE_DISPCLASS_SWAPCHAIN;



typedef struct PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN_TAG
{
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hSwapChain;
	IMG_UINT32			ui32SwapChainID;

} PVRSRV_BRIDGE_OUT_CREATE_DISPCLASS_SWAPCHAIN;



typedef struct PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hSwapChain;

} PVRSRV_BRIDGE_IN_DESTROY_DISPCLASS_SWAPCHAIN;



typedef struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hSwapChain;
	IMG_RECT			sRect;

} PVRSRV_BRIDGE_IN_SET_DISPCLASS_RECT;



typedef struct PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hSwapChain;
	IMG_UINT32			ui32CKColour;

} PVRSRV_BRIDGE_IN_SET_DISPCLASS_COLOURKEY;



typedef struct PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hSwapChain;

} PVRSRV_BRIDGE_IN_GET_DISPCLASS_BUFFERS;



typedef struct PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS_TAG
{
	PVRSRV_ERROR		eError;
	IMG_UINT32			ui32BufferCount;
	IMG_HANDLE			ahBuffer[PVRSRV_MAX_DC_SWAPCHAIN_BUFFERS];

} PVRSRV_BRIDGE_OUT_GET_DISPCLASS_BUFFERS;



typedef struct PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hBuffer;
	IMG_UINT32			ui32SwapInterval;
	IMG_HANDLE			hPrivateTag;
	IMG_UINT32			ui32ClipRectCount;
	IMG_RECT			sClipRect[PVRSRV_MAX_DC_CLIP_RECTS];

} PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_BUFFER;



typedef struct PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_HANDLE			hSwapChain;

} PVRSRV_BRIDGE_IN_SWAP_DISPCLASS_TO_SYSTEM;



typedef struct PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_UINT32			ui32DeviceID;
	IMG_HANDLE			hDevCookie;

} PVRSRV_BRIDGE_IN_OPEN_BUFFERCLASS_DEVICE;



typedef struct PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE_TAG
{
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hDeviceKM;

} PVRSRV_BRIDGE_OUT_OPEN_BUFFERCLASS_DEVICE;



typedef struct PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO_TAG
{
	PVRSRV_ERROR		eError;
	BUFFER_INFO			sBufferInfo;

} PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_INFO;



typedef struct PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDeviceKM;
	IMG_UINT32			ui32BufferIndex;

} PVRSRV_BRIDGE_IN_GET_BUFFERCLASS_BUFFER;



typedef struct PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER_TAG
{
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hBuffer;

} PVRSRV_BRIDGE_OUT_GET_BUFFERCLASS_BUFFER;



typedef struct PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO_TAG
{
	PVRSRV_ERROR		eError;
	IMG_UINT32			ui32ClientHeapCount;
	PVRSRV_HEAP_INFO	sHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];

} PVRSRV_BRIDGE_OUT_GET_DEVMEM_HEAPINFO;



typedef struct PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT_TAG
{
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hDevMemContext;
	IMG_UINT32			ui32ClientHeapCount;
	PVRSRV_HEAP_INFO	sHeapInfo[PVRSRV_MAX_CLIENT_HEAPS];

} PVRSRV_BRIDGE_OUT_CREATE_DEVMEMCONTEXT;



typedef struct PVRSRV_BRIDGE_OUT_CREATE_DEVMEMHEAP_TAG
{
	PVRSRV_ERROR		eError;
	IMG_HANDLE			hDevMemHeap;

} PVRSRV_BRIDGE_OUT_CREATE_DEVMEMHEAP;



typedef struct PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM_TAG
{
	PVRSRV_ERROR			eError;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;

} PVRSRV_BRIDGE_OUT_ALLOCDEVICEMEM;



typedef struct PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM_TAG
{
	PVRSRV_ERROR			eError;
	IMG_HANDLE				hMemInfo;
#if defined(SUPPORT_MEMINFO_IDS)
	IMG_UINT64				ui64Stamp;
#endif

} PVRSRV_BRIDGE_OUT_EXPORTDEVICEMEM;


typedef struct PVRSRV_BRIDGE_OUT_MAPMEMINFOTOUSER_TAG
{
	PVRSRV_ERROR			eError;
	IMG_PVOID				pvLinAddr;
	IMG_HANDLE				hMappingInfo;

}PVRSRV_BRIDGE_OUT_MAPMEMINFOTOUSER;



typedef struct PVRSRV_BRIDGE_OUT_GETFREEDEVICEMEM_TAG
{
	PVRSRV_ERROR eError;
	IMG_SIZE_T ui32Total;
	IMG_SIZE_T ui32Free;
	IMG_SIZE_T ui32LargestBlock;

} PVRSRV_BRIDGE_OUT_GETFREEDEVICEMEM;


#include "pvrmmap.h"
typedef struct PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA_TAG
{
    PVRSRV_ERROR		eError;


     IMG_UINT32			ui32MMapOffset;


    IMG_UINT32			ui32ByteOffset;


    IMG_UINT32 			ui32RealByteSize;


    IMG_UINT32			ui32UserVAddr;

} PVRSRV_BRIDGE_OUT_MHANDLE_TO_MMAP_DATA;

typedef struct PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA_TAG
{
    PVRSRV_ERROR		eError;


    IMG_BOOL			bMUnmap;


    IMG_UINT32			ui32UserVAddr;


    IMG_UINT32			ui32RealByteSize;
} PVRSRV_BRIDGE_OUT_RELEASE_MMAP_DATA;

typedef struct PVRSRV_BRIDGE_IN_GET_MISC_INFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	PVRSRV_MISC_INFO	sMiscInfo;

}PVRSRV_BRIDGE_IN_GET_MISC_INFO;



typedef struct PVRSRV_BRIDGE_OUT_GET_MISC_INFO_TAG
{
	PVRSRV_ERROR		eError;
	PVRSRV_MISC_INFO	sMiscInfo;

}PVRSRV_BRIDGE_OUT_GET_MISC_INFO;



typedef struct PVRSRV_BRIDGE_IN_RELEASE_MISC_INFO_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	PVRSRV_MISC_INFO	sMiscInfo;

}PVRSRV_BRIDGE_IN_RELEASE_MISC_INFO;



typedef struct PVRSRV_BRIDGE_OUT_RELEASE_MISC_INFO_TAG
{
	PVRSRV_ERROR		eError;
	PVRSRV_MISC_INFO	sMiscInfo;

}PVRSRV_BRIDGE_OUT_RELEASE_MISC_INFO;




typedef struct PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING_TAG
{
	PVRSRV_ERROR eError;
	IMG_BOOL bIsCapturing;

} PVRSRV_BRIDGE_OUT_PDUMP_ISCAPTURING;


typedef struct PVRSRV_BRIDGE_IN_GET_FB_STATS_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_SIZE_T ui32Total;
	IMG_SIZE_T ui32Available;

} PVRSRV_BRIDGE_IN_GET_FB_STATS;



typedef struct PVRSRV_BRIDGE_IN_MAPPHYSTOUSERSPACE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_SYS_PHYADDR		sSysPhysAddr;
	IMG_UINT32			uiSizeInBytes;

} PVRSRV_BRIDGE_IN_MAPPHYSTOUSERSPACE;



typedef struct PVRSRV_BRIDGE_OUT_MAPPHYSTOUSERSPACE_TAG
{
	IMG_PVOID			pvUserAddr;
	IMG_UINT32			uiActualSize;
	IMG_PVOID			pvProcess;

} PVRSRV_BRIDGE_OUT_MAPPHYSTOUSERSPACE;



typedef struct PVRSRV_BRIDGE_IN_UNMAPPHYSTOUSERSPACE_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_PVOID			pvUserAddr;
	IMG_PVOID			pvProcess;

} PVRSRV_BRIDGE_IN_UNMAPPHYSTOUSERSPACE;



typedef struct PVRSRV_BRIDGE_OUT_GETPHYSTOUSERSPACEMAP_TAG
{
	IMG_PVOID			*ppvTbl;
	IMG_UINT32			uiTblSize;

} PVRSRV_BRIDGE_OUT_GETPHYSTOUSERSPACEMAP;



typedef struct PVRSRV_BRIDGE_IN_REGISTER_SIM_PROCESS_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_PVOID			pvProcess;

} PVRSRV_BRIDGE_IN_REGISTER_SIM_PROCESS;


typedef struct PVRSRV_BRIDGE_OUT_REGISTER_SIM_PROCESS_TAG
{
	IMG_SYS_PHYADDR		sRegsPhysBase;
	IMG_VOID			*pvRegsBase;
	IMG_PVOID			pvProcess;
	IMG_UINT32			ulNoOfEntries;
	IMG_PVOID			pvTblLinAddr;

} PVRSRV_BRIDGE_OUT_REGISTER_SIM_PROCESS;


typedef struct PVRSRV_BRIDGE_IN_UNREGISTER_SIM_PROCESS_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_PVOID			pvProcess;
	IMG_VOID			*pvRegsBase;

} PVRSRV_BRIDGE_IN_UNREGISTER_SIM_PROCESS;

typedef struct PVRSRV_BRIDGE_IN_PROCESS_SIMISR_EVENT_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_HANDLE			hDevCookie;
	IMG_UINT32			ui32StatusAndMask;
	PVRSRV_ERROR 		eError;

} PVRSRV_BRIDGE_IN_PROCESS_SIMISR_EVENT;

typedef struct PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT_TAG
{
	IMG_UINT32			ui32BridgeFlags;
	IMG_BOOL			bInitSuccesful;
} PVRSRV_BRIDGE_IN_INITSRV_DISCONNECT;


typedef struct PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM_TAG
{
	IMG_UINT32 ui32BridgeFlags;
    IMG_UINT32 ui32Flags;
    IMG_SIZE_T ui32Size;
}PVRSRV_BRIDGE_IN_ALLOC_SHARED_SYS_MEM;

typedef struct PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM_TAG
{
	PVRSRV_ERROR			eError;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
}PVRSRV_BRIDGE_OUT_ALLOC_SHARED_SYS_MEM;

typedef struct PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM_TAG
{
	IMG_UINT32				ui32BridgeFlags;
	PVRSRV_KERNEL_MEM_INFO	*psKernelMemInfo;
	PVRSRV_CLIENT_MEM_INFO	sClientMemInfo;
}PVRSRV_BRIDGE_IN_FREE_SHARED_SYS_MEM;

typedef struct PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM_TAG
{
	PVRSRV_ERROR eError;
}PVRSRV_BRIDGE_OUT_FREE_SHARED_SYS_MEM;

typedef struct PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE hKernelMemInfo;
}PVRSRV_BRIDGE_IN_MAP_MEMINFO_MEM;

typedef struct PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM_TAG
{
	PVRSRV_CLIENT_MEM_INFO  sClientMemInfo;
	PVRSRV_CLIENT_SYNC_INFO	sClientSyncInfo;
	PVRSRV_KERNEL_MEM_INFO  *psKernelMemInfo;
	PVRSRV_KERNEL_SYNC_INFO	*psKernelSyncInfo;
	PVRSRV_ERROR eError;
}PVRSRV_BRIDGE_OUT_MAP_MEMINFO_MEM;

typedef struct PVRSRV_BRIDGE_IN_UNMAP_MEMINFO_MEM_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	PVRSRV_CLIENT_MEM_INFO sClientMemInfo;
}PVRSRV_BRIDGE_IN_UNMAP_MEMINFO_MEM;

typedef struct PVRSRV_BRIDGE_OUT_UNMAP_MEMINFO_MEM_TAG
{
	PVRSRV_ERROR eError;
}PVRSRV_BRIDGE_OUT_UNMAP_MEMINFO_MEM;

typedef struct PVRSRV_BRIDGE_IN_GETMMU_PD_DEVPADDR_TAG
{
	IMG_UINT32 ui32BridgeFlags;
    IMG_HANDLE hDevMemContext;
}PVRSRV_BRIDGE_IN_GETMMU_PD_DEVPADDR;

typedef struct PVRSRV_BRIDGE_OUT_GETMMU_PD_DEVPADDR_TAG
{
    IMG_DEV_PHYADDR sPDDevPAddr;
	PVRSRV_ERROR eError;
}PVRSRV_BRIDGE_OUT_GETMMU_PD_DEVPADDR;

typedef struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAI_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE	hOSEventKM;
} PVRSRV_BRIDGE_IN_EVENT_OBJECT_WAIT;

typedef struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN_TAG
{
	PVRSRV_EVENTOBJECT sEventObject;
} PVRSRV_BRIDGE_IN_EVENT_OBJECT_OPEN;

typedef struct	PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN_TAG
{
	IMG_HANDLE hOSEvent;
	PVRSRV_ERROR eError;
} PVRSRV_BRIDGE_OUT_EVENT_OBJECT_OPEN;

typedef struct PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE_TAG
{
	PVRSRV_EVENTOBJECT sEventObject;
	IMG_HANDLE hOSEventKM;
} PVRSRV_BRIDGE_IN_EVENT_OBJECT_CLOSE;

typedef struct PVRSRV_BRIDGE_IN_MODIFY_PENDING_SYNC_OPS_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE hKernelSyncInfo;
	IMG_UINT32 ui32ModifyFlags;

} PVRSRV_BRIDGE_IN_MODIFY_PENDING_SYNC_OPS;

typedef struct PVRSRV_BRIDGE_IN_MODIFY_COMPLETE_SYNC_OPS_TAG
{
	IMG_UINT32 ui32BridgeFlags;
	IMG_HANDLE hKernelSyncInfo;
	IMG_UINT32 ui32ModifyFlags;

} PVRSRV_BRIDGE_IN_MODIFY_COMPLETE_SYNC_OPS;

typedef struct PVRSRV_BRIDGE_OUT_MODIFY_PENDING_SYNC_OPS_TAG
{
	PVRSRV_ERROR eError;


	IMG_UINT32 ui32ReadOpsPending;
	IMG_UINT32 ui32WriteOpsPending;

} PVRSRV_BRIDGE_OUT_MODIFY_PENDING_SYNC_OPS;

#if defined (__cplusplus)
}
#endif

#endif

