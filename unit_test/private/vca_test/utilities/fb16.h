/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __FB_H__
#define __FB_H__

#include <ambas_fdet.h>

int open_fb(void);
int get_fb_size(int *w, int *h);
int blank_fb(void);
int render_frame_gray(char *d, int w, int h);
int render_frame_bgr888(char *d, int w, int h);
int annotate_faces(struct fdet_face *faces, int num);
int close_fb(void);

#endif
