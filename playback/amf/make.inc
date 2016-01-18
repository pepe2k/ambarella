##
## ambarella/playback/amf/make.inc
##
## History:
##    2013/04/01 - [Roy Su] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_PLAYBACK_AMF), y)

#os
#AMF_PLATFORM_ANDROID := false
AMF_PLATFORM_LINUX := true

#platform diversity
AMF_PLATFROM_NEED_ATOMICS := true

AMF_TOP	:= $(call my-dir)
PLAYBACK_FFMPEG_TOP := $(AMB_TOPDIR)/playback/ffmpeg_v0.7

V_COMPILE_CFLAGS :=

ifeq ($(CONFIG_ARCH_I1), y)
V_COMPILE_CFLAGS += -DTARGET_USE_AMBARELLA_I1_DSP=1
endif
ifeq ($(CONFIG_ARCH_S2), y)
V_COMPILE_CFLAGS += -DTARGET_USE_AMBARELLA_S2_DSP=1
endif

ifeq ($(AMF_PLATFORM_ANDROID), true)
V_COMPILE_CFLAGS += -DPLATFORM_ANDROID=1
V_COMPILE_CFLAGS += -DCONFIG_ENABLE_RECTEST=1
V_COMPILE_CFLAGS += -DCONFIG_IMGPROC_IN_AMF_UT=1
endif

ifeq ($(AMF_PLATFORM_LINUX), true)
V_COMPILE_CFLAGS += -DPLATFORM_LINUX=1
endif
ifeq ($(AMF_PLATFORM_ANDROID_ITRON), true)
V_COMPILE_CFLAGS += -DPLATFORM_ANDROID_ITRON=1
endif

ifeq ($(AMF_PLATFROM_NEED_ATOMICS), true)
V_COMPILE_CFLAGS += -DPLATFROM_NEED_ATOMICS=1
endif

V_COMPILE_CFLAGS += -DFFMPEG_VER_0_6=0
V_COMPILE_CFLAGS += -DCONFIG_AM_ENABLE_MDEC=1
V_COMPILE_CFLAGS += -DNEW_RTSP_CLIENT=1

AMF_CFLAG		?= -I$(AMF_TOP)/include -I$(PLAYBACK_FFMPEG_TOP) -I$(PLAYBACK_FFMPEG_TOP)/libavutil -I$(PLAYBACK_FFMPEG_TOP)/libavcodec -I$(PLAYBACK_FFMPEG_TOP)/libavformat $(AMBARELLA_CFLAGS)
AMF_CFLAG += $(AMBARELLA_APP_CFLAGS)
AMF_CFLAG += -I$(AMB_TOPDIR)/include
AMF_CFLAG += $(V_COMPILE_CFLAGS)
AMF_LDFLAG		?= $(AMBARELLA_LDFLAGS)
AMF_ARFLAG := rucs

include $(call all-subdir-makefiles)

endif
