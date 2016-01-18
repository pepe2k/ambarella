/**
 * version.h
 *
 *  History:
 *		Jan 3, 2014 - [binwang] created file
 *
 * Copyright (C) 2014-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef VERSION_H_
#define VERSION_H_

#define AUDIOANALY_LIB_MAJOR 0
#define AUDIOANALY_LIB_MINOR 1
#define AUDIOANALY_LIB_PATCH 0
#define AUDIOANALY_LIB_VERSION ((AUDIOANALY_LIB_MAJOR << 16) | \
                             (AUDIOANALY_LIB_MINOR << 8)  | \
                             AUDIOANALY_LIB_PATCH)

#define AUDIOANALY_VERSION_STR "0.1.0"

#endif /* VERSION_H_ */
