##
## camera/modules/vdevice/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_VDEVICE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
VDEVICE_VER_FILE   = $(CAMERA_DIR)/include/vdevice/version.h
VDEVICE_VER_PREFIX = VDEVICE

VDEVICE_TARGET  = libamvdevice.so
LOCAL_TARGET := $(VDEVICE_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(CAMERA_DIR)/include \
                -I$(LOCAL_PATH)/../include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

VDEVICE_SO_VER_MAJOR  := $(shell awk '/define\ $(VDEVICE_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(VDEVICE_VER_FILE))
VDEVICE_SO_VER_MINOR  := $(shell awk '/define\ $(VDEVICE_VER_PREFIX)_LIB_MINOR/{print $$3}' $(VDEVICE_VER_FILE))
VDEVICE_SO_VER_PATCH  := $(shell awk '/define\ $(VDEVICE_VER_PREFIX)_LIB_PATCH/{print $$3}' $(VDEVICE_VER_FILE))
VDEVICE_SO_VER_STRING := $(VDEVICE_SO_VER_MAJOR).$(VDEVICE_SO_VER_MINOR).$(VDEVICE_SO_VER_PATCH)

VDEVICE_SO_NAME   = $(patsubst %.so,%.so.$(VDEVICE_SO_VER_MAJOR), $(VDEVICE_TARGET))
VDEVICE_SO_TARGET = $(patsubst %.so,%.so.$(VDEVICE_SO_VER_STRING), $(VDEVICE_TARGET))

LOCAL_SO_NAME := $(VDEVICE_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(VDEVICE_SO_TARGET)
	@ln -sf $(VDEVICE_SO_TARGET) $(CAMERA_LIB_DIR)/$(VDEVICE_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(call all-subdir-makefiles)
endif
