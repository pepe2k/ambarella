##
## packages/img_mw/mw/arch_s2/src/make.inc
##
## History:
##    2012/06/01 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)
MW_PATH	:= $(LOCAL_PATH)
ADJ_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/adj_params
LENS_PARAMS_PATH	:= $(AMB_TOPDIR)/prebuild/imgproc/img_data/arch_$(AMBARELLA_ARCH)/lens_params

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= libamp.so
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_CFLAGS	:= -I$(LOCAL_PATH)/../include
LOCAL_CFLAGS	+= -I$(ADJ_PARAMS_PATH)

AMP_MODULE = libamp.so
AMP_VER_PREFIX = AMP
AMP_VERSION_FILE = $(LOCAL_PATH)/mw_version.c
AMP_SO_VER_MAJOR  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_MAJOR/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_MINOR  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_MINOR/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_PATCH  := $(shell awk '/define\ $(AMP_VER_PREFIX)_LIB_PATCH/{print $$3}' $(AMP_VERSION_FILE))
AMP_SO_VER_STRING := $(AMP_SO_VER_MAJOR).$(AMP_SO_VER_MINOR).$(AMP_SO_VER_PATCH)
AMP_SO_NAME   = $(patsubst %.so,%.so.$(AMP_SO_VER_MAJOR), $(AMP_MODULE))
AMP_SO_TARGET = $(patsubst %.so,%.so.$(AMP_SO_VER_STRING), $(AMP_MODULE))

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)
$(LOCAL_SRCS): create_aaa_binary

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/usr/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/usr/lib/$(AMP_SO_TARGET)
	@ln -sf $(AMP_SO_TARGET) $(FAKEROOT_DIR)/usr/lib/$(AMP_SO_NAME)
	@ln -sf $(AMP_SO_NAME) $(FAKEROOT_DIR)/usr/lib/$(AMP_MODULE)
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

########
include $(CLEAR_VARS)

MW_HEADER_PATH:=$(AMB_TOPDIR)/packages/img_mw/mw/arch_$(AMBARELLA_ARCH)/include
ADJ_PARAMS_CFLAGS:=	-I $(MW_HEADER_PATH) -I $(AMBARELLA_CFLAGS)

PREBUILD_PARAM_FOLDER:=$(AMB_BOARD_OUT)/preubuild/img_data
PREBUILD_LENS_PARAM_FOLDER:=$(AMB_BOARD_OUT)/preubuild/img_data/lens_params

#PARSE_AAA_PARAM	:= $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.c, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
#S_AAA_PARAM	:= $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.s, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
#OBJS_AAA_PARAM = $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.o, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
BIN_AAA_PARAM = $(patsubst ${ADJ_PARAMS_PATH}/%.c, ${PREBUILD_PARAM_FOLDER}/%.bin, $(wildcard ${ADJ_PARAMS_PATH}/*.c))
BIN_LENS_PARAM := $(patsubst ${LENS_PARAMS_PATH}/%.c, ${PREBUILD_LENS_PARAM_FOLDER}/%.bin, $(wildcard ${LENS_PARAMS_PATH}/*.c))

.PHONY: create_aaa_binary

.PHONY:	$(PREBUILD_PARAM_FOLDER)/%.o
.PHONY:	$(PREBUILD_PARAM_FOLDER)/%.bin
.PHONY:	$(PREBUILD_PARAM_FOLDER)/%.c
.PHONY:	$(PREBUILD_PARAM_FOLDER)/%.s

${PREBUILD_PARAM_FOLDER}/%.s: ${ADJ_PARAMS_PATH}/%.c
	@mkdir -p ${PREBUILD_PARAM_FOLDER}
	@gcc ${ADJ_PARAMS_CFLAGS} -S $^ -o $@

${PREBUILD_PARAM_FOLDER}/%.c : ${PREBUILD_PARAM_FOLDER}/%.s ${ADJ_PARAMS_PATH}/%.c
	@sh ${MW_PATH}/build_aaa_binary.sh $@ $^

$(PREBUILD_PARAM_FOLDER)/%.o : ${PREBUILD_PARAM_FOLDER}/%.c
	@gcc ${ADJ_PARAMS_CFLAGS} -o $@ -c $^
	@rm $^

$(PREBUILD_PARAM_FOLDER)/%.bin : $(PREBUILD_PARAM_FOLDER)/%.o
	@mkdir -p ${PREBUILD_PARAM_FOLDER}
	@objcopy -O binary $^ $@
	@rm $^


.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.o
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.bin
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.c
.PHONY:	$(PREBUILD_LENS_PARAM_FOLDER)/%.s

${PREBUILD_LENS_PARAM_FOLDER}/%.s: ${LENS_PARAMS_PATH}/%.c
	@mkdir -p ${PREBUILD_LENS_PARAM_FOLDER}
	@gcc ${ADJ_PARAMS_CFLAGS} -S $^ -o $@

${PREBUILD_LENS_PARAM_FOLDER}/%.c : ${PREBUILD_LENS_PARAM_FOLDER}/%.s ${LENS_PARAMS_PATH}/%.c
	@sh ${MW_PATH}/build_lens_binary.sh $@ $^

$(PREBUILD_LENS_PARAM_FOLDER)/%.o : ${PREBUILD_LENS_PARAM_FOLDER}/%.c
	@gcc ${ADJ_PARAMS_CFLAGS} -o $@ -c $^
	@rm $^

$(PREBUILD_LENS_PARAM_FOLDER)/%.bin : $(PREBUILD_LENS_PARAM_FOLDER)/%.o
	@mkdir -p ${PREBUILD_LENS_PARAM_FOLDER}
	@objcopy -O binary $^ $@
	@rm $^

create_aaa_binary: $(BIN_AAA_PARAM) $(BIN_LENS_PARAM)
	@mkdir -p $(UNIT_TEST_IMGPROC_PATH)/adj_params/
	@cp -f $(BIN_AAA_PARAM) $(BIN_LENS_PARAM) $(UNIT_TEST_IMGPROC_PATH)/adj_params/
	@echo "Build $@ Done."

$(call add-target-into-build, create_aaa_binary)
