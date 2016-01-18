##
## camera/modules/configure/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_SIMPLECAM), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera

LOCAL_TARGET := test_simplecam
LOCAL_SRCS   := $(LOCAL_PATH)/test_simplecam_main.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -lpthread \
                -lstdc++
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamsimplecam.so \
                libamvdevice.so

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

########
ifeq ($(CONFIG_ARCH_A5S), y)
ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_SIMPLEPHOTO), y)
LOCAL_TARGET := test_simplephoto
LOCAL_SRCS   := $(LOCAL_PATH)/test_simplephoto_main.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -lpthread \
                -lstdc++
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamsimplephoto.so \
                libamvdevice.so

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
endif

########
ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_ENCDEV), y)
LOCAL_TARGET := test_encdev
LOCAL_SRCS   := $(LOCAL_PATH)/test_encdev_main.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -lpthread \
                -lstdc++
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamencdev.so \
                libamvdevice.so \
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
