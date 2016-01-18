/**
 * am_utils.h
 *
 * History:
 *	2012/2/29 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_UTILS_H__
#define __AM_UTILS_H__

inline AM_U16 GetBe16(AM_U8* pData)
{
  return (pData[1] | (pData[0]<<8));
}

inline AM_U32 GetBe32(AM_U8* pData)
{
  return (pData[3] | (pData[2]<<8) | (pData[1]<<16) | (pData[0]<<24));
}

inline AM_U32 LeToBe32(AM_U32 data)
{
  AM_U32 rval;
  rval  = (data&0x000000FF)<<24;
  rval += (data&0x0000FF00)<<8;
  rval += (data&0x00FF0000)>>8;
  rval += (data&0xFF000000)>>24;
  return rval;
}

inline AM_U32 BeToLe32(AM_U32 data)
{
  AM_U32 rval;
  rval  = (data&0x000000FF)<<24;
  rval += (data&0x0000FF00)<<8;
  rval += (data&0x00FF0000)>>8;
  rval += (data&0xFF000000)>>24;
  return rval;
}

#endif
