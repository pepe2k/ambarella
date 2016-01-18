##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

##----------------------------------------------------------------------------
LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= liblive555.a
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/live555/BasicUsageEnvironment/*.cpp) \
	$(wildcard $(LOCAL_PATH)/live555/BasicUsageEnvironment/*.c) \
	$(wildcard $(LOCAL_PATH)/live555/groupsock/*.cpp) \
	$(wildcard $(LOCAL_PATH)/live555/groupsock/*.c) \
	$(wildcard $(LOCAL_PATH)/live555/UsageEnvironment/*.cpp) \
	$(wildcard $(LOCAL_PATH)/live555/UsageEnvironment/*.c) \
	$(wildcard $(LOCAL_PATH)/live555/liveMedia/*.cpp) \
	$(wildcard $(LOCAL_PATH)/live555/liveMedia/*.c)

LOCAL_CFLAGS	:= -DLINUX
LOCAL_CFLAGS += -I./
LOCAL_CFLAGS += -I$(LOCAL_PATH)/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/BasicUsageEnvironment/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/UsageEnvironment/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/groupsock/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/liveMedia/include

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

##----------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_TARGET	:= librtspvod.a
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/librtspvod/*.cpp)

LOCAL_CFLAGS	:= -DLINUX
LOCAL_CFLAGS += -I./
LOCAL_CFLAGS += -I$(LOCAL_PATH)/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/BasicUsageEnvironment/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/UsageEnvironment/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/groupsock/include
LOCAL_CFLAGS += -I$(LOCAL_PATH)/live555/liveMedia/include

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."
	
$(call add-target-into-build, $(LOCAL_TARGET))
