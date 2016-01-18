##
## Makefile
##
## History:
##    2011/01/07 -  [Zhi He]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk

FILTER_CFLAG := -I$(MW_CE_TOPDIR)/amf/src/engines -I$(MW_CE_TOPDIR)/amf/src/dsp
FILTER_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/audio

FILTER_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/misc/include
FILTER_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/alsa/include/1.0.24


.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects = audio_decoder_hw.o audio_renderer.o amba_video_decoder.o ffmpeg_decoder.o video_renderer.o amdsp_common.o amba_video_sink.o

# Choose ffmpeg_demux for prja
# !!!FIX ME: replace false with MARCO
ifeq (false, true)
objects += ffmpeg_demuxer_prja.o
else
objects += ffmpeg_demuxer.o
endif

objects += video_encoder.o simple_muxer.o ffmpeg_muxer2.o audio_input2.o aac_encoder.o ffmpeg_muxer.o ffmpeg_encoder.o audio_effecter.o video_effecter_preview.o video_transcoder.o video_mem_encoder.o pridata_composer.o pridata_parser.o
objects += am_ffmpeg.o ffmpeg_util.o
objects += video_decoder_dsp_ve.o general_decode_filter_ve.o

all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/src/filters/]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C general $@

$(objects): %.o: %.cpp
	#echo $(TARGET_USE_AMBARELLA_A5S_DSP)
	#echo $(FILTER_CFLAG)
	$(CXX) -c $(AMF_CFLAG) $(FILTER_CFLAG) $(A5SAUDIO_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/filters]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/filters/general $@
	-rm *.o -rf


