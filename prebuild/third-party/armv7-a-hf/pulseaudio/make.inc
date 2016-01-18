##
## prebuild/third-party/armv7-a-hf/pulseaudio/make.inc
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

ifeq ($(CONFIG_AMBARELLA_PULSEAUDIO_SUPPORT), y)

LOCAL_PATH	:= $(call my-dir)

PA_MAJOR = $(shell awk '/define\ PA_MAJOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PA_MINOR = $(shell awk '/define\ PA_MINOR/{print $$3}' $(PREBUILD_3RD_PARTY_DIR)/pulseaudio/include/pulse/version.h)
PULSE_VERSION = $(PA_MAJOR).$(PA_MINOR)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= prebuild_pulseaudio
LOCAL_SRCS	:= etc usr
.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE) prebuild_alsa
	$(call prebuild-links)
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_UTILS_SUPPORT), y)
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pacat
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pacmd
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pactl
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pamon
	@rm -rf $(FAKEROOT_DIR)/usr/bin/paplay
	@rm -rf $(FAKEROOT_DIR)/usr/bin/parec
	@rm -rf $(FAKEROOT_DIR)/usr/bin/parecord
	@rm -rf $(FAKEROOT_DIR)/usr/bin/pasuspender
endif
ifneq ($(CONFIG_AMBARELLA_PULSEAUDIO_BLUETOOTH_SUPPORT), y)
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/libblue*.so
	@rm -f $(FAKEROOT_DIR)/usr/lib/pulse-$(PULSE_VERSION)/modules/module-blue*.so
endif
	@mkdir -p $(FAKEROOT_DIR)/usr/libexec/pulse
	@echo "# Make PulseAudio the default device" >> $(FAKEROOT_DIR)/etc/asound.conf
	@echo "pcm.!default { type pulse }" >> $(FAKEROOT_DIR)/etc/asound.conf
	@echo "ctl.!default { type pulse }" >> $(FAKEROOT_DIR)/etc/asound.conf
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

endif

