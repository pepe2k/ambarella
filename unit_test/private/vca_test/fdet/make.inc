##
## unit_test/private/fdet_test/make.inc
##
## History:
##    2012/07/10 - [Zhenwu Xue] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_FDET), y)

LOCAL_PATH	:= $(call my-dir)
UTILITIES_PATH	:= $(LOCAL_PATH)/../utilities

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_fdet
LOCAL_SRCS	:= $(LOCAL_PATH)/test_fdet.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@mkdir -p $(UNIT_TEST_PATH)/vca_data/fdet/classifiers
	@mkdir -p $(UNIT_TEST_PATH)/vca_data/fdet/samples
	@cp -dpRf $(PRIVATE_PATH)/classifiers/* $(UNIT_TEST_PATH)/vca_data/fdet/classifiers
	@cp -dpRf $(PRIVATE_PATH)/samples/* $(UNIT_TEST_PATH)/vca_data/fdet/samples
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_fdet2
LOCAL_CFLAGS	+= -I$(UTILITIES_PATH)
LOCAL_SRCS	:= $(LOCAL_PATH)/test_fdet2.cpp $(UTILITIES_PATH)/pgm.cpp $(UTILITIES_PATH)/fb16.cpp $(UTILITIES_PATH)/fdet.cpp $(UTILITIES_PATH)/iav.cpp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)	
	@cp -dpRf $< $(UNIT_TEST_PATH)/	
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

