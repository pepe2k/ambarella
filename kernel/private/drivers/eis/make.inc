##
## kernel/private/drivers/eis/make.inc
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

ifeq ($(BUILD_AMBARELLA_EIS), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifndef CONFIG_AMBARELLA_EIS_IPC

ifndef CONFIG_AMBARELLA_EIS_CORTEX_WITH_QUEUE
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/Kbuild
else
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_queue/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_queue/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_queue/Kbuild
endif

else
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_ipc/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_ipc/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)_ipc/Kbuild
endif

include $(BUILD_DRIVER)

endif

