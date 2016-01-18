/*
 * ak4954_amb.h  --  AK4954 Soc Audio driver
 *
 * Copyright 2014 Ambarella Ltd.
 *
 * Author: [Ken He]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _AK4954_AMB_H
#define _AK4954_AMB_H

struct ak4954_platform_data {
	unsigned int	rst_pin;
	unsigned int 	rst_delay;
};
#endif
