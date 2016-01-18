/*
 * ts_file_writer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 19/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __TS_FILE_WRITER_H__
#define __TS_FILE_WRITER_H__

#ifndef DATA_CACHE_SIZE
#define DATA_CACHE_SIZE (1 << 21) /* 2MB */
#endif

class CTsFileWriter: public ITsDataWriter
{
public:
   static ITsDataWriter *Create ();

public:
   /* Interfaces declared at IInterface. */
   virtual void *GetInterface (AM_REFIID refiid);
   virtual void Delete ();

   /* Interfaces declared at ITsDataWriter. */
   virtual AM_ERR Init ();
   virtual AM_ERR Deinit ();
   virtual void OnEOF (int streamType);
   virtual void OnEOS (int streamType);
   virtual void OnEvent ();
   virtual AM_ERR SetMediaSink (const char *destStr);
   virtual AM_ERR SetSplitDuration (AM_U64 splitDuration);
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen, int dataType);

protected:
   CTsFileWriter ();
   AM_ERR Construct ();
   virtual ~CTsFileWriter ();

protected:
   virtual AM_ERR CreateNextSplitFile ();

protected:
   bool           mbEventFlag;
   int            mEOFMap;
   int            mEOSMap;
   int            mFileCounter;
   int            mEventCounter;
   char          *mpFileName;
   char          *mpTmpName;
   char          *mpPathName;
   char          *mpBaseName;
   unsigned char *mpData;
   unsigned int   mDataLen;
   IFileWriter   *mpDataWriter;
};

#endif
