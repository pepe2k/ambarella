##
## camera/modules/event/motion_detect/md_mse/make.inc
##
## History:
##    Mar-25-2014 - [HuaiShun Hu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MOTION_DETECT_MSE), y)

LOCAL_PATH  := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
MD_ALGOS_INSTALL_DIR := $(CAMERA_EVENT_PLUGIN_DIR)/md_algos
MSE_TARGET			 := md_mse.so

LOCAL_LIBS     :=
LOCAL_TARGET   := $(MSE_TARGET)
LOCAL_SRCS     := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS   := -I$(CAMERA_DIR)/include  \
                  -I$(CAMERA_DIR)/include/event/framework \
                  -I$(CAMERA_DIR)/include/event/motion_detect \
                  -I$(CAMERA_DIR)/modules/event/motion_detect/framework \
                  -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

LOCAL_SO_NAME := $(LOCAL_TARGET)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(MD_ALGOS_INSTALL_DIR)/
	@cp -dpRf $< $(MD_ALGOS_INSTALL_DIR)/$(MSE_TARGET)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
