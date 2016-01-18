##
## packages/lbr/arch_s2/make.inc
##
## History:
##    2014/02/17 - [Louis Sun] Created file
##
## Copyright (C) 2014-2018, Ambarella, Inc.
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
LIB_DIR = $(FAKEROOT_DIR)/usr/lib

LOCAL_TARGET  := liblbr.so

LOCAL_SRCS    := $(wildcard $(LOCAL_PATH)/*.c )
LOCAL_CFLAGS  := -Wall -O3 -I. -I$(AMB_TOPDIR)/build/include \
                 -I$(LOCAL_PATH)/../include
LOCAL_LDFLAGS := -lpthread -lrt

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(LIB_DIR)/
	@cp -dpRf $< $(LIB_DIR)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
