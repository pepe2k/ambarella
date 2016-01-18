##
## kernel/private/drivers/vin/sensors/sony_imx226/make.inc
##
## History:
##    2013/12/20 - [Long Zhao] Created file
##
## Copyright (C) 2012-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

ifeq ($(CONFIG_SENSOR_IMX226), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

UGLY_ARCH	:= true
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/*.c) $(wildcard $(LOCAL_PATH)/*.h)
LOCAL_SRCS	+= $(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.c) \
			$(wildcard $(LOCAL_PATH)/arch_$(AMBARELLA_ARCH)/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/Kbuild

include $(BUILD_DRIVER)

endif

