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



.PHONY: all clean

objects = general_muxer_save.o gmf_osd_shower.o
objects += g_injector_rtmp.o g_muxer_ffmpeg.o g_simple_save.o
objects += g_injector_rtsp.o
objects += general_layout_manager.o
objects += general_pipeline.o

all: $(objects)

$(objects): %.o: %.cpp
	#echo $(TARGET_USE_AMBARELLA_A5S_DSP)
	#echo $(FILTER_CFLAG)
	echo $@
	$(CXX) -c $(AMF_CFLAG) $(GENERAL_FILTER_CFLAG) $(A5SAUDIO_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/filters/general/misc]:"
	-rm *.o -rf
