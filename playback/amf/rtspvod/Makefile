##
## Makefile
##
## Copyright (C) 2007-2013, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

PWD			:= $(shell pwd)
MW_CE_TOPDIR	?= $(PWD)/../..

include $(MW_CE_TOPDIR)/config.mk
include $(MW_CE_TOPDIR)/buildspec.mk

.PHONY: all clean

all:
	$(AMBA_MAKEFILE_V) echo "  [compile rtspvod/]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspvod/live555 $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspvod/librtspvod $@

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean compile rtspvod]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspvod/live555 $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/rtspvod/librtspvod $@

