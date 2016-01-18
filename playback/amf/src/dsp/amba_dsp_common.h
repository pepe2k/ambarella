/**
 * amba_dsp_common.h
 *
 * History:
 *  2011/07/12 - [Zhi He] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBA_DSP_COMMON_H__
#define __AMBA_DSP_COMMON_H__

AM_ERR GetVoutSize(AM_INT mode, AM_UINT* width, AM_UINT* height);
AM_ERR GetSinkId(AM_INT iavfd, AM_INT type, AM_INT* pSinkId);

AM_INT RectOutofRange(AM_INT size_x, AM_INT size_y, AM_INT offset_x, AM_INT offset_y, AM_INT range_size_x, AM_INT range_size_y, AM_INT range_offset_x, AM_INT range_offset_y);
#endif


