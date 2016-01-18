##
## Makefile
##
## History:
##    2011/01/07 - [Zhi He]
##
## Copyright (C) 2004-2011, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

default: all

.PHONY: all clean

all:
	@echo "    compile amf/src:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/audio $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/misc $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/framework $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/filters $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/engines $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/dsp $@
	#$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/audio $@
	@echo "    link lib $(MODULE_NAME):"

clean:
	$(AMBA_MAKEFILE_V)echo "  [Clean amf/src]:"
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/audio $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/misc $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/framework $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/filters $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/engines $@
	$(AMBA_MAKEFILE_V)$(MAKE) -C $(MW_CE_TOPDIR)/amf/src/dsp $@




