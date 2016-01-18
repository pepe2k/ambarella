##
## playback/ffmpeg_v0.7/libavutil/make.inc
##
## History:
##    2013/03/26 - [Roy Su] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

include $(CLEAR_VARS)
include $(ConfigFilesPath)/config.mak
include $(ConfigFilesPath)/config-$(FFMPEG_ARCH).mak

OBJS :=
OBJS-yes :=
MMX-OBJS-yes :=
C_OBJS	:=
S_OBJS	:=
C_FILES	:=
S_FILES	:=
ALL_S_FILES	:=
ALL_C_FILES	:=
OBJS = adler32.o                                                        \
       aes.o                                                            \
       audioconvert.o                                                   \
       avstring.o                                                       \
       base64.o                                                         \
       cpu.o                                                            \
       crc.o                                                            \
       des.o                                                            \
       error.o                                                          \
       eval.o                                                           \
       fifo.o                                                           \
       file.o                                                           \
       imgutils.o                                                       \
       intfloat_readwrite.o                                             \
       inverse.o                                                        \
       lfg.o                                                            \
       lls.o                                                            \
       log.o                                                            \
       lzo.o                                                            \
       mathematics.o                                                    \
       md5.o                                                            \
       mem.o                                                            \
       opt.o                                                            \
       parseutils.o                                                     \
       pixdesc.o                                                        \
       random_seed.o                                                    \
       rational.o                                                       \
       rc4.o                                                            \
       samplefmt.o                                                      \
       sha.o                                                            \
       tree.o                                                           \
       utils.o

OBJS-$(ARCH_ARM) += arm/cpu.o
OBJS-$(ARCH_PPC) += ppc/cpu.o
OBJS-$(ARCH_X86) += x86/cpu.o
OBJS-$(HAVE_MMX) += $(MMX-OBJS-yes)
OBJS += $(OBJS-yes)
OBJS_S2 = $(addprefix $(LOCAL_PATH)/, $(OBJS))

SORT_OBJS := $(sort $(OBJS_S2))
ALL_S_FILES := $(wildcard $(LOCAL_PATH)/$(FFMPEG_ARCH)/*.S)
ALL_S_FILES := $(addprefix $(FFMPEG_ARCH)/, $(notdir $(ALL_S_FILES)))

ifneq ($(ALL_S_FILES),)
ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
C_OBJS := $(filter-out $(ALL_S_OBJS),$(SORT_OBJS))
S_OBJS := $(filter $(ALL_S_OBJS),$(SORT_OBJS))
else
C_OBJS := $(SORT_OBJS)
S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))
FFFILES := $(sort $(S_FILES)) $(sort $(C_FILES))



MODULE_INCLUDES_AVUTIL	:=	-DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign \
				-DTARGET_CONFIG=\"$(ConfigFilesPath)/config-$(FFMPEG_ARCH).h\" \
 				-I$(LOCAL_PATH)/ \
 				-I$(LOCAL_PATH)/arm/ \
				$(FFMPEG_CFLAG) \
				$(AMBARELLA_APP_CFLAGS)

LOCAL_TARGET	:= libavutil.so
LOCAL_SRCS	:= $(FFFILES)
LOCAL_CFLAGS	:= $(MODULE_INCLUDES_AVUTIL)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(FAKEROOT_DIR)/lib/
	@cp -dpRf $< $(FAKEROOT_DIR)/lib/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

