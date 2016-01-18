##
## kernel/private/drivers/fdet/make.inc
##
## History:
##    2012/06/28 - [Zhenwu Xue] Created file
##
## Copyright (C) 2004-2012, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(BUILD_AMBARELLA_FDET), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/Kbuild

include $(BUILD_DRIVER)

###

include $(call all-makefiles-under, $(LOCAL_PATH)/*)
include $(call all-makefiles-under, $(LOCAL_PATH)/)

endif

