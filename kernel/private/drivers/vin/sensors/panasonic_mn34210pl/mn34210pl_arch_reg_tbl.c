/*
 * kernel/private/drivers/ambarella/vin/sensors/panasonic_mn34210pl/mn34210pl_arch_reg_tbl.c
 *
 * History:
 *    2012/12/24 - [Long Zhao] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

/* ========================================================================== */
static const struct mn34210pl_reg_table mn34210pl_linear_share_regs[] = {
	//A-4_S12_1x4_FULL_60p_594MHz_MCLK27_v306_140212_Ambarella.txt
	//VCYCLE:1125 HCYCLE:400 (@MCLK)
	{0x300B, 0x0000},// master mode
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000C},
	{0x0113, 0x000C},
	{0x3018, 0x0043},
	{0x301A, 0x0089},
	{0x301F, 0x001F},
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x00FF, 0x0000},
	{0x0202, 0x0004},
	{0x0203, 0x0063},
	{0x0342, 0x000B},
	{0x0343, 0x0075},
	{0x3039, 0x002A},
	{0x303E, 0x0044},
	{0x3040, 0x00C0},
	{0x3058, 0x000F},
	{0x305D, 0x0040},
	{0x3066, 0x000A},
	{0x3067, 0x00B0},
	{0x3068, 0x0008},
	{0x3069, 0x0000},
	{0x306A, 0x000B},
	{0x306B, 0x0060},
	{0x306C, 0x0009},
	{0x306D, 0x00D0},
	{0x306E, 0x0009},
	{0x306F, 0x0078},
	{0x3074, 0x0001},
	{0x3075, 0x0041},
	{0x3104, 0x0004},
	{0x3132, 0x0060},
	{0x3148, 0x0006},
	{0x314D, 0x003F},
	{0x314F, 0x0034},
	{0x3151, 0x0038},
	{0x3153, 0x0060},
	{0x3155, 0x0078},
	{0x3157, 0x0078},
	{0x3159, 0x0078},
	{0x315B, 0x0078},
	{0x315C, 0x0040},
	{0x315F, 0x0033},
	{0x3160, 0x0002},
	{0x316A, 0x0002},
	{0x3177, 0x002F},
	{0x3179, 0x0020},
	{0x317C, 0x0025},
	{0x3182, 0x0020},
	{0x3183, 0x0020},
	{0x31AB, 0x0001},
	{0x31AC, 0x0009},
	{0x31AD, 0x00FC},
	{0x31AE, 0x0001},
	{0x31AF, 0x0000},
	{0x320F, 0x001B},
	{0x3241, 0x0067},
	{0x3242, 0x0005},
	{0x3243, 0x0019},
	{0x3244, 0x0018},
	{0x3247, 0x001E},
	{0x3248, 0x0000},
	{0x324B, 0x0066},
	{0x324C, 0x0080},
	{0x324F, 0x0020},
	{0x3254, 0x0007},
	{0x3255, 0x000E},
	{0x3256, 0x001E},
	{0x3257, 0x001C},
	{0x3258, 0x0085},
	{0x3259, 0x0017},
	{0x325A, 0x001E},
	{0x325B, 0x00B7},
	{0x3261, 0x0020},
	{0x3263, 0x000B},
	{0x3267, 0x001F},
	{0x3303, 0x0005},
	{0x330A, 0x0000},
	{0x330B, 0x0000},
	{0x330F, 0x0000},
	{0x3318, 0x0000},
	{0x3319, 0x001E},
	{0x331D, 0x0000},
	{0x331E, 0x0000},
	{0x331F, 0x0000},
	{0x3324, 0x0000},
	{0x3325, 0x0000},
	{0x332F, 0x0000},
	{0x333E, 0x0000},
	{0x3340, 0x0000},
	{0x3341, 0x0000},
	{0x3352, 0x0004},
	{0x3354, 0x000F},
	{0x3359, 0x0006},
	{0x335F, 0x0003},
	{0x3365, 0x0007},
	{0x3367, 0x000D},
	{0x336A, 0x0005},
	{0x336B, 0x0005},
	{0x336F, 0x0002},
	{0x3371, 0x0007},
	{0x3375, 0x0006},
	{0x3377, 0x0005},
	{0x3378, 0x0006},
	{0x337A, 0x0004},
	{0x3380, 0x0002},
	{0x3381, 0x0005},
	{0x3384, 0x0001},
	{0x3385, 0x0004},
	{0x3395, 0x0000},
	{0x3397, 0x0007},
	{0x3399, 0x0000},
	{0x339B, 0x0007},
	{0x33B8, 0x0001},
	{0x33B9, 0x0000},
	{0x33BA, 0x0001},
	{0x33BB, 0x0004},
	{0x33C7, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};
