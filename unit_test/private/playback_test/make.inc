##
## unit_test/private/playback_test/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)


ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PLAYBACK), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test2
LOCAL_SRCS	:= $(LOCAL_PATH)/test2.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= udec
LOCAL_SRCS	:= $(LOCAL_PATH)/udec.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= mudec
LOCAL_SRCS	:= $(LOCAL_PATH)/mudec.c $(LOCAL_PATH)/udecutil.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PLAYBACK_EXP), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= duplex
LOCAL_SRCS	:= $(LOCAL_PATH)/duplex.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PLAYBACK_EXP), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= pb_fb
LOCAL_SRCS	:= $(LOCAL_PATH)/test_fb.c $(LOCAL_PATH)/udecutil.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_PLAYBACK_EXP), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= test3
LOCAL_SRCS	:= $(LOCAL_PATH)/test3.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
endif

