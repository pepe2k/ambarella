/*
 * ts_http_writer.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 25/09/2012 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "output_record_if.h"
#include "mpeg_ts_defs.h"
#include "am_media_info.h"
#include "am_include.h"

#include "ts_builder.h"
#include "ts_muxer.h"
#include <curl/curl.h>
#include "file_sink.h"
#include "ts_file_writer.h"
#include "ts_http_writer.h"

#ifndef CURL_TRANSFER_TIMEOUT
#define CURL_TRANSFER_TIMEOUT (2 * mSplitDuration)
#endif

ITsDataWriter *CTsHttpWriter::Create (CPacketFilter *pFilter)
{
   CTsHttpWriter *result = new CTsHttpWriter (pFilter);
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CTsHttpWriter::CTsHttpWriter (CPacketFilter *pFilter):
   mbRun (false),
   mbCurlRun (false),
   mpDestURL (NULL),
   mUploadFile (-1),
   mUploadIndex (0),
   mSplitDuration (0),
   mpCurlHandle (NULL),
   mpWaitCond (NULL),
   mpMutex (NULL),
   mpServerThread (NULL),
   mpOwner (pFilter)
{}

AM_ERR CTsHttpWriter::Construct ()
{
   if (inherited::Construct () != ME_OK) {
      ERROR ("Failed to construct parent!");
      return ME_ERROR;
   }

   if ((mpMutex = CMutex::Create (false)) == NULL) {
      ERROR ("Failed to create a mutex instance.\n");
      return ME_OS_ERROR;
   }

   if ((mpWaitCond = CCondition::Create ()) == NULL) {
      ERROR ("Failed to create a condition instance.\n");
      return ME_OS_ERROR;
   }

   return ME_OK;
}

AM_ERR CTsHttpWriter::Init ()
{
   return CurlInit ();
}

CTsHttpWriter::~CTsHttpWriter ()
{
   AM_DELETE (mpMutex);
   AM_DELETE (mpWaitCond);
   delete[] mpDestURL;
   if (mUploadFile >= 0) {
     close(mUploadFile);
   }
}

AM_ERR CTsHttpWriter::SetMediaSink (const char *pDestURL)
{
   if (inherited::SetMediaSink ("/tmp/ts_file") != ME_OK) {
      ERROR ("Failed to set media sink for parent!");
      return ME_ERROR;
   }

   if (pDestURL == NULL) {
      ERROR ("Destination url should not be empty.\n");
      return ME_ERROR;
   }

   if (mpDestURL != NULL) {
      INFO ("Destination URL: %s will be rewrited.\n", mpDestURL);
      delete[] mpDestURL;
      mpDestURL = NULL;
   }
   mpDestURL = amstrdup(pDestURL);
   if (AM_UNLIKELY(!mpDestURL)) {
     ERROR("Failed to duplicate destination url!");
     return ME_NO_MEMORY;
   }

   return ME_OK;
}

AM_ERR CTsHttpWriter::SetSplitDuration (AM_U64 duration)
{
   if (duration % 90000L == 0) {
      mSplitDuration = (int)(duration / 90000L);
   } else {
      mSplitDuration = (int)(duration * 2997 / 270000000LL);
   }

   NOTICE ("mSplitDuration = %d", mSplitDuration);
   return ME_OK;
}

AM_ERR CTsHttpWriter::Deinit ()
{
   CurlDeinit ();
   return ME_OK;
}

void CTsHttpWriter::OnEOF (int streamType)
{
   inherited::OnEOF (streamType);

   /* Latest file has been written completed. */
   mpWaitCond->Signal ();
}

void CTsHttpWriter::OnEOS (int streamType)
{
   inherited::OnEOS (streamType);

   mbRun = false;
   mpWaitCond->Signal ();
}

AM_ERR CTsHttpWriter::ServerThread (void *p)
{
   return ((CTsHttpWriter *)p)->ServerThreadLoop ();
}

