##
## camera/modules/vdevice/simplecam/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_SIMPLECAM), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
SIMPLECAM_VER_FILE   = $(CAMERA_DIR)/include/vdevice/version.h
SIMPLECAM_VER_PREFIX = SIMPLECAM

SIMPLECAM_TARGET  = libamsimplecam.so
LOCAL_TARGET := $(SIMPLECAM_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(LOCAL_PATH)/../include \
                -I$(CAMERA_DIR)/include

SIMPLECAM_SO_VER_MAJOR  := $(shell awk '/define\ $(SIMPLECAM_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(SIMPLECAM_VER_FILE))
SIMPLECAM_SO_VER_MINOR  := $(shell awk '/define\ $(SIMPLECAM_VER_PREFIX)_LIB_MINOR/{print $$3}' $(SIMPLECAM_VER_FILE))
SIMPLECAM_SO_VER_PATCH  := $(shell awk '/define\ $(SIMPLECAM_VER_PREFIX)_LIB_PATCH/{print $$3}' $(SIMPLECAM_VER_FILE))
SIMPLECAM_SO_VER_STRING := $(SIMPLECAM_SO_VER_MAJOR).$(SIMPLECAM_SO_VER_MINOR).$(SIMPLECAM_SO_VER_PATCH)

SIMPLECAM_SO_NAME   = $(patsubst %.so,%.so.$(SIMPLECAM_SO_VER_MAJOR), $(SIMPLECAM_TARGET))
SIMPLECAM_SO_TARGET = $(patsubst %.so,%.so.$(SIMPLECAM_SO_VER_STRING), $(SIMPLECAM_TARGET))

LOCAL_SO_NAME := $(SIMPLECAM_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(SIMPLECAM_SO_TARGET)
	@ln -sf $(SIMPLECAM_SO_TARGET) $(CAMERA_LIB_DIR)/$(SIMPLECAM_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif