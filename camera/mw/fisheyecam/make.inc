##
## camera/mw/fisheyecam/make.inc
##
## History:
##    2012/03/22 - [Qian Shen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MW), y)
ifeq ($(CONFIG_ARCH_S2), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
FISHEYECAM_VER_FILE   = $(CAMERA_DIR)/include/mw/version.h
FISHEYECAM_VER_PREFIX = FISHEYECAM

FISHEYECAM_TARGET  = libamfisheyecam.so
LOCAL_TARGET := $(FISHEYECAM_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/vdevice \
                -I$(CAMERA_DIR)/include/dewarp \
                -I$(CAMERA_DIR)/include \
                -I$(AMB_TOPDIR)/packages/dewarp/arch_$(AMBARELLA_ARCH)

FISHEYECAM_SO_VER_MAJOR  := $(shell awk '/define\ $(FISHEYECAM_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(FISHEYECAM_VER_FILE))
FISHEYECAM_SO_VER_MINOR  := $(shell awk '/define\ $(FISHEYECAM_VER_PREFIX)_LIB_MINOR/{print $$3}' $(FISHEYECAM_VER_FILE))
FISHEYECAM_SO_VER_PATCH  := $(shell awk '/define\ $(FISHEYECAM_VER_PREFIX)_LIB_PATCH/{print $$3}' $(FISHEYECAM_VER_FILE))
FISHEYECAM_SO_VER_STRING := $(FISHEYECAM_SO_VER_MAJOR).$(FISHEYECAM_SO_VER_MINOR).$(FISHEYECAM_SO_VER_PATCH)

FISHEYECAM_SO_NAME   = $(patsubst %.so,%.so.$(FISHEYECAM_SO_VER_MAJOR), $(FISHEYECAM_TARGET))
FISHEYECAM_SO_TARGET = $(patsubst %.so,%.so.$(FISHEYECAM_SO_VER_STRING), $(FISHEYECAM_TARGET))

LOCAL_SO_NAME := $(FISHEYECAM_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(FISHEYECAM_SO_TARGET)
	@ln -sf $(FISHEYECAM_SO_TARGET) $(CAMERA_LIB_DIR)/$(FISHEYECAM_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

