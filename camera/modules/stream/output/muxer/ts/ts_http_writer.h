/*
 * ts_http_writer.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 19/09/2012 [Created]
 *          06/01/2013 [Rewrited]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __TS_HTTP_WRITER_H__
#define __TS_HTTP_WRITER_H__

class CTsHttpWriter: public CTsFileWriter
{
   typedef CTsFileWriter inherited;

public:
   static ITsDataWriter *Create (CPacketFilter *);

private:
   CTsHttpWriter (CPacketFilter *);
   AM_ERR Construct ();
   virtual ~CTsHttpWriter ();

public:
   /* Interfaces delcared at ITsDataWriter */
   virtual AM_ERR Init ();
   virtual AM_ERR Deinit ();
   virtual void OnEOF (int streamType);
   virtual void OnEOS (int streamType);
   virtual void OnEvent () {}
   virtual AM_ERR SetMediaSink (const char *destStr);
   virtual AM_ERR SetSplitDuration (AM_U64 duration);

private:
   static size_t CurlReadCallback (void *ptr,
         size_t size, size_t nmemb, void *userp);
   size_t CurlRead (void *pDestBuf, size_t size, size_t nmemb);
   AM_ERR CurlInit ();
   void CurlDeinit ();

   static AM_ERR ServerThread (void *p);
   AM_ERR ServerThreadLoop ();

private:
   bool mbRun;
   bool mbCurlRun;
   char *mpDestURL;
   int mUploadFile;
   int mUploadIndex;
   int mSplitDuration;
   CURL *mpCurlHandle;
   CCondition *mpWaitCond;
   CMutex *mpMutex;
   CThread *mpServerThread;
   CPacketFilter *mpOwner;
};

#endif
