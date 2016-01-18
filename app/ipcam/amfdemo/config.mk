#
# config.mk
#
# History:
#    2010/02/03 - [Jian Tang] created file
#
# Copyright (C) 2010, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#


PREBUILD_DIR = $(AMBABUILD_TOPDIR)/prebuild
MW_LIB_DIR = $(PREBUILD_DIR)/mw
EXT_LIB_DIR = $(PREBUILD_DIR)/third-party
EXT_LIB_PREBUILD_DIR = $(PREBUILD_DIR)/mw/$(PREBUILD_DIR)

SYS_LIBS = pthread m dl rt
MW_AMF_LIBS = amf
EXT_LIBS = asound aacenc aacdec cavlc
EXT_SVMCPP_LIBS = stdc++ cv cxcore cvaux ml 

SYS_LIB_LD = $(addprefix -l, $(SYS_LIBS))
MW_LIB_LD =  $(addprefix -l, $(MW_LIBS))
MW_AMF_LIB_LD = $(addprefix -l, $(MW_AMF_LIBS))
EXT_LIB_LD = $(addprefix -l, $(EXT_LIBS))
EXT_SVMCPP_LIB_LD = $(addprefix -l, $(EXT_SVMCPP_LIBS))

COMPILER := gcc
ifeq ($(CROSS_COMPILE),)
CROSS_COMPILE   := arm-linux-
endif
ifeq ($(TOOLCHAIN_PATH),)
TOOLCHAIN_PATH  := $(ARM_LINUX_TOOLCHAIN_DIR)/staging_dir/usr/bin
endif
ifeq ($(SYS_LIB_DIR),)
SYS_LIB_DIR     := $(ARM_LINUX_TOOLCHAIN_DIR)/staging_dir/lib
endif
ifeq ($(PREBUILD_DIR),)
PREBUILD_DIR	:= arm_926t_oabi
export PREBUILD_DIR
endif
