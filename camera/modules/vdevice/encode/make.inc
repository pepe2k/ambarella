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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_ENCDEV), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
ENCDEV_VER_FILE   = $(CAMERA_DIR)/include/vdevice/version.h
ENCDEV_VER_PREFIX = ENCDEV

ENCDEV_TARGET  = libamencdev.so
LOCAL_TARGET := $(ENCDEV_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(LOCAL_PATH)/../include \
                -I$(CAMERA_DIR)/include

ENCDEV_SO_VER_MAJOR  := $(shell awk '/define\ $(ENCDEV_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(ENCDEV_VER_FILE))
ENCDEV_SO_VER_MINOR  := $(shell awk '/define\ $(ENCDEV_VER_PREFIX)_LIB_MINOR/{print $$3}' $(ENCDEV_VER_FILE))
ENCDEV_SO_VER_PATCH  := $(shell awk '/define\ $(ENCDEV_VER_PREFIX)_LIB_PATCH/{print $$3}' $(ENCDEV_VER_FILE))
ENCDEV_SO_VER_STRING := $(ENCDEV_SO_VER_MAJOR).$(ENCDEV_SO_VER_MINOR).$(ENCDEV_SO_VER_PATCH)

ENCDEV_SO_NAME   = $(patsubst %.so,%.so.$(ENCDEV_SO_VER_MAJOR), $(ENCDEV_TARGET))
ENCDEV_SO_TARGET = $(patsubst %.so,%.so.$(ENCDEV_SO_VER_STRING), $(ENCDEV_TARGET))

LOCAL_SO_NAME := $(ENCDEV_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(ENCDEV_SO_TARGET)
	@ln -sf $(ENCDEV_SO_TARGET) $(CAMERA_LIB_DIR)/$(ENCDEV_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif