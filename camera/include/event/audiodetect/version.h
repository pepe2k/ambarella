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

#ifndef __AM_AUDIO_DETECT_VERSION_H__
#define __AM_AUDIO_DETECT_VERSION_H__

#define AUDIO_DETECT_LIB_MAJOR 0
#define AUDIO_DETECT_LIB_MINOR 1
#define AUDIO_DETECT_LIB_PATCH 0
#define AUDIO_DETECT_LIB_VERSION ((AUDIO_DETECT_LIB_MAJOR << 16) | \
                                  (AUDIO_DETECT_LIB_MINOR << 8)  | \
                                   AUDIO_DETECT_LIB_PATCH)

#define AUDIO_DETECT_VERSION_STR "0.1.0"

#endif
