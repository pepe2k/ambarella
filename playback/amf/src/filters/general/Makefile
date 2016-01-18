##
## Makefile
##
## History:
##    2012/06/07 -  [Qingxiong]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

#MW_CE_TOPDIR := $(PWD)/../../../../

include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk
include $(MW_CE_TOPDIR)/config.mk
#include $(MW_CE_TOPDIR)/buildspec.mk

GENERAL_FILTER_CFLAG := -I$(MW_CE_TOPDIR)/amf/src/engines -I$(MW_CE_TOPDIR)/amf/src/dsp
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/audio -I$(MW_CE_TOPDIR)/amf/src/filters
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/framework
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/filters/general/misc
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/filters/general

GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/misc/include
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/alsa/include/1.0.24
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/libaac/include

GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/utils
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_manager
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_rtp

GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/injector/include
GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspvod/include

GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/cloudService/aliyunoss/include -I$(MW_CE_TOPDIR)/cloudService/storage/include

GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/extern/g711

#GENERAL_FILTER_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/coreavc/include

#FFMPEG_TOP = $(MW_CE_TOPDIR)/ffmpeg_v0.7/
#AMF_CFLAG := -I$(MW_CE_TOPDIR)/amf/include -I$(MW_CE_TOPDIR)/amf/src/filters
#AMF_CFLAG += -I$(FFMPEG_TOP) -I$(FFMPEG_TOP)/libavutil -I$(FFMPEG_TOP)/libavcodec -I$(FFMPEG_TOP)/libavformat
#AMF_CFLAG += $(V_COMPILE_CFLAGS)
#AMF_CFLAG += -I$(MW_CE_TOPDIR)/../build/include
export GENERAL_FILTER_CFLAG

.PHONY: all clean

objects = general_mw.o general_header.o general_parse.o
objects += general_demuxer_filter.o general_decoder_filter.o general_renderer_filter.o general_audio_manager.o
objects += general_transc_audiosink_filter.o general_transcoder_filter.o
objects += g_ffmpeg_demuxer_exqueue.o g_ffmpeg_demuxer_exnet.o gvideo_decoder_dsp_exqueue.o g_ffmpeg_decoder.o g_sync_renderer.o
objects += g_render_out.o g_ffmpeg_video_decoder.o
ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
objects += general_loader_filter.o
endif
objects += simple_audio_sink.o
objects += resample_filter.o
objects += simple_audio_recorder.o

#objects += g_ffmpeg_demuxer.o g_ffmpeg_demuxer_net.o g_ffmpeg_demuxer_net_test.o
#objects += gvideo_decoder_dsp.o gaudio_decoder_hw.o gvideo_decoder_ffmpeg.o

all: $(objects)
	$(AMBA_MAKEFILE_V)$(MAKE) -C misc $@

$(objects): %.o: %.cpp
	#echo $(TARGET_USE_AMBARELLA_A5S_DSP)
	#echo $(FILTER_CFLAG)
	echo $@
	$(CXX) -c $(AMF_CFLAG) $(GENERAL_FILTER_CFLAG) $(A5SAUDIO_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/filters/general]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/filters/general/misc $@
	-rm *.o -rf

