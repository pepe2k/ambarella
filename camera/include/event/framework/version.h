/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2012-08-09 2012 - [Zhikan Yang] created file
 *
 * Copyright (C) 2008-2012, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_EVENT_VERSION_H_
#define AM_EVENT_VERSION_H_

#define EVENT_LIB_MAJOR 0
#define EVENT_LIB_MINOR 1
#define EVENT_LIB_PATCH 0
#define EVENT_LIB_VERSION ((EVENT_LIB_MAJOR << 16) | \
                           (EVENT_LIB_MINOR << 8)  | \
                           EVENT_LIB_PATCH)
#define EVENT_VERSION_STR "0.1.0"

#endif
