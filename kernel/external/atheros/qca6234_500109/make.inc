##
## kernel/external/atheros/ar9374_35028
##
## History:
##    2013/09/16 - [Tao Wu] Created file
##
## Copyright (C) 2004-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

LOCAL_PATH	:= $(call my-dir)

###
include $(CLEAR_VARS)

ifeq ($(ATH6KL_AR9375_SDIO), y)
export CONFIG_ATH6KL_AR9375_SDIO=m
endif

ifeq ($(ATH6KL_AR9375_USB), y)
export CONFIG_ATH6KL_AR9375_USB=m
endif

LOCAL_SRCS	:= $(shell find $(LOCAL_PATH) -name "*.[ch]")
LOCAL_SRCS	+= $(LOCAL_PATH)/drivers/ath6kl/Kbuild

include $(BUILD_EXT_DRIVER)

