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
MODULE_NAME	:= record

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/app/common.mk
include $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/config.mk

ALSA_VER_FILE   := $(AMBABUILD_TOPDIR)/kernel/linux/include/sound/version.h
ALSA_DRV_VER    := $(word 3, $(shell [ -f $(ALSA_VER_FILE) ] && grep CONFIG_SND_VERSION $(ALSA_VER_FILE)))

LINK_FLAG = -L $(MW_LIB_DIR) \
		-L $(SYS_LIB_DIR) \
		-L $(EXT_LIB_DIR) \
		-L $(EXT_LIB_PREBUILD_DIR) \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/third-party/misc/lib \
		-L $(AMBABUILD_TOPDIR)/prebuild/cavlc \
		$(MW_AMF_LIB_LD) \
		$(SYS_LIB_LD) \
		$(EXT_LIB_LD)
MODULE_INC = -I$(AMBABUILD_TOPDIR)/build/include \
		-I$(AMBABUILD_TOPDIR)/build/include/arch \
		-I$(AMBABUILD_TOPDIR)/mw/amf/include \
		-I$(AMBABUILD_TOPDIR)/prebuild/third-party/alsa/include

MODULE_DEF =
CFLAG = $(MODULE_INC) -g -O0 -Wall

.PHONY: all clean

all:
	@echo "    compile record..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(CFLAG) -o record record.cpp $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(APP_PATH)
	$(AMBA_MAKEFILE_V)cp -a record $(APP_PATH)
	$(AMBA_MAKEFILE_V)chmod +x $(APP_PATH)/record

clean:
	$(AMBA_MAKEFILE_V)rm -f record

