##
## camera/modules/configure/make.inc
##
## History:
##    2012/05/30 - [Yupeng Chang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_CAMERA_MODULES_CONFIGURE), y)
LOCAL_PATH   := $(call my-dir)
include $(CLEAR_VARS)

CAMERA_DIR             = $(AMB_TOPDIR)/camera
CAMERA_CONFIG_FILE_DIR = $(CAMERA_DIR)/modules/configure/config
CONFIG_VER_FILE        = $(CAMERA_DIR)/include/configure/version.h
CONFIG_VER_PREFIX      = CONFIGURE
CAMERA_CONFIG_FILES = vdevice.conf \
                      record.conf  \
                      record.conf.hls  \
                      record.conf.http  \
                      photo.conf   \
                      audio.conf   \
                      audiodetect.conf \
                      lbr.conf

ifeq ($(BUILD_AMBARELLA_CAMERA_MOTION_DETECT_MSE), y)
	CAMERA_CONFIG_FILES += motiondetect_mse.conf
endif
ifeq ($(BUILD_AMBARELLA_CAMERA_MOTION_DETECT_MOG2), y)
	CAMERA_CONFIG_FILES += motiondetect_mog2.conf
endif

ifeq ($(CONFIG_ARCH_S2), y)
CAMERA_CONFIG_FILES += s2/fisheye.conf \
                       s2/fisheye.conf.imx172.japan \
                       s2/fisheye.conf.imx172.focusafe125 \
                       s2/fisheye.conf.imx172.focusafe155 \
                       s2/fisheye.conf.imx172.sunex216

ENCODER_CONFIG = s2/vdev_encoder.conf \
                 s2/vdev_stream0.conf \
                 s2/vdev_stream1.conf \
                 s2/vdev_stream2.conf \
                 s2/vdev_stream3.conf
HIGHMEGA_ENCODER_CONFIG = s2/vdev_encoder.highmega.conf \
                 s2/vdev_stream0.highmega.conf \
                 s2/vdev_stream1.conf \
                 s2/vdev_stream2.conf \
                 s2/vdev_stream3.conf
endif

ifeq ($(CONFIG_ARCH_A5S), y)
ENCODER_CONFIG = a5s/vdev_encoder.conf \
                 a5s/vdev_stream0.conf \
                 a5s/vdev_stream1.conf \
                 a5s/vdev_stream2.conf \
                 a5s/vdev_stream3.conf
endif

VIN_CONFIG     = vin.conf
VOUT_CONFIG    = vout.conf

CONFIG_MODULE = libamconfigure.so
LOCAL_TARGET := $(CONFIG_MODULE)
LOCAL_SRCS   := $(LOCAL_PATH)/am_config_audio.cpp        \
                $(LOCAL_PATH)/am_config_base.cpp         \
                $(LOCAL_PATH)/am_config.cpp              \
                $(LOCAL_PATH)/am_config_photo.cpp        \
                $(LOCAL_PATH)/am_config_record.cpp       \
                $(LOCAL_PATH)/am_config_vdev_encoder.cpp \
                $(LOCAL_PATH)/am_config_vdevice.cpp      \
                $(LOCAL_PATH)/am_config_vdev_stream.cpp  \
                $(LOCAL_PATH)/am_config_vin.cpp          \
                $(LOCAL_PATH)/am_config_vout.cpp         \
                $(LOCAL_PATH)/am_config_wifi.cpp         \
                $(LOCAL_PATH)/am_config_audiodetect.cpp  \
                $(LOCAL_PATH)/am_config_motiondetect.cpp \
                $(LOCAL_PATH)/am_config_lbrcontrol.cpp

ifeq ($(CONFIG_ARCH_S2), y)
LOCAL_SRCS +=   $(LOCAL_PATH)/am_config_fisheye.cpp
endif

LOCAL_CFLAGS := -I$(PREBUILD_3RD_PARTY_DIR)/iniparser/include \
                -I$(CAMERA_DIR)/include/configure \
                -I$(CAMERA_DIR)/include \
                -I$(LOCAL_PATH)/include -Werror

ifeq ($(BUILD_AMBARELLA_CAMERA_DEBUG), y)
LOCAL_CFLAGS += -O0 -g
endif

CONFIG_SO_VER_MAJOR  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_MINOR  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_MINOR/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_PATCH  := $(shell awk '/define\ $(CONFIG_VER_PREFIX)_LIB_PATCH/{print $$3}' $(CONFIG_VER_FILE))
CONFIG_SO_VER_STRING := $(CONFIG_SO_VER_MAJOR).$(CONFIG_SO_VER_MINOR).$(CONFIG_SO_VER_PATCH)

CONFIG_SO_NAME   = $(patsubst %.so,%.so.$(CONFIG_SO_VER_MAJOR), $(CONFIG_MODULE))
CONFIG_SO_TARGET = $(patsubst %.so,%.so.$(CONFIG_SO_VER_STRING), $(CONFIG_MODULE))

LOCAL_SO_NAME := $(CONFIG_SO_NAME)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(CAMERA_LIB_DIR)/
	@cp -dpRf $< $(CAMERA_LIB_DIR)/$(CONFIG_SO_TARGET)
	@ln -sf $(CONFIG_SO_TARGET) $(CAMERA_LIB_DIR)/$(CONFIG_SO_NAME)
	@mkdir -p $(CAMERA_CONF_DIR)/
	@for file in $(CAMERA_CONFIG_FILES); do \
	echo "      Processing $$file."; \
	filename=`basename $$file`; \
	sed -e '/^\#/d' $(CAMERA_CONFIG_FILE_DIR)/$$file | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$$filename; \
	echo "" >> $(CAMERA_CONF_DIR)/$$filename; \
	done
