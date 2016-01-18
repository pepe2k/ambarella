##
## camera/modules/utilities/make.inc
##
## History:
##    2013/04/07 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_WSDISCOVERY), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
WSDISCOVERY_VER_FILE   = $(CAMERA_DIR)/include/wsdiscovery/version.h
WSDISCOVERY_VER_PREFIX = WSDISCOVERY

WSDISCOVERY_TARGET  = libamwsdiscovery.so
LOCAL_TARGET := $(WSDISCOVERY_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/wsdiscovery \
                -I$(LOCAL_PATH) \
                -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

WSDISCOVERY_SO_VER_MAJOR  := $(shell awk '/define\ $(WSDISCOVERY_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(WSDISCOVERY_VER_FILE))
WSDISCOVERY_SO_VER_MINOR  := $(shell awk '/define\ $(WSDISCOVERY_VER_PREFIX)_LIB_MINOR/{print $$3}' $(WSDISCOVERY_VER_FILE))
WSDISCOVERY_SO_VER_PATCH  := $(shell awk '/define\ $(WSDISCOVERY_VER_PREFIX)_LIB_PATCH/{print $$3}' $(WSDISCOVERY_VER_FILE))
WSDISCOVERY_SO_VER_STRING := $(WSDISCOVERY_SO_VER_MAJOR).$(WSDISCOVERY_SO_VER_MINOR).$(WSDISCOVERY_SO_VER_PATCH)

WSDISCOVERY_SO_NAME   = $(patsubst %.so,%.so.$(WSDISCOVERY_SO_VER_MAJOR), $(WSDISCOVERY_TARGET))
WSDISCOVERY_SO_TARGET = $(patsubst %.so,%.so.$(WSDISCOVERY_SO_VER_STRING), $(WSDISCOVERY_TARGET))

LOCAL_SO_NAME := $(WSDISCOVERY_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(WSDISCOVERY_SO_TARGET)
	@ln -sf $(WSDISCOVERY_SO_TARGET) $(CAMERA_LIB_DIR)/$(WSDISCOVERY_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif