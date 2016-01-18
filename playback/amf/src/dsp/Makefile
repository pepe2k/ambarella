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

DSP_CFLAG := -I$(FFMPEG_TOPDIR)/libavcodec
DSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/filters/general
DSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/engines
DSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/framework
DSP_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/audio

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects := amba_dsp_common.o
ifeq ($(TARGET_USE_AMBARELLA_I1_DSP), true)
objects += amba_dsp_ione.o
endif
ifeq ($(TARGET_USE_AMBARELLA_A5S_DSP), true)
objects += amba_dsp_a5s.o
endif

all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/src/dsp/]:"
$(objects): %.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $(DSP_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/filters]:"
	-rm *.o -rf