ifeq ($(BUILD_AMBARELLA_CAMERA_AUDIO_MONO), y)
	@sed -i -e 's/.*\(AudioChannelNumber\ *=\).*/\1\ 1/' $(CAMERA_CONF_DIR)/audio.conf
endif
	@for file in $(ENCODER_CONFIG); do \
	echo "      Processing $$file."; \
	sed -e '/^\#/d' $(CAMERA_CONFIG_FILE_DIR)/$$file | sed -e '/^$$/d' >> $(CAMERA_CONF_DIR)/vdevice.conf; \
	echo "" >> $(CAMERA_CONF_DIR)/vdevice.conf; \
	done
	@for file in $(HIGHMEGA_ENCODER_CONFIG); do \
	echo "      Processing $$file."; \
	sed -e '/^\#/d' $(CAMERA_CONFIG_FILE_DIR)/$$file | sed -e '/^$$/d' >> $(CAMERA_CONF_DIR)/highmega_vdevice.conf; \
	echo "" >> $(CAMERA_CONF_DIR)/highmega_vdevice.conf; \
	done
	@echo "      Processing $(VIN_CONFIG)."
	@sed -e '/^\#/d' $(CAMERA_CONFIG_FILE_DIR)/$(VIN_CONFIG) | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VIN_CONFIG).template;
	@echo "" >> $(CAMERA_CONF_DIR)/$(VIN_CONFIG).template;
	@cp $(CAMERA_CONF_DIR)/$(VIN_CONFIG).template $(CAMERA_CONF_DIR)/video_$(VIN_CONFIG)
	@cp $(CAMERA_CONF_DIR)/$(VIN_CONFIG).template $(CAMERA_CONF_DIR)/photo_$(VIN_CONFIG)
	@echo "      Processing $(VOUT_CONFIG)."
ifeq ($(CONFIG_LCD_PANEL_TD043), y)
	@sed 's/\(Type\ *=\ \)[0-9A-Za-z]*/\1TD043/g' $(CAMERA_CONFIG_FILE_DIR)/$(VOUT_CONFIG) | \
		sed -e '/^\#/d' | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdRotateVideo\ *=\ \).*/\10/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdFlipVideo\ *=\ \).*/\1Normal/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@echo "" >> $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
else
ifeq ($(CONFIG_LCD_PANEL_1P3828), y)
	@sed 's/\(Type\ *=\ \)[0-9A-Za-z]*/\11P3828/g' $(CAMERA_CONFIG_FILE_DIR)/$(VOUT_CONFIG) | \
		sed -e '/^\#/d' | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdRotateVideo\ *=\ \).*/\190/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdFlipVideo\ *=\ \).*/\1HV/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@echo "" >> $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
else
ifeq ($(CONFIG_LCD_PANEL_1P3831), y)
	@sed 's/\(Type\ *=\ \)[0-9A-Za-z]*/\11P3831/g' $(CAMERA_CONFIG_FILE_DIR)/$(VOUT_CONFIG) | \
		sed -e '/^\#/d' | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdRotateVideo\ *=\ \).*/\190/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdFlipVideo\ *=\ \).*/\1HV/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@echo "" >> $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
else
ifeq ($(CONFIG_LCD_PANEL_TPO489), y)
	@sed 's/\(Type\ *=\ \)[0-9A-Za-z]*/\1TPO489/g' $(CAMERA_CONFIG_FILE_DIR)/$(VOUT_CONFIG) | \
		sed -e '/^\#/d' | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdRotateVideo\ *=\ \).*/\10/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(LcdFlipVideo\ *=\ \).*/\1Normal/g' $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@echo "" >> $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
else
	@sed -e '/^\#/d' $(CAMERA_CONFIG_FILE_DIR)/$(VOUT_CONFIG) | sed -e '/^$$/d' > $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@echo "" >> $(CAMERA_CONF_DIR)/$(VOUT_CONFIG)
	@sed -i 's/\(Vout0\ *=\ \).*/\1None/' $(CAMERA_CONF_DIR)/vdevice.conf
endif
endif
endif
endif
ifeq ($(CONFIG_ARCH_S2), y)
	@echo "      Processing S2 related config files."
	@cp $(CAMERA_CONF_DIR)/vdevice.conf $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(EncodeMode\ *=\ \)[0-9A-Za-z]*/\11/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(BufferSub2MaxWidth\ *=\ \)[0-9A-Za-z\-]*/\10/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(BufferSub2MaxHeight\ *=\ \)[0-9A-Za-z\-]*/\10/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(BufferSub3MaxWidth\ *=\ \)[0-9A-Za-z\-]*/\11280/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(BufferSub3MaxHeight\ *=\ \)[0-9A-Za-z\-]*/\11280/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(YUVDataWidth\ *=\ \)[0-9A-Za-z\-]*/\10/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@sed -i 's/\(YUVDataHeight\ *=\ \)[0-9A-Za-z\-]*/\10/g' $(CAMERA_CONF_DIR)/warp_vdevice.conf
	@cp $(CAMERA_CONF_DIR)/$(VIN_CONFIG).template $(CAMERA_CONF_DIR)/warp_$(VIN_CONFIG)
	@sed -i 's/\(Mode\ *=\ \)[0-9A-Za-z]*/\14000x3000/g' $(CAMERA_CONF_DIR)/warp_$(VIN_CONFIG)
endif
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
