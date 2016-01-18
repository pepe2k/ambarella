/**
 * am_pridata.cpp
 *
 * History:
 *	2011/11/09 - [Zhi He] created file
 *
 * Desc: privata data implement
 *
 * Copyright (C) 2007-2010, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_util.h"
#include "am_pridata.h"

AM_ERR AM_PackingPriData(AM_U8* dest, AM_U8* src, AM_UINT data_len, AM_U16 data_type, AM_U16 sub_type)
{
    SPriDataHeader* pheader = (SPriDataHeader*) dest;
    if (!dest || !src) {
        AM_ERROR("NULL pointer in AM_PackingPriData dest %p, src %p.\n", dest, src);
        return ME_BAD_PARAM;
    }

    AM_ASSERT(data_len < DMAX_PRIDATA_LENGTH);
    if (data_len >= DMAX_PRIDATA_LENGTH) {
        AM_WARNING("data length %d exceed max len %d.\n", data_len, DMAX_PRIDATA_LENGTH);
        data_len = DMAX_PRIDATA_LENGTH - 1 - sizeof(SPriDataHeader);
    }

    pheader->magic_number = DAM_PRI_MAGIC_TAG;
    pheader->data_length = data_len;

    pheader->type = data_type;
    pheader->subtype = sub_type;
    pheader->reserved = 0;

    memcpy(dest + sizeof(SPriDataHeader), src, pheader->data_length);
    return ME_OK;
}

AM_U8* AM_UnPackingPriData(AM_U8* src, AM_UINT& data_len, AM_U16& data_type, AM_U16& sub_type)
{
    SPriDataHeader* pheader = (SPriDataHeader*) src;
    if (!src) {
        AM_ERROR("NULL pointer in AM_PackingPriData src %p.\n", src);
        return NULL;
    }

    AM_ASSERT(DAM_PRI_MAGIC_TAG == pheader->magic_number);
    data_len =  pheader->data_length;

    data_type = pheader->type;
    sub_type = pheader->subtype;
    AM_ASSERT(0 == pheader->reserved);
    return (src+sizeof(SPriDataHeader));
}

