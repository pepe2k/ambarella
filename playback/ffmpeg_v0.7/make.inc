##
## ambarella/playback/ffmpeg_v0.7/make.inc
##
## History:
##    2013/03/25 - [Roy Su] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_PLAYBACK_FFMPEG), y)

#arch
FFMPEG_ARCH		:= arm

#os
#FFMPEG_PLATFORM_ANDROID := false
FFMPEG_PLATFORM_LINUX := true

#platform diversity
CONFIG_PRJA := false

FFMPEG_TOP := $(call my-dir)

ConfigFilesPath :=
ifeq ($(FFMPEG_PLATFORM_LINUX), true)
	ifeq ($(CONFIG_ARCH_I1), y)
		ConfigFilesPath := $(FFMPEG_TOP)/config/I1_Linux
	else ifeq ($(CONFIG_ARCH_A5S), y)
		ConfigFilesPath := $(FFMPEG_TOP)/config/A5S_Linux
	else ifeq ($(CONFIG_ARCH_S2), y)
		ConfigFilesPath := $(FFMPEG_TOP)/config/S2_Linux
	endif
endif

FFMPEG_CFLAG		?= -DHAVE_AV_CONFIG_H -DHAVE_LOG2 -I$(ConfigFilesPath) -I$(FFMPEG_TOP)/libavutil -I$(FFMPEG_TOP)/libavcodec -I$(FFMPEG_TOP)/libavformat -I$(FFMPEG_TOP)/ $(AMBARELLA_CFLAGS)
FFMPEG_CFLAG+= -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
FFMPEG_CFLAG+= $(if $(findstring linaro, $(TOOLCHAIN_PATH)),-fno-strict-aliasing -fno-aggressive-loop-optimizations -Wno-array-bounds)
FFMPEG_LDFLAG		?= $(AMBARELLA_LDFLAGS)
FFMPEG_ARFLAG		?= rucs

ifeq ($(FFMPEG_PLATFORM_LINUX), true)
FFMPEG_CFLAG += -DPLATFORM_LINUX=1
endif

ifeq ($(PROJECT_NVR), true)
FFMPEG_CFLAG += -DPROJECT_NVR=1
FFMPEG_CFLAG += -DPROJECT_NVR_RTP_PATCH=1
#FFMPEG_CFLAG += -DPROJECT_NVR_MEM_CHECK=1
endif

ifeq ($(CONFIG_ARCH_I1), y)
FFMPEG_CFLAG		+= -DTARGET_USE_AMBARELLA_I1_DSP=1
else ifeq ($(CONFIG_ARCH_A5S), y)
FFMPEG_CFLAG		+= -DTARGET_USE_AMBARELLA_A5S_DSP=1
else ifeq ($(CONFIG_ARCH_S2), y)
FFMPEG_CFLAG		+= -DTARGET_USE_AMBARELLA_S2_DSP=1
endif

ifeq ($(CONFIG_PRJA), true)
FFMPEG_CFLAG += -DCONFIG_PRJA=1
endif

include $(call all-subdir-makefiles)

endif