#define MN34210PL_LINEAR_SHARE_REG_SIZE		ARRAY_SIZE(mn34210pl_linear_share_regs)

static const struct mn34210pl_reg_table mn34210pl_2x_wdr_share_regs[] = {
	//A-22-3_S10_1x4_FULL_WDR_30px2_594MHz_MCLK27_Slave_LVDS_H5280_V1500_v306e_140213.txt
	//VCYCLE:1500 HCYCLE:600 (@MCLK)
	{0x300B, 0x0000},// master mode
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000A},
	{0x0113, 0x000A},
	{0x3018, 0x0043},
	{0x301A, 0x0089},
	{0x301F, 0x0010}, // 0x10 for CS, MP, 0x1F for ES
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x00FF, 0x0000},
	{0x0202, 0x0001},
	{0x0203, 0x0013},
	{0x0340, 0x0005},
	{0x0341, 0x00DC},
	{0x0342, 0x0014},
	{0x0343, 0x00A0},
	{0x3039, 0x002A},
	{0x3040, 0x00C0},
	{0x3058, 0x0003},
	{0x305D, 0x0073},
	{0x3078, 0x0001},
	{0x306E, 0x0008},
	{0x306F, 0x0064},
	{0x3074, 0x0001},
	{0x3075, 0x0041},
	{0x3097, 0x00D1},
	{0x3098, 0x0080},
	{0x3101, 0x0001},
	{0x3104, 0x0004},
	{0x3106, 0x0000},
	{0x3107, 0x0080},
	{0x3126, 0x0001},
	{0x3127, 0x0013},
	{0x312A, 0x0001},
	{0x312B, 0x0013},
	{0x312E, 0x0001},
	{0x312F, 0x0013},
	{0x3132, 0x0060},
	{0x3140, 0x0000},
	{0x3145, 0x0066},
	{0x3146, 0x0066},
	{0x3147, 0x0066},
	{0x3148, 0x0006},
	{0x3149, 0x0000},
	{0x314A, 0x0000},
	{0x314B, 0x0000},
	{0x314C, 0x0000},
	{0x314F, 0x0017},
	{0x3151, 0x003C},
	{0x3153, 0x003E},
	{0x3155, 0x0028},
	{0x3157, 0x002E},
	{0x3159, 0x0060},
	{0x315B, 0x0061},
	{0x315C, 0x0000},
	{0x315F, 0x0033},
	{0x3160, 0x0002},
	{0x316A, 0x0002},
	{0x3177, 0x002F},
	{0x3180, 0x001C},
	{0x3181, 0x0024},
	{0x3182, 0x0020},
	{0x3183, 0x0020},
	{0x31AB, 0x0001},
	{0x31AC, 0x0009},
	{0x31AD, 0x0001},
	{0x31AE, 0x0001},
	{0x31AF, 0x0000},
	{0x3207, 0x0018},
	{0x320F, 0x001B},
	{0x3238, 0x0070},
	{0x323A, 0x0000},
	{0x3241, 0x0065},
	{0x3242, 0x0005},
	{0x3243, 0x0018},
	{0x3244, 0x000A},
	{0x3245, 0x0033},
	{0x3246, 0x0059},
	{0x3247, 0x001A},
	{0x3248, 0x0000},
	{0x3249, 0x0000},
	{0x324B, 0x0029},
	{0x324C, 0x0080},
	{0x324F, 0x0000},
	{0x3252, 0x000D},
	{0x3253, 0x0038},
	{0x3254, 0x0003},
	{0x3255, 0x000E},
	{0x3256, 0x001C},
	{0x3257, 0x001B},
	{0x3258, 0x0055},
	{0x3259, 0x003D},
	{0x325A, 0x001A},
	{0x325B, 0x0080},
	{0x3261, 0x0020},
	{0x3263, 0x000B},
	{0x3267, 0x001F},
	{0x326A, 0x0000},
	{0x326B, 0x0040},
	{0x3303, 0x0005},
	{0x330A, 0x0000},
	{0x330B, 0x0000},
	{0x330F, 0x001B},
	{0x3318, 0x0000},
	{0x3319, 0x0018},
	{0x331D, 0x0000},
	{0x331E, 0x0000},
	{0x331F, 0x0000},
	{0x3324, 0x0000},
	{0x3325, 0x0000},
	{0x332F, 0x001B},
	{0x3340, 0x0000},
	{0x3341, 0x0000},
	{0x3352, 0x0004},
	{0x3354, 0x000F},
	{0x3359, 0x0005},
	{0x335F, 0x0002},
	{0x3365, 0x0007},
	{0x3367, 0x000D},
	{0x336A, 0x000C},
	{0x336B, 0x000C},
	{0x336F, 0x0002},
	{0x3371, 0x0007},
	{0x3375, 0x0006},
	{0x3377, 0x0005},
	{0x3378, 0x0006},
	{0x337A, 0x0004},
	{0x3380, 0x0002},
	{0x3381, 0x000C},
	{0x3384, 0x0001},
	{0x3385, 0x0001},
	{0x3395, 0x0000},
	{0x3397, 0x0007},
	{0x3399, 0x0000},
	{0x339B, 0x0007},
	{0x33B8, 0x0001},
	{0x33B9, 0x0000},
	{0x33BA, 0x0001},
	{0x33BB, 0x0004},
	{0x33C7, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};
