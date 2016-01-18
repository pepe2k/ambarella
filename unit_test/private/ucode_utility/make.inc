##
## unit_test/private/ucode_utility/make.inc
##
## History:
##    2013/06/05 - [Jian Tang] Created file
##
## Copyright (C) 2013-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)


ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_UCODE), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= load_ucode
LOCAL_SRCS	:= $(LOCAL_PATH)/load_ucode.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= dsplog
LOCAL_SRCS	:= $(LOCAL_PATH)/dsplog.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@if [ ! -e $(UNIT_TEST_PATH)/capture_log ]; then ln -s dsplog $(UNIT_TEST_PATH)/capture_log; fi
	@if [ -e $(UNIT_TEST_PATH)/capture_log.sh ]; then cp -dpRf capture_log.sh $(UNIT_TEST_PATH)/; fi
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= dsplog2
LOCAL_SRCS	:= $(LOCAL_PATH)/dsplog2.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@if [ ! -e $(UNIT_TEST_PATH)/parse_log ]; then ln -s dsplog2 $(UNIT_TEST_PATH)/parse_log; fi
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= dsplog_cap
LOCAL_SRCS	:= $(LOCAL_PATH)/dsplog_cap.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))



##
endif

