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

ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_ADEVICE), y)

LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)
CAMERA_DIR = $(AMB_TOPDIR)/camera

PA_MAJOR = $(shell awk '/define\ PA_MAJOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PA_MINOR = $(shell awk '/define\ PA_MINOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PULSE_VERSION = $(PA_MAJOR).$(PA_MINOR)

LOCAL_TARGET := test_adevice
LOCAL_SRCS   := $(wildcard $(LOCAL_PATH)/test_adevice.cpp)
LOCAL_CFLAGS := -I$(LOCAL_PATH) \
                -I$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include \
                -I$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/include \
                -I$(CAMERA_DIR)/include -Werror

LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/pulseaudio/usr/lib                 \
                -lpulse -lpulsecommon-$(PULSE_VERSION)                         \
                -L$(PREBUILD_3RD_PARTY_DIR)/glib2/usr/lib                      \
                -lglib-2.0 -lgobject-2.0 -lgmodule-2.0 -lgio-2.0 -lgthread-2.0 \
                -L$(PREBUILD_3RD_PARTY_DIR)/libffi/usr/lib -lffi               \
                -L$(PREBUILD_3RD_PARTY_DIR)/expat/usr/lib -lexpat              \
                -L$(PREBUILD_3RD_PARTY_DIR)/libsndfile/usr/lib -lsndfile       \
                -L$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib -ldbus-1              \
                -L$(PREBUILD_3RD_PARTY_DIR)/libtool/usr/lib -lltdl             \
                -L$(PREBUILD_3RD_PARTY_DIR)/libsamplerate/usr/lib -lsamplerate \
                -L$(PREBUILD_3RD_PARTY_DIR)/speex/usr/lib -lspeex -lspeexdsp   \
                -L$(PREBUILD_3RD_PARTY_DIR)/json-c/usr/lib -ljson -ljson-c     \
                -L$(PREBUILD_3RD_PARTY_DIR)/orc/usr/lib -lorc-0.4              \
                -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound          \
                -L$(PREBUILD_3RD_PARTY_DIR)/libcurl/usr/lib -lcurl             \
                -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto     \
                -L$(PREBUILD_3RD_PARTY_DIR)/libssh2/usr/lib -lssh2             \
                -L$(PREBUILD_3RD_PARTY_DIR)/libidn/usr/lib -lidn               \
                -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz                   \
                -L$(PREBUILD_3RD_PARTY_DIR)/udev/usr/lib -ludev -lgudev-1.0    \
                -L$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2                \
                -L$(PREBUILD_3RD_PARTY_DIR)/libselinux/usr/lib -lselinux       \
                -L$(PREBUILD_3RD_PARTY_DIR)/libsepol/usr/lib -lsepol           \
                -L$(PREBUILD_3RD_PARTY_DIR)/pam/usr/lib -lpam                  \
                -L$(PREBUILD_3RD_PARTY_DIR)/libcap/usr/lib -lcap               \
                -L$(PREBUILD_3RD_PARTY_DIR)/libattr/usr/lib -lattr             \
                -L$(PREBUILD_3RD_PARTY_DIR)/pcre/usr/lib -lpcre                \
                -L$(PREBUILD_3RD_PARTY_DIR)/readline/usr/lib -lreadline        \
                -lhistory                                                      \
                -L$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/usr/lib -ldbus-glib-1    \
                -L$(PREBUILD_3RD_PARTY_DIR)/ncurses/usr/lib                    \
                -lpanelw -lmenuw -lformw -lncursesw -ltinfo                    \
                -lpthread -lm -ldl -lrt -lstdc++ -lresolv
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/libusbx/usr/lib -lusb-1.0
endif
LOCAL_LIBS   := libamutils.so \
                libadevice.so

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