#define MN34210PL_2X_WDR_SHARE_REG_SIZE		ARRAY_SIZE(mn34210pl_2x_wdr_share_regs)

static const struct mn34210pl_reg_table mn34210pl_3x_wdr_share_regs[] = {
	//A-23_S10_1x4_FULL_WDR_30px3_594MHz_MCLK27_Slave_LVDS_H5280_V1500_v306e_140206.txt
	//VCYCLE:1500 HCYCLE:600 (@MCLK)
	{0x300B, 0x0000},// master mode
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000A},
	{0x0113, 0x000A},
	{0x3018, 0x0043},
	{0x301A, 0x0089},
	{0x301F, 0x0010}, // 0x10 for CS, MP, 0x1F for ES
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x00FF, 0x0000},
	{0x0202, 0x0001},
	{0x0203, 0x0013},
	{0x0340, 0x0005},
	{0x0341, 0x00DC},
	{0x0342, 0x0014},
	{0x0343, 0x00A0},
	{0x3039, 0x002A},
	{0x3040, 0x00C0},
	{0x3058, 0x0003},
	{0x305D, 0x0073},
	{0x3078, 0x0001},
	{0x306E, 0x0008},
	{0x306F, 0x0064},
	{0x3074, 0x0001},
	{0x3075, 0x0041},
	{0x3097, 0x00D1},
	{0x3098, 0x0080},
	{0x3101, 0x0002},
	{0x3104, 0x0004},
	{0x3106, 0x0000},
	{0x3107, 0x0080},
	{0x3126, 0x0001},
	{0x3127, 0x0013},
	{0x312A, 0x0001},
	{0x312B, 0x0013},
	{0x312E, 0x0001},
	{0x312F, 0x0013},
	{0x3132, 0x0060},
	{0x3140, 0x0000},
	{0x3145, 0x0066},
	{0x3146, 0x0066},
	{0x3147, 0x0066},
	{0x3148, 0x0006},
	{0x3149, 0x0000},
	{0x314A, 0x0000},
	{0x314B, 0x0000},
	{0x314C, 0x0000},
	{0x314F, 0x0017},
	{0x3151, 0x003C},
	{0x3153, 0x003E},
	{0x3155, 0x0028},
	{0x3157, 0x002E},
	{0x3159, 0x0060},
	{0x315B, 0x0061},
	{0x315C, 0x0000},
	{0x315F, 0x0033},
	{0x3160, 0x0002},
	{0x316A, 0x0002},
	{0x3177, 0x002F},
	{0x3180, 0x001C},
	{0x3181, 0x0024},
	{0x3182, 0x0020},
	{0x3183, 0x0020},
	{0x31AB, 0x0001},
	{0x31AC, 0x0009},
	{0x31AD, 0x0054},
	{0x31AE, 0x0001},
	{0x31AF, 0x0000},
	{0x3201, 0x0001},
	{0x3202, 0x0001},
	{0x3203, 0x0064},
	{0x3207, 0x0018},
	{0x320F, 0x001B},
	{0x3238, 0x0070},
	{0x323A, 0x0000},
	{0x3241, 0x0065},
	{0x3242, 0x0005},
	{0x3243, 0x0018},
	{0x3244, 0x000A},
	{0x3245, 0x0033},
	{0x3247, 0x001A},
	{0x3248, 0x0000},
	{0x3249, 0x0000},
	{0x324B, 0x0029},
	{0x324C, 0x00F6},
	{0x324F, 0x0000},
	{0x3252, 0x000D},
	{0x3253, 0x0038},
	{0x3254, 0x0003},
	{0x3255, 0x000E},
	{0x3256, 0x001C},
	{0x3257, 0x001B},
	{0x3258, 0x0056},
	{0x3259, 0x003D},
	{0x325A, 0x001A},
	{0x325B, 0x0025},
	{0x3261, 0x0020},
	{0x3263, 0x000B},
	{0x3267, 0x001F},
	{0x326A, 0x0000},
	{0x326B, 0x0040},
	{0x3303, 0x0005},
	{0x330A, 0x0000},
	{0x330B, 0x0000},
	{0x330F, 0x001B},
	{0x3318, 0x0000},
	{0x3319, 0x0018},
	{0x331D, 0x0000},
	{0x331E, 0x0000},
	{0x331F, 0x0000},
	{0x3324, 0x0000},
	{0x3325, 0x0000},
	{0x332F, 0x001B},
	{0x3340, 0x0000},
	{0x3341, 0x0000},
	{0x3352, 0x0004},
	{0x3354, 0x000F},
	{0x3359, 0x0005},
	{0x335F, 0x0002},
	{0x3365, 0x0007},
	{0x3367, 0x000D},
	{0x336A, 0x000C},
	{0x336B, 0x000C},
	{0x336F, 0x0002},
	{0x3371, 0x0007},
	{0x3375, 0x0006},
	{0x3377, 0x0005},
	{0x3378, 0x0006},
	{0x337A, 0x0004},
	{0x3380, 0x0002},
	{0x3381, 0x000C},
	{0x3384, 0x0001},
	{0x3385, 0x0001},
	{0x3395, 0x0000},
	{0x3397, 0x0007},
	{0x3399, 0x0000},
	{0x339B, 0x0007},
	{0x33B8, 0x0001},
	{0x33B9, 0x0000},
	{0x33BA, 0x0001},
	{0x33BB, 0x0004},
	{0x33C7, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};
