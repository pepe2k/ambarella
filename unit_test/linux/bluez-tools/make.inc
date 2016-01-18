##
## unit_test/linux/audio/make.inc
##
## History:
##    2012/05/31 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_BLUEZ_TOOLS), y)

LOCAL_PATH	:= $(call my-dir)

BLUEZ_TOOLS_CFLAGS	:=  -std=gnu99 \
		-DHAVE_CONFIG_H	\
		-include$(LOCAL_PATH)/src/lib/config.h 	\
		-I$(LOCAL_PATH)/src/lib 			\
		-I$(PREBUILD_3RD_PARTY_DIR)/dbus/include/dbus-1.0	\
		-I$(PREBUILD_3RD_PARTY_DIR)/glib2/include/glib-2.0	\
		-I$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/include 	\
		-I$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/include/dbus-1.0 \

BLUEZ_TOOLS_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib -ldbus-1\
		-L$(PREBUILD_3RD_PARTY_DIR)/expat/usr/lib -lexpat	\
		-L$(PREBUILD_3RD_PARTY_DIR)/glib2/usr/lib -lglib-2.0 \
		-L$(PREBUILD_3RD_PARTY_DIR)/libffi/usr/lib -lffi 	\
		-L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz 	\
		-lgobject-2.0 -lgmodule-2.0  -lgthread-2.0 -lgio-2.0 	\
		-L$(PREBUILD_3RD_PARTY_DIR)/dbus-glib/usr/lib -ldbus-glib-1 -lpthread -ldl -lrt -lresolv

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bt-device
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/src/bt-device.c) \
		$(wildcard $(LOCAL_PATH)/src/lib/*.c) 	\
		$(wildcard $(LOCAL_PATH)/src/lib/bluez/*.c)

LOCAL_CFLAGS	:= $(BLUEZ_TOOLS_CFLAGS)

LOCAL_LDFLAGS	:= $(BLUEZ_TOOLS_LDFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bt-adapter
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/src/bt-adapter.c) \
		$(wildcard $(LOCAL_PATH)/src/lib/*.c) 	\
		$(wildcard $(LOCAL_PATH)/src/lib/bluez/*.c)

LOCAL_CFLAGS	:= $(BLUEZ_TOOLS_CFLAGS)

LOCAL_LDFLAGS	:= $(BLUEZ_TOOLS_LDFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bt-agent
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/src/bt-agent.c) \
		$(wildcard $(LOCAL_PATH)/src/lib/*.c) 	\
		$(wildcard $(LOCAL_PATH)/src/lib/bluez/*.c)

LOCAL_CFLAGS	:= $(BLUEZ_TOOLS_CFLAGS)

LOCAL_LDFLAGS	:= $(BLUEZ_TOOLS_LDFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bt-audio
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/src/bt-audio.c) \
		$(wildcard $(LOCAL_PATH)/src/lib/*.c) 	\
		$(wildcard $(LOCAL_PATH)/src/lib/bluez/*.c)

LOCAL_CFLAGS	:= $(BLUEZ_TOOLS_CFLAGS)

LOCAL_LDFLAGS	:= $(BLUEZ_TOOLS_LDFLAGS)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

# end of BUILD_AMBARELLA_UNIT_TESTS_BLUEZ_TOOLS
endif


