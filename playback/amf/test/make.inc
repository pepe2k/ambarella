##
## ambarella/playback/amf/test/make.inc
##
## History:
##    2013/04/01 - [Roy Su] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_PLAYBACK_AMF), y)

LOCAL_PATH	:= $(call my-dir)
PLAYBACK_TOPDIR	:= $(AMB_TOPDIR)/playback

include $(CLEAR_VARS)

MODULE_INCLUDES	:= $(AMBARELLA_CFLAGS) -I$(AMF_TOP)/include -I$(PLAYBACK_FFMPEG_TOP) \
			$(V_COMPILE_CFLAGS) \
			$(AMBARELLA_APP_CFLAGS) \
			-I$(AMF_TOP)/android \
			-I$(AMF_TOP)/src/filters/general \
			-I$(AMF_TOP)/src/engines \
			-I$(AMF_TOP)/src/framework \
			-I$(AMF_TOP)/src/audio \
			-I$(AMF_TOP)/src/filters/general/misc

LIBS_INCLUDES	:= libamf_linux.so libavformat.so libavcodec.so libavutil.so libinjector.a librtmp.a librtspvod.a liblive555.a librtspclient.a

LOCAL_LDFLAGS:= $(AMBARELLA_LDFLAGS) -lpthread -ldl -lrt
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/alsa-lib/usr/lib -lasound
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/aac/lib -laacdec -laacenc
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/openssl/usr/lib -lssl -lcrypto
LOCAL_LDFLAGS += -L$(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz

LOCAL_LIBS		:=  $(LIBS_INCLUDES)
LOCAL_CFLAGS	:=	$(MODULE_INCLUDES) -Wall

##pbtest
ifeq ($(CONFIG_PLAYBACK_AMF_ENABLE_PBTEST), y)
CFG_FILE_PATH := $(LOCAL_PATH)
LOCAL_TARGET	:= pbtest
LOCAL_SRCS	:=  $(LOCAL_PATH)/pbtest.cpp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@cp $(CFG_FILE_PATH)/pb.config $(APP_INSTALL_PATH)/
	@cp $(CFG_FILE_PATH)/log.config $(APP_INSTALL_PATH)/
	@cp $(CFG_FILE_PATH)/autoplay $(APP_INSTALL_PATH)/
	@cp $(CFG_FILE_PATH)/test_vout.sh $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

##mdectest
ifeq ($(CONFIG_PLAYBACK_AMF_ENABLE_MDECTEST), y)
MDECTEST_FLAG += -L$(PLAYBACK_TOPDIR)/amf/separate -ljpegenc
MDECTEST_FLAG += -L$(PLAYBACK_TOPDIR)/external/jpeg-6b -ljpegdec -I$(PLAYBACK_TOPDIR)/external/jpeg-6b -I$(PLAYBACK_TOPDIR)/external/jpeg-6b/test
LOCAL_LDFLAGS += $(MDECTEST_FLAG)

LOCAL_TARGET	:= mdectest
LOCAL_SRCS	:=  $(LOCAL_PATH)/mdectest.cpp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

##rectest
ifeq ($(CONFIG_PLAYBACK_AMF_ENABLE_RECTEST), y)
endif

endif

