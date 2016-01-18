#
# Makefile
#
# History:
#    2010/12/13 - [Yi Zhu] created file
#
# Copyright (C) 2008, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

default: all

AMBABUILD_TOPDIR = $(word 1, $(subst /app/ipcam, ,$(shell pwd)))
MODULE_NAME	:= ustream_demo

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/app/common.mk
include $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/config.mk

ALSA_VER_FILE   := $(AMBABUILD_TOPDIR)/kernel/linux/include/sound/version.h
ALSA_DRV_VER    := $(word 3, $(shell [ -f $(ALSA_VER_FILE) ] && grep CONFIG_SND_VERSION $(ALSA_VER_FILE)))

ANL_LIB = anl
IMG_ALGO_A5S = amp img_algo_a5s
USTREAM_LIBS = rtmp_c curl

ANL_LIB_LD = $(addprefix -l, $(ANL_LIB))
IMG_ALGO_A5S_LD = $(addprefix -l, $(IMG_ALGO_A5S))
USTREAM_LIB_LD  = $(addprefix -l, $(USTREAM_LIBS))

USTREAM_LIB_POS = $(AMBABUILD_TOPDIR)/prebuild/third-party/ustream/lib

LINK_FLAG = -L $(MW_LIB_DIR) \
		-L $(SYS_LIB_DIR) \
		-L $(EXT_LIB_DIR) \
		-L $(EXT_LIB_PREBUILD_DIR) \
		-L $(AMBABUILD_TOPDIR)/prebuild/imgproc \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/ustream/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/misc/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/cavlc \
		$(MW_AMF_LIB_LD) \
		$(SYS_LIB_LD) \
		$(EXT_LIB_LD) \
		$(ANL_LIB_LD) \
		$(IMG_ALGO_A5S_LD) \
		$(USTREAM_LIB_LD)

MODULE_INC = -I$(AMBABUILD_TOPDIR)/build/include \
		-I$(AMBABUILD_TOPDIR)/build/include/arch \
		-I$(AMBABUILD_TOPDIR)/mw/amf/include \
		-I$(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/include \
		-I$(AMBABUILD_TOPDIR)/prebuild/third-party/ustream/include

MODULE_DEF =
CFLAG = $(MODULE_INC) -g -O0 -Wall

.PHONY: all clean

ln:
	@if [ ! -e $(USTREAM_LIB_POS)/librtmp_c.so ]; then \
	ln -s $(USTREAM_LIB_POS)/librtmp_c.so.0 $(USTREAM_LIB_POS)/librtmp_c.so; \
	ln -s $(USTREAM_LIB_POS)/libcurl.so.4 $(USTREAM_LIB_POS)/libcurl.so; \
	fi

rm_ln:
	@if [ -e $(USTREAM_LIB_POS)/librtmp_c.so ]; then \
	rm $(USTREAM_LIB_POS)/librtmp_c.so; \
	rm $(USTREAM_LIB_POS)/libcurl.so; \
	fi

all: ln
ifeq ($(CONFIG_AMBARELLA_USTREAM_SUPPORT), y)
	@echo "    compile ustream_demo..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(CFLAG) -o ustream_demo ustream_demo.cpp $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(APP_PATH)
	$(AMBA_MAKEFILE_V)cp -a ustream_demo $(APP_PATH)
	$(AMBA_MAKEFILE_V)chmod +x $(APP_PATH)/ustream_demo
endif

clean: rm_ln
	$(AMBA_MAKEFILE_V)rm -f ustream_demo
