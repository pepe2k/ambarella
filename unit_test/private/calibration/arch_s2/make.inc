##
## unit_test/private/calibration/make.inc
##
## History:
##    2013/04/26 - [Qiu Jingyang] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

SCRIPT_FILE_BAD_PIXEL	:= $(LOCAL_PATH)/bad_pixel_calibration.sh
LOCAL_TARGET	:= cali_bad_pixel
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_bad_pixel.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_FILE_BAD_PIXEL) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bitmap_merger
LOCAL_SRCS	:= $(LOCAL_PATH)/bitmap_merger.c
LOCAL_LDFLAGS	:=
LOCAL_LIBS	:=

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

SCRIPT_FILE_LENS_SHADING	:= $(LOCAL_PATH)/lens_shading_calibration.sh
LOCAL_TARGET	:= cali_lens_shading
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_lens_shading.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_FILE_LENS_SHADING) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

SCRIPT_FILE_AWB	:= $(LOCAL_PATH)/awb_calibration.sh
LOCAL_TARGET	:= cali_awb
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_awb.c

LOCAL_CFLAGS	:= \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params \
	-I$(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/aeb_params
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_FILE_AWB) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
###

include $(CLEAR_VARS)

#SCRIPT_FILE_FISH_CENTER	:= $(LOCAL_PATH)/fish_center_calibration.sh
LOCAL_TARGET	:= cali_fish_center
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_fish_center.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/utils
LOCAL_LIBS	:= libutils.so
LOCAL_LDFLAGS := -lrt -lm -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_FILE_FISH_CENTER) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

SCRIPT_FILE_FISH_CENTER := $(LOCAL_PATH)/fish_center_calibration.sh
LOCAL_TARGET  := cali_fisheye_center
LOCAL_SRCS  := $(LOCAL_PATH)/cali_fisheye_center.c $(LOCAL_PATH)/dlist.c
LOCAL_CFLAGS  :=
LOCAL_LIBS  :=
LOCAL_LDFLAGS := -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_FILE_FISH_CENTER) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

SCRIPT_PIRIS	:= $(LOCAL_PATH)/piris_calibration.sh
LOCAL_TARGET	:= cali_piris
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_piris.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/utils
LOCAL_LDFLAGS := -lrt -lm -lpthread
LOCAL_LIBS	:= libutils.so
LOCAL_LIBS 	+= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp $(SCRIPT_PIRIS) $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= cali_lens_focus
LOCAL_SRCS	:= $(LOCAL_PATH)/cali_lens_focus.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:=
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

LOCAL_SRCS	+= $(AMB_TOPDIR)/unit_test/private/mw_test/arch_$(AMBARELLA_ARCH)/fb_image.c
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/unit_test/private/mw_test/arch_$(AMBARELLA_ARCH)/


ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/packages/textinsert
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                 -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz            \
                 -L$(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2
LOCAL_LIBS += libtextinsert.a
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
