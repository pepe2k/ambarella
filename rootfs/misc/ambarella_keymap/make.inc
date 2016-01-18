##
## prebuild/imgproc/misc/ambarella_keymap/make.inc
##
## History:
##    2013/04/10 - [Jingyang Qiu] modified file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

###
ROOTFS_KEYMAP_PATH := $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(CONFIG_AMBARELLA_KEYMAP), y)

#PREBUILD_KEYMAP_PATH := $(call my-dir)

KEYMAP_OUT_PATH:=$(AMB_BOARD_OUT)/rootfs/ambarella_keymap
KEYMAP_FAKEROOT_PATH:=$(AMB_BOARD_OUT)/fakeroot/lib/firmware
KEYMAP_FILE_NAME:=ambarella-keymap.bin

.PHONY: ambarella_keymap

ambarella_keymap:
	@mkdir -p $(KEYMAP_OUT_PATH)/
	@gcc -o $(KEYMAP_OUT_PATH)/keymap $(ROOTFS_KEYMAP_PATH)/keymap.c
	@$(KEYMAP_OUT_PATH)/keymap $(KEYMAP_OUT_PATH)/$(KEYMAP_FILE_NAME)
	@mkdir -p $(KEYMAP_FAKEROOT_PATH)
	@mv -f $(KEYMAP_OUT_PATH)/$(KEYMAP_FILE_NAME) $(KEYMAP_FAKEROOT_PATH)
	@echo "Build $@ Done."

$(call add-target-into-build, ambarella_keymap)

endif

