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

ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_CLOUD_UPLOADER), y)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_TARGET := test_cloud_uploader
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/*.cpp)
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -I$(CAMERA_DIR)/test/clouduploader/cloud_libs/include -Werror
LOCAL_LIBS   := libamutils.so libamdatatransfer.so
LOCAL_LDFLAGS:= -L$(CAMERA_DIR)/test/clouduploader/cloud_libs/lib/$(AMBARELLA_CPU_ARCH)/ -lmwcg_cloud_lib -lmwcg_common -lpthread

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
endif

