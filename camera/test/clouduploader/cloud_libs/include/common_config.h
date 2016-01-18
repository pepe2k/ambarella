/**
 * common_config.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2012, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __COMMON_CONFIG_H__
#define __COMMON_CONFIG_H__

#define DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN \
extern "C" { \
    namespace mw_cg {

#define DCONFIG_COMPILE_OPTION_HEADERFILE_END \
    } \
}

#define DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN \
namespace mw_cg {

#define DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END \
}


#define DCONFIG_COMPILE_OPTION_CPPFILE_BEGIN \
using namespace mw_cg;

#define DCODE_DELIMITER

//debug related
#define DCONFIG_ENABLE_DEBUG_CHECK

#endif

