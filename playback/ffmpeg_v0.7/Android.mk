ifeq ($(strip $(TARGET_USE_AMBARELLA_AMF)),true)
	ifeq ($(AM_USING_FFMPEG_VER), 0.7)
		# Invoke subdir makefile
		include $(call all-subdir-makefiles)
	endif
endif
