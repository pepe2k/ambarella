##
## camera/modules/utilities/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_UTILS), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
UTILS_VER_FILE   = $(CAMERA_DIR)/include/utilities/version.h
UTILS_VER_PREFIX = UTILITY

UTILS_TARGET  = libamutils.so
LOCAL_TARGET := $(UTILS_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/libatomic_ops/include \
                -I$(CAMERA_DIR)/include \
                -Wno-psabi -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

UTILS_SO_VER_MAJOR  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(UTILS_VER_FILE))
UTILS_SO_VER_MINOR  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_MINOR/{print $$3}' $(UTILS_VER_FILE))
UTILS_SO_VER_PATCH  := $(shell awk '/define\ $(UTILS_VER_PREFIX)_LIB_PATCH/{print $$3}' $(UTILS_VER_FILE))
UTILS_SO_VER_STRING := $(UTILS_SO_VER_MAJOR).$(UTILS_SO_VER_MINOR).$(UTILS_SO_VER_PATCH)

UTILS_SO_NAME   = $(patsubst %.so,%.so.$(UTILS_SO_VER_MAJOR), $(UTILS_TARGET))
UTILS_SO_TARGET = $(patsubst %.so,%.so.$(UTILS_SO_VER_STRING), $(UTILS_TARGET))

LOCAL_SO_NAME := $(UTILS_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(UTILS_SO_TARGET)
	@ln -sf $(UTILS_SO_TARGET) $(CAMERA_LIB_DIR)/$(UTILS_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif