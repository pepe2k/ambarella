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

export AMBABUILD_TOPDIR
export ANDROID_ROOT
export KERNELDIR
export CROSS_COMPILE
export ARCH

include $(AMBABUILD_TOPDIR)/kernel/private/common.mk

.PHONY: all clean link_arch unlink_arch

all:
ifdef CONFIG_AMBARELLA_GPU_DEBUG
	ln -fsn ddk1.5_debug $(SGX_LIB_PATH)/ddk
else
	ln -fsn ddk1.5 $(SGX_LIB_PATH)/ddk
endif
	$(AMBA_MAKEFILE_V)$(MAKE) -C eurasiacon/build/linux/ione_android/kbuild/ all

clean:
	if [ -d $(SGX_LIB_PATH)/ddk ]; then unlink $(SGX_LIB_PATH)/ddk; fi
	$(AMBA_MAKEFILE_V)$(MAKE) -C eurasiacon/build/linux/ione_android/kbuild/ clean

link_arch:
	$(MAKE) common_link_arch

unlink_arch:
	$(MAKE) common_unlink_arch

