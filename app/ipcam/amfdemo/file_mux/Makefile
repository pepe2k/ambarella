#
# Makefile
#
# History:
#    2011/6/13 - [Yi Zhu] created file
#
# Copyright (C) 2008, Ambarella, Inc.
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Ambarella, Inc.
#

default: all

AMBABUILD_TOPDIR = $(word 1, $(subst /app/ipcam, ,$(shell pwd)))
MODULE_NAME	:= file_mux

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/app/common.mk
include $(AMBABUILD_TOPDIR)/app/ipcam/amfdemo/config.mk

LINK_FLAG = -L $(MW_LIB_DIR) \
		-L $(SYS_LIB_DIR) \
		$(MW_AMF_LIB_LD) \
		$(SYS_LIB_LD)
MODULE_INC = -I$(AMBABUILD_TOPDIR)/build/include \
		-I$(AMBABUILD_TOPDIR)/build/include/arch \
		-I$(AMBABUILD_TOPDIR)/mw/amf/include \

MODULE_DEF =
CFLAG = $(MODULE_INC) -g -O0 -Wall

.PHONY: all clean

all:
	@echo "    compile file_mux..."
	@$(AMBA_MAKEFILE_V)$(CROSS_COMPILE)gcc $(CFLAG) -o file_mux file_mux.cpp $(LINK_FLAG)
	$(AMBA_MAKEFILE_V)mkdir -p $(APP_PATH)
	$(AMBA_MAKEFILE_V)cp -a file_mux $(APP_PATH)
	$(AMBA_MAKEFILE_V)chmod +x $(APP_PATH)/file_mux

clean:
	$(AMBA_MAKEFILE_V)rm -f file_mux

