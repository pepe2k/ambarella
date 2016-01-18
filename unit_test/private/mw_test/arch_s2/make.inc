##
## unit_test/private/mw_test/make.inc
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

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_MW), y)
ifeq ($(BUILD_AMBARELLA_MIDDLEWARE_PROC), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_image
LOCAL_SRCS	:= $(LOCAL_PATH)/test_image.c $(LOCAL_PATH)/fb_image.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
LOCAL_CFLAGS 	:=

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/packages/textinsert
LOCAL_LDFLAGS	+= -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                   -L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
                   -L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz

LOCAL_LIBS += libtextinsert.a
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/test_image.cfg $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_IDSP_HISO), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_image_hiso
LOCAL_SRCS	:= $(LOCAL_PATH)/test_image_hiso.c $(LOCAL_PATH)/fb_image.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp_hiso.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2_hiso
LOCAL_CFLAGS 	:=

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_PACKAGE), y)
LOCAL_CFLAGS += -I$(AMB_TOPDIR)/packages/textinsert
LOCAL_LDFLAGS	+= -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                   -L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
                   -L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz

LOCAL_LIBS += libtextinsert.a
endif

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)


$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/test_image.cfg $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_motion_detect
LOCAL_SRCS	:= $(LOCAL_PATH)/test_motion_detect.c
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	:= libamp.so
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