#define MN34210PL_3X_WDR_SHARE_REG_SIZE		ARRAY_SIZE(mn34210pl_3x_wdr_share_regs)

static const struct mn34210pl_reg_table mn34210pl_4x_wdr_share_regs[] = {
	// 4x WDR
	{0x300E, 0x0001},
	{0x3001, 0x0003},
	{0x0112, 0x000A},
	{0x0113, 0x000A},
	{0x3007, 0x0010},
	{0x300A, 0x003F},
	{0x300B, 0x0000},// master mode
	{0x300C, 0x0010},
	{0x3018, 0x0043},
	{0x301F, 0x001F},
	{0x3001, 0x0053},
	{0x300E, 0x0000},
	{0x0202, 0x0000},
	{0x0203, 0x0016},
	{0x0342, 0x001B},
	{0x0343, 0x0080},
	{0x303C, 0x0002},
	{0x303D, 0x0028},
	{0x303E, 0x0047},
	{0x3041, 0x002C},
	{0x3042, 0x0070},
	{0x3052, 0x0090},
	{0x3053, 0x0022},
	{0x3054, 0x0030},
	{0x3058, 0x0003},
	{0x305C, 0x000E},
	{0x305D, 0x0073},
	{0x3062, 0x0020},
	{0x3063, 0x0031},
	{0x3064, 0x0010},
	{0x3065, 0x0032},
	{0x3076, 0x0002},
	{0x3078, 0x0001},
	{0x3087, 0x0003},
	{0x3088, 0x0014},
	{0x3089, 0x0004},
	{0x308B, 0x000B},
	{0x308C, 0x0008},
	{0x308D, 0x0004},
	{0x308E, 0x0004},
	{0x308F, 0x0009},
	{0x3090, 0x0005},
	{0x3101, 0x0003},
	{0x3124, 0x0004},
	{0x3125, 0x004F},
	{0x3127, 0x0016},
	{0x312B, 0x0016},
	{0x312F, 0x0016},
	{0x3132, 0x0060},
	{0x3137, 0x00FF},
	{0x313B, 0x0001},
	{0x3142, 0x0011},
	{0x3143, 0x000B},
	{0x3144, 0x000B},
	{0x3145, 0x0077},
	{0x3146, 0x0077},
	{0x3147, 0x0077},
	{0x3148, 0x0007},
	{0x3149, 0x0011},
	{0x314A, 0x0011},
	{0x314B, 0x0011},
	{0x314C, 0x0001},
	{0x314F, 0x0056},
	{0x3151, 0x0056},
	{0x3153, 0x0056},
	{0x3155, 0x0056},
	{0x3157, 0x0056},
	{0x3159, 0x0056},
	{0x315B, 0x0056},
	{0x315F, 0x0033},
	{0x3161, 0x0080},
	{0x3163, 0x0020},
	{0x3177, 0x002F},
	{0x3179, 0x0020},
	{0x317C, 0x0025},
	{0x3180, 0x00D4},
	{0x3181, 0x00C4},
	{0x3182, 0x0021},
	{0x3207, 0x000A},
	{0x320F, 0x001B},
	{0x3212, 0x0003},
	{0x3213, 0x000B},
	{0x3214, 0x0003},
	{0x3215, 0x000B},
	{0x3218, 0x0004},
	{0x3219, 0x001A},
	{0x321E, 0x00FF},
	{0x321F, 0x00FF},
	{0x3220, 0x00FF},
	{0x3221, 0x00FF},
	{0x3222, 0x00FF},
	{0x3223, 0x00FF},
	{0x3224, 0x00FF},
	{0x3225, 0x00FF},
	{0x3226, 0x00FF},
	{0x3227, 0x00FF},
	{0x3228, 0x00FF},
	{0x3229, 0x00FF},
	{0x322A, 0x00FF},
	{0x322B, 0x00FF},
	{0x322C, 0x00FF},
	{0x322D, 0x00FF},
	{0x3237, 0x0081},
	{0x323A, 0x0000},
	{0x3241, 0x0078},
	{0x3243, 0x0009},
	{0x3244, 0x000B},
	{0x3245, 0x0046},
	{0x3246, 0x0000},
	{0x3247, 0x0018},
	{0x3248, 0x0000},
	{0x3249, 0x0000},
	{0x324B, 0x0010},
	{0x324C, 0x0069},
	{0x324F, 0x0000},
	{0x3252, 0x0007},
	{0x3253, 0x006E},
	{0x3254, 0x0003},
	{0x3255, 0x0004},
	{0x3256, 0x0031},
	{0x3257, 0x0001},
	{0x3258, 0x000A},
	{0x3259, 0x000B},
	{0x325A, 0x0006},
	{0x325B, 0x00D6},
	{0x3261, 0x0020},
	{0x3262, 0x0010},
	{0x3263, 0x000B},
	{0x326B, 0x0040},
	{0x326C, 0x0046},
	{0x326D, 0x0004},
	{0x326E, 0x0072},
	{0x3308, 0x0000},
	{0x3309, 0x0000},
	{0x330A, 0x0000},
	{0x330B, 0x0000},
	{0x330F, 0x0000},
	{0x3318, 0x0000},
	{0x3319, 0x0018},
	{0x331B, 0x0000},
	{0x331C, 0x0000},
	{0x331D, 0x0000},
	{0x331E, 0x0003},
	{0x331F, 0x0003},
	{0x3322, 0x0000},
	{0x3323, 0x0000},
	{0x3324, 0x0000},
	{0x3325, 0x0000},
	{0x3327, 0x0000},
	{0x332F, 0x0000},
	{0x3334, 0x0007},
	{0x333A, 0x0007},
	{0x333D, 0x0000},
	{0x333E, 0x0001},
	{0x333F, 0x0002},
	{0x3340, 0x0000},
	{0x3341, 0x0000},
	{0x3344, 0x0017},
	{0x3345, 0x0000},
	{0x3352, 0x000E},
	{0x3354, 0x000F},
	{0x3355, 0x0001},
	{0x3356, 0x000F},
	{0x3359, 0x0005},
	{0x335A, 0x0002},
	{0x335B, 0x0002},
	{0x335D, 0x000C},
	{0x335E, 0x0009},
	{0x335F, 0x0002},
	{0x3361, 0x000F},
	{0x3363, 0x000D},
	{0x3365, 0x0001},
	{0x3367, 0x0007},
	{0x3369, 0x0002},
	{0x336A, 0x0004},
	{0x336B, 0x0004},
	{0x336C, 0x0001},
	{0x336D, 0x0001},
	{0x336F, 0x0005},
	{0x3371, 0x0007},
	{0x3375, 0x0004},
	{0x3377, 0x0005},
	{0x3378, 0x000C},
	{0x337A, 0x0004},
	{0x337B, 0x0000},
	{0x337E, 0x0007},
	{0x337F, 0x0007},
	{0x3380, 0x0004},
	{0x3381, 0x0004},
	{0x3382, 0x0005},
	{0x3383, 0x000C},
	{0x3384, 0x0000},
	{0x3385, 0x0000},
	{0x3386, 0x0002},
	{0x3389, 0x000C},
	{0x3392, 0x0007},
	{0x3393, 0x0007},
	{0x3395, 0x0001},
	{0x3397, 0x0000},
	{0x3399, 0x0006},
	{0x33B7, 0x00F6},
	{0x33B8, 0x0001},
	{0x33B9, 0x0000},
	{0x33BA, 0x0001},
	{0x33BB, 0x0004},
	{0x33C7, 0x0001},
	{0x33C9, 0x0001},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
};
#define MN34210PL_4X_WDR_SHARE_REG_SIZE		ARRAY_SIZE(mn34210pl_4x_wdr_share_regs)

