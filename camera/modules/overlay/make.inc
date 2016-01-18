##
## camera/modules/overlay/make.inc
##
## History:
##    2012/12/30 - [Qian Shen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_OVERLAY), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
OVERLAY_VER_FILE   = $(CAMERA_DIR)/include/overlay/version.h
OVERLAY_VER_PREFIX = UTILITY

OVERLAY_TARGET  = libamoverlay.so
LOCAL_TARGET := $(OVERLAY_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/overlay \
                -I$(CAMERA_DIR)/include/        \
                -I$(AMB_TOPDIR)/packages/textinsert \
                -I$(PREBUILD_3RD_PARTY_DIR)/freetype/include
LOCAL_LDFLAGS = -L$(AMB_BOARD_OUT)/packages/textinsert -ltextinsert

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

OVERLAY_SO_VER_MAJOR  := $(shell awk '/define\ $(OVERLAY_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(OVERLAY_VER_FILE))
OVERLAY_SO_VER_MINOR  := $(shell awk '/define\ $(OVERLAY_VER_PREFIX)_LIB_MINOR/{print $$3}' $(OVERLAY_VER_FILE))
OVERLAY_SO_VER_PATCH  := $(shell awk '/define\ $(OVERLAY_VER_PREFIX)_LIB_PATCH/{print $$3}' $(OVERLAY_VER_FILE))
OVERLAY_SO_VER_STRING := $(OVERLAY_SO_VER_MAJOR).$(OVERLAY_SO_VER_MINOR).$(OVERLAY_SO_VER_PATCH)

OVERLAY_SO_NAME   = $(patsubst %.so,%.so.$(OVERLAY_SO_VER_MAJOR), $(OVERLAY_TARGET))
OVERLAY_SO_TARGET = $(patsubst %.so,%.so.$(OVERLAY_SO_VER_STRING), $(OVERLAY_TARGET))

LOCAL_SO_NAME := $(OVERLAY_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_MODULE): libtextinsert.a
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(OVERLAY_SO_TARGET)
	@ln -sf $(OVERLAY_SO_TARGET) $(CAMERA_LIB_DIR)/$(OVERLAY_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
