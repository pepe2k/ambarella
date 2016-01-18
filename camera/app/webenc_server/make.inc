##
## camera/app/webenc_server/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_APP_WEBENC_SERVER), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera

LOCAL_TARGET := webenc_server

ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_SRCS   := $(LOCAL_PATH)/webenc_server_s2.cpp
else
LOCAL_SRCS   := $(LOCAL_PATH)/webenc_server_a5s.cpp
endif

LOCAL_CFLAGS := -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/freetype/include -Werror \
                -I$(AMB_TOPDIR)/packages/utils/
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib \
				-L$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2\
				-L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz -lfreetype \
                -lrt \
                -lpthread \
                -lstdc++
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamencdev.so \
                libamcam.so    \
                libamwebcam.so \
                libamvdevice.so \
                libamoverlay.so \
                libampmdptz.so

ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS +=   libamfisheyecam.so \
                libamwarpdev.so \
                libamdewarp.so \
                libutils.so \
				libmainpp.so
LOCAL_LDFLAGS+= -L$(AMB_TOPDIR)/prebuild/dewarp -ldewarp
LOCAL_LDFLAGS	+= -lpthread -lm
endif

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

########
LOCAL_TARGET := test_cmd_sender
LOCAL_SRCS   := $(LOCAL_PATH)/test_cmd_sender.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/freetype/include -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -lrt \
                -lpthread \
                -lstdc++
LOCAL_LIBS   := libamutils.so

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

