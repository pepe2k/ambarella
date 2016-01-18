##
## unit_test/private/gyro_test/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)


###

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_GYRO), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_gyro
LOCAL_SRCS	:= $(LOCAL_PATH)/test_gyro.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_timer2
LOCAL_SRCS	:= $(LOCAL_PATH)/test_timer2.c $(LOCAL_PATH)/gyro.c
LOCAL_LDFLAGS	:= -lpthread -lrt

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

