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

#ifndef SGX_AUTOCONF_INCLUDED
#include <linux/config.h>
#endif

#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/mutex.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>

#include <plat/ambasyncproc.h>
#include <plat/ambevent.h>

#if defined(DEBUG)
#define	PVR_DEBUG DEBUG
#undef DEBUG
#endif

#if defined(DEBUG)
#undef DEBUG
#endif
#if defined(PVR_DEBUG)
#define	DEBUG PVR_DEBUG
#undef PVR_DEBUG
#endif

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "fb2.h"
#include "pvrmodule.h"
#include "stream_texture.h"

#if !defined(PVR_LINUX_USING_WORKQUEUES)
#error "PVR_LINUX_USING_WORKQUEUES must be defined"
#endif

MODULE_SUPPORTED_DEVICE(DEVNAME);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34))
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_driver *drv = (dev) != NULL ? (dev)->driver : NULL
#define IONE_DSS_MANAGER(man, dev) struct ione_overlay_manager *man = (dev) != NULL ? (dev)->manager : NULL
#define	WAIT_FOR_VSYNC(man)	((man)->wait_for_vsync)
#else
#define IONE_DSS_DRIVER(drv, dev) struct ione_dss_device *drv = (dev)
#define IONE_DSS_MANAGER(man, dev) struct ione_dss_device *man = (dev)
#define	WAIT_FOR_VSYNC(man)	((man)->wait_vsync)
#endif

#define SGX_VSYNC0_IRQ		VOUT_IRQ
#define SGX_VSYNC1_IRQ		ORC_VOUT0_IRQ

static					DEFINE_MUTEX(g_fb2_mtx);
static int				g_fb2_alive;
static STREAM_TEXTURE_FB2INFO		g_fb2_info;
static struct amb_async_proc_info	event_proc;
static struct amb_event_pool		event_pool;
static wait_queue_head_t		g_wq;
static int				g_waiting;
static spinlock_t			g_waiting_lock;
static unsigned char			pool_index;

#if defined(CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED) || defined(CONFIG_SGX_STREAMING_BUFFER_KMALLOC)
extern int amba_gpu_update_osd(unsigned long);
#endif

/*static irqreturn_t sgx_vsync_isr(int irqno, void *dev_id)
{
	FB2_SWAPCHAIN *psSwapChain = (FB2_SWAPCHAIN *)dev_id;
	unsigned long flags;

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	if (psSwapChain->wait_num) {
		psSwapChain->wait_num = 0;
		psSwapChain->irq_interval = 0;
	} else {
		psSwapChain->irq_interval++;
	}
	psSwapChain->timeout = 0;
	spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	wake_up_interruptible(&psSwapChain->vsync_wq);

	return IRQ_HANDLED;
}*/

