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

#ifndef __FDET_H__
#define __FDET_H__

int load_classifier(const char *cls);
int load_still(const char *d, unsigned int sz);
int fdet_start(struct fdet_configuration *cfg);
int fdet_get_faces(struct fdet_face *faces, unsigned int *num);
int fdet_track_faces(char *buf);

#endif
