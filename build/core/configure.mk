## History:
##    2011/12/09 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

AMB_BOARD_DIR		:= $(shell pwd)
DOT_CONFIG		:= $(AMB_BOARD_DIR)/.config
export DOT_CONFIG

ifeq ($(wildcard $(DOT_CONFIG)),$(DOT_CONFIG))
include $(DOT_CONFIG)
endif

-include $(AMB_BOARD_DIR)/.config.cmd

AMBA_MAKEFILE_V		:= @
AMBA_MAKE_PARA		:= -s
ALL_TOPDIR		:= $(shell cd $(AMB_TOPDIR)/.. && pwd)
AMB_TOPDIR		:= $(shell cd $(AMB_TOPDIR) && pwd)
export AMB_TOPDIR

######################## TOOLCHAIN #####################################

ifndef ARM_ELF_TOOLCHAIN_DIR
$(error Can not find arm-elf toolchain, please source ENV, eg. CodeSourcery.env)
endif
ifndef ARM_LINUX_TOOLCHAIN_DIR
$(error Can not find arm-linux toolchain, please source ENV, eg. CodeSourcery.env)
endif
ifndef TOOLCHAIN_PATH
$(error Can not find arm-linux path, please source ENV, eg. CodeSourcery.env)
endif

export PATH:=$(TOOLCHAIN_PATH):$(PATH)


########################## BUILD COMMAND ###############################

CLEAR_VARS		:= $(AMB_TOPDIR)/build/core/clear_vars.mk
BUILD_APP		:= $(AMB_TOPDIR)/build/core/build_app.mk
BUILD_DRIVER		:= $(AMB_TOPDIR)/build/core/build_driver.mk
BUILD_EXT_DRIVER	:= $(AMB_TOPDIR)/build/core/build_ext_driver.mk
BUILD_PREBUILD		:= $(AMB_TOPDIR)/build/core/build_prebuild.mk


########################## BUILD IMAGE TOOLS ###########################

CPU_BIT_WIDTH		:= $(shell uname -m)

ifeq ($(CPU_BIT_WIDTH), x86_64)
MAKEDEVS		:= $(AMB_TOPDIR)/rootfs/bin/makedevs-64
MKUBIFS			:= $(AMB_TOPDIR)/rootfs/bin/mkfs.ubifs-64
UBINIZE			:= $(AMB_TOPDIR)/rootfs/bin/ubinize-64
else
MAKEDEVS		:= $(AMB_TOPDIR)/rootfs/bin/makedevs-32
MKUBIFS			:= $(AMB_TOPDIR)/rootfs/bin/mkfs.ubifs-32
UBINIZE			:= $(AMB_TOPDIR)/rootfs/bin/ubinize-32
endif
MKYAFFS2		:= $(AMB_TOPDIR)/rootfs/bin/mkfs.yaffs2
MKSQUASHFS		:= $(AMB_TOPDIR)/rootfs/bin/mkfs.squashfs


########################## BOARD #####################################

ifeq ($(DOT_CONFIG), $(wildcard $(DOT_CONFIG)))
AMBARELLA_ARCH		:= $(shell grep ^CONFIG_ARCH $(DOT_CONFIG) | \
				sed -e s/^CONFIG_ARCH_// | \
				sed -e s/=y// | \
				tr [:upper:] [:lower:])
export AMBARELLA_ARCH

AMB_BOARD		:= $(shell grep ^CONFIG_BSP_BOARD $(DOT_CONFIG) | \
				sed -e s/^CONFIG_BSP_BOARD_// | \
				sed -e s/=y// | \
				tr [:upper:] [:lower:])

