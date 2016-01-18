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

LOCAL_TARGET	:= librtspclient.a
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.cpp) \
	$(wildcard $(LOCAL_PATH)/utils/*.cpp) \
	$(wildcard $(LOCAL_PATH)/rtsp_manager/*.cpp) \
	$(wildcard $(LOCAL_PATH)/rtsp_message/*.cpp) \
	$(wildcard $(LOCAL_PATH)/rtsp_tcp/*.cpp) \
	$(wildcard $(LOCAL_PATH)/rtsp_rtp/*.cpp)

LOCAL_CFLAGS	:= $(AMF_CFLAG)
LOCAL_CFLAGS += -I./ \
	-I$(LOCAL_PATH)/utils \
	-I$(LOCAL_PATH)/rtsp_tcp \
	-I$(LOCAL_PATH)/rtsp_rtp \
	-I$(LOCAL_PATH)/rtsp_message \
	-I$(LOCAL_PATH)/rtsp_manager

LOCAL_CFLAGS += -I$(PLAYBACK_FFMPEG_TOP) \
	-I$(PLAYBACK_FFMPEG_TOP)/libavformat

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))