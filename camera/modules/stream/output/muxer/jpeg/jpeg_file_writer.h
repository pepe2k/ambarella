/*
 * jpeg_file_writer.h
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
#ifndef __JPEG_FILE_WRITER_H__
#define __JPEG_FILE_WRITER_H__

class CJpegFileWriter: public IJpegDataWriter
{
public:
   static IJpegDataWriter *Create ();
   CJpegFileWriter ();
   AM_ERR Construct ();
   virtual ~CJpegFileWriter ();

public:
   /* Interfaces declared at IInterface. */
   virtual void *GetInterface (AM_REFIID refiid);
   virtual void Delete ();

   /* Interfaces declared at IJpegDataWriter. */
   virtual AM_ERR Init ();
   virtual AM_ERR Deinit ();
   virtual AM_ERR SetMediaSink (const char *destStr);
   virtual AM_ERR SetMaxFileAmount (AM_UINT fileAmount);
   virtual AM_ERR WriteData (AM_U8 *pData, int dataLen);

private:
   AM_ERR CreateSymbolLinks ();

private:
   bool mbOldTmpFile;
   int mFileCounter;
   int mMaxFileAmount;
   char *mpFileName;
   char *mpCurFileName;
   char *mpTmpFileName;
   IFileWriter *mpDataWriter;
};

#endif
