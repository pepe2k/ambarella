##
## Makefile
##
## History:
##    2010/07/19 - [Zhenwu Xue]
##
## Copyright (C) 2004-2010, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /kernel/private/, ,$(PWD)))
TMP_DIR			:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(TMP_DIR)/../..
MODULE_NAME		:= SGX

ANDROID_ROOT		:= $(AMBABUILD_TOPDIR)
KERNELDIR		:= $(ANDROID_ROOT)/kernel/linux
CC_PATH			:= $(ANDROID_ROOT)/../android/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin
CROSS_COMPILE		:= $(CC_PATH)/arm-eabi-
ARCH			:= arm

SGX_LIB_PATH		:= $(KERNEL_MODULE_PATH)/lib/sgx540
SYSTEM_PROP_PATH	:= $(KERNEL_MODULE_PATH)/../elephant
AMLAUNCHER_PATH		:= $(ANDROID_ROOT)/device/ambarella/apps/AmLauncher

export AMBABUILD_TOPDIR
export ANDROID_ROOT
export KERNELDIR
export CROSS_COMPILE
export ARCH

include $(AMBABUILD_TOPDIR)/kernel/private/common.mk

.PHONY: all clean link_arch unlink_arch

all:
ifdef CONFIG_AMBARELLA_GPU_DEBUG
	mkdir -p $(KERNEL_MODULE_LIB_PATH)/extra
	ln -fsn $(PWD)/eurasiacon/binary_debug/pvrsrvkm.ko $(KERNEL_MODULE_LIB_PATH)/extra/pvrsrvkm.ko
	ln -fsn $(PWD)/eurasiacon/binary_debug/ionelfb.ko $(KERNEL_MODULE_LIB_PATH)/extra/ionelfb.ko
	ln -fsn $(PWD)/eurasiacon/binary_debug/stream_texture.ko $(KERNEL_MODULE_LIB_PATH)/extra/stream_texture.ko
	ln -fsn ddk1.6_debug $(SGX_LIB_PATH)/ddk
else
	mkdir -p $(KERNEL_MODULE_LIB_PATH)/extra
	ln -fsn $(PWD)/eurasiacon/binary_release/pvrsrvkm.ko $(KERNEL_MODULE_LIB_PATH)/extra/pvrsrvkm.ko
	ln -fsn $(PWD)/eurasiacon/binary_release/ionelfb.ko $(KERNEL_MODULE_LIB_PATH)/extra/ionelfb.ko
	ln -fsn $(PWD)/eurasiacon/binary_release/stream_texture.ko $(KERNEL_MODULE_LIB_PATH)/extra/stream_texture.ko
	ln -fsn ddk1.6 $(SGX_LIB_PATH)/ddk
endif
clean:
	if [ -f $(KERNEL_MODULE_LIB_PATH)/extra/pvrsrvkm.ko ]; then unlink $(KERNEL_MODULE_LIB_PATH)/extra/pvrsrvkm.ko; fi
	if [ -f $(KERNEL_MODULE_LIB_PATH)/extra/ionelfb.ko ]; then unlink $(KERNEL_MODULE_LIB_PATH)/extra/ionelfb.ko; fi
	if [ -f $(KERNEL_MODULE_LIB_PATH)/extra/stream_texture.ko ]; then unlink $(KERNEL_MODULE_LIB_PATH)/extra/stream_texture.ko; fi
	if [ -d $(SGX_LIB_PATH)/ddk ]; then unlink $(SGX_LIB_PATH)/ddk; fi
	$(AMBA_MAKEFILE_V)$(MAKE) -C eurasiacon/build/linux/ione_android/kbuild/ clean

link_arch:
	#$(MAKE) common_link_arch

unlink_arch:
	#$(MAKE) common_unlink_arch

