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
MODULE_NAME		:= "SGX GPU"

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/kernel/private/common.mk

SGX_DIR := none

ifeq ($(CONFIG_SGX_DDK_1_5), y)
SGX_DIR := ddk1.5
endif
ifeq ($(CONFIG_SGX_DDK_1_6), y)
SGX_DIR := ddk1.6
endif
ifeq ($(CONFIG_SGX_DDK_1_7), y)
SGX_DIR := ddk1.7
endif
ifeq ($(CONFIG_SGX_DDK_1_8), y)
SGX_DIR := ddk1.8
endif

SGX_BUILD_TYPE := release
ifeq ($(CONFIG_AMBARELLA_GPU_DEBUG), y)
SGX_BUILD_TYPE := debug
endif

export SGX_BUILD_TYPE

.PHONY: all clean link_arch unlink_arch

all clean:
	$(AMBA_MAKEFILE_V)echo "  [$(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx/$(SGX_DIR) $@

link_arch:
ifneq ($(SGX_DIR), none)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx/$(SGX_DIR) $@
endif

unlink_arch:
ifneq ($(SGX_DIR), none)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx/$(SGX_DIR) $@
endif

