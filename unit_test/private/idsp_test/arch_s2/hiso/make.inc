##
## unit_test/private/idsp_test/arch_s2/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_hiso_idsp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_hiso_idsp.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2_hiso
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_hiso_tuning
LOCAL_SRCS	:= $(LOCAL_PATH)/test_hiso_tuning.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2_hiso
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

####
### Remove this BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP_1 to compile if needed.
ifeq ($(BUILD_AMBARELLA_IMGPROC), y)
ifeq ($(BUILD_AMBARELLA_IDSP_HISO), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_hiso_ituner
LOCAL_SRCS	:= $(LOCAL_PATH)/test_hiso_ituner.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_HdlrManager.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_TextHdlr.c
LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaDSP_ImgCalibItunerImpl.c

LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2_hiso
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(AMB_TOPDIR)/packages/img_dsp/arch_a9/

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif

