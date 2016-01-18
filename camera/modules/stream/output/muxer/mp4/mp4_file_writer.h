/*
 * mp4_file_writer.h
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __MP4_FILE_WRITER_H__
#define __MP4_FILE_WRITER_H__

class CMp4FileWriter: public IMp4DataWriter
{
public:
   static IMp4DataWriter *Create ();

protected:
   CMp4FileWriter ();
   AM_ERR Construct ();
   virtual ~CMp4FileWriter ();

protected:
   /* Interfaces declared at IInterface. */
   virtual void *GetInterface (AM_REFIID refiid);
   virtual void Delete ();

   /* Interfaces declared at IMp4DataWriter. */
   virtual AM_ERR Init ();
   virtual AM_ERR Deinit ();
   virtual void OnEOF ();
   virtual void OnEvent ();
   virtual void OnEOS ();
   virtual AM_ERR SetMediaSink (const char *destStr);
   virtual AM_ERR SetSplitDuration (AM_U64 splitDuration);
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen);
   virtual AM_ERR SeekData(am_file_off_t offset, AM_UINT whence);

protected:
   virtual AM_ERR CreateNextSplitFile ();

protected:
   int          mEventMap;
   int          mFileCounter;
   int          mEventFileCounter;
   char        *mpFileName;
   char        *mpTmpName;
   char        *mpPathName;
   char        *mpBaseName;
   IFileWriter *mpWriter;
};

#endif
