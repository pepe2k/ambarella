/*******************************************************************************
 * am_vdevice.h
 *
 * Histroy:
 *  2012-3-8 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMVDEVICE_H_
#define AMVDEVICE_H_

#include "vdevice/version.h"
#include "vdevice/am_video_device.h"

/* Following header files are video device's implementations */
#include "vdevice/simplecam/am_simplecam.h"
#include "vdevice/simplephoto/am_simplephoto.h"
#include "vdevice/encode/am_encode_device.h"
#ifdef CONFIG_ARCH_S2
#include "vdevice/warp/am_warp_device.h"
#endif
#endif /* AMVDEVICE_H_ */
