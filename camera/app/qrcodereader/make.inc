##
## camera/app/qrcodereader/make.inc
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

ifeq ($(BUILD_AMBARELLA_CAMERA_APP_QRCODEREADER), y)
###############################qrcodereader#####################################
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera

LOCAL_TARGET := qrcodereader
LOCAL_SRCS   := $(LOCAL_PATH)/qr_code_reader.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/zbar/include \
                -fexceptions -Werror
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -L$(PREBUILD_3RD_PARTY_DIR)/zbar/usr/lib -lzbar \
                -lstdc++ -lpthread -lrt
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

###################################qrcode#######################################
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera

LOCAL_TARGET := qrcode
LOCAL_SRCS   := $(LOCAL_PATH)/qr_code.cpp
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/zbar/include \
                -fexceptions
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/iniparser/usr/lib -liniparser \
                -L$(PREBUILD_3RD_PARTY_DIR)/zbar/usr/lib -lzbar \
                -lstdc++ -lpthread -lrt
LOCAL_LIBS   := libamutils.so \
                libamconfigure.so \
                libamsimplecam.so \
                libamvdevice.so

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_BIN_DIR)/
	@cp -dpRf $< $(CAMERA_BIN_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
