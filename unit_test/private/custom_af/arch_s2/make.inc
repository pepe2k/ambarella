##
## unit_test/private/custom_af/make.inc
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
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= custom_af
LOCAL_SRCS	:= $(LOCAL_PATH)/main.c $(LOCAL_PATH)/af_algo.c \
			$(LOCAL_PATH)/tamronDF003_drv.c \
			$(LOCAL_PATH)/dummy_af_drv.c
LOCAL_CFLAGS	:= -lrt
LOCAL_CFLAGS	+= -I$(ADJ_PARAMS_PATH)
LOCAL_LIBS	:=
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))


