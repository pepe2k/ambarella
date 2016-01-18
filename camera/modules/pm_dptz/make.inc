##
## camera/modules/vdevice/simplecam/make.inc
##
## History:
##    2012/11/21 - [Zhikan Yang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_PMDPTZ), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
PMDPTZ_VER_FILE   = $(CAMERA_DIR)/include/pm_dptz/version.h
PMDPTZ_VER_PREFIX = PM_DPTZ

PMDPTZ_TARGET  = libampmdptz.so
LOCAL_TARGET := $(PMDPTZ_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/am_pm_dptz.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(LOCAL_PATH)/../include \
                -I$(CAMERA_DIR)/include
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_CFLAGS +=	-I$(AMB_TOPDIR)/packages/main_preproc
LOCAL_LIBS   := libmainpp.so
endif

PMDPTZ_SO_VER_MAJOR  := $(shell awk '/define\ $(PMDPTZ_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(PMDPTZ_VER_FILE))
PMDPTZ_SO_VER_MINOR  := $(shell awk '/define\ $(PMDPTZ_VER_PREFIX)_LIB_MINOR/{print $$3}' $(PMDPTZ_VER_FILE))
PMDPTZ_SO_VER_PATCH  := $(shell awk '/define\ $(PMDPTZ_VER_PREFIX)_LIB_PATCH/{print $$3}' $(PMDPTZ_VER_FILE))
PMDPTZ_SO_VER_STRING := $(PMDPTZ_SO_VER_MAJOR).$(PMDPTZ_SO_VER_MINOR).$(PMDPTZ_SO_VER_PATCH)

PMDPTZ_SO_NAME   = $(patsubst %.so,%.so.$(PMDPTZ_SO_VER_MAJOR), $(PMDPTZ_TARGET))
PMDPTZ_SO_TARGET = $(patsubst %.so,%.so.$(PMDPTZ_SO_VER_STRING), $(PMDPTZ_TARGET))

LOCAL_SO_NAME := $(PMDPTZ_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(PMDPTZ_SO_TARGET)
	@ln -sf $(PMDPTZ_SO_TARGET) $(CAMERA_LIB_DIR)/$(PMDPTZ_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
