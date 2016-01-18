##
## Makefile
##
## History:
##    2008/10/23 - [Anthony Ginger] Create
##
## Copyright (C) 2004-2009, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

PWD			:= $(shell pwd)
AMBABUILD_TOPDIR	?= $(word 1, $(subst /build/include, ,$(PWD)))

export AMBABUILD_TOPDIR

include $(AMBABUILD_TOPDIR)/build/core/common.mk


.PHONY: link_arch unlink_arch

link_arch:
ifeq ($(CONFIG_A5S_IAV_FOR_DV),y)
	$(AMBA_MAKEFILE_V)echo "  SYMLIMK[arch -> arch_a5s_dv] @ $(PWD)"
	-$(AMBA_MAKEFILE_V)ln -fsn arch_a5s_dv arch
else
	$(MAKE) common_link_arch
endif

unlink_arch:
	$(MAKE) common_unlink_arch