void *FB2AllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void FB2FreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void FB2CreateSwapChainLockInit(FB2_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void FB2CreateSwapChainLockDeInit(FB2_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void FB2CreateSwapChainLock(FB2_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void FB2CreateSwapChainUnLock(FB2_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void FB2AtomicBoolInit(FB2_ATOMIC_BOOL *psAtomic, FB2_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void FB2AtomicBoolDeInit(FB2_ATOMIC_BOOL *psAtomic)
{
}

void FB2AtomicBoolSet(FB2_ATOMIC_BOOL *psAtomic, FB2_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

FB2_BOOL FB2AtomicBoolRead(FB2_ATOMIC_BOOL *psAtomic)
{
	return (FB2_BOOL)atomic_read(psAtomic);
}

void FB2AtomicIntInit(FB2_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void FB2AtomicIntDeInit(FB2_ATOMIC_INT *psAtomic)
{
}

void FB2AtomicIntSet(FB2_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int FB2AtomicIntRead(FB2_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void FB2AtomicIntInc(FB2_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

FB2_ERROR FB2GetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (FB2_ERROR_INVALID_PARAMS);
	}


	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (FB2_OK);
}

void FB2QueueBufferForSwap(FB2_SWAPCHAIN *psSwapChain, FB2_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

static void WorkQueueHandler(struct work_struct *psWork)
{
	FB2_BUFFER *psBuffer = container_of(psWork, FB2_BUFFER, sWork);

	FB2SwapHandler(psBuffer);
}

FB2_ERROR FB2CreateSwapQueue(FB2_SWAPCHAIN *psSwapChain)
{
	//int	errorCode;

	psSwapChain->psWorkQueue = create_singlethread_workqueue(DEVNAME);
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: create_singlethreaded_workqueue failed\n", __FUNCTION__, psSwapChain->uiFBDevID);
		return (FB2_ERROR_INIT_FAILURE);
	}

	init_waitqueue_head(&psSwapChain->vsync_wq);
	spin_lock_init(&psSwapChain->wait_num_lock);
	psSwapChain->wait_num = 0;
	psSwapChain->irq_interval = 0;
	psSwapChain->timeout = 0;
	/*if (psSwapChain->uiFBDevID == 0) {
		psSwapChain->irq_no = SGX_VSYNC0_IRQ;
		errorCode = request_irq(psSwapChain->irq_no, sgx_vsync_isr,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "sgx fb0", psSwapChain);
	} else {
		psSwapChain->irq_no = SGX_VSYNC1_IRQ;
		errorCode = request_irq(psSwapChain->irq_no, sgx_vsync_isr,
			IRQF_TRIGGER_RISING | IRQF_SHARED, "sgx fb1", psSwapChain);
	}
	if (errorCode) {
		printk(KERN_WARNING DRIVER_PREFIX "Request SGX Vsync IRQ %d Failed!\n", psSwapChain->irq_no);
		return (FB2_ERROR_INIT_FAILURE);
	}*/

	return (FB2_OK);
}

void FB2InitBufferForSwap(FB2_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

void FB2DestroySwapQueue(FB2_SWAPCHAIN *psSwapChain)
{
	//free_irq(psSwapChain->irq_no, psSwapChain);
	destroy_workqueue(psSwapChain->psWorkQueue);
}

void FB2Flip(FB2_DEVINFO *psDevInfo, FB2_BUFFER *psBuffer)
{
	struct amb_event		event;
	unsigned long			flags;
	unsigned int			fb_id;

	event.type = AMB_EV_FB2_PAN_DISPLAY;
	fb_id = psBuffer->ulYOffset / psDevInfo->sFBInfo.ulHeight;
	memcpy(event.data, &fb_id, sizeof(fb_id));
	amb_event_pool_affuse(&event_pool, event);
	spin_lock_irqsave(&g_waiting_lock, flags);
	g_waiting = 0;
	spin_unlock_irqrestore(&g_waiting_lock, flags);
	wake_up_interruptible(&g_wq);
	/*if (!errorCode && event_proc.fasync_queue) {
		kill_fasync(&event_proc.fasync_queue, SIGIO, POLL_IN);
	}*/

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	amba_gpu_update_osd((unsigned long)psBuffer->psSysAddr[0].uiAddr);
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	amba_gpu_update_osd(psBuffer->sSysAddr.uiAddr + psBuffer->ulYOffset);
#endif
}

FB2_UPDATE_MODE FB2GetUpdateMode(FB2_DEVINFO *psDevInfo)
{
	return FB2_UPDATE_MODE_AUTO;
}

FB2_BOOL FB2SetUpdateMode(FB2_DEVINFO *psDevInfo, FB2_UPDATE_MODE eMode)
{
	return 0;
}

#ifdef CONFIG_SGX_SYNCHRONIZE_NONE
FB2_BOOL FB2WaitForVSync(FB2_DEVINFO *psDevInfo)
{
	return FB2_TRUE;
}
#endif

#ifdef CONFIG_SGX_SYNCHRONIZE_NEXT_VSYNC
FB2_BOOL FB2WaitForVSync(FB2_DEVINFO *psDevInfo)
{
#if 0
	FB2_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	unsigned long flags;

	/* Vout is not running, do not wait */
	if (psSwapChain->timeout) {
		return FB2_TRUE;
	}

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	psSwapChain->wait_num = 1;
	spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	if (!wait_event_interruptible_timeout(psSwapChain->vsync_wq, psSwapChain->wait_num == 0, HZ / 10)) {
		spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
		psSwapChain->timeout = 1;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
		printk("Wait for vout%d irq timeout, it might die!\n", psSwapChain->uiFBDevID);
	}
#endif

	return FB2_TRUE;
}
#endif

#ifdef CONFIG_SGX_SYNCHRONIZE_INTERVAL
FB2_BOOL FB2WaitForVSync(FB2_DEVINFO *psDevInfo)
{
#if 0
	FB2_SWAPCHAIN *psSwapChain = psDevInfo->psSwapChain;
	unsigned long flags;

	/* Vout is not running, do not wait */
	if (psSwapChain->timeout) {
		return FB2_TRUE;
	}

	spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
	if (psSwapChain->irq_interval) {
		psSwapChain->irq_interval = 0;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
	} else {
		psSwapChain->wait_num = 1;
		spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
		if (!wait_event_interruptible_timeout(psSwapChain->vsync_wq, psSwapChain->wait_num == 0, HZ / 10)) {
			spin_lock_irqsave(&psSwapChain->wait_num_lock, flags);
			psSwapChain->timeout = 1;
			spin_unlock_irqrestore(&psSwapChain->wait_num_lock, flags);
			printk("Wait for vout%d irq timeout, it might die!\n", psSwapChain->uiFBDevID);
		}
	}
#endif

	return FB2_TRUE;
}
#endif

FB2_BOOL FB2ManualSync(FB2_DEVINFO *psDevInfo)
{
	return FB2_TRUE;
}

FB2_BOOL FB2CheckModeAndSync(FB2_DEVINFO *psDevInfo)
{
	FB2_UPDATE_MODE eMode = FB2GetUpdateMode(psDevInfo);

	switch(eMode)
	{
		case FB2_UPDATE_MODE_AUTO:
		case FB2_UPDATE_MODE_MANUAL:
			return FB2ManualSync(psDevInfo);
		default:
			break;
	}

	return FB2_TRUE;
}

static int FB2FrameBufferEvents(struct notifier_block *psNotif,
                             unsigned long event, void *data)
{
	FB2_DEVINFO *psDevInfo;
	struct fb_event *psFBEvent = (struct fb_event *)data;
	struct fb_info *psFBInfo = psFBEvent->info;
	FB2_BOOL bBlanked;


	if (event != FB_EVENT_BLANK)
	{
		return 0;
	}

	bBlanked = (*(IMG_INT *)psFBEvent->data != 0) ? FB2_TRUE: FB2_FALSE;

	psDevInfo = FB2GetDevInfoPtr(psFBInfo->node);

	if (psDevInfo != NULL)
	{
		FB2AtomicBoolSet(&psDevInfo->sBlanked, bBlanked);
		FB2AtomicIntInc(&psDevInfo->sBlankEvents);
	}

	return 0;
}

FB2_ERROR FB2UnblankDisplay(FB2_DEVINFO *psDevInfo)
{
	/*int res;

	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (FB2_ERROR_GENERIC);
	}*/

	return (FB2_OK);
}

#ifdef CONFIG_HAS_EARLYSUSPEND

static void FB2BlankDisplay(FB2_DEVINFO *psDevInfo)
{
	//fb_blank(psDevInfo->psLINFBInfo, 1);
}

static void FB2EarlySuspendHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = 3;
	unsigned i;

	for (i=2; i < uiMaxFBDevIDPlusOne; i++)
	{
		FB2_DEVINFO *psDevInfo = FB2GetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			FB2AtomicBoolSet(&psDevInfo->sEarlySuspendFlag, FB2_TRUE);
			FB2BlankDisplay(psDevInfo);
		}
	}
}

static void FB2EarlyResumeHandler(struct early_suspend *h)
{
	unsigned uiMaxFBDevIDPlusOne = 3;
	unsigned i;

	for (i=2; i < uiMaxFBDevIDPlusOne; i++)
	{
		FB2_DEVINFO *psDevInfo = FB2GetDevInfoPtr(i);

		if (psDevInfo != NULL)
		{
			FB2UnblankDisplay(psDevInfo);
			FB2AtomicBoolSet(&psDevInfo->sEarlySuspendFlag, FB2_FALSE);
		}
	}
}

#endif

FB2_ERROR FB2EnableLFBEventNotification(FB2_DEVINFO *psDevInfo)
{
//	int                res;
	FB2_ERROR         eError;


	memset(&psDevInfo->sLINNotifBlock, 0, sizeof(psDevInfo->sLINNotifBlock));

	psDevInfo->sLINNotifBlock.notifier_call = FB2FrameBufferEvents;

	FB2AtomicBoolSet(&psDevInfo->sBlanked, FB2_FALSE);
	FB2AtomicIntSet(&psDevInfo->sBlankEvents, 0);

	/*res = fb_register_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_register_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);

		return (FB2_ERROR_GENERIC);
	}*/

	eError = FB2UnblankDisplay(psDevInfo);
	if (eError != FB2_OK)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: UnblankDisplay failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, eError);
		return eError;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	psDevInfo->sEarlySuspend.suspend = FB2EarlySuspendHandler;
	psDevInfo->sEarlySuspend.resume = FB2EarlyResumeHandler;
	psDevInfo->sEarlySuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	register_early_suspend(&psDevInfo->sEarlySuspend);
#endif

	return (FB2_OK);
}

FB2_ERROR FB2DisableLFBEventNotification(FB2_DEVINFO *psDevInfo)
{
//	int res;

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&psDevInfo->sEarlySuspend);
#endif


	/*res = fb_unregister_client(&psDevInfo->sLINNotifBlock);
	if (res != 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX
			": %s: Device %u: fb_unregister_client failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (FB2_ERROR_GENERIC);
	}*/

	FB2AtomicBoolSet(&psDevInfo->sBlanked, FB2_FALSE);

	return (FB2_OK);
}

int create_fb2(STREAM_TEXTURE_FB2INFO __user *pfb2_info)
{
	STREAM_TEXTURE_FB2INFO		fb2_info;
	FB2_ERROR         		eError = 0;
	unsigned int			bytes_per_pixel;

	mutex_lock(&g_fb2_mtx);

	if (g_fb2_alive) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2 has been created\n", __FUNCTION__);
		eError = -EAGAIN;
		goto create_fb2_exit;
	}

	eError = copy_from_user(&fb2_info, pfb2_info, sizeof(fb2_info));
	if (eError < 0) {
		eError = -EFAULT;
		goto create_fb2_exit;
	}

	if (!fb2_info.width && !fb2_info.height) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Incorrect FB2 width or height\n", __FUNCTION__);
		eError = -EINVAL;
		goto create_fb2_exit;
	}

	if (fb2_info.width % 32) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2 width must be a multiple of 32\n", __FUNCTION__);
		eError = -EINVAL;
		goto create_fb2_exit;
	}

	if (fb2_info.format != PVRSRV_PIXEL_FORMAT_RGB565 &&
		fb2_info.format != PVRSRV_PIXEL_FORMAT_ARGB8888) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2 can only be RGB565 or ARGB8888\n", __FUNCTION__);
		eError = -EINVAL;
		goto create_fb2_exit;
	}

	if (fb2_info.format == PVRSRV_PIXEL_FORMAT_RGB565) {
		bytes_per_pixel = 2;
	} else {
		bytes_per_pixel = 4;
	}

	if (fb2_info.n_buffer < 5) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2 should at least contain 4 back buffers\n", __FUNCTION__);
		eError = -EINVAL;
		goto create_fb2_exit;
	}

	if (fb2_info.size <  fb2_info.width * fb2_info.height * bytes_per_pixel * 5) {
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2 should at least contain 4 back buffers\n", __FUNCTION__);
		eError = -EINVAL;
		goto create_fb2_exit;
	}

	eError = FB2Init(&fb2_info);
	if(eError != FB2_OK)	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2Init failed\n", __FUNCTION__);
		eError = -ENODEV;
		goto create_fb2_exit;
	}

	g_fb2_alive = 1;
	memcpy(&g_fb2_info, &fb2_info, sizeof(fb2_info));

