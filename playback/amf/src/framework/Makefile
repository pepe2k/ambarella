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

.PHONY: all clean
include $(MW_CE_TOPDIR)/buildspec.mk
include $(MW_CE_TOPDIR)/amf/build/core/buildflags.mk

objects := am_base.o am_futif.o am_if.o am_new.o am_queue.o msgsys.o osal_linux.o am_param.o
all: $(objects)
	$(AMBA_MAKEFILE_V) echo "  [compile amf/src/framework/]:"
$(objects): %.o: %.cpp
	$(CXX) -c $(AMF_CFLAG) $< -o $@
	cp *.o $(MW_CE_TOPDIR)/amf/build/linux/

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile amf/src/framework]:"
	-rm *.o -rf


