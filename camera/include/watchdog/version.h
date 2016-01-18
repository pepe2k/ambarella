/*
 * version.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 27/12/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_WATCHDOG_VERSION_H__
#define __AM_WATCHDOG_VERSION_H__

#define WATCHDOG_LIB_MAJOR 0
#define WATCHDOG_LIB_MINOR 1
#define WATCHDOG_LIB_PATCH 0
#define WATCHDOG_LIB_VERSION ((WATCHDOG_LIB_MAJOR << 16) | \
                             (WATCHDOG_LIB_MINOR << 8)  | \
                             WATCHDOG_LIB_PATCH)

#define WATCHDOG_VERSION_STR "0.1.0"

#endif
