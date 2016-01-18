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

ENGINE_CFLAG := -I$(MW_CE_TOPDIR)/amf/src/filters/general  -I$(MW_CE_TOPDIR)/amf/src/filters/general/misc
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/src/framework -I$(MW_CE_TOPDIR)/amf/src/engines -I$(MW_CE_TOPDIR)/amf/src/audio

ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/utils
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_manager
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_message
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_tcp
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspclient/rtsp_rtp
ENGINE_CFLAG += -I$(MW_CE_TOPDIR)/amf/rtspvod/include

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects = engine_guids.o filter_list.o filter_registry.o pb_engine.o active_pb_engine.o pbif.o active_record_engine.o active_duplex_engine.o record_if.o active_ve_engine.o ve_if.o active_mdec_engine.o mdec_if.o streamming_if.o streamming_server.o random.o ws_discovery.o ws_discovery_impl.o motiondetectreceiver.o
all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/src/engines/]:"
$(objects): %.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $(ENGINE_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/engines]:"
	-rm *.o -rf


