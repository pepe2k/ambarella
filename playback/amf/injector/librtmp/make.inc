##
## playback/amf/injector/librtmp/make.inc
##
## History:
##    2013/05/27 - [Roy Su] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)

MODULE_SRCS	:=
MODULE_INCLUDES_RTMP	:= $(AMF_CFLAG)

MODULE_SRCS += $(wildcard $(LOCAL_PATH)/*.c)
MODULE_INCLUDES_RTMP += -I./ \
						-I$(PREBUILD_3RD_PARTY_DIR)/openssl/include \
						-I$(PREBUILD_3RD_PARTY_DIR)/zlib/include

LOCAL_TARGET	:= librtmp.a
LOCAL_SRCS	:= $(MODULE_SRCS)
LOCAL_CFLAGS	:= $(MODULE_INCLUDES_RTMP)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

