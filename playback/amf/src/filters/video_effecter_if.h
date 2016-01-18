/*
 * video_effecter_if.h
 *
 * History:
 *    2011/6/4 - [LiuGang] create file
 *
 * Copyright (C) 2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __VIDEO_EFFECRER_IF_H__
#define __VIDEO_EFFECRER_IF_H__

//GPU Oprating Functions realized in Application
// These Funcs should be defined in app
//int (*GPUInitStreamTexture)();
//int (*GPUInitGLES)();
extern int (*GPUCreateStreamTextureDevice)(bool [],int [], int [], int [], unsigned int [][5]);
extern int (*GPURenderTextureBuffer)(bool [],int []);
//int (*GPUDestroyStreamTextureDevice)();

extern void SetGPUFuncs(
        //int (*InitStreamTexture)(), int (*InitGLES)(), 
        int (*RenderTextureBuffer)(bool [],int []),
        int (*CreateStreamTextureDevice)(bool [],int [], int [], int [], unsigned int [][5])
        //int (*DestroyStreamTextureDevice)()
        );

typedef struct{
    enum transform_type{
        transform_type_start=0,
        transform_type_fade,
        transform_type_transparency,
        transform_type_flip,
        transform_type_rotate,
        transform_type_3d,
        transform_type_end,
    };
    enum action_speed{
        speed_slow,
        speed_mid,
        speed_fast,
        speed_none,
    };
    enum rotate{
        rotate_constant_speed,
        rotate_accelerated_speed,
        rotate_none,
    };
    int action_start;
    int action_step;
    int action_end;

} v_transform_component;
#endif //__VIDEO_EFFECRER_IF_H__