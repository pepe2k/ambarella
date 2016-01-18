##
## camera/mw/webcam/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_MW), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
WEBCAM_VER_FILE   = $(CAMERA_DIR)/include/mw/version.h
WEBCAM_VER_PREFIX = WEBCAM

WEBCAM_TARGET  = libamwebcam.so
LOCAL_TARGET := $(WEBCAM_TARGET)
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include/vdevice \
                -I$(CAMERA_DIR)/include \
                -I$(AMB_TOPDIR)/packages/textinsert \
                -I$(PREBUILD_3RD_PARTY_DIR)/freetype/include

WEBCAM_SO_VER_MAJOR  := $(shell awk '/define\ $(WEBCAM_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(WEBCAM_VER_FILE))
WEBCAM_SO_VER_MINOR  := $(shell awk '/define\ $(WEBCAM_VER_PREFIX)_LIB_MINOR/{print $$3}' $(WEBCAM_VER_FILE))
WEBCAM_SO_VER_PATCH  := $(shell awk '/define\ $(WEBCAM_VER_PREFIX)_LIB_PATCH/{print $$3}' $(WEBCAM_VER_FILE))
WEBCAM_SO_VER_STRING := $(WEBCAM_SO_VER_MAJOR).$(WEBCAM_SO_VER_MINOR).$(WEBCAM_SO_VER_PATCH)

WEBCAM_SO_NAME   = $(patsubst %.so,%.so.$(WEBCAM_SO_VER_MAJOR), $(WEBCAM_TARGET))
WEBCAM_SO_TARGET = $(patsubst %.so,%.so.$(WEBCAM_SO_VER_STRING), $(WEBCAM_TARGET))

LOCAL_SO_NAME := $(WEBCAM_SO_NAME)



include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)



$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(WEBCAM_SO_TARGET)
	@ln -sf $(WEBCAM_SO_TARGET) $(CAMERA_LIB_DIR)/$(WEBCAM_SO_NAME)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

