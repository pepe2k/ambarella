/*
 * include/ambas_event.h
 *
 * History:
 *    2010/07/23 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBAS_EVENT_H
#define __AMBAS_EVENT_H

#include <ambas_common.h>

enum amb_event_type {
	/* No Event */
	AMB_EV_NONE				= 0x00000000,

	/* VIN Event */
	AMB_EV_VIN_DECODER_SOURCE_PLUG		= 0x00010000,
	AMB_EV_VIN_DECODER_SOURCE_REMOVE,

	/* VOUT Event */
	AMB_EV_VOUT_CVBS_PLUG			= 0x00020000,
	AMB_EV_VOUT_CVBS_REMOVE,
	AMB_EV_VOUT_YPBPR_PLUG,
	AMB_EV_VOUT_YPBPR_REMOVE,
	AMB_EV_VOUT_HDMI_PLUG,
	AMB_EV_VOUT_HDMI_REMOVE,

	/* SENSOR Event*/
	AMB_EV_ACCELEROMETER_REPORT		= 0x00030000,
	AMB_EV_MAGNETIC_FIELD_REPORT,
	AMB_EV_LIGHT_REPORT,
	AMB_EV_PROXIMITY_REPORT,
	AMB_EV_GYROSCOPE_REPORT,
	AMB_EV_TEMPERATURE_REPORT,

	/* FB2 Event */
	AMB_EV_FB2_PAN_DISPLAY			= 0x00040000,

	/* FDET_EVENT */
	AMB_EV_FDET_FACE_DETECTED		= 0x00050000,
};

struct amb_event {
	u32			sno;		//sequential number
	u64			time_code;
	enum amb_event_type	type;
	u8			data[32];
};

#endif
