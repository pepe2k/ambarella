/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-01-09 2014 - [HuaiShun Hu] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_MD_VERSION_H_
#define AM_MD_VERSION_H_

#define MD_LIB_MAJOR 0
#define MD_LIB_MINOR 1
#define MD_LIB_PATCH 0
#define MD_LIB_VERSION ((MD_LIB_MAJOR << 16) | \
                        (MD_LIB_MINOR << 8)  | \
                         MD_LIB_PATCH)
#define MD_VERSION_STR "0.1.0"

#endif
