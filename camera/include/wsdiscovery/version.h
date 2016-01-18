/*
 * version.h
 *
 * @Author: Yupeng Chang
 * @Time  : 2013-04-07 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_WSDISCOVERY_VERSION_H__
#define __AM_WSDISCOVERY_VERSION_H__

#define WSDISCOVERY_LIB_MAJOR 0
#define WSDISCOVERY_LIB_MINOR 1
#define WSDISCOVERY_LIB_PATCH 0
#define WSDISCOVERY_LIB_VERSION ((WSDISCOVERY_LIB_MAJOR << 16) | \
                                 (WSDISCOVERY_LIB_MINOR << 8)  | \
                                 WSDISCOVERY_LIB_PATCH)

#define WSDISCOVERY_VERSION_STR "0.1.0"

#endif /* __AM_WSDISCOVERY_VERSION_H__ */
