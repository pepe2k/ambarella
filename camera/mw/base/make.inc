##
## camera/mw/base/make.inc
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

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
CAM_VER_FILE   = $(CAMERA_DIR)/include/mw/version.h
CAM_VER_PREFIX = CAM

CAM_TARGET  = libamcam.so
LOCAL_TARGET := $(CAM_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/vdevice \
                -I$(CAMERA_DIR)/include

CAM_SO_VER_MAJOR  := $(shell awk '/define\ $(CAM_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(CAM_VER_FILE))
CAM_SO_VER_MINOR  := $(shell awk '/define\ $(CAM_VER_PREFIX)_LIB_MINOR/{print $$3}' $(CAM_VER_FILE))
CAM_SO_VER_PATCH  := $(shell awk '/define\ $(CAM_VER_PREFIX)_LIB_PATCH/{print $$3}' $(CAM_VER_FILE))
CAM_SO_VER_STRING := $(CAM_SO_VER_MAJOR).$(CAM_SO_VER_MINOR).$(CAM_SO_VER_PATCH)

CAM_SO_NAME   = $(patsubst %.so,%.so.$(CAM_SO_VER_MAJOR), $(CAM_TARGET))
CAM_SO_TARGET = $(patsubst %.so,%.so.$(CAM_SO_VER_STRING), $(CAM_TARGET))

LOCAL_SO_NAME := $(CAM_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(CAM_SO_TARGET)
	@ln -sf $(CAM_SO_TARGET) $(CAMERA_LIB_DIR)/$(CAM_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif