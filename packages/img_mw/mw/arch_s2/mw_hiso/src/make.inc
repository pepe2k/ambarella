##
## packages/img_mw/mw/arch_s2/src/make.inc
##
## History:
##    2012/06/01 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
MW_HISO_PATH	:= $(LOCAL_PATH)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LENS_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params

###
include $(CLEAR_VARS)


LOCAL_SRCS	:= $(wildcard $(MW_HISO_PATH)/*.c)
LOCAL_CFLAGS	:= -I$(MW_HISO_PATH)/../include
LOCAL_CFLAGS	+= -I$(ADJ_PARAMS_PATH)

AMP_HISO_MODULE = libamp_hiso.so
AMP_HISO_VER_PREFIX = AMP_HISO
AMP_HISO_VERSION_FILE = $(MW_HISO_PATH)/mw_version_hiso.c
AMP_HISO_SO_VER_MAJOR  := $(shell awk '/define\ $(AMP_HISO_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMP_HISO_VERSION_FILE))
AMP_HISO_SO_VER_MINOR  := $(shell awk '/define\ $(AMP_HISO_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMP_HISO_VERSION_FILE))
AMP_HISO_SO_VER_PATCH  := $(shell awk '/define\ $(AMP_HISO_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMP_HISO_VERSION_FILE))
AMP_HISO_SO_VER_STRING := $(AMP_HISO_SO_VER_MAJOR).$(AMP_HISO_SO_VER_MINOR).$(AMP_HISO_SO_VER_PATCH)
AMP_HISO_SO_NAME   = $(patsubst %.so,%.so.$(AMP_HISO_SO_VER_MAJOR), $(AMP_HISO_MODULE))
AMP_HISO_SO_TARGET = $(patsubst %.so,%.so.$(AMP_HISO_SO_VER_STRING), $(AMP_HISO_MODULE))

LOCAL_TARGET	:= $(AMP_HISO_MODULE)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(AMP_HISO_SO_TARGET)
	@ln -sf $(AMP_HISO_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(AMP_HISO_SO_NAME)
	@ln -sf $(AMP_HISO_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(AMP_HISO_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
