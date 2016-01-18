##
## camera/test/network/make.inc
##
## History:
##    2012/11/05 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_TEST_NETWORK), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR = $(AMB_TOPDIR)/camera
LOCAL_TARGET := test_network_manager
LOCAL_SRCS   := $(LOCAL_PATH)/test_network_manager.cpp
LOCAL_LIBS   := libamutils.so libamnetwork.so
LOCAL_CFLAGS := -I$(CAMERA_DIR)/include
LOCAL_LDFLAGS:= -L$(PREBUILD_3RD_PARTY_DIR)/glib2/usr/lib \
                -lglib-2.0 -lgio-2.0 -lgmodule-2.0 -lgobject-2.0 -lgthread-2.0 \
                -L$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/usr/lib -ldbus-glib-1 \
                -L$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib -ldbus-1              \
                -L$(PREBUILD_3RD_PARTY_DIR)/expat/usr/lib -lexpat              \
                -L$(PREBUILD_3RD_PARTY_DIR)/networkmanager/usr/lib             \
                -lnm-glib -lnm-util                                            \
                -L$(PREBUILD_3RD_PARTY_DIR)/util-linux/usr/lib -luuid          \
                -L$(PREBUILD_3RD_PARTY_DIR)/gnutls/usr/lib -lgnutls            \
                -L$(PREBUILD_3RD_PARTY_DIR)/libgcrypt/usr/lib -lgcrypt         \
                -L$(PREBUILD_3RD_PARTY_DIR)/libgpg-error/usr/lib -lgpg-error   \
                -L$(PREBUILD_3RD_PARTY_DIR)/nettle/usr/lib -lnettle -lhogweed  \
                -L$(PREBUILD_3RD_PARTY_DIR)/gmp/usr/lib -lgmp                  \
                -L$(PREBUILD_3RD_PARTY_DIR)/libtasn1/usr/lib -ltasn1           \
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
                -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz \
                -L$(PREBUILD_3RD_PARTY_DIR)/libffi/usr/lib -lffi \
                -L$(PREBUILD_3RD_PARTY_DIR)/ncurses/usr/lib \
                -lpanelw -lmenuw -lformw -lncursesw -ltinfo \
                -lpthread -lstdc++ -ldl -lrt -lresolv

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
