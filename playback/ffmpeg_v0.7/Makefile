##
## Makefile
##
## History:
##    2011/01/07 - [Zhi He]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

ConfigFilesPath :=
ifeq ($(PLATFORM_LINUX), true)
	ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
		ConfigFilesPath := $(FFMPEG_TOP)/config/I1_Linux
	else ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
		ConfigFilesPath := $(FFMPEG_TOP)/config/A5S_Linux
	endif
endif

#ifeq ($(PLATFORM_ANDROID), true)
#	ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
#		ConfigFilesPath := $(FFMPEG_TOP)/config/I1_Android
#	else ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
#		ConfigFilesPath := $(FFMPEG_TOP)/config/A5S_Android
#	endif
#endif
export ConfigFilesPath

MODULE_NAME		:= "Ffmpeg"
FFMPEG_LINK_FLAG	:= -L $(MW_CE_TOPDIR)/../prebuild/lib -lpthread
FFMPEG_CFLAG		?= -DHAVE_AV_CONFIG_H -DHAVE_LOG2 -I$(ConfigFilesPath) -I$(FFMPEG_TOP)/libavutil -I$(FFMPEG_TOP)/libavcodec -I$(FFMPEG_TOP)/libavformat -I$(FFMPEG_TOP)/ $(AMBARELLA_CFLAGS)
FFMPEG_CFLAG		+= -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
FFMPEG_CFLAG		+= $(OPT_CFLAG)
FFMPEG_LDFLAG		?= $(AMBARELLA_LDFLAGS)
FFMPEG_ARFLAG		?= rucs

ifeq ($(PLATFORM_LINUX), true)
FFMPEG_CFLAG += -DPLATFORM_LINUX=1
endif

ifeq ($(PROJECT_NVR), true)
FFMPEG_CFLAG += -DPROJECT_NVR=1
FFMPEG_CFLAG += -DPROJECT_NVR_RTP_PATCH=1
#FFMPEG_CFLAG += -DPROJECT_NVR_MEM_CHECK=1
endif

ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
FFMPEG_CFLAG		+= -DTARGET_USE_AMBARELLA_I1_DSP=1
else ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
FFMPEG_CFLAG		+= -DTARGET_USE_AMBARELLA_A5S_DSP=1
endif

CONFIG_PRJA := false
ifeq ($(CONFIG_PRJA), true)
FFMPEG_CFLAG += -DCONFIG_PRJA=1
endif
export CONFIG_PRJA

export TARGET_ARCH
export MW_CE_TOPDIR
export FFMPEG_TOP
export FFMPEG_LINK_FLAG
export FFMPEG_CFLAG
export FFMPEG_LDFLAG
export FFMPEG_ARFLAG
export AM_PBTEST_DEBUG

.PHONY: all clean

all:
	@echo "    compile $(MODULE_NAME) "
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavutil $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavcodec $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavformat $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libswscale $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/s_tools $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/tests $@

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean $(MODULE_NAME)]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavutil $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavcodec $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libavformat $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/libswscale $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(FFMPEG_TOP)/tests $@
