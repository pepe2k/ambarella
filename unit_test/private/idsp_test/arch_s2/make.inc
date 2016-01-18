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
UNIT_IDSP_PATH	:= $(call my-dir)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LENS_PARAMS_PATH := $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_idsp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_idsp.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
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

LOCAL_TARGET	:= test_get_statistics
LOCAL_SRCS	:= $(LOCAL_PATH)/test_get_statistics.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
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

LOCAL_TARGET	:= test_tuning
LOCAL_SRCS	:= $(LOCAL_PATH)/test_tuning.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(LENS_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_hdr_tuning
LOCAL_SRCS	:= $(LOCAL_PATH)/test_hdr_tuning.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
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

LOCAL_TARGET	:= customer_loop
LOCAL_SRCS	:= $(LOCAL_PATH)/customer_loop.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
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

LOCAL_TARGET	:= hdr_customer_loop
LOCAL_SRCS	:= $(LOCAL_PATH)/customer_loop_hdr.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)
#include $(CLEAR_VARS)

#LOCAL_TARGET	:= test_ik_a9
#LOCAL_SRCS	:= $(LOCAL_PATH)/test_ik_a9.c
#LOCAL_LDFLAGS	:= -lpthread -lm
#LOCAL_LIBS	:=
#LOCAL_LIBS	+= libimg_dsp_a9.a
#LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(AMB_TOPDIR)/packages/img_dsp/arch_a9/

#include $(BUILD_APP)

#.PHONY: $(LOCAL_TARGET)

#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(UNIT_TEST_PATH)/
#	@cp -dpRf $< $(UNIT_TEST_PATH)/
#	@echo "Build $@ Done."

#$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)
#include $(CLEAR_VARS)

#LOCAL_TARGET	:= test_ituner
#LOCAL_SRCS	:= $(LOCAL_PATH)/test_ituner.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_HdlrManager.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaTUNE_TextHdlr.c
#LOCAL_SRCS	+= $(LOCAL_PATH)/AmbaDSP_ImgCalibItunerImpl.c

#LOCAL_LDFLAGS	:= -lpthread -lm
#LOCAL_LIBS	:=
#LOCAL_LIBS	+= libimg_dsp_a9.a
#LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH) -I$(AMB_TOPDIR)/packages/img_dsp/arch_a9/

#include $(BUILD_APP)

#.PHONY: $(LOCAL_TARGET)

#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(UNIT_TEST_PATH)/
#	@cp -dpRf $< $(UNIT_TEST_PATH)/
#	@echo "Build $@ Done."

#$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_raw
LOCAL_SRCS	:= $(LOCAL_PATH)/test_raw.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_raw_seq
LOCAL_SRCS	:= $(LOCAL_PATH)/test_raw_seq.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##$$$$$$$
#include $(CLEAR_VARS)
#
#LOCAL_TARGET	:= test_ae
#LOCAL_SRCS	:= $(LOCAL_PATH)/test_ae.c
#LOCAL_LDFLAGS	:= -lpthread -lm
#LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
#LOCAL_CFLAGS	:= -I$(ADJ_PARAMS_PATH)
#include $(BUILD_APP)
#
#.PHONY: $(LOCAL_TARGET)
#
#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(UNIT_TEST_PATH)/
#	@cp -dpRf $< $(UNIT_TEST_PATH)/
#	@echo "Build $@ Done."
#
#$(call add-target-into-build, $(LOCAL_TARGET))

##$$$$$$$
endif

###
ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)
ifeq ($(BUILD_AMBARELLA_EIS), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_eis
LOCAL_SRCS	:= $(LOCAL_PATH)/test_eis.c $(LOCAL_PATH)/gyro.c
LOCAL_LDFLAGS	:= -lpthread -lrt
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_IDSP_EXP), y)


include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mctf
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mctf.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
ifeq ($(BUILD_AMBARELLA_IDSP_HISO), y)

include $(UNIT_IDSP_PATH)/hiso/make.inc

endif
