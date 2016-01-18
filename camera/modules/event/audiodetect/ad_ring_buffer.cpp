/*
 * ad_ring_buffer.cpp
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
#include "am_include.h"
#include "am_utility.h"
#include "am_event.h"

void AudioDetectRingBufferInit(AudioDetectRingBuffer *rb, int size) {
  rb->size  = size;
  rb->start = 0;
  rb->count = 0;
  if (rb->elems) {
    free(rb->elems);
    rb->elems = NULL;
  }
  rb->elems = (audio_detect_msg_s *)calloc(rb->size, sizeof(audio_detect_msg_s));
}

void AudioDetectRingBufferFree(AudioDetectRingBuffer *rb) {
  if (rb->elems) {
    free(rb->elems); /* OK if null */
    rb->elems = NULL;
  }
}

void AudioDetectRingBufferWrite(AudioDetectRingBuffer *rb, audio_detect_msg_s *elem) {
  int end = (rb->start + rb->count) % rb->size;
  rb->elems[end] = *elem;
  if (rb->count == rb->size)
    rb->start = (rb->start + 1) % rb->size; /* full, overwrite */
  else
    ++ rb->count;
}

void AudioDetectRingBufferRead(AudioDetectRingBuffer *rb, audio_detect_msg_s *elem) {
  *elem = rb->elems[rb->start];
  rb->start = (rb->start + 1) % rb->size;
  -- rb->count;
}
