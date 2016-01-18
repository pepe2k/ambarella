/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2012-3-30 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_CONFIGURE_VERSION_H_
#define AM_CONFIGURE_VERSION_H_

#define CONFIGURE_LIB_MAJOR 0
#define CONFIGURE_LIB_MINOR 1
#define CONFIGURE_LIB_PATCH 0
#define CONFIGURE_LIB_VERSION ((CONFIGURE_LIB_MAJOR << 16) | \
                               (CONFIGURE_LIB_MINOR << 8)  | \
                               CONFIGURE_LIB_PATCH)
#define CONFIGURE_VERSION_STR "0.1.0"

#endif /* AM_CONFIGURE_VERSION_H_ */
