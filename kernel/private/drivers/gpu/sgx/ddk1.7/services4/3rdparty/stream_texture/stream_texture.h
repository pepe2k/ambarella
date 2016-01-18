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

#ifndef __STREAM_TEXTURE_H__
#define __STREAM_TEXTURE_H__

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <plat/ambevent.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kernelbuffer.h"
#include "ambas_stream_texture.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MAX_STREAM_DEVICES		8

typedef enum tag_bce_bool
{
	BCE_FALSE = 0,
	BCE_TRUE  = 1,
} BCE_BOOL, *BCE_PBOOL;

#ifndef UNREFERENCED_PARAMETER
#define	UNREFERENCED_PARAMETER(param) (param) = (param)
#endif

#ifndef NULL
#define NULL	0
#endif

int create_stream_device(STREAM_TEXTURE_DEVINFO __user *);
int destroy_stream_device(unsigned int);
int get_stream_device(STREAM_TEXTURE_DEVINFO __user *);
int set_stream_buffer(STREAM_TEXTURE_BUFFER __user *);
int get_stream_buffer(STREAM_TEXTURE_BUFFER __user *);
int map_stream_buffer(STREAM_TEXTURE_BUFFER __user *);
int mmap_stream_buffer(struct file *filp, struct vm_area_struct *vma);

int create_fb2(STREAM_TEXTURE_FB2INFO __user *);
int destroy_fb2(void);
int get_fb2(STREAM_TEXTURE_FB2INFO __user *);
int map_fb2(void);
int mmap_fb2(struct file *filp, struct vm_area_struct *vma);
int get_fb2_display_offset(struct amb_event __user *);

BCE_ERROR __stream_texture_init(void);
BCE_ERROR __stream_texture_exit(void);
BCE_ERROR __fb2_init(void);
BCE_ERROR __fb2_exit(void);

void *BCAllocKernelMem(unsigned long ulSize);
void BCFreeKernelMem(void *pvMem);

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC

BCE_ERROR BCAllocContigMemory(unsigned long    ulSize,
                              BCE_HANDLE       *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR  *pPhysAddr);

void BCFreeContigMemory(unsigned long ulSize,
                        BCE_HANDLE hMemHandle,
                        IMG_CPU_VIRTADDR LinAddr,
                        IMG_SYS_PHYADDR PhysAddr);

#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC

BCE_ERROR BCAllocDiscontigMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr);

void BCFreeDiscontigMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr);

#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT

BCE_ERROR BCAllocATTMemory(unsigned long ulSize,
                              BCE_HANDLE unref__ *phMemHandle,
                              IMG_CPU_VIRTADDR *pLinAddr,
                              IMG_SYS_PHYADDR **ppPhysAddr);

void BCFreeATTMemory(unsigned long ulSize,
                         BCE_HANDLE unref__ hMemHandle,
                         IMG_CPU_VIRTADDR LinAddr,
                         IMG_SYS_PHYADDR *pPhysAddr);

#endif

IMG_SYS_PHYADDR CpuPAddrToSysPAddrBC(IMG_CPU_PHYADDR cpu_paddr);
IMG_CPU_PHYADDR SysPAddrToCpuPAddrBC(IMG_SYS_PHYADDR sys_paddr);

void *MapPhysAddr(IMG_SYS_PHYADDR sSysAddr, unsigned long ulSize);
void UnMapPhysAddr(void *pvAddr, unsigned long ulSize);

#if defined(__cplusplus)
}
#endif

#endif