static const struct mn34210pl_reg_table mn34210pl_720p_share_regs[] = {
	// A-16-2_S10_1x4_HD720p_120p_594MHz_MCLK27_Slave_LVDS_V1125_v306e_Ambarella_140626.txt
	// VCYCLE:1125 HCYCLE:200 (@MCLK)
	{0x300B, 0x0000},// master mode
	{0x300E, 0x0001},
	{0x300F, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x0003},
	{0x0112, 0x000A},
	{0x0113, 0x000A},
	{0x3018, 0x0043},
	{0x301A, 0x0089},
	{0x3000, 0x0000},
	{0x3001, 0x0053},
	{0x300E, 0x0000},
	{0x300F, 0x0000},
	{0x00FF, 0x0000},
	{0x0202, 0x0004},
	{0x0203, 0x0063},
	{0x0342, 0x0006},
	{0x0343, 0x00E0},
	{0x0347, 0x0098},
	{0x034A, 0x0003},
	{0x034B, 0x006F},
	{0x034E, 0x0002},
	{0x034F, 0x00D8},
	{0x3039, 0x002A},
	{0x3040, 0x00C0},
	{0x3058, 0x0003},
	{0x305D, 0x0040},
	{0x3066, 0x0002},
	{0x3067, 0x00AC},
	{0x3068, 0x0002},
	{0x3069, 0x0000},
	{0x306A, 0x0002},
	{0x306B, 0x00D8},
	{0x306C, 0x0002},
	{0x306D, 0x0074},
	{0x306E, 0x0008},
	{0x306F, 0x0064},
	{0x3074, 0x0001},
	{0x3075, 0x0041},
	{0x3097, 0x00D1},
	{0x3098, 0x0080},
	{0x3104, 0x0004},
	{0x3106, 0x0000},
	{0x3107, 0x0080},
	{0x3132, 0x0060},
	{0x3140, 0x0000},
	{0x3145, 0x0066},
	{0x3146, 0x0066},
	{0x3147, 0x0066},
	{0x3148, 0x0006},
	{0x3149, 0x0000},
	{0x314A, 0x0000},
	{0x314B, 0x0000},
	{0x314C, 0x0000},
	{0x314F, 0x0017},
	{0x3151, 0x003C},
	{0x3153, 0x003E},
	{0x3155, 0x0028},
	{0x3157, 0x002E},
	{0x3159, 0x0060},
	{0x315B, 0x0061},
	{0x315C, 0x0000},
	{0x315F, 0x0033},
	{0x3160, 0x0002},
	{0x316A, 0x0002},
	{0x3177, 0x002F},
	{0x3180, 0x001C},
	{0x3181, 0x0024},
	{0x3182, 0x0020},
	{0x3183, 0x0020},
	{0x31AB, 0x0001},
	{0x31AC, 0x0009},
	{0x31AD, 0x0054},
	{0x31AE, 0x0001},
	{0x31AF, 0x0000},
	{0x3207, 0x0018},
	{0x320F, 0x001B},
	{0x3241, 0x0065},
	{0x3242, 0x0005},
	{0x3243, 0x0018},
	{0x3244, 0x000A},
	{0x3245, 0x0033},
	{0x3247, 0x001A},
	{0x3248, 0x0000},
	{0x3249, 0x0000},
	{0x324B, 0x0029},
	{0x324C, 0x00F6},
	{0x324F, 0x0000},
	{0x3252, 0x000D},
	{0x3253, 0x0038},
	{0x3254, 0x0003},
	{0x3255, 0x000E},
	{0x3256, 0x001C},
	{0x3257, 0x001B},
	{0x3258, 0x0056},
	{0x3259, 0x003D},
	{0x325A, 0x001A},
	{0x325B, 0x0025},
	{0x3261, 0x0020},
	{0x3263, 0x000B},
	{0x3267, 0x001F},
	{0x326A, 0x0000},
	{0x326B, 0x0040},
	{0x3303, 0x0005},
	{0x330A, 0x0000},
	{0x330B, 0x0000},
	{0x330F, 0x001B},
	{0x3318, 0x0000},
	{0x3319, 0x0018},
	{0x331D, 0x0000},
	{0x331E, 0x0000},
	{0x331F, 0x0000},
	{0x3324, 0x0000},
	{0x3325, 0x0000},
	{0x332F, 0x001B},
	{0x3340, 0x0000},
	{0x3341, 0x0000},
	{0x3352, 0x0004},
	{0x3354, 0x000F},
	{0x3359, 0x0005},
	{0x335F, 0x0002},
	{0x3365, 0x0007},
	{0x3367, 0x000D},
	{0x336A, 0x000C},
	{0x336B, 0x000C},
	{0x336F, 0x0002},
	{0x3371, 0x0007},
	{0x3375, 0x0006},
	{0x3377, 0x0005},
	{0x3378, 0x0006},
	{0x337A, 0x0004},
	{0x3380, 0x0002},
	{0x3381, 0x000C},
	{0x3384, 0x0001},
	{0x3385, 0x0001},
	{0x3395, 0x0000},
	{0x3397, 0x0007},
	{0x3399, 0x0000},
	{0x339B, 0x0007},
	{0x33B8, 0x0001},
	{0x33B9, 0x0000},
	{0x33BA, 0x0001},
	{0x33BB, 0x0004},
	{0x33C7, 0x0000},
	{0x3000, 0x0000},
	{0x3001, 0x00D3},
	{0x0100, 0x0001},
	{0x0101, 0x0000},
};
#define MN34210PL_720P_SHARE_REG_SIZE		ARRAY_SIZE(mn34210pl_720p_share_regs)

static const struct mn34210pl_pll_reg_table mn34210pl_pll_tbl[] = {
	[0] = {// linear 30fps
		.pixclk = 197977500,
		.extclk = 26987750,
		.regs = {
		}
	},
	[1] = {// for 2x WDR
		.pixclk = 237600000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
		}
	},
	[2] = {// for 3x WDR
		.pixclk = 237600000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
		}
	},
	[3] = {// for 4x WDR
		.pixclk = 237600000,
		.extclk = PLL_CLK_27MHZ,
		.regs = {
		}
	},
	[4] = {// linear 29.97fps
		.pixclk = 197955326,
		.extclk = 26984750,
		.regs = {
		}
	},
};
