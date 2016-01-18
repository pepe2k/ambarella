#
# app/ipcam/demo/Makefile
#
# History:
#    2010/2/3 - [Jian Tang] created file
#
# Copyright (C) 2010, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

default: all

PWD			:= $(shell pwd)
AMBABUILD_TOPDIR	?= $(word 1, $(subst /app/ipcam, ,$(PWD)))

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/app/common.mk

.PHONY: all clean

all:
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AMFDEMOS), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/record $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/ustream $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/playback $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/file_mux $@
endif

clean:
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_AMFDEMOS), y)
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/record $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/ustream $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/playback $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/file_mux $@
	$(AMBA_MAKEFILE_V)$(RM) -rf $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/tspb
endif

