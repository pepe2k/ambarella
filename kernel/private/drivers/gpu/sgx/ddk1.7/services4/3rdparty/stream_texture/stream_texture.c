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

#if defined(__linux__)
#include <linux/string.h>
#else
#include <string.h>
#endif

#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include "stream_texture.h"
#include <plat/ambcache.h>
#include <plat/atag.h>

#define BUFFERCLASS_DEVICE_NAME		"Stream Texture"

static STREAM_TEXTURE_DEVINFO		*DevInfo[MAX_STREAM_DEVICES];
static PVRSRV_BC_BUFFER2SRV_KMJTABLE	PVRJTable;
static PVRSRV_BC_SRV2BUFFER_KMJTABLE	BCJTable;
static STREAM_TEXTURE_BUFFER		mapped_buffer;
static					DEFINE_MUTEX(mmap_mtx);
static					DEFINE_MUTEX(dev_mtx);

int create_stream_device(STREAM_TEXTURE_DEVINFO __user *arg)
{
	unsigned int		dev_id, i = 0;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_DEVINFO	user_info;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_info, arg, sizeof(user_info));
	if (eError) {
		printk("Error: %s %d\n", __func__, __LINE__);
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	for (dev_id = 0; dev_id < MAX_STREAM_DEVICES; dev_id++) {
		if (DevInfo[dev_id] == NULL) {
			break;
		}
	}
	if (dev_id >= MAX_STREAM_DEVICES) {
		eError = BCE_ERROR_DEVICE_REGISTER_FAILED;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

	psDevInfo = (STREAM_TEXTURE_DEVINFO *)BCAllocKernelMem(sizeof(*psDevInfo));
	if (psDevInfo == NULL) {
		eError = BCE_ERROR_OUT_OF_MEMORY;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

	psBuffer = BCAllocKernelMem(sizeof(*psBuffer) * user_info.ulNumBuffers);
	if (psBuffer == NULL) {
		eError = BCE_ERROR_OUT_OF_MEMORY;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

	psDevInfo->ulRefCount				= 0;
 	psDevInfo->ulNumBuffers				= user_info.ulNumBuffers;
	psDevInfo->sBufferInfo.pixelformat		= user_info.sBufferInfo.pixelformat;
	psDevInfo->sBufferInfo.ui32Width		= user_info.sBufferInfo.ui32Width;
	psDevInfo->sBufferInfo.ui32Height		= user_info.sBufferInfo.ui32Height;
	psDevInfo->sBufferInfo.ui32ByteStride		= user_info.sBufferInfo.ui32ByteStride;
	psDevInfo->sBufferInfo.ui32BufferDeviceID	= dev_id;
	psDevInfo->sBufferInfo.ui32Flags		= user_info.sBufferInfo.ui32Flags;
	psDevInfo->sBufferInfo.ui32BufferCount		= user_info.ulNumBuffers;
	strncpy(psDevInfo->sBufferInfo.szDeviceName, user_info.sBufferInfo.szDeviceName, strlen(user_info.sBufferInfo.szDeviceName));

	eError = copy_from_user(psBuffer, user_info.psSystemBuffer, sizeof(*psBuffer) * user_info.ulNumBuffers);
	if (eError) {
		eError = BCE_ERROR_INVALID_PARAMS;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	for (i = 0; i < user_info.ulNumBuffers; i++) {
		if (i + 1 < user_info.ulNumBuffers) {
			psBuffer[i].psNext = &psBuffer[i + 1];
		} else {
			psBuffer[i].psNext = &psBuffer[0];
		}
		psBuffer[i].sCPUVAddr = (void *)ambarella_phys_to_virt(psBuffer[i].sSysAddr.uiAddr);
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	for (i = 0; i < user_info.ulNumBuffers; i++) {
		if (i + 1 < user_info.ulNumBuffers) {
			psBuffer[i].psNext = &psBuffer[i + 1];
		} else {
			psBuffer[i].psNext = &psBuffer[0];
		}
		eError = BCAllocContigMemory(psBuffer[i].ulSize, NULL, &psBuffer[i].sCPUVAddr, &psBuffer[i].sSysAddr);
		if (eError) {
			int	j;
			for (j = 0; j < i; j++) {
				BCFreeContigMemory(psBuffer[j].ulSize, NULL, &psBuffer[j].sCPUVAddr, psBuffer[j].sSysAddr);
			}
			printk("Error: %s %d\n", __func__, __LINE__);
			goto create_stream_device_exit;
		} else {
			psBuffer[i].sPageAlignSysAddr.uiAddr = psBuffer[i].sSysAddr.uiAddr & 0xfffff000;
		}
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	for (i = 0; i < user_info.ulNumBuffers; i++) {
		if (i + 1 < user_info.ulNumBuffers) {
			psBuffer[i].psNext = &psBuffer[i + 1];
		} else {
			psBuffer[i].psNext = &psBuffer[0];
		}
		eError = BCAllocDiscontigMemory(psBuffer[i].ulSize, NULL, &psBuffer[i].sCPUVAddr, &psBuffer[i].psSysAddr);
		if (eError) {
			int	j;
			for (j = 0; j < i; j++) {
				BCFreeDiscontigMemory(psBuffer[j].ulSize, NULL, &psBuffer[j].sCPUVAddr, psBuffer[j].psSysAddr);
			}
			printk("Error: %s %d\n", __func__, __LINE__);
			goto create_stream_device_exit;
		}
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	for (i = 0; i < user_info.ulNumBuffers; i++) {
		if (i + 1 < user_info.ulNumBuffers) {
			psBuffer[i].psNext = &psBuffer[i + 1];
		} else {
			psBuffer[i].psNext = &psBuffer[0];
		}
		eError = BCAllocATTMemory(psBuffer[i].ulSize, NULL, &psBuffer[i].sCPUVAddr, &psBuffer[i].psSysAddr);
		if (eError) {
			int	j;
			for (j = 0; j < i; j++) {
				BCFreeATTMemory(psBuffer[j].ulSize, NULL, &psBuffer[j].sCPUVAddr, psBuffer[j].psSysAddr);
			}
			printk("Error: %s %d\n", __func__, __LINE__);
			goto create_stream_device_exit;
		}
	}
#endif

	psDevInfo->psSystemBuffer	= psBuffer;

	user_info.sBufferInfo.ui32BufferDeviceID = dev_id;
	eError = copy_to_user(arg, &user_info, sizeof(user_info));
	if (eError) {
		eError = BCE_ERROR_INVALID_PARAMS;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

	eError = PVRJTable.pfnPVRSRVRegisterBCDevice(&BCJTable, (IMG_UINT32 *)&psDevInfo->ulDeviceID);
	if (eError) {
		eError = BCE_ERROR_DEVICE_REGISTER_FAILED;
		printk("Error: %s %d\n", __func__, __LINE__);
		goto create_stream_device_exit;
	}

	psDevInfo->ulRefCount++;
	DevInfo[dev_id] = psDevInfo;

create_stream_device_exit:
	if (eError != BCE_OK && psDevInfo != NULL) {
		if (psBuffer) {
			BCFreeKernelMem(psBuffer);
		}
		BCFreeKernelMem(psDevInfo);
	}
	mutex_unlock(&dev_mtx);
	return eError;
}

int destroy_stream_device(unsigned int arg)
{
	unsigned int		dev_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	dev_id = arg;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];
	if (psDevInfo) {
		PVRJTable.pfnPVRSRVRemoveBCDevice(psDevInfo->ulDeviceID);
		psBuffer = psDevInfo->psSystemBuffer;
		if (psBuffer) {
			BCFreeKernelMem(psBuffer);
		}
		BCFreeKernelMem(psDevInfo);
	}

	DevInfo[dev_id] = NULL;
	mutex_unlock(&dev_mtx);
	return eError;
}

int get_stream_device(STREAM_TEXTURE_DEVINFO __user *arg)
{
	unsigned int		dev_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_DEVINFO	user_info;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_info, arg, sizeof(user_info));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_info.sBufferInfo.ui32BufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_device_exit;
	}

	eError = copy_to_user(arg, &user_info, sizeof(user_info));
	if (eError) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_device_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		goto get_stream_device_exit;
	}

	eError = copy_to_user(user_info.psSystemBuffer, psBuffer, sizeof(*psBuffer) * psDevInfo->ulNumBuffers);
	if (eError) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_device_exit;
	}

get_stream_device_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED

int set_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	psBuffer[buf_id].sSysAddr = user_buffer.sSysAddr;
	psBuffer[buf_id].sPageAlignSysAddr = user_buffer.sPageAlignSysAddr;
	psBuffer[buf_id].sCPUVAddr = (void *)ambarella_phys_to_virt(psBuffer[buf_id].sSysAddr.uiAddr);
	ambcache_clean_range((void *)psBuffer[buf_id].sCPUVAddr, psBuffer[buf_id].ulSize);

set_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC

int set_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	ambcache_clean_range((void *)psBuffer[buf_id].sCPUVAddr, psBuffer[buf_id].ulSize);

set_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC

int set_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	ambcache_clean_range((void *)psBuffer[buf_id].sCPUVAddr, psBuffer[buf_id].ulSize);

set_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT

#define RANGE_TO_PAGES(range)		(((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

int set_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id, i;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto set_stream_buffer_exit;
	}

	for (i = 0; i < RANGE_TO_PAGES(psBuffer[buf_id].ulSize); i++) {
		ambcache_clean_range(phys_to_virt(psBuffer[buf_id].psSysAddr[i].uiAddr), PAGE_SIZE);
	}

set_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

#endif

int get_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_buffer_exit;
	}

	eError = copy_to_user(arg, &psBuffer[buf_id], sizeof(*psBuffer));
	if (eError) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto get_stream_buffer_exit;
	}

get_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

int map_stream_buffer(STREAM_TEXTURE_BUFFER __user *arg)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_BUFFER	user_buffer;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;

	eError = copy_from_user(&user_buffer, arg, sizeof(user_buffer));
	if (eError) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	dev_id = user_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto map_stream_buffer_exit;
	}

	buf_id = user_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto map_stream_buffer_exit;
	}

	mutex_lock(&mmap_mtx);
	mapped_buffer = user_buffer;

