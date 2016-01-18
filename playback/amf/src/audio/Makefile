##
## Makefile
##
## History:
##    2011/07/15 -  [QXZ]
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

AUDIO_CFLAG :=

AUDIO_CFLAG += -I$(MW_CE_TOPDIR)/../prebuild/third-party/alsa/include


.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects := audio_if.o

objects += audio_alsa.o


all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/src/audio/]:"
$(objects): %.o: %.cpp
	#echo $(AUDIO_CFLAG)
	$(CXX) -c $(AMF_CFLAG) $(AUDIO_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/audio]:"
	-rm *.o -rf