create_fb2_exit:
	mutex_unlock(&g_fb2_mtx);
	return eError;
}

int destroy_fb2(void)
{
	mutex_lock(&g_fb2_mtx);
	if (g_fb2_alive) {
		if(FB2DeInit() != FB2_OK) {
			printk(KERN_WARNING DRIVER_PREFIX ": %s: FB2DeInit failed\n", __FUNCTION__);
		}
		g_fb2_alive = 0;
		memset(&g_fb2_info, 0, sizeof(g_fb2_info));
	}
	mutex_unlock(&g_fb2_mtx);

	return 0;
}

int get_fb2(STREAM_TEXTURE_FB2INFO __user *pfb2_info)
{
	int		err;

	mutex_lock(&g_fb2_mtx);
	if (g_fb2_alive) {
		err = copy_to_user(pfb2_info, &g_fb2_info, sizeof(*pfb2_info));
	} else {
		err = -EINVAL;
	}
	mutex_unlock(&g_fb2_mtx);

	return err;
}

int map_fb2(void)
{
	return 0;
}

int mmap_fb2(struct file *filp, struct vm_area_struct *vma)
{

	BCE_ERROR		eError = BCE_OK;
	FB2_DEVINFO		*psDevInfo;
	FB2_BUFFER		*psBuffer;

	psDevInfo = FB2GetDevInfoPtr(2);
	if (!psDevInfo) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto mmap_fb2_exit;
	}

	psBuffer = &psDevInfo->sSystemBuffer;
	if (!psBuffer) {
		eError = BCE_ERROR_INVALID_PARAMS;
		goto mmap_fb2_exit;
	}

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
{
	unsigned int		pages, page;
	unsigned int		vstart;

	pages	= (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
	vstart	= vma->vm_start;
	for (page = 0; page < pages; page++) {
		if (remap_pfn_range(vma, vstart, psBuffer->psSysAddr[page].uiAddr >> PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot))
			break;
		vstart += PAGE_SIZE;
	}

	if (page < pages) {
		eError = BCE_ERROR_INVALID_PARAMS;
	}
}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start,	psBuffer->sSysAddr.uiAddr >> PAGE_SHIFT,
		vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
			eError = BCE_ERROR_INVALID_PARAMS;
	}
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_vmalloc_range(vma, psBuffer->sCPUVAddr, 0)) {
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
		if (remap_pfn_range(vma, vstart, psBuffer->psSysAddr[page].uiAddr >> PAGE_SHIFT, PAGE_SIZE, vma->vm_page_prot))
			break;
		vstart += PAGE_SIZE;
	}

	if (page < pages) {
		eError = BCE_ERROR_INVALID_PARAMS;
	}
}
#endif

