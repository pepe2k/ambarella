##
## camera/modules/configure/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_DATATRANSFER), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
DATA_TRANSFER_VER_FILE   = $(CAMERA_DIR)/include/transfer/version.h
DATA_TRANSFER_VER_PREFIX = DATA_TRANSFER

DATA_TRANSFER_TARGET  = libamdatatransfer.so
LOCAL_TARGET := $(DATA_TRANSFER_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

DATA_TRANSFER_SO_VER_MAJOR  := $(shell awk '/define\ $(DATA_TRANSFER_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(DATA_TRANSFER_VER_FILE))
DATA_TRANSFER_SO_VER_MINOR  := $(shell awk '/define\ $(DATA_TRANSFER_VER_PREFIX)_LIB_MINOR/{print $$3}' $(DATA_TRANSFER_VER_FILE))
DATA_TRANSFER_SO_VER_PATCH  := $(shell awk '/define\ $(DATA_TRANSFER_VER_PREFIX)_LIB_PATCH/{print $$3}' $(DATA_TRANSFER_VER_FILE))
DATA_TRANSFER_SO_VER_STRING := $(DATA_TRANSFER_SO_VER_MAJOR).$(DATA_TRANSFER_SO_VER_MINOR).$(DATA_TRANSFER_SO_VER_PATCH)

DATA_TRANSFER_SO_NAME   = $(patsubst %.so,%.so.$(DATA_TRANSFER_SO_VER_MAJOR), $(DATA_TRANSFER_TARGET))
DATA_TRANSFER_SO_TARGET = $(patsubst %.so,%.so.$(DATA_TRANSFER_SO_VER_STRING), $(DATA_TRANSFER_TARGET))

LOCAL_SO_NAME := $(DATA_TRANSFER_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(DATA_TRANSFER_SO_TARGET)
	@ln -sf $(DATA_TRANSFER_SO_TARGET) $(CAMERA_LIB_DIR)/$(DATA_TRANSFER_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
