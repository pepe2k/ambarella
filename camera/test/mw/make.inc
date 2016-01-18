##
## camera/test/mw/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##    2012/11/21 - [Zhikan Yang] Modified file
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

ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_WEBCAM), y)

LOCAL_TARGET := test_webcam
LOCAL_SRCS   := $(LOCAL_PATH)/test_webcam_main.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                -L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
                -L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz \
                -lpthread \
                -lstdc++ \
                -lrt
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamcam.so   \
                libamwebcam.so \
                libamencdev.so \
                libamvdevice.so \
                libamoverlay.so \
                libutils.so \
                libmainpp.so \
                libampmdptz.so

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

###############################
ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_FISHCAM), y)
LOCAL_TARGET := test_fisheyecam
LOCAL_SRCS   := $(LOCAL_PATH)/test_fisheyecam_main.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror -I$(AMB_TOPDIR)/packages/utils/
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                -L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
                -L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz \
                -lpthread \
                -lstdc++ \
                -lrt
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamcam.so   \
                libamfisheyecam.so \
                libamwarpdev.so \
                libamvdevice.so \
                libamoverlay.so \
                libamdewarp.so \
                libutils.so \
                libmainpp.so \
                libampmdptz.so


LOCAL_LDFLAGS+= -L$(AMB_TOPDIR)/prebuild/dewarp -ldewarp

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
