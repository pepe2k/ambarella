/**
 * am_pridata.h
 *
 * History:
 *	2011/09/11 - [Zhi He] created file
 *
 * Desc: privata data define
 *
 * Copyright (C) 2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_PRIDATA_H__
#define __AM_PRIDATA_H__

//is this unique?
#define DAM_PRI_MAGIC_TAG MK_FOURCC_TAG('A', 'M', 'P', 'D')

//if excced this, report warnning
#define DMAX_PRIDATA_LENGTH (1024 - sizeof(SPriDataHeader) - sizeof(CBuffer) - 32)

AM_ERR AM_PackingPriData(AM_U8* dest, AM_U8* src, AM_UINT data_len, AM_U16 data_type, AM_U16 sub_type);
AM_U8* AM_UnPackingPriData(AM_U8* src, AM_UINT& data_len, AM_U16& data_type, AM_U16& sub_type);

#endif


