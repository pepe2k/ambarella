# LOCAL_PATH is one of libavutil, libavcodec, libavformat, or libswscale

ConfigFilesPath :=
ifeq ($(PLATFORM_ANDROID), true)
	ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
		ConfigFilesPath := $(LOCAL_PATH)/../config/I1_Android
	#else ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
	#	ConfigFilesPath := $(LOCAL_PATH)/../config/A5S_Android
	endif
endif

include $(ConfigFilesPath)/config-$(TARGET_ARCH).mak
include $(ConfigFilesPath)/config.mak

OBJS :=
OBJS-yes :=
MMX-OBJS-yes :=
include $(LOCAL_PATH)/Makefile

# collect objects
OBJS-$(HAVE_MMX) += $(MMX-OBJS-yes)
OBJS += $(OBJS-yes)

FFNAME := lib$(NAME)
FFLIBS := $(foreach,NAME,$(FFLIBS),lib$(NAME))

FFCFLAGS  = -DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign
FFCFLAGS += -DTARGET_CONFIG=\"config-$(TARGET_ARCH).h\"
FFCFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
#FFCFLAGS += -g 
#FFCFLAGS += -fprefetch-loop-arrays  

# define PRJA_CONFIG Macro for AOF PJ203
ifeq ($(AMF_AOF_PJ203_CONFIG), true)
CONFIG_PRJA := true
else
CONFIG_PRJA := false
endif
ifeq ($(CONFIG_PRJA), true)
FFCFLAGS += -DCONFIG_PRJA=1
endif

ALL_S_FILES := $(wildcard $(LOCAL_PATH)/$(TARGET_ARCH)/*.S)
ALL_S_FILES := $(addprefix $(TARGET_ARCH)/, $(notdir $(ALL_S_FILES)))

ifneq ($(ALL_S_FILES),)
ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
C_OBJS := $(filter-out $(ALL_S_OBJS),$(OBJS))
S_OBJS := $(filter $(ALL_S_OBJS),$(OBJS))
else
C_OBJS := $(OBJS)
S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))

FFFILES := $(sort $(S_FILES)) $(sort $(C_FILES))
