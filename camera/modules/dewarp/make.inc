##
## camera/modules/dewarp/make.inc
##
## History:
##    2013/03/27 - [Qian Shen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_DEWARP), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
DEWARP_VER_FILE   = $(CAMERA_DIR)/include/dewarp/version.h
DEWARP_VER_PREFIX = DEWARP

DEWARP_TARGET  = libamdewarp.so
LOCAL_TARGET := $(DEWARP_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/dewarp \
                -I$(CAMERA_DIR)/include -Werror \
                -I$(AMB_TOPDIR)/packages/utils/ \
                -I$(AMB_TOPDIR)/packages/dewarp/arch_$(AMBARELLA_ARCH)
LOCAL_LIBS   := libutils.so

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

DEWARP_SO_VER_MAJOR  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(DEWARP_VER_FILE))
DEWARP_SO_VER_MINOR  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(DEWARP_VER_FILE))
DEWARP_SO_VER_PATCH  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(DEWARP_VER_FILE))
DEWARP_SO_VER_STRING := $(DEWARP_SO_VER_MAJOR).$(DEWARP_SO_VER_MINOR).$(DEWARP_SO_VER_PATCH)

DEWARP_SO_NAME   = $(patsubst %.so,%.so.$(DEWARP_SO_VER_MAJOR), $(DEWARP_TARGET))
DEWARP_SO_TARGET = $(patsubst %.so,%.so.$(DEWARP_SO_VER_STRING), $(DEWARP_TARGET))

LOCAL_SO_NAME := $(DEWARP_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(DEWARP_SO_TARGET)
	@ln -sf $(DEWARP_SO_TARGET) $(CAMERA_LIB_DIR)/$(DEWARP_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

