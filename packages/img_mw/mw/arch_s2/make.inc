##
## packages/img_mw/mw/arch_s2/make.inc
##
## History:
##    2012/06/01 - [Jian Tang] Created file
##
## Copyright (C) 2012-2016, Ambarella, Inc.
##
## All rights reserved. No Part of this file may be reproduced, stored
## in a retrieval system, or transmitted, in any form, or by any means,
## electronic, mechanical, photocopying, recording, or otherwise,
## without the prior consent of Ambarella, Inc.
##

IMG_MW_PATH	:= $(call my-dir)

ifeq ($(BUILD_AMBARELLA_MIDDLEWARE_PROC), y)

include $(IMG_MW_PATH)/src/make.inc
include $(IMG_MW_PATH)/mw_hiso/src/make.inc
endif

