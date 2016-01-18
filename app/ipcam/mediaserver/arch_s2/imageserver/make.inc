##
## app/ipcam/mediaserver/arch_a5s/imageserver/make.inc
##
## History:
##    2012/10/25 - [Zhaoyang Chen] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifeq ($(CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON), y)
LOCAL_TARGET	:= ImageServerDaemon
else
LOCAL_TARGET    :=  image_server
endif

LOCAL_SRCS	:= $(LOCAL_PATH)/image_server.c
MODULE_INCLUDES	:= -I$(AMB_TOPDIR) \
		-I$(AMB_TOPDIR)/build/include \
		-I$(AMB_TOPDIR)/build/include/arch \
		-I$(AMB_TOPDIR)/prebuild/third-party/armv6k/libdaemon/include

MODULE_CFGS	:= *.cfg
INIT_SCRIPT	:= ImageServerDaemon.sh
CONFIG_PATH	:= $(APP_IPCAM_CONFIG_PATH)/mediaserver/config
DAEMON_PATH	:= $(FAKEROOT_DIR)/sbin

LOCAL_CFLAGS	:= $(MODULE_INCLUDES) -g -O3 -Wall
LOCAL_LIBS	:=
LOCAL_LIBS	+= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2
LOCAL_LDFLAGS	:= -L$(PREBUILD_3RD_PARTY_DIR)/dbus/usr/lib \
		-L$(PREBUILD_3RD_PARTY_DIR)/libdaemon/usr/lib \
		-limg_algo_s2 -lpthread -ldaemon -lm \
		-lrt

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
ifeq ($(CONFIG_AMBARELLA_IMAGE_SERVER_DAEMON), y)
	@echo $(PRIVATE_PATH)
	@echo $(APP_INSTALL_PATH)
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp $(PRIVATE_PATH)/$(INIT_SCRIPT) $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$(INIT_SCRIPT)
	@mkdir -p $(DAEMON_PATH)/
	@cp -dpRf $< $(DAEMON_PATH)/
	@chmod +x $(DAEMON_PATH)/$@
	@mkdir -p $(CONFIG_PATH)/
	@cp -a $(PRIVATE_PATH)/$(MODULE_CFGS) $(CONFIG_PATH)/
	@echo "Build $@ Done."
else
	@mkdir -p $(APP_INSTALL_PATH)/
	@cp -dpRf $< $(APP_INSTALL_PATH)/
	@chmod +x $(APP_INSTALL_PATH)/$@
	@mkdir -p $(CONFIG_PATH)/
	@cp -a $(PRIVATE_PATH)/$(MODULE_CFGS) $(CONFIG_PATH)/
endif
$(call add-target-into-build, $(LOCAL_TARGET))

