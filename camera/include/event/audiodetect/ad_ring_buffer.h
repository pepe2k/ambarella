/*
 * ad_ring_buffer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 14/01/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AD_RING_BUFFER_H__
#define __AD_RING_BUFFER_H__

#include <stdio.h>
#include <malloc.h>

struct audio_detect_msg_s;

/* Circular buffer object */
struct AudioDetectRingBuffer{
    int                 size;   /* maximum number of elements */
    int                 start;  /* index of oldest element */
    int                 count;  /* counter */
    audio_detect_msg_s *elems;  /* vector of elements */
};

void AudioDetectRingBufferInit(AudioDetectRingBuffer *rb, int size);
void AudioDetectRingBufferFree(AudioDetectRingBuffer *rb);

inline bool AudioDetectRingBufferIsFull(AudioDetectRingBuffer *rb)
{
  return rb->count == rb->size ? true: false;
}

inline bool AudioDetectRingBufferIsEmpty(AudioDetectRingBuffer *rb)
{
  return rb->count == 0? true: false;
}

void AudioDetectRingBufferWrite(AudioDetectRingBuffer *rb, audio_detect_msg_s *elem);
void AudioDetectRingBufferRead(AudioDetectRingBuffer *rb, audio_detect_msg_s *elem);

#endif