BOARD_VERSION_STR	:= $(shell grep ^CONFIG_BOARD_VERSION $(DOT_CONFIG) | \
				sed -e s/^CONFIG_BOARD_VERSION_// | \
				sed -e s/=y//)
BOARD_INITD_DIR		:= $(AMB_TOPDIR)/boards/$(AMB_BOARD)/init.d
BOARD_ROOTFS_DIR	:= $(AMB_TOPDIR)/boards/$(AMB_BOARD)/rootfs/default

else
AMBARELLA_ARCH		:= unknown
AMB_BOARD		:= unknown

endif


########################## OUT DIRECTORY ################################

AMB_BOARD_OUT		:= $(AMB_TOPDIR)/out/$(AMB_BOARD)
# export for building images
export AMB_BOARD_OUT

KERNEL_OUT_DIR		:= $(AMB_BOARD_OUT)/kernel
ROOTFS_OUT_DIR		:= $(AMB_BOARD_OUT)/rootfs
FAKEROOT_DIR		:= $(AMB_BOARD_OUT)/fakeroot
IMAGES_OUT_DIR		:= $(AMB_BOARD_OUT)/images


########################## GLOBAL FLAGS ################################

AMBARELLA_CFLAGS	:= -I$(AMB_BOARD_DIR) -I$(AMB_TOPDIR)/include \
                     -I$(AMB_TOPDIR)/include/arch_$(AMBARELLA_ARCH) \
                     -Wformat -Werror=format-security
AMBARELLA_AFLAGS	:=
AMBARELLA_LDFLAGS	:=
AMBARELLA_CPU_ARCH	:=

ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
AMBARELLA_CPU_ARCH = armv7-a-hf
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
AMBARELLA_CPU_ARCH = armv7-a
else ifeq ($(CONFIG_CPU_ARM1136JS), y)
AMBARELLA_CPU_ARCH = armv6k
else ifeq ($(CONFIG_CPU_ARM926EJS), y)
AMBARELLA_CPU_ARCH = armv5te
endif

########################## APP FLAGS ###################################

PREBUILD_3RD_PARTY_DIR	:= $(AMB_TOPDIR)/prebuild/third-party/$(AMBARELLA_CPU_ARCH)
UNIT_TEST_PATH		:= $(shell echo $(CONFIG_UNIT_TEST_INSTALL_DIR))
UNIT_TEST_IMGPROC_PATH	:= $(shell echo $(CONFIG_UNIT_TEST_IMGPROC_PARAM_DIR))
IMGPROC_INSTALL_PATH	:= $(shell echo $(CONFIG_IMGPROC_INSTALL_DIR))
APP_INSTALL_PATH	:= $(shell echo $(CONFIG_APP_INSTALL_DIR))
APP_IPCAM_CONFIG_PATH	:= $(shell echo $(CONFIG_APP_IPCAM_CONFIG_DIR))

###################### CAMERA MIDDLE-WARE FLAGS ########################

CAMERA_DIR         := $(AMB_TOPDIR)/camera
CAMERA_BIN_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_BIN_DIR))
CAMERA_LIB_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_LIB_DIR))
CAMERA_CONF_DIR    := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_CONF_DIR))
CAMERA_DEMUXER_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_DEMUXER_DIR))
CAMERA_EVENT_PLUGIN_DIR := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_CAMERA_EVENT_PLUGIN_DIR))

###################### VENDOR FLAGS ########################

VENDORS_BIN_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_VENDORS_BIN_DIR))
VENDORS_LIB_DIR     := $(FAKEROOT_DIR)/$(shell echo $(BUILD_AMBARELLA_VENDORS_LIB_DIR))

#
AMBARELLA_APP_AFLAGS	:= $(AMBARELLA_AFLAGS)

#
AMBARELLA_APP_LDFLAGS	:= $(AMBARELLA_LDFLAGS)

#
ifeq ($(BUILD_AMBARELLA_APP_DEBUG), y)
AMBARELLA_APP_CFLAGS	:= $(AMBARELLA_CFLAGS) -g -O0 -Wall -fPIC -D_REENTRENT -D_GNU_SOURCE
else
AMBARELLA_APP_CFLAGS	:= $(AMBARELLA_CFLAGS) -O3 -Wall -fPIC -D_REENTRENT -D_GNU_SOURCE
endif

AMBARELLA_APP_CFLAGS	+= $(call cc-option,-mno-unaligned-access,)

#Disable lto for all gcc 4.x, no prove to be good.
#Impact 3A & algo, let's test in gcc 5.x.
AMBARELLA_APP_CFLAGS	+= $(call cc-ifversion, -ge, 0500, -flto,)
AMBARELLA_APP_LDFLAGS	+= $(call cc-ifversion, -ge, 0500, -flto,)

