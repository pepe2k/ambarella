##
## unit_test/private/gyro_test/make.inc
##
## History:
##    2012/08/23 - [Bingliang Hu] Created file
##
## Copyright (C) 2011-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##


ifeq ($(BUILD_GYRO), y)

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifdef CONFIG_GYRO_SENSOR_MPU6000_POLLING
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/mpu6000_polling/*.c) $(wildcard $(LOCAL_PATH)/mpu6000_polling/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/mpu6000_polling/Kbuild
endif

ifdef CONFIG_GYRO_SENSOR_MPU6000_INTERRUPT
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/mpu6000_interrupt/*.c) $(wildcard $(LOCAL_PATH)/mpu6000_interrupt/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/mpu6000_interrupt/Kbuild
endif

ifdef CONFIG_GYRO_SENSOR_MPU6000_INTERRUPT_SPI_POLL
LOCAL_SRCS	:= $(wildcard $(LOCAL_PATH)/mpu6000/*.c) $(wildcard $(LOCAL_PATH)/mpu6000/*.h)
LOCAL_SRCS	+= $(LOCAL_PATH)/mpu6000/Kbuild
endif

include $(BUILD_DRIVER)

endif