mmap_fb2_exit:
	return eError;
}

int get_fb2_display_offset(struct amb_event __user *buf)
{
	BCE_ERROR				eError = BCE_OK;
	struct amb_event			event;
	size_t					i;
	unsigned long				flags;

	if (!g_fb2_alive) {
		eError = -BCE_ERROR_INVALID_DEVICE;
		goto get_fb2_display_offset_exit;
	}

	for (i = 0; i < 256; i++) {
		if (amb_event_pool_query_event(&event_pool, &event, pool_index))
			break;
		eError = copy_to_user(&buf[i * sizeof(struct amb_event)],
			&event, sizeof(event));
		if (eError) {
			eError = -EFAULT;
			goto get_fb2_display_offset_exit;

		}
		pool_index++;
	}

	if (i != 0) {
		eError = i * sizeof(struct amb_event);
		goto get_fb2_display_offset_exit;
	}

	spin_lock_irqsave(&g_waiting_lock, flags);
	g_waiting = 1;
	spin_unlock_irqrestore(&g_waiting_lock, flags);
	if (wait_event_interruptible(g_wq, g_waiting == 0)) {
		eError = -BCE_ERROR_GENERIC;
		goto get_fb2_display_offset_exit;
	}

	for (i = 0; i < 256; i++) {
		if (amb_event_pool_query_event(&event_pool, &event, pool_index))
			break;
		eError = copy_to_user(&buf[i * sizeof(struct amb_event)],
			&event, sizeof(event));
		if (eError) {
			eError = -EFAULT;
			goto get_fb2_display_offset_exit;

		}
		pool_index++;
	}

	eError = i * sizeof(struct amb_event);

get_fb2_display_offset_exit:
	return eError;
}