ifeq ($(BUILD_AMBARELLA_UNIT_TESTS_STATIC), y)
AMBARELLA_APP_LDFLAGS	+= -static
endif

ifeq ($(CONFIG_CPU_CORTEXA9_HF), y)
AMBARELLA_APP_CFLAGS	+= -mthumb -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=hard -mfpu=neon -Wa,-mimplicit-it=thumb
AMBARELLA_APP_AFLAGS	+= -mthumb -march=armv7-a -mfloat-abi=hard -mfpu=neon -mimplicit-it=thumb
AMBARELLA_APP_LDFLAGS	+= -mthumb -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=hard -mfpu=neon -Wa,-mimplicit-it=thumb
else ifeq ($(CONFIG_CPU_CORTEXA9), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=softfp -mfpu=neon
AMBARELLA_APP_AFLAGS	+= -march=armv7-a -mfloat-abi=softfp -mfpu=neon
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv7-a -mtune=cortex-a9 -mlittle-endian -mfloat-abi=softfp -mfpu=neon
else ifeq ($(CONFIG_CPU_ARM1136JS), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv6k -mtune=arm1136j-s -mlittle-endian -msoft-float -Wa,-mfpu=softvfp
AMBARELLA_APP_AFLAGS	+= -march=armv6k -mfloat-abi=soft -mfpu=softvfp
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv6k -mtune=arm1136j-s -mlittle-endian -msoft-float -Wa,-mfpu=softvfp
else ifeq ($(CONFIG_CPU_ARM926EJS), y)
AMBARELLA_APP_CFLAGS	+= -marm -march=armv5te -mtune=arm9tdmi -msoft-float -Wa,-mfpu=softvfp
AMBARELLA_APP_AFLAGS	+= -march=armv5te -mfloat-abi=soft -mfpu=softvfp
AMBARELLA_APP_LDFLAGS	+= -marm -march=armv5te -mtune=arm9tdmi -msoft-float -Wa,-mfpu=softvfp
endif

########################## DRIVER FLAGS ###################################

KERNEL_DEFCONFIG	:= $(shell echo $(CONFIG_KERNEL_DEFCONFIG_STRING))
KERNEL_INSTALL_PATH	:= $(shell echo $(CONFIG_KERNEL_MODULES_INSTALL_DIR))
FIRMWARE_INSTALL_PATH	:= $(KERNEL_INSTALL_PATH)/lib/firmware
LINUX_INSTALL_FLAG	:= INSTALL_MOD_PATH=$(KERNEL_INSTALL_PATH)

LINUX_SRC_DIR		:= $(AMB_TOPDIR)/kernel/linux
LINUX_OUT_DIR		:= $(KERNEL_OUT_DIR)/linux_$(strip \
				$(shell echo $(KERNEL_DEFCONFIG) | \
				sed -e s/ambarella_// -e s/_defconfig// -e s/$(AMB_BOARD)_// -e s/_kernel_config//))

AMB_DRIVERS_BIN_DIR	:= $(AMB_TOPDIR)/prebuild/ambarella/drivers
AMB_DRIVERS_SRCS_DIR	:= $(AMB_TOPDIR)/kernel/private/drivers
AMB_DRIVERS_OUT_DIR	:= $(KERNEL_OUT_DIR)/private/drivers
EXT_DRIVERS_SRCS_DIR	:= $(AMB_TOPDIR)/kernel/external
EXT_DRIVERS_OUT_DIR	:= $(KERNEL_OUT_DIR)/external

AMBARELLA_DRV_CFLAGS	:= $(AMBARELLA_CFLAGS) \
				-I$(AMB_TOPDIR)/kernel/private/include \
				-I$(AMB_TOPDIR)/kernel/private/include/arch_$(AMBARELLA_ARCH) \
				-I$(AMB_TOPDIR)/kernel/private/lib/firmware_$(AMBARELLA_ARCH)

AMBARELLA_DRV_AFLAGS		:= $(AMBARELLA_AFLAGS)
AMBARELLA_DRV_LDFLAGS		:= $(AMBARELLA_LDFLAGS)

### export for private driver building
export AMBARELLA_DRV_CFLAGS
export AMBARELLA_DRV_AFLAGS
export AMBARELLA_DRV_LDFLAGS

