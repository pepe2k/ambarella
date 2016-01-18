##
## app/ipcam/web/arch_s2/clearsilver/make.inc
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

ifeq ($(BUILD_AMBARELLA_APP_IPCAM_WEB_CGI_CS), y)

LOCAL_PATH	:= $(call my-dir)
WEB_PATH		:= $(call my-dir)
MODULE_INCLUDES := -I$(LOCAL_PATH)/cgi-bin/include \
                   -I$(PREBUILD_3RD_PARTY_DIR)/clearsilver/include/ClearSilver \
                   -I$(PREBUILD_3RD_PARTY_DIR)/mxml/include
MODULE_LDFLAGS := -L$(PREBUILD_3RD_PARTY_DIR)/clearsilver/usr/lib \
                  -L$(PREBUILD_3RD_PARTY_DIR)/mxml/usr/lib \
                  -lneo_cgi -lneo_cs -lneo_utl -lmxml -lpthread -lm
###

include $(CLEAR_VARS)

LOCAL_TARGET := web_clearsilver
WEB_SRCS := $(WEB_PATH)/activeX $(WEB_PATH)/css $(WEB_PATH)/html\
			$(WEB_PATH)/js $(WEB_PATH)/index.html
WEB_TARGET := router.cgi verif_update.cgi enc_query.cgi enc_update.cgi \
              dwp_query.cgi dwp_update.cgi pm_query.cgi pm_update.cgi  \
              zoompantilt.cgi liveview_query.cgi liveview_update.cgi   \
              osd_query.cgi osd_update.cgi view_mjpg.cgi

.PHONY: $(LOCAL_TARGET)

include $(BUILD_PREBUILD)

CREATE_DIR:
	@mkdir -p $(FAKEROOT_DIR)/webSvr/web/cgi-bin

$(LOCAL_TARGET): $(LOCAL_MODULE) CREATE_DIR $(WEB_SRCS) $(WEB_TARGET)
	$(call prebuild-links)
	@cp -dpRf $(WEB_SRCS) $(FAKEROOT_DIR)/webSvr/web/
	@echo "Copy $@ Done"

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	router.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/router.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $(WEB_PATH)/cgi-bin/src/router.conf $(FAKEROOT_DIR)/webSvr/web/cgi-bin/
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	verif_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/verif_update.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/.

$(call add-target-into-build, $(LOCAL_TARGET))

###


include $(CLEAR_VARS)

LOCAL_TARGET	:=	enc_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/enc_query.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	enc_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/enc_update.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###


include $(CLEAR_VARS)

LOCAL_TARGET	:=	dwp_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/dwp_query.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	dwp_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/dwp_update.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	pm_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/pm_query.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	pm_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/pm_update.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	zoompantilt.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/zoompantilt.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	liveview_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/liveview_query.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	liveview_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/liveview_update.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	osd_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/osd_query.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	osd_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/osd_update.c \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	view_mjpg.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/view_mjpg.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS)


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	iq_query.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/iq_query.c  \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###

include $(CLEAR_VARS)

LOCAL_TARGET	:=	iq_update.cgi
LOCAL_SRCS	:=	$(LOCAL_PATH)/cgi-bin/src/iq_update.c  \
				$(LOCAL_PATH)/cgi-bin/src/utils.c
LOCAL_CFLAGS	:= $(MODULE_INCLUDES)
LOCAL_LDFLAGS	:= $(MODULE_LDFLAGS) -lrt


include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@cp -dpRf $< $(FAKEROOT_DIR)/webSvr/web/cgi-bin/

$(call add-target-into-build, $(LOCAL_TARGET))

###
endif
