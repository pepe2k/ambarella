/*
 * ts_hls_writer.h
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
#ifndef __TS_HLS_WRITER_H__
#define __TS_HLS_WRITER_H__

class CTsHlsWriter: public CTsFileWriter
{
   typedef CTsFileWriter inherited;

public:
   static ITsDataWriter *Create ();

public:
   virtual AM_ERR Init () { return ME_OK; }
   virtual AM_ERR Deinit () { return ME_OK; }
   virtual void OnEOF (int streamType);
   virtual void OnEOS (int streamType);
   virtual void OnEvent () {}
   virtual AM_ERR SetMediaSink (const char *destStr);
   virtual AM_ERR SetSplitDuration (AM_U64 splitDuration);

private:
   CTsHlsWriter ();
   AM_ERR Construct ();
   virtual ~CTsHlsWriter ();

private:
   AM_ERR CreateM3u8IndexFile ();
   AM_ERR UpdateM3u8IndexFile ();
   AM_ERR FetchHostIPAddress ();
   AM_ERR CreateSymbolLinks ();
   void AppendEntry (FILE *);
   void DeleteAndAppendEntry (FILE *, FILE *);

private:
   char *mpM3u8FileName;
   char *mpBaseFileName;
   char *mpDirectoryName;
   char *mpTmpM3u8Name;
   char *mpHostIPAddress;
   AM_U64 mSplitDuration;
};

#endif