AM_ERR CTsHttpWriter::ServerThreadLoop ()
{
   CURLcode res;
   double uploadSpeed, totalTime;
   char readFileName[MAX_FILE_NAME_LEN];

   mbRun = true;
   while (mbCurlRun) {
      if (mUploadIndex >= (mFileCounter - 1)) {
         /* Latest file has not been written completed. */
         mpWaitCond->Wait (mpMutex);
      }

      snprintf (readFileName, sizeof (readFileName),
            "/tmp/ts_file_%d.ts", mUploadIndex ++);

      /* Open latest file to prepare reading. */
      if ((mUploadFile = open (readFileName, O_RDONLY)) < 0) {
         ERROR ("Failed to open %s: %s",
               readFileName,
               strerror (errno));
         mpOwner->PostEngineMsg (IEngine::MSG_ERROR);

         mbCurlRun = false;
         return ME_ERROR;
      }

      if ((res = curl_easy_perform (mpCurlHandle)) != CURLE_OK) {
         mbCurlRun = false;
         ERROR ("curl_easy_perform error: %d\n", res);
         if ((res == CURLE_COULDNT_RESOLVE_HOST) ||
               (res == CURLE_COULDNT_CONNECT)) {
            ERROR ("Cann't connect to %s!", mpDestURL);
            mpOwner->PostEngineMsg (IEngine::MSG_ERROR);
         } else {
            NOTICE ("Recording will restart!");
            mpOwner->PostEngineMsg (IEngine::MSG_OVFL);
         }

         close(mUploadFile);
         mUploadFile = -1;
         return ME_ERROR;
      }

      /* Now, extract transfer info */
      curl_easy_getinfo (mpCurlHandle, CURLINFO_SPEED_UPLOAD, &uploadSpeed);
      curl_easy_getinfo (mpCurlHandle, CURLINFO_TOTAL_TIME, &totalTime);
      NOTICE ("\nSize: %.3f bytes/sec during %.3f seconds\n\n",
            uploadSpeed, totalTime);

      close (mUploadFile);
      mUploadFile = -1;
      if (remove (readFileName) < 0) {
         ERROR ("Failed to remove %s: %s",
               readFileName,
               strerror (errno));
      }

      if (totalTime >= (double)(CURL_TRANSFER_TIMEOUT)) {
         NOTICE ("Network is not good!");

         mbCurlRun = false;
         mpOwner->PostEngineMsg (IEngine::MSG_OVFL);
         return ME_ERROR;
      }

      if (!mbRun) {
         break;
      }
   }

   INFO ("Curl thread exit mainloop");
   return ME_OK;
}

size_t CTsHttpWriter::CurlReadCallback (void *pDestBuf,
      size_t size, size_t nmemb, void *userp)
{
   return ((CTsHttpWriter *)userp)->CurlRead (pDestBuf, size, nmemb);
}

size_t CTsHttpWriter::CurlRead (void *pDestBuf, size_t size, size_t nmemb)
{
   AM_U32 readSize;
   AM_U32 maxSizeToRead = size * nmemb;

   AM_ASSERT (maxSizeToRead > 0);

   if ((readSize = read (mUploadFile,
               pDestBuf, maxSizeToRead)) < 0) {
      ERROR ("Failed to read %d from /tmp/ts_file_%d: %s",
            maxSizeToRead,
            mUploadIndex,
            strerror (errno));
      return 0;
   }

   return readSize;
}

AM_ERR CTsHttpWriter::CurlInit ()
{
   mpCurlHandle = curl_easy_init ();
   if (!mpCurlHandle) {
      ERROR ("Failed to initialize curl.\n");
      return ME_ERROR;
   }

   /* Upload to this place */
   curl_easy_setopt (mpCurlHandle, CURLOPT_URL, mpDestURL);

   /* Tell it to "POST" to the URL */
   curl_easy_setopt (mpCurlHandle, CURLOPT_POST, 1L);

   /* We want to use our own read function */
   curl_easy_setopt (mpCurlHandle, CURLOPT_READFUNCTION, CurlReadCallback);

   /* Set where to read from (on windows you need to use READFUNCTION too) */
   curl_easy_setopt (mpCurlHandle, CURLOPT_READDATA, this);

   /* Enable verbose for easier tracing */
   curl_easy_setopt (mpCurlHandle, CURLOPT_VERBOSE, 1L);

   /* Modify HTTP Header */
   struct curl_slist *chunk = NULL;
   chunk = curl_slist_append (chunk, "Content-Type: video/ts");
   curl_easy_setopt (mpCurlHandle, CURLOPT_HTTPHEADER, chunk);

   chunk = curl_slist_append (chunk, "Transfer-Encoding: chunked");
   curl_easy_setopt (mpCurlHandle, CURLOPT_HTTPHEADER, chunk);

   chunk = curl_slist_append (chunk, "Expect:");
   curl_easy_setopt (mpCurlHandle, CURLOPT_HTTPHEADER, chunk);

   curl_easy_setopt (mpCurlHandle, CURLOPT_VERBOSE, 0);
   curl_easy_setopt (mpCurlHandle, CURLOPT_CONNECTTIMEOUT, 15);
   curl_easy_setopt (mpCurlHandle, CURLOPT_TIMEOUT, 30);

   mbCurlRun = true;
   if ((mpServerThread = CThread::Create ("Connection Server",
               ServerThread, this)) == NULL) {
      ERROR ("Failed to create connection server thread.\n");
      return ME_ERROR;
   }

#ifdef BUILD_AMBARELLA_CAMERA_ENABLE_RT
   mpServerThread->SetRTPriority (CThread::PRIO_LOW);
#endif

   return ME_OK;
}

void CTsHttpWriter::CurlDeinit ()
{
   AM_DELETE (mpServerThread);
   curl_easy_cleanup (mpCurlHandle);
   mbCurlRun = false;
}
