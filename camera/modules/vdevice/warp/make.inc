##
## camera/modules/vdevice/simplecam/make.inc
##
## History:
##    2013/03/20 - [Qian Shen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_ARCH_S2), y)
ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_WARPDEV), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
WARPDEV_VER_FILE   = $(CAMERA_DIR)/include/vdevice/version.h
WARPDEV_VER_PREFIX = WARPDEV

WARPDEV_TARGET  = libamwarpdev.so
LOCAL_TARGET := $(WARPDEV_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(LOCAL_PATH)/../include \
                -I$(CAMERA_DIR)/include

WARPDEV_SO_VER_MAJOR  := $(shell awk '/define\ $(WARPDEV_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(WARPDEV_VER_FILE))
WARPDEV_SO_VER_MINOR  := $(shell awk '/define\ $(WARPDEV_VER_PREFIX)_LIB_MINOR/{print $$3}' $(WARPDEV_VER_FILE))
WARPDEV_SO_VER_PATCH  := $(shell awk '/define\ $(WARPDEV_VER_PREFIX)_LIB_PATCH/{print $$3}' $(WARPDEV_VER_FILE))
WARPDEV_SO_VER_STRING := $(WARPDEV_SO_VER_MAJOR).$(WARPDEV_SO_VER_MINOR).$(WARPDEV_SO_VER_PATCH)

WARPDEV_SO_NAME   = $(patsubst %.so,%.so.$(WARPDEV_SO_VER_MAJOR), $(WARPDEV_TARGET))
WARPDEV_SO_TARGET = $(patsubst %.so,%.so.$(WARPDEV_SO_VER_STRING), $(WARPDEV_TARGET))

LOCAL_SO_NAME := $(WARPDEV_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(WARPDEV_SO_TARGET)
	@ln -sf $(WARPDEV_SO_TARGET) $(CAMERA_LIB_DIR)/$(WARPDEV_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif
