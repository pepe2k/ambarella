##
## app/ipcam/rtsp/testProgs/make.inc
##
## History:
##    2012/06/25 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
RTSP_LIBS	:= libliveMedia.a libgroupsock.a \
		libBasicUsageEnvironment.a libUsageEnvironment.a
RTSP_INCLUDES	:= -I$(LOCAL_PATH)/../BasicUsageEnvironment/include \
		-I$(LOCAL_PATH)/../groupsock/include \
		-I$(LOCAL_PATH)/../liveMedia/include \
		-I$(LOCAL_PATH)/../UsageEnvironment/include
C_CFLAGS		:= -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
CPP_CFLAGS	:= -DBSD=1 -g -O2

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= openRTSP
LOCAL_SRCS	:= $(LOCAL_PATH)/openRTSP.cpp \
		$(LOCAL_PATH)/playCommon.cpp
LOCAL_LDFLAGS	:= -lpthread
ifeq ($(CONFIG_ARCH_A5S), y)
ifeq ($(BUILD_AMBARELLA_CAVLC_PACKAGE), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
LOCAL_LIBS	+= libcavlc.a
LOCAL_LDFLAGS	+=
endif
endif
ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
endif
ifeq ($(CONFIG_ARCH_A7L), y)
ifeq ($(BUILD_AMBARELLA_CAVLC_PACKAGE), y)
LOCAL_LIBS	:= libbsreader.a $(RTSP_LIBS)
LOCAL_LIBS	+= libcavlc.a
LOCAL_LDFLAGS	+=
endif
endif
LOCAL_CFLAGS	:= $(C_CFLAGS) $(CPP_CFLAGS) $(RTSP_INCLUDES)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
