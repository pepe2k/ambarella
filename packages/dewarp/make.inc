##
## packages/dewarp/make.inc
##
## History:
##    2012/09/13 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_DEWARP_PACKAGE), y)

DEWARP_INSTALL_DIR	:= $(shell echo $(CONFIG_DEWARP_INSTALL_DIR))
DEWARP_SO	:= libdewarp.so

LOCAL_PATH	:= $(call my-dir)

###
ifeq ($(BUILD_AMBARELLA_DEWARP_DYNAMIC), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= libdewarp.so
LOCAL_SRCS	:=

LOCAL_CFLAGS	:= -Wall -O3 -I $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH) -I $(LOCAL_PATH) -I$(AMB_TOPDIR)/packages/utils

DEWARP_VER_PREFIX = DEWARP
DEWARP_VERSION_FILE = $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/dewarp_version.c
DEWARP_SO_VER_MAJOR  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(DEWARP_VERSION_FILE))
DEWARP_SO_VER_MINOR  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(DEWARP_VERSION_FILE))
DEWARP_SO_VER_PATCH  := $(shell awk '/define\ $(DEWARP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(DEWARP_VERSION_FILE))
DEWARP_SO_VER_STRING := $(DEWARP_SO_VER_MAJOR).$(DEWARP_SO_VER_MINOR).$(DEWARP_SO_VER_PATCH)
DEWARP_SO_NAME   = $(patsubst %.so,%.so.$(DEWARP_SO_VER_MAJOR), $(DEWARP_SO))
DEWARP_SO_TARGET = $(patsubst %.so,%.so.$(DEWARP_SO_VER_STRING), $(DEWARP_SO))

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET):
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $(DEWARP_INSTALL_DIR)/$(DEWARP_SO) $(FAKEROOT_DIR)/usr/lib/$(DEWARP_SO_TARGET)
	@ln -sf $(DEWARP_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(DEWARP_SO_NAME)
	@ln -sf $(DEWARP_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(DEWARP_SO)
	@cp -dpRf $(PRIVATE_PATH)/lens_data/ $(FAKEROOT_DIR)/usr/local/bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	# ifeq ($(BUILD_AMBARELLA_DEWARP_DYNAMIC), y)

###
ifeq ($(BUILD_AMBARELLA_DEWARP_STATIC), y)

#
# Before compiling this target and release, make sure "-fPIC" option is
# DISABLED ! It is included in line 154 of "ambarella/build/core/configure.mk" file.
#
include $(CLEAR_VARS)

LOCAL_TARGET	:= libdewarp_noglibc.a
LOCAL_SRCS	:=

LOCAL_CFLAGS	:= -Wall -O3 -I $(LOCAL_PATH) -I$(AMB_TOPDIR)/packages/utils -D NO_GLIBC
LOCAL_CFLAGS	+= -mabi=aapcs-linux

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET):
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@mkdir -p $(FAKEROOT_DIR)/usr/local/bin
	@cp -dpRf $(PRIVATE_PATH)/lens_data/ $(FAKEROOT_DIR)/usr/local/bin/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	# ifeq ($(BUILD_AMBARELLA_DEWARP_STATIC), y)

endif	# ifeq ($(BUILD_AMBARELLA_DEWARP_PACKAGE), y)

