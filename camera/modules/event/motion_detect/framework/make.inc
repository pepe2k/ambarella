##
## camera/modules/configure/make.inc
##
## History:
##    Jan-09-2014 - [HuaiShun Hu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_EVENT), y)

LOCAL_PATH  := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
MD_VER_FILE  = $(CAMERA_DIR)/include/event/motion_detect/version.h
MD_VER_PREFIX  = MD

MD_TARGET        = motion-detect.so

LOCAL_LIBS     :=
LOCAL_TARGET   := $(MD_TARGET)
LOCAL_SRCS     := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS   := -I$(CAMERA_DIR)/include  \
                  -I$(CAMERA_DIR)/include/event/motion_detect \
                  -I$(CAMERA_DIR)/include/event/framework \
                  -I$(CAMERA_DIR)/modules/event/motion_detect/framework \
                  -I$(CAMERA_DIR)/modules/event/motion_detect/md_mse \
                  -I$(CAMERA_DIR)/modules/event/motion_detect/md_mog2 \
                  -Werror
LOCAL_LDFLAGS := -ldl

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

LOCAL_SO_NAME := $(MD_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_EVENT_PLUGIN_DIR)/
	@cp -dpRf $< $(CAMERA_EVENT_PLUGIN_DIR)/$(MD_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
