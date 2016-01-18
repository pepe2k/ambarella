##
## build_targets.mk
##
## History:
##    2012/05/31 - [Cao Rongrong] Create
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

$(if $(LOCAL_TARGET),,$(error $(LOCAL_PATH): LOCAL_TARGET is not defined))

$(LOCAL_TARGET).PATH := $(LOCAL_PATH)

# if any .cpp files existed, build all files as c++ codes.
LOCAL_CXX	:= $(if $(filter %.cpp,$(LOCAL_SRCS)),$(CROSS_COMPILE)g++,$(CROSS_COMPILE)gcc)

# convert .c files to objs
LOCAL_OBJS	:= $(patsubst $(AMB_TOPDIR)/%.c, $(AMB_BOARD_OUT)/%.o,$(LOCAL_SRCS))
# convert .cpp files to objs
LOCAL_OBJS	:= $(patsubst $(AMB_TOPDIR)/%.cpp, $(AMB_BOARD_OUT)/%.o,$(LOCAL_OBJS))
# convert .S files to objs
LOCAL_OBJS	:= $(patsubst $(AMB_TOPDIR)/%.S, $(AMB_BOARD_OUT)/%.o,$(LOCAL_OBJS))
# convert .asm files to objs
LOCAL_OBJS	:= $(patsubst $(AMB_TOPDIR)/%.asm, $(AMB_BOARD_OUT)/%.o,$(LOCAL_OBJS))
# sometimes we want to merge external static library into target,
# so filter out the external static library to avoid take it as objs.
LOCAL_OBJS	:= $(filter-out %.a, $(LOCAL_OBJS))
# final targets
LOCAL_MODULE	:= $(patsubst $(AMB_TOPDIR)/%, $(AMB_BOARD_OUT)/%, $(LOCAL_PATH))/$(LOCAL_TARGET)
# define shared library name
LOCAL_SO_NAME ?= $(LOCAL_TARGET)

# define variables owned by specific $(LOCAL_MODULE)
__AMB_LIBS__	:= $(patsubst lib%.so, -l%, $(filter %.so,$(LOCAL_LIBS)))
__AMB_LIBS__	+= $(patsubst lib%.a, -l%, $(filter %.a,$(LOCAL_LIBS)))
$(LOCAL_MODULE): PRIVATE_AMB_LIBS := $(LOCAL_LIBS)
$(LOCAL_MODULE): PRIVATE_CFLAGS := $(LOCAL_CFLAGS)
$(LOCAL_MODULE): PRIVATE_AFLAGS := $(LOCAL_AFLAGS)
$(LOCAL_MODULE): PRIVATE_LDFLAGS := $(__AMB_LIBS__) $(LOCAL_LDFLAGS)
$(LOCAL_MODULE): PRIVATE_SO_FLAGS := -Wl,-soname,$(LOCAL_SO_NAME)

# include dependency files
-include $(LOCAL_OBJS:%.o=%.P)

# check
$(LOCAL_TARGET).check: TARGET_SRCS := $(filter-out %.a %.so, $(LOCAL_SRCS))
$(LOCAL_TARGET).check:
	@cppcheck $(TARGET_SRCS)

# compile
$(AMB_BOARD_OUT)/%.o: $(AMB_TOPDIR)/%.c $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(AMBA_MAKEFILE_V)$(PRIVATE_CXX) $(AMBARELLA_APP_CFLAGS) $(PRIVATE_CFLAGS) -MMD -c $< -o $@
	$(call transform-d-to-p)

$(AMB_BOARD_OUT)/%.o: $(AMB_TOPDIR)/%.cpp $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(AMBA_MAKEFILE_V)$(PRIVATE_CXX) $(AMBARELLA_APP_CFLAGS) $(PRIVATE_CFLAGS) -MMD -c $< -o $@
	$(call transform-d-to-p)

$(AMB_BOARD_OUT)/%.o: $(AMB_TOPDIR)/%.S $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(if $(findstring COMPILE_S_WITH_AS, $(PRIVATE_CFLAGS)), \
		$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)as $(AMBARELLA_APP_AFLAGS) $(PRIVATE_AFLAGS) -o $@ $<, \
		$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(AMBARELLA_APP_CFLAGS) $(PRIVATE_CFLAGS) -c $< -o $@)

$(AMB_BOARD_OUT)/%.o: $(AMB_TOPDIR)/%.asm $(LOCAL_PATH)/make.inc
	@mkdir -p $(dir $@)
	$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)as $(AMBARELLA_APP_AFLAGS) $(PRIVATE_AFLAGS) -o $@ $<

# link
LOCAL_MODULE_TYPE := $(if $(filter %.a, $(LOCAL_MODULE)), static_library, \
			$(if $(filter %.so, $(LOCAL_MODULE)), shared_library, exectutable))

$(LOCAL_MODULE): PRIVATE_CXX:=$(LOCAL_CXX)
$(LOCAL_MODULE): PRIVATE_MODULE_TYPE:=$(LOCAL_MODULE_TYPE)
$(LOCAL_MODULE): $(LOCAL_OBJS) $(filter %.a, $(LOCAL_SRCS)) $(LOCAL_LIBS) $(LOCAL_PATH)/make.inc
	$(if $(findstring static_library, $(PRIVATE_MODULE_TYPE)), $(build-static-library))
	$(if $(findstring shared_library, $(PRIVATE_MODULE_TYPE)), \
		$(AMBA_MAKEFILE_V)$(PRIVATE_CXX) $(AMBARELLA_APP_LDFLAGS) $(PRIVATE_SO_FLAGS) -shared -o $@ $(filter-out %.a %.so %make.inc, $^) \
		$(foreach p, $(PRIVATE_AMB_LIBS), -L$(subst $(AMB_TOPDIR)/,$(AMB_BOARD_OUT)/,$($(p).PATH))) $(PRIVATE_LDFLAGS) && \
		$(strip-library))
	$(if $(findstring exectutable, $(PRIVATE_MODULE_TYPE)), \
		$(AMBA_MAKEFILE_V)$(PRIVATE_CXX) $(AMBARELLA_APP_LDFLAGS) \
		$(foreach p, $(PRIVATE_AMB_LIBS), -L$(subst $(AMB_TOPDIR)/,$(AMB_BOARD_OUT)/,$($(p).PATH))) \
		-o $@ $(filter-out %.a %.so %make.inc, $^)  $(PRIVATE_LDFLAGS) &&\
		$(strip-binary))
