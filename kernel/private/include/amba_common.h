/*
 * kernel/private/include/amba_common.h
 *
 * History:
 *    2008/10/23 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_COMMON_H
#define __AMBA_COMMON_H

#include <config.h>

#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ambbus.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0)
#include <linux/i2c-id.h>
#endif
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <asm/dma.h>
#include <asm/uaccess.h>

#include <linux/semaphore.h>
#include <mach/hardware.h>
#include <plat/fb.h>
#include <plat/ambsyncproc.h>
#include <plat/ambasyncproc.h>
#include <plat/ambevent.h>
#include <plat/debug.h>
#include <plat/ambcache.h>
#include <mach/init.h>
#include <mach/board.h>
#define I2C_DRIVERID_I2CDEV		(0)

#include <ambas_common.h>
#include <amba_arch.h>

#if defined(CONFIG_PLAT_AMBARELLA_SUPPORT_HAL)
#include <hal/hal.h>
#endif

#endif

