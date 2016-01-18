include $(MW_CE_TOPDIR)/config.mk
# collect objects
OBJS-$(HAVE_MMX) += $(MMX-OBJS-yes)
OBJS += $(OBJS-yes)

SORT_OBJS := $(sort $(OBJS))

FFNAME := lib$(NAME)
FFLIBS := $(foreach,NAME,$(FFLIBS),lib$(NAME))

FFCFLAGS  = $(FFMPEG_CFLAG)
FFCFLAGS += -DHAVE_AV_CONFIG_H -Wno-sign-compare -Wno-switch -Wno-pointer-sign
FFCFLAGS += -DTARGET_CONFIG=\"config-$(TARGET_ARCH).h\"
#FFCFLAGS += -g
#FFCFLAGS += -fprefetch-loop-arrays


ifeq ($(HAVE_ARMV6), yes)
FFCFLAGS += -march=armv6k -mtune=arm1136j-s
else
ifeq ($(HAVE_ARMV5TE), yes)
FFCFLAGS += -march=armv5te -mtune=arm9tdmi
endif
endif

ALL_S_FILES := $(wildcard $(LOCAL_PATH)/$(TARGET_ARCH)/*.S)
ALL_S_FILES := $(addprefix $(TARGET_ARCH)/, $(notdir $(ALL_S_FILES)))

ifneq ($(ALL_S_FILES),)
ALL_S_OBJS := $(patsubst %.S,%.o,$(ALL_S_FILES))
C_OBJS := $(filter-out $(ALL_S_OBJS),$(SORT_OBJS))
S_OBJS := $(filter $(ALL_S_OBJS),$(SORT_OBJS))
else
C_OBJS := $(SORT_OBJS)
S_OBJS :=
endif

C_FILES := $(patsubst %.o,%.c,$(C_OBJS))
S_FILES := $(patsubst %.o,%.S,$(S_OBJS))

all: buildobj linklib

buildobj: $(C_OBJS) $(S_OBJS)
$(C_OBJS): %.o: %.c
	$(CC) $(FFCFLAGS) -c $< -o $@

$(S_OBJS): %.o: %.S
	$(CC) $(FFCFLAGS) -c $< -o $@

%.d: %.c
	@echo -n "$@ " | awk -F/ '{printf("%s ", $$NF)}' > $@
	$(CC) $(FFCFLAGS) -MM $< >> $@

%.d: %.S
	@echo -n "$@ " | awk -F/ '{printf("%s ", $$NF)}' > $@
	$(CC) $(FFCFLAGS) -MM $< >> $@

-include $(C_OBJS:.o=.d)
-include $(S_OBJS:.o=.d)

linklib:
ifeq ($(AM_PBTEST_DEBUG), false)
	@echo "    gcc -shared lib$(NAME).so:"
	$(AMBA_MAKEFILE_V)$(CC) -shared -o lib$(NAME).so $(C_OBJS) $(S_OBJS)
	cp *.so $(FFMPEG_TOP)/build/linux/
	$(STRIP) lib$(NAME).so
	cp lib$(NAME).so $(OUT_LIB_DIR)
else
	@echo "    ar lib$(NAME).a:"
	$(AMBA_MAKEFILE_V)$(AR) $(FFMPEG_ARFLAG) lib$(NAME).a $(C_OBJS) $(S_OBJS)
	cp *.a $(FFMPEG_TOP)/build/linux/
endif

clean:
	-rm *.o *.d *.so *.a -rf
	-rm $(TARGET_ARCH)/*.o $(TARGET_ARCH)/*.d -rf
	-rm $(FFMPEG_TOP)/build/linux/*.a -rf
	-rm $(FFMPEG_TOP)/build/linux/*.so -rf

FFFILES := $(sort $(S_FILES)) $(sort $(C_FILES))
