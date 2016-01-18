##
## amf/build/core/buildflags.mk
##
## History:
##    2011/01/20 - [Zhi He] Create
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

V_COMPILE_CFLAGS :=

ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)

V_COMPILE_CFLAGS += -DTARGET_USE_AMBARELLA_I1_DSP=1

endif

ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)

V_COMPILE_CFLAGS += -DTARGET_USE_AMBARELLA_A5S_DSP=1

endif

ifeq ($(PLATFORM_ANDROID), true)

V_COMPILE_CFLAGS += -DPLATFORM_ANDROID=1
V_COMPILE_CFLAGS += -DCONFIG_ENABLE_RECTEST=1
V_COMPILE_CFLAGS += -DCONFIG_IMGPROC_IN_AMF_UT=1

endif

ifeq ($(PLATFORM_LINUX), true)

V_COMPILE_CFLAGS += -DPLATFORM_LINUX=1

endif

ifeq ($(PLATFORM_ANDROID_ITRON), true)

V_COMPILE_CFLAGS += -DPLATFORM_ANDROID_ITRON=1

endif

ifeq ($(PLATFROM_NEED_ATOMICS), true)

V_COMPILE_CFLAGS += -DPLATFROM_NEED_ATOMICS=1

endif

ifeq ($(AM_USING_FFMPEG_VER), 0.6)

V_COMPILE_CFLAGS += -DFFMPEG_VER_0_6=1

else

V_COMPILE_CFLAGS += -DFFMPEG_VER_0_6=0

endif

#ifeq ($(AM_MDEC_CONFIG), true)

V_COMPILE_CFLAGS += -DCONFIG_AM_ENABLE_MDEC=1

#endif

export V_COMPILE_CFLAGS

