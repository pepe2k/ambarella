##
## packages/md_motbuf/arch_s2/make.inc
##
## History:
##    2013/02/22 - [Qiu jingyang] Created file
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

LOCAL_TARGET	:= libmd_motbuf.a
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS	:= -Wall -O3

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

