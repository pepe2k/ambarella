##
## unit_test/private/ipcam_test/arch_s2/make.inc
##
## History:
##    2012/06/05 - [Cao Rongrong] Created file
##
## Copyright (C) 2011-2015, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_encode
LOCAL_SRCS	:= $(LOCAL_PATH)/test_encode.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/script/mode_switch_test.sh $(UNIT_TEST_PATH)/
	@chmod +x $(UNIT_TEST_PATH)/mode_switch_test.sh
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= bscheck
LOCAL_SRCS	:= $(LOCAL_PATH)/bscheck.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_MIDDLEWARE_PROC), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_warp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_warp.c
LOCAL_LIBS	:=
LOCAL_LDFLAGS	:= -lpthread -lm
LOCAL_LIBS	+= libamp.so
LOCAL_LDFLAGS	+= -L$(AMB_TOPDIR)/prebuild/imgproc/img_lib -limg_algo_s2

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/warp.cfg $(PRIVATE_PATH)/data/clear.txt $(UNIT_TEST_PATH)/data/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###
ifeq ($(BUILD_AMBARELLA_DEWARP_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_dewarp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_dewarp.c
LOCAL_LIBS	:=
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/dewarp/arch_$(AMBARELLA_ARCH) -I$(AMB_TOPDIR)/packages/utils/
LOCAL_LDFLAGS	:= -lm -lrt -lpthread
LOCAL_LIBS	+= libutils.so
LOCAL_LDFLAGS+= -L$(AMB_TOPDIR)/prebuild/dewarp -ldewarp

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/script/dewarp_pt.sh $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/lut_imx172_sunex257.txt $(UNIT_TEST_PATH)/data/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_playback
LOCAL_SRCS	:= $(LOCAL_PATH)/test_playback.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
##include $(CLEAR_VARS)

#LOCAL_TARGET	:= rtmp_video
#LOCAL_SRCS	:= $(LOCAL_PATH)/rtmp_video.cpp
#LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/bsreader \
#	-I$(AMB_TOPDIR)/packages/data_transfer \
#	-I/home/shawn/rtmpdump/librtmp
#LOCAL_LDFLAGS	:= -lpthread -lrtmp -L/usr/local/lib
#LOCAL_LIBS	:= libbsreader.a libdatatx.a

##include $(BUILD_APP)

#.PHONY: $(LOCAL_TARGET)

#$(LOCAL_TARGET): $(LOCAL_MODULE)
#	@mkdir -p $(UNIT_TEST_PATH)/
#	@cp -dpRf $< $(UNIT_TEST_PATH)/
#	@echo "Build $@ Done."

#$(call add-target-into-build, $(LOCAL_TARGET))
###

ifeq ($(BUILD_AMBARELLA_BSREADER_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_bsreader
LOCAL_SRCS	:= $(LOCAL_PATH)/test_bsreader.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/bsreader \
				-I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libbsreader.a libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_stream
LOCAL_SRCS	:= $(LOCAL_PATH)/test_stream.c $(LOCAL_PATH)/../encrypt/encrypt.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_uvc
LOCAL_SRCS	:= $(LOCAL_PATH)/test_uvc.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_privacymask
LOCAL_SRCS	:= $(LOCAL_PATH)/test_privacymask.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_MAINPREPROC_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_mainpp
LOCAL_LIBS	:= libmainpp.so
LOCAL_SRCS	:= $(LOCAL_PATH)/test_mainpp.c

LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/main_preproc \
                   -I$(AMB_TOPDIR)/packages/utils
LOCAL_LDFLAGS	:=
LOCAL_LIBS	+= libutils.so
LOCAL_LDFLAGS	+= -lrt -lpthread -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)/data
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/overlay_data.bin $(PRIVATE_PATH)/overlay_clut.bin \
	$(PRIVATE_PATH)/overlay_ayuv_16bit.bin $(PRIVATE_PATH)/overlay_argb_16bit.bin $(UNIT_TEST_PATH)/data/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
endif
###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_statistics
LOCAL_SRCS	:= $(LOCAL_PATH)/test_statistics.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_dptz
LOCAL_SRCS	:= $(LOCAL_PATH)/test_dptz.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_yuvcap
LOCAL_SRCS	:= $(LOCAL_PATH)/test_yuvcap.c $(LOCAL_PATH)/chrome_convert.S
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif

###
ifeq ($(BUILD_AMBARELLA_DATATX_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_raw_yuvcap
LOCAL_SRCS	:= $(LOCAL_PATH)/test_raw_yuvcap.c
LOCAL_LDFLAGS	:= -lpthread
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/data_transfer
LOCAL_LIBS	:= libdatatx.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
endif
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_overlay
LOCAL_SRCS	:= $(LOCAL_PATH)/test_overlay.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_loadbmp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_loadbmp.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@cp -dpRf $(PRIVATE_PATH)/data/Ambarella-256x128-8bit.bmp $(UNIT_TEST_PATH)/data/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
ifeq ($(CONFIG_IAV_FOR_DEBUG_DSP), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_frame_sync
LOCAL_SRCS	:= $(LOCAL_PATH)/test_frame_sync.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_qproi
LOCAL_SRCS	:= $(LOCAL_PATH)/test_qproi.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

else

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_qproi
LOCAL_SRCS	:= $(LOCAL_PATH)/test_qproi_ipb.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif	###ifeq ($(CONFIG_IAV_FOR_DEBUG_DSP), y)

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_qp_overlay
LOCAL_SRCS	:= $(LOCAL_PATH)/test_qp_overlay.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_shmoo
LOCAL_SRCS	:= $(LOCAL_PATH)/test_shmoo.c
LOCAL_LDFLAGS	:= -lm

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###
include $(CLEAR_VARS)

LOCAL_TARGET	:= test_memcpy
LOCAL_SRCS	:= $(LOCAL_PATH)/test_memcpy.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

###

ifeq ($(BUILD_AMBARELLA_TEXTINSERT_TEST), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_textinsert
LOCAL_SRCS	:= $(LOCAL_PATH)/test_textinsert.c
LOCAL_LIBS	:= libtextinsert.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/textinsert/
LOCAL_LDFLAGS	:= -L $(PREBUILD_3RD_PARTY_DIR)/freetype/usr/lib -lfreetype \
                   -L $(PREBUILD_3RD_PARTY_DIR)/bzip2/usr/lib -lbz2 \
                   -L $(PREBUILD_3RD_PARTY_DIR)/zlib/usr/lib -lz

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###

ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_md_motbuf
LOCAL_SRCS	:= $(LOCAL_PATH)/test_md_motbuf.c
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/md_motbuf/arch_$(AMBARELLA_ARCH)
LOCAL_LDFLAGS	:= -lpthread
LOCAL_LIBS	:= libmd_motbuf.a

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###

ifeq ($(BUILD_AMBARELLA_EIS2_TEST), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_eis2
LOCAL_SRCS	:= $(LOCAL_PATH)/test_eis2.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif

###

ifeq ($(CONFIG_GUARD_VSYNC_LOSS), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= guard_vsync_loss
LOCAL_SRCS	:= $(LOCAL_PATH)/guard_vsync_loss.c

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
###
ifeq ($(BUILD_AMBARELLA_EIS_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_UTILITIES_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_eis_warp
LOCAL_SRCS	:= $(LOCAL_PATH)/test_eis_warp.c
LOCAL_LIBS	:=
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/eis/ -I$(AMB_TOPDIR)/packages/utils/
LOCAL_LDFLAGS	:= -lm -lrt -lpthread
LOCAL_LIBS	+= libutils.so
LOCAL_LDFLAGS+= -L$(AMB_TOPDIR)/prebuild/eis -leis

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): PRIVATE_PATH:=$(LOCAL_PATH)
$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/data/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###

ifeq ($(BUILD_AMBARELLA_LBR_PACKAGE), y)
ifeq ($(BUILD_AMBARELLA_MDMOTBUF_PACKAGE), y)

include $(CLEAR_VARS)

LOCAL_TARGET	:= test_lbr
LOCAL_SRCS	:= $(LOCAL_PATH)/test_lbr.c
LOCAL_LDFLAGS	:= -lpthread -lrt
LOCAL_LIBS	:= liblbr.so
LOCAL_LIBS	+= libmd_motbuf.a
LOCAL_CFLAGS	:= -I$(AMB_TOPDIR)/packages/lbr/include
LOCAL_CFLAGS	+= -I$(AMB_TOPDIR)/packages/md_motbuf/arch_$(AMBARELLA_ARCH)

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))

endif
endif

###
include $(CLEAR_VARS)

LOCAL_TARGET    := test_feed_yuv
LOCAL_SRCS      := $(LOCAL_PATH)/test_feed_yuv.c
LOCAL_LDFLAGS	:= -lpthread

include $(BUILD_APP)

.PHONY: $(LOCAL_TARGET)

$(LOCAL_TARGET): $(LOCAL_MODULE)
	@mkdir -p $(UNIT_TEST_PATH)/
	@cp -dpRf $< $(UNIT_TEST_PATH)/
	@echo "Build $@ Done."

$(call add-target-into-build, $(LOCAL_TARGET))
###

