##
## Makefile
##
## History:
##    2011/08/31 -  [Jay Zhang]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

MODULE_SRCS = $(wildcard *.cpp)
MODULE_OBJS = $(MODULE_SRCS:%.cpp=%.o)

include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk


.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects := helper.o

all: $(MODULE_OBJS)
	$(AMBA_MAKEFILE_V)echo "  [compile amf/src/filters/rtsp_filter]:"

%.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $< -o $@
	mv *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/filters/rtsp_filter]:"