static ssize_t fb2_event_proc_read(struct file *filp,
	char __user *buf, size_t count, loff_t *offset)
{
	struct amb_event_pool			*pool;
	struct amb_event			event;
	size_t					i;
	int					ret;

	pool = (struct amb_event_pool *)filp->private_data;
	if (!pool) {
		return -EINVAL;
	}

	for (i = 0; i < count / sizeof(struct amb_event); i++) {
		if (amb_event_pool_query_event(pool, &event, *offset))
			break;
		ret = copy_to_user(&buf[i * sizeof(struct amb_event)],
			&event, sizeof(event));
		if (ret)
			return -EFAULT;
		(*offset)++;
	}

	if (i == 0) {
		*offset = amb_event_pool_query_index(pool);
		if (!amb_event_pool_query_event(pool, &event, *offset)) {
			ret = copy_to_user(buf, &event, sizeof(event));
			if (ret)
				return -EFAULT;
			(*offset)++;
			i++;
		}
	}

	return i * sizeof(struct amb_event);
}

BCE_ERROR __fb2_init(void)
{
	mutex_lock(&g_fb2_mtx);

	g_fb2_alive = 0;
	memset(&g_fb2_info, 0, sizeof(g_fb2_info));
	init_waitqueue_head(&g_wq);
	g_waiting = 0;
	spin_lock_init(&g_waiting_lock);

	amb_event_pool_init(&event_pool);
	snprintf(event_proc.proc_name, sizeof(event_proc.proc_name), "fb2_event");
	event_proc.fops.read = fb2_event_proc_read;
	event_proc.private_data	= &event_pool;
	/*if (amb_async_proc_create(&event_proc)) {
		printk("Create fb2 event proc failed!\n");
	}*/
	pool_index = 0;

	mutex_unlock(&g_fb2_mtx);

	return BCE_OK;
}

BCE_ERROR __fb2_exit(void)
{
	destroy_fb2();
	amb_async_proc_remove(&event_proc);

	return BCE_OK;
}
