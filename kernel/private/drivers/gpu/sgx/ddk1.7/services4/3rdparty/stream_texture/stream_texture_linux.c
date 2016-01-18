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
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>

#include "stream_texture.h"
#include "pvrmodule.h"

#define DEVNAME		"stream_texture"
#define	DRVNAME		DEVNAME

#if defined(BCE_USE_SET_MEMORY)
#undef BCE_USE_SET_MEMORY
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

static					DEFINE_MUTEX(mmap_dev_mtx);
int					mmap_dev;

long stream_texture_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
int stream_texture_mmap(struct file *filp, struct vm_area_struct *vma);

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
static struct class	*psPvrClass;
#endif

static int		major;

static struct file_operations stream_texture_fops = {
	.unlocked_ioctl	= stream_texture_ioctl,
	.mmap		= stream_texture_mmap,
};

#define unref__ __attribute__ ((unused))

static int __init stream_texture_init(void)
{
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
    struct device *psDev;
#endif

	major = register_chrdev(0, DEVNAME, &stream_texture_fops);

	if (major <= 0)
	{
		printk(KERN_ERR DRVNAME ": %s: unable to get major number\n", __func__);

		goto ExitDisable;
	}

#if defined(DEBUG)
	printk(KERN_ERR DRVNAME ": %s: major device %d\n", __func__, major);
#endif

#if defined(LDM_PLATFORM) || defined(LDM_PCI)

	psPvrClass = class_create(THIS_MODULE, "stream_texture");

	if (IS_ERR(psPvrClass))
	{
		printk(KERN_ERR DRVNAME ": %s: unable to create class (%ld)", __func__, PTR_ERR(psPvrClass));
		goto ExitUnregister;
	}

	psDev = device_create(psPvrClass, NULL, MKDEV(major, 0), NULL, DEVNAME);
	if (IS_ERR(psDev))
	{
		printk(KERN_ERR DRVNAME ": %s: unable to create device (%ld)", __func__, PTR_ERR(psDev));
		goto ExitDestroyClass;
	}
#endif

	if(__stream_texture_init() != BCE_OK)
	{
		printk (KERN_ERR DRVNAME ": %s: can't init stream_texture\n", __func__);
		goto ExitUnregister;
	}

	if(__fb2_init() != BCE_OK)
	{
		printk (KERN_ERR DRVNAME ": %s: can't init fb2\n", __func__);
		goto ExitUnregister;
	}

	return 0;

#if defined(LDM_PLATFORM) || defined(LDM_PCI)
ExitDestroyClass:
	class_destroy(psPvrClass);
#endif
ExitUnregister:
	unregister_chrdev(major, DEVNAME);
ExitDisable:
	return -EBUSY;
}

static void __exit stream_texture_exit(void)
{
#if defined(LDM_PLATFORM) || defined(LDM_PCI)
	device_destroy(psPvrClass, MKDEV(major, 0));
	class_destroy(psPvrClass);
#endif

	unregister_chrdev(major, DEVNAME);

	if(__stream_texture_exit() != BCE_OK)
	{
		printk (KERN_ERR DRVNAME ": stream_texture_exit: can't deinit device\n");
	}

}

void *BCAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void BCFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC

BCE_ERROR BCAllocContigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR *pPhysAddr)
{
#if defined(BCE_USE_SET_MEMORY)
	void *pvLinAddr;
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);
	int iError;

	pvLinAddr = kmalloc(ulAlignedSize, GFP_KERNEL);
	BUG_ON(((unsigned long)pvLinAddr)  & ~PAGE_MASK);

	iError = set_memory_wc((unsigned long)pvLinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": %s:  set_memory_wc failed (%d)\n", , __func__, iError);
		return (BCE_ERROR_OUT_OF_MEMORY);
	}

	pPhysAddr->uiAddr = virt_to_phys(pvLinAddr);
	*pLinAddr = pvLinAddr;

	return (BCE_OK);
#else
	dma_addr_t dma;
	void *pvLinAddr;

	pvLinAddr = dma_alloc_coherent(NULL, ulSize, &dma, GFP_KERNEL);
	if (pvLinAddr == NULL)
	{
		return (BCE_ERROR_OUT_OF_MEMORY);
	}

	pPhysAddr->uiAddr = dma;
	*pLinAddr = pvLinAddr;

	return (BCE_OK);
#endif
}

void BCFreeContigMemory(unsigned long ulSize,
                        BCE_HANDLE unref__ hMemHandle,
                        IMG_CPU_VIRTADDR LinAddr,
                        IMG_SYS_PHYADDR PhysAddr)
{
#if defined(BCE_USE_SET_MEMORY)
	unsigned long ulAlignedSize = PAGE_ALIGN(ulSize);
	int iError;
	int iPages = (int)(ulAlignedSize >> PAGE_SHIFT);

	iError = set_memory_wb((unsigned long)LinAddr, iPages);
	if (iError != 0)
	{
		printk(KERN_ERR DRVNAME ": %s:  set_memory_wb failed (%d)\n", __func__, iError);
	}
	kfree(LinAddr);
#else
	dma_free_coherent(NULL, ulSize, LinAddr, (dma_addr_t)PhysAddr.uiAddr);
#endif
}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC

#define RANGE_TO_PAGES(range)		(((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)
#define	VMALLOC_TO_PAGE_PHYS(vAddr)	page_to_phys(vmalloc_to_page(vAddr))

BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr)
{
	unsigned long ulPages = RANGE_TO_PAGES(ulSize);
	IMG_SYS_PHYADDR *pPhysAddr;
	unsigned long ulPage;
	IMG_CPU_VIRTADDR LinAddr;

	//LinAddr = __vmalloc(ulSize, GFP_KERNEL | __GFP_HIGHMEM, pgprot_noncached(PAGE_KERNEL));
	LinAddr = vmalloc_user(ulSize);
	if (!LinAddr)
	{
		return BCE_ERROR_OUT_OF_MEMORY;
	}

	pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!pPhysAddr)
	{
		vfree(LinAddr);
		return BCE_ERROR_OUT_OF_MEMORY;
	}

	*pLinAddr = LinAddr;

	for (ulPage = 0; ulPage < ulPages; ulPage++)
	{
		pPhysAddr[ulPage].uiAddr = VMALLOC_TO_PAGE_PHYS(LinAddr);

		LinAddr += PAGE_SIZE;
	}

	*ppPhysAddr = pPhysAddr;

	return BCE_OK;
}

void BCFreeDiscontigMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
	kfree(pPhysAddr);
	vfree(LinAddr);
}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT

#define RANGE_TO_PAGES(range)		(((range) + (PAGE_SIZE - 1)) >> PAGE_SHIFT)

BCE_ERROR BCAllocATTMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr)
{
	unsigned long		ulPages = RANGE_TO_PAGES(ulSize);
	IMG_SYS_PHYADDR		*pPhysAddr;
	unsigned long		ulPage;
	struct page		*page;

	pPhysAddr = kmalloc(ulPages * sizeof(IMG_SYS_PHYADDR), GFP_KERNEL);
	if (!pPhysAddr)
	{
		return BCE_ERROR_OUT_OF_MEMORY;
	}

	for (ulPage = 0; ulPage < ulPages; ulPage++) {
		page = alloc_page(GFP_KERNEL | __GFP_HIGHMEM | __GFP_ZERO);
		if (page == NULL)
			break;
		pPhysAddr[ulPage].uiAddr = page_to_pfn(page) << PAGE_SHIFT;
	}

	if (ulPage < ulPages) {
		for (ulPage--; ulPage >= 0; ulPage--)
			__free_page(pfn_to_page(pPhysAddr[ulPage].uiAddr));

		kfree(pPhysAddr);
		*ppPhysAddr = NULL;

		return BCE_ERROR_OUT_OF_MEMORY;
	} else {
		*pLinAddr	= NULL;
		*ppPhysAddr	= pPhysAddr;
		return BCE_OK;
	}
}

void BCFreeATTMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr)
{
	unsigned long		ulPages = RANGE_TO_PAGES(ulSize);
	unsigned long		ulPage;

	for (ulPage = 0; ulPage < ulPages; ulPage++) {
		__free_page(pfn_to_page(pPhysAddr[ulPage].uiAddr));
	}

	kfree(pPhysAddr);
}
#endif

IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;


	sys_paddr.uiAddr = cpu_paddr.uiAddr;
	return sys_paddr;
}

IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	cpu_paddr.uiAddr = sys_paddr.uiAddr;
	return cpu_paddr;
}

long stream_texture_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int			err = -ENOIOCTLCMD;

	switch(cmd)
	{
	case STREAM_TEXTURE_CREATE_INPUT_DEVICE:
		err = create_stream_device((STREAM_TEXTURE_DEVINFO __user *)arg);
		break;

	case STREAM_TEXTURE_DESTROY_INPUT_DEVICE:
		err = destroy_stream_device((unsigned int)arg);
		break;

	case STREAM_TEXTURE_GET_INPUT_DEVICE:
		err = get_stream_device((STREAM_TEXTURE_DEVINFO __user *)arg);
		break;

	case STREAM_TEXTURE_SET_INPUT_BUFFER:
		err = set_stream_buffer((STREAM_TEXTURE_BUFFER __user *)arg);
		break;

	case STREAM_TEXTURE_GET_INPUT_BUFFER:
		err = get_stream_buffer((STREAM_TEXTURE_BUFFER __user *)arg);
		break;

	case STREAM_TEXTURE_MAP_INPUT_BUFFER:
		mutex_lock(&mmap_dev_mtx);
		err = map_stream_buffer((STREAM_TEXTURE_BUFFER __user *)arg);
		if (!err) {
			mmap_dev = 0;
		}
		mutex_unlock(&mmap_dev_mtx);
		break;

	case STREAM_TEXTURE_CREATE_OUTPUT_DEVICE:
		err = create_fb2((STREAM_TEXTURE_FB2INFO __user *)arg);
		break;

	case STREAM_TEXTURE_DESTROY_OUTPUT_DEVICE:
		err = destroy_fb2();
		break;

	case STREAM_TEXTURE_GET_OUTPUT_DEVICE:
		err = get_fb2((STREAM_TEXTURE_FB2INFO __user *)arg);
		break;

	case STREAM_TEXTURE_MAP_OUTPUT_BUFFER:
		mutex_lock(&mmap_dev_mtx);
		err = map_fb2();
		if (!err) {
			mmap_dev = 1;
		}
		mutex_unlock(&mmap_dev_mtx);
		break;

	case STREAM_TEXTURE_GET_DISPLAY_OFFSET:
		err = get_fb2_display_offset((struct amb_event __user *)arg);
		break;

	default:
		break;
	}

	if (err != BCE_OK) {
		//printk("%s: ioctl error: %d!\n", __func__, err);
	}

	return err;
}

int stream_texture_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int 	err;

	mutex_lock(&mmap_dev_mtx);
	if (!mmap_dev) {
		err = mmap_stream_buffer(filp, vma);
	} else {
		err = mmap_fb2(filp, vma);
	}
	mutex_unlock(&mmap_dev_mtx);

	return err;
}

module_init(stream_texture_init);
module_exit(stream_texture_exit);

