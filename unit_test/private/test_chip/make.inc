##
## unit_test/private/test_chip/make.inc
##
## History:
##    2012/12/10 - [BingLiang Hu] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_CHIP), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_chip
LOCAL_SRCS	:= $(LOCAL_PATH)/test_chip.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libdatatx.a
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

