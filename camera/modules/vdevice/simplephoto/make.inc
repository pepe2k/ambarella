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
ifeq ($(CONFIG_ARCH_A5S), y)
ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_SIMPLEPHOTO), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
SIMPLEPHOTO_VER_FILE   = $(CAMERA_DIR)/include/vdevice/version.h
SIMPLEPHOTO_VER_PREFIX = SIMPLEPHOTO

SIMPLEPHOTO_TARGET  = libamsimplephoto.so
LOCAL_TARGET := $(SIMPLEPHOTO_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_LIBS	:=
LOCAL_LDFLAGS	:=
LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/vdevice \
                -I$(LOCAL_PATH)/../include \
                -I$(CAMERA_DIR)/include

SIMPLEPHOTO_SO_VER_MAJOR  := $(shell awk '/define\ $(SIMPLEPHOTO_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(SIMPLEPHOTO_VER_FILE))
SIMPLEPHOTO_SO_VER_MINOR  := $(shell awk '/define\ $(SIMPLEPHOTO_VER_PREFIX)_LIB_MINOR/{print $$3}' $(SIMPLEPHOTO_VER_FILE))
SIMPLEPHOTO_SO_VER_PATCH  := $(shell awk '/define\ $(SIMPLEPHOTO_VER_PREFIX)_LIB_PATCH/{print $$3}' $(SIMPLEPHOTO_VER_FILE))
SIMPLEPHOTO_SO_VER_STRING := $(SIMPLEPHOTO_SO_VER_MAJOR).$(SIMPLEPHOTO_SO_VER_MINOR).$(SIMPLEPHOTO_SO_VER_PATCH)

SIMPLEPHOTO_SO_NAME   = $(patsubst %.so,%.so.$(SIMPLEPHOTO_SO_VER_MAJOR), $(SIMPLEPHOTO_TARGET))
SIMPLEPHOTO_SO_TARGET = $(patsubst %.so,%.so.$(SIMPLEPHOTO_SO_VER_STRING), $(SIMPLEPHOTO_TARGET))

LOCAL_SO_NAME := $(SIMPLEPHOTO_SO_NAME)

LOCAL_LIBS	+= libimg_algo_a5s.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(SIMPLEPHOTO_SO_TARGET)
	@ln -sf $(SIMPLEPHOTO_SO_TARGET) $(CAMERA_LIB_DIR)/$(SIMPLEPHOTO_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif
