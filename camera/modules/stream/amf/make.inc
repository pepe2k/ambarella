##
## camera/modules/stream/amf/make.inc
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

include $(call all-subdir-makefiles)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
AMF_VER_FILE   = $(CAMERA_DIR)/include/stream/version.h
AMF_VER_PREFIX = STREAM

AMF_TARGET  = libamstreamamf.so
LOCAL_TARGET := $(AMF_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp $(LOCAL_PATH)/*.S)
LOCAL_CFLAGS := -I$(LOCAL_PATH)/include \
                -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

AMF_SO_VER_MAJOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_MINOR  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_PATCH  := $(shell awk '/define\ $(AMF_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMF_VER_FILE))
AMF_SO_VER_STRING := $(AMF_SO_VER_MAJOR).$(AMF_SO_VER_MINOR).$(AMF_SO_VER_PATCH)

AMF_SO_NAME   = $(patsubst %.so,%.so.$(AMF_SO_VER_MAJOR), $(AMF_TARGET))
AMF_SO_TARGET = $(patsubst %.so,%.so.$(AMF_SO_VER_STRING), $(AMF_TARGET))

LOCAL_SO_NAME := $(AMF_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(AMF_SO_TARGET)
	@ln -sf $(AMF_SO_TARGET) $(CAMERA_LIB_DIR)/$(AMF_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

