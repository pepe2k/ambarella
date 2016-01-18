##
## app/ipcam/web/arch_s2/c/make.inc
##
## History:
##    2012/11/20 - [Yinzhu Liu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_WEB_CGI_C), y)

LOCAL_PATH	:= $(call my-dir)
WEB_PATH		:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= web_c_version
WEB_SRCS	:= $(WEB_PATH)/activeX $(WEB_PATH)/css $(WEB_PATH)/images \
			$(WEB_PATH)/img $(WEB_PATH)/js $(WEB_PATH)/config.xsl \
			$(WEB_PATH)/credits.html $(WEB_PATH)/index.html

.PHONY: $(LOCAL_TARGET) $(WEB_SRCS)

include $(BUILD_PREBUILD)

$(LOCAL_TARGET): $(LOCAL_MODULE) $(WEB_SRCS) webdemo.cgi
	$(call prebuild-links)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $(WEB_SRCS) $(FAKEROOT_DIR)/webSvr/web/.
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:=	webdemo.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/appweb_cgi.c \
		$(LOCAL_PATH)/cgi-bin/src/base_page.c \
		$(LOCAL_PATH)/cgi-bin/src/cam_basic_page.c \
		$(LOCAL_PATH)/cgi-bin/src/config_page.c \
		$(LOCAL_PATH)/cgi-bin/src/enc_page.c \
		$(LOCAL_PATH)/cgi-bin/src/help_page.c \
		$(LOCAL_PATH)/cgi-bin/src/iq_basic_page.c \
		$(LOCAL_PATH)/cgi-bin/src/live_view_page.c \
		$(LOCAL_PATH)/cgi-bin/src/md5_check.c \
		$(LOCAL_PATH)/cgi-bin/src/osd_page.c \
		$(LOCAL_PATH)/cgi-bin/src/page.c \
		$(LOCAL_PATH)/cgi-bin/src/pm_page.c \
		$(LOCAL_PATH)/cgi-bin/src/sys_page.c \
		$(LOCAL_PATH)/cgi-bin/src/utils.c \
		$(LOCAL_PATH)/cgi-bin/src/video_page.c
MODULE_INCLUDES	:=	-I$(LOCAL_PATH)/cgi-bin/include \
		-I$(AMB_TOPDIR)

LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= -lpthread


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin/.
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/.

$(call add-target-into-build, $(LOCAL_TARGET))

endif

