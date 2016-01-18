##
## camera/modules/low_bitrate/make.inc
##
## History:
##    2014/03/13 - [Bin Wang] Created file
##
## Copyright (C) 2014-2017, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_LBR_PACKAGE), y)

LOCAL_PATH := $(call my-dir)

###
include $(CLEAR_VARS)
AIR_LIB_DIR   = $(FAKEROOT_DIR)/usr/lib

LOCAL_TARGET  := liblbrctrl.so

LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.cpp )
LOCAL_CFLAGS  := -Wall -O3 -I. -I$(AMB_TOPDIR)/build/include \
                 -I$(AMB_TOPDIR)/packages/lbr/include \
                 -I$(AMB_TOPDIR)/camera/include \
                 -I$(AMB_TOPDIR)/camera/include/datastructure

LOCAL_LIBS := liblbr.so libamp.a libamd.a
LOCAL_LIBS += libimg_algo_a5s.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(AIR_LIB_DIR)/
	@cp -dpRf $< $(AIR_LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif