/*
 * include/ambas_ain.h
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBAS_AIN_H
#define __AMBAS_AIN_H

struct amba_ain_audio_param{
	snd_pcm_format_t format;
	AM_UINT channels;
	AM_UINT rate;
	AM_UINT bits_per_sample;
	AM_UINT bits_per_frame;
};

#endif
