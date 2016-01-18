##
## camera/test/motion_detect/make.inc
##
## History:
##    2014/01/10 - [HuaiShun Hu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
MD_HEADER_DIR = $(CAMERA_DIR)/include/event/motion_detect

LOCAL_TARGET := test_am_motd
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -I$(MD_HEADER_DIR)  \
                -I$(CAMERA_DIR)/include                     \
                -I$(CAMERA_DIR)/include/datastructure       \
                -I$(CAMERA_DIR)/include/event/framework     \
                -Werror
LOCAL_LDFLAGS:= -lpthread -ldl \
                -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser
LOCAL_LIBS   := libamutils.so libamevent.so libamconfigure.so

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_BIN_DIR)/
	@cp -dpRf $< $(CAMERA_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

