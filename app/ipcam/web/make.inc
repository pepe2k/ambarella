##
## app/ipcam/web/make.inc
##
## History:
##    2012/11/20 - [Yinzhu Liu] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##
ifeq ($(BUILD_AMBARELLA_APP_IPCAM_WEBCGI), y)

include $(call my-dir)/arch_$(AMBARELLA_ARCH)/make.inc

endif