map_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

int mmap_stream_buffer(struct file *filp, struct vm_area_struct *vma)
{
	unsigned int		dev_id, buf_id;
	BCE_ERROR		eError = BCE_OK;
	STREAM_TEXTURE_DEVINFO	*psDevInfo = NULL;
	STREAM_TEXTURE_BUFFER	*psBuffer = NULL;

	dev_id = mapped_buffer.ulBufferDeviceID;
	if (dev_id >= MAX_STREAM_DEVICES) {
		mutex_unlock(&mmap_mtx);
		return BCE_ERROR_INVALID_PARAMS;
	}

	mutex_lock(&dev_mtx);
	psDevInfo = DevInfo[dev_id];

	if (psDevInfo == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto mmap_stream_buffer_exit;
	}

	buf_id = mapped_buffer.ulBufferID;
	if (buf_id >= psDevInfo->ulNumBuffers) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto mmap_stream_buffer_exit;
	}

	psBuffer = psDevInfo->psSystemBuffer;
	if (psBuffer == NULL) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto mmap_stream_buffer_exit;
	}

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,	psBuffer[buf_id].sSysAddr.uiAddr >> PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			eError = BCE_ERROR_INVALID_PARAMS;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,	psBuffer[buf_id].sSysAddr.uiAddr >> PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			eError = BCE_ERROR_INVALID_PARAMS;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_vmalloc_range(vma, psBuffer[buf_id].sCPUVAddr, 0)) {
			eError = BCE_ERROR_INVALID_PARAMS;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
{
	unsigned int		pages, page;
	unsigned int		vstart;

	pages	= (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	vstart	= vma->vm_start;
	for (page = 0; page < pages; page++) {
		if (remap_pfn_range(vma, vstart, psBuffer[buf_id].psSysAddr[page].uiAddr >> PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot))
			break;
		vstart += PAGE_SIZE;
	}

	if (page < pages) {
		eError = BCE_ERROR_INVALID_PARAMS;
	}
}
#endif

mmap_stream_buffer_exit:
	mutex_unlock(&dev_mtx);
	mutex_unlock(&mmap_mtx);
	return eError;
}

static PVRSRV_ERROR OpenBCDevice(IMG_UINT32 ui32DeviceID, IMG_HANDLE *phDevice)
{
	STREAM_TEXTURE_DEVINFO *psDevInfo = NULL;
	unsigned int		dev_id;

	UNREFERENCED_PARAMETER(ui32DeviceID);

	mutex_lock(&dev_mtx);

	for (dev_id = 0; dev_id < MAX_STREAM_DEVICES; dev_id++) {
		psDevInfo = DevInfo[dev_id];
		if (psDevInfo && psDevInfo->ulDeviceID == ui32DeviceID) {
			break;
		} else {
			psDevInfo = NULL;
		}
	}

	*phDevice = (IMG_HANDLE)psDevInfo;

	mutex_unlock(&dev_mtx);

	return (PVRSRV_OK);
}

static PVRSRV_ERROR CloseBCDevice(IMG_UINT32 ui32DeviceID, IMG_HANDLE hDevice)
{
	UNREFERENCED_PARAMETER(ui32DeviceID);
	UNREFERENCED_PARAMETER(hDevice);

	return (PVRSRV_OK);
}

static PVRSRV_ERROR GetBCBuffer(IMG_HANDLE          hDevice,
                                IMG_UINT32          ui32BufferNumber,
                                PVRSRV_SYNC_DATA   *psSyncData,
                                IMG_HANDLE         *phBuffer)
{
	STREAM_TEXTURE_DEVINFO	*psDevInfo;

	if(!hDevice || !phBuffer)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (STREAM_TEXTURE_DEVINFO*)hDevice;

	if( ui32BufferNumber < psDevInfo->sBufferInfo.ui32BufferCount )
	{
		mutex_lock(&dev_mtx);
		psDevInfo->psSystemBuffer[ui32BufferNumber].psSyncData = psSyncData;
		*phBuffer = (IMG_HANDLE)&psDevInfo->psSystemBuffer[ui32BufferNumber];
		mutex_unlock(&dev_mtx);
	}
	else
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	return (PVRSRV_OK);
}

static PVRSRV_ERROR GetBCInfo(IMG_HANDLE hDevice, BUFFER_INFO *psBCInfo)
{
	STREAM_TEXTURE_DEVINFO	*psDevInfo;

	if(!hDevice || !psBCInfo)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psDevInfo = (STREAM_TEXTURE_DEVINFO*)hDevice;
	mutex_lock(&dev_mtx);
	*psBCInfo = psDevInfo->sBufferInfo;
	mutex_unlock(&dev_mtx);

	return (PVRSRV_OK);
}

static PVRSRV_ERROR GetBCBufferAddr(IMG_HANDLE      hDevice,
                                    IMG_HANDLE      hBuffer,
                                    IMG_SYS_PHYADDR **ppsSysAddr,
                                    IMG_UINT32      *pui32ByteSize,
                                    IMG_VOID        **ppvCpuVAddr,
                                    IMG_HANDLE      *phOSMapInfo,
                                    IMG_BOOL        *pbIsContiguous,
                                    IMG_UINT32      *pui32TilingStride)
{
	STREAM_TEXTURE_BUFFER *psBuffer;

	PVR_UNREFERENCED_PARAMETER(pui32TilingStride);

	if(!hDevice || !hBuffer || !ppsSysAddr || !pui32ByteSize)
	{
		return (PVRSRV_ERROR_INVALID_PARAMS);
	}

	psBuffer	= (STREAM_TEXTURE_BUFFER *) hBuffer;
	*ppvCpuVAddr	= psBuffer->sCPUVAddr;

	*phOSMapInfo	= IMG_NULL;
	*pui32ByteSize	= (IMG_UINT32)psBuffer->ulSize;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	*ppsSysAddr	= &psBuffer->sPageAlignSysAddr;
	*pbIsContiguous	= IMG_TRUE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	*ppsSysAddr	= &psBuffer->sPageAlignSysAddr;
	*pbIsContiguous	= IMG_TRUE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	*ppsSysAddr	= psBuffer->psSysAddr;
	*pbIsContiguous	= IMG_FALSE;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	*ppsSysAddr	= psBuffer->psSysAddr;
	*pbIsContiguous	= IMG_FALSE;
#endif

	return (PVRSRV_OK);
}


BCE_ERROR __stream_texture_init(void)
{
	BCE_ERROR		eError = BCE_OK;
	unsigned int		i;

	mutex_lock(&dev_mtx);

	for (i = 0; i < MAX_STREAM_DEVICES; i++) {
		DevInfo[i] = NULL;
	}

	if(!PVRGetBufferClassJTable(&PVRJTable)) {
		eError = BCE_ERROR_INIT_FAILURE;
		goto __stream_texture_init_exit;
	}

	BCJTable.ui32TableSize    = sizeof(PVRSRV_BC_SRV2BUFFER_KMJTABLE);
	BCJTable.pfnOpenBCDevice  = OpenBCDevice;
	BCJTable.pfnCloseBCDevice = CloseBCDevice;
	BCJTable.pfnGetBCBuffer   = GetBCBuffer;
	BCJTable.pfnGetBCInfo     = GetBCInfo;
	BCJTable.pfnGetBufferAddr = GetBCBufferAddr;

__stream_texture_init_exit:
	mutex_unlock(&dev_mtx);
	return eError;
}

BCE_ERROR __stream_texture_exit(void)
{
	BCE_ERROR		eError = BCE_OK;
	unsigned int		i;
	STREAM_TEXTURE_DEVINFO	*psDevInfo;

	mutex_lock(&dev_mtx);

	for (i = 0; i < MAX_STREAM_DEVICES; i++) {
		psDevInfo = DevInfo[i];
		if (psDevInfo) {
			PVRJTable.pfnPVRSRVRemoveBCDevice(psDevInfo->ulDeviceID);
			if (psDevInfo->psSystemBuffer) {
				BCFreeKernelMem(psDevInfo->psSystemBuffer);
			}
			BCFreeKernelMem(psDevInfo);
		}
		DevInfo[i] = NULL;
	}

	mutex_unlock(&dev_mtx);
	return eError;
}

