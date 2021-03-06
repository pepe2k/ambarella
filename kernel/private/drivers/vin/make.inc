##
## kernel/private/drivers/vin/make.inc
##
## History:
##    2012/06/04 - [Cao Rongrong] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_VIN), y)

LOCAL_PATH	:= $(call my-dir)
VIN_TOP_DIR	:= $(LOCAL_PATH)

###
include $(CLEAR_VARS)

LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/Kbuild

include $(BUILD_DRIVER)

###

include $(call all-makefiles-under, $(VIN_TOP_DIR)/*)
include $(call all-makefiles-under, $(VIN_TOP_DIR)/)

endif

