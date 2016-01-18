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

#ifndef __IAV_H__
#define __IAV_H__

int init_iav(void);
int get_second_buf_size(int *w, int *h, int *p);
int get_second_y_buf(char *buf);
int get_second_rgb_buf(char *buf);
int get_second_buf_offset(char *buf, unsigned int *offset);
int get_me1_buf_size(int *w, int *h, int *p);
int get_me1_buf(char *buf);
int exit_iav(void);

#endif
