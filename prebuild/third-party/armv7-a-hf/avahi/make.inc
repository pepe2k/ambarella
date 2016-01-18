##
## prebuild/third-party/avahi/make.inc
##
## History:
##    2012/05/30 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_AMBARELLA_AVAHI_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_avahi
LOCAL_SRCS	:= etc lib sbin usr var

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/etc/avahi/services
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

