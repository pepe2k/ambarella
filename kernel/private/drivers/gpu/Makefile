##
## Makefile
##
## History:
##    2010/07/19 - [Zhenwu Xue]
##
## Copyright (C) 2004-2010, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MODULE_DIR		:= $(word 2, $(subst /kernel/private/, ,$(PWD)))
TMP_DIR			:= $(shell echo ./$(MODULE_DIR)|sed 's/\/[0-9a-zA-Z_]*/\/../g' )
AMBABUILD_TOPDIR	?= $(PWD)/$(TMP_DIR)/../..
MODULE_NAME		:= GPU

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/kernel/private/common.mk

.PHONY: all clean link_arch unlink_arch

all:
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx $@

clean:
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx $@
	-$(AMBA_MAKEFILE_V)rm -f *.o

link_arch:
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx $@

unlink_arch:
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(AMBABUILD_TOPDIR)/kernel/private/drivers/ambarella/gpu/sgx $@
