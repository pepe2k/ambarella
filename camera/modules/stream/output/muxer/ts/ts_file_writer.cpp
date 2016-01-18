/*
 * ts_file_writer.cpp
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
#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_if.h"
#include "am_include.h"
#include "am_mw.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "output_record_if.h"
#include "am_media_info.h"
#include "file_sink.h"
#include "ts_file_writer.h"

#include <libgen.h>

#ifndef EXTRA_NAME_LEN
#define EXTRA_NAME_LEN 60
#endif

ITsDataWriter *CTsFileWriter::Create ()
{
   CTsFileWriter *result = new CTsFileWriter ();
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CTsFileWriter::CTsFileWriter ():
   mbEventFlag (false),
   mEOFMap (0),
   mEOSMap (0),
   mFileCounter (0),
   mEventCounter (0),
   mpFileName (NULL),
   mpTmpName (NULL),
   mpPathName (NULL),
   mpBaseName (NULL),
   mpData (NULL),
   mDataLen (0),
   mpDataWriter (NULL)
{}

CTsFileWriter::~CTsFileWriter()
{
   AM_DELETE (mpDataWriter);

   delete[] mpFileName;
   delete[] mpTmpName;
   delete[] mpData;
   delete[] mpPathName;
   delete[] mpBaseName;
}

void *CTsFileWriter::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_ITsDataWriter) {
      return (ITsDataWriter *)this;
   }

   ERROR ("No such interface.\n");
   return NULL;
}

void CTsFileWriter::Delete ()
{
   delete this;
}

AM_ERR CTsFileWriter::Construct ()
{
   if ((mpData = new unsigned char[DATA_CACHE_SIZE]) == NULL) {
      ERROR ("Failed to allocate memory for data cache!");
      return ME_NO_MEMORY;
   }

   if ((mpDataWriter = CFileWriter::Create ()) == NULL) {
      ERROR ("Failed to create an instance of CFilerWriter.\n");
      return ME_ERROR;
   }

   ((CFileWriter *)mpDataWriter)->SetEnableDirectIO (false);

   return ME_OK;
}

AM_ERR CTsFileWriter::Init ()
{
   return ME_OK;
}

AM_ERR CTsFileWriter::SetMediaSink (const char *pFileName)
{
   char *copyName, *str;

   if (pFileName == NULL) {
      ERROR ("File's name should not be empty.\n");
      return ME_ERROR;
   }

   INFO ("pFileName = %s\n", pFileName);

   if (mpFileName != NULL) {
      INFO ("File name: %s will be rewritten.\n", mpFileName);
      delete[] mpFileName;
      mpFileName = NULL;
   }

   if (mpTmpName != NULL) {
      delete[] mpTmpName;
      mpTmpName = NULL;
   }

   if (AM_UNLIKELY(NULL == (mpFileName = amstrdup(pFileName)))) {
      ERROR("Failed to duplicate file name string!");
      return ME_NO_MEMORY;
   }

   if (AM_UNLIKELY((mpTmpName = new char [strlen (pFileName) +
               EXTRA_NAME_LEN]) == NULL)) {
      ERROR ("Failed to allocate memory for tmp name.\n");
      return ME_NO_MEMORY;
   } else {
      memset(mpTmpName, 0, sizeof(strlen (pFileName) + EXTRA_NAME_LEN));
   }

   /* Allocate memory for copyName */
   if (AM_UNLIKELY((copyName = new char [strlen (pFileName)]) == NULL)) {
      ERROR ("Failed to allocate memory for tmp name.\n");
      return ME_NO_MEMORY;
   }

   /* Allocate memory for mpPathName */
   if (AM_UNLIKELY((mpPathName = new char [strlen (pFileName)]) == NULL)) {
      ERROR ("Failed to allocate memory for tmp name.\n");
      delete[] copyName;
      return ME_NO_MEMORY;
   } else {
      /* Fetch path name from pFileName. */
      strcpy (copyName, pFileName);
      str = dirname (copyName);
      strcpy (mpPathName, str);

      /* Append "event" to mpPathName. */
      sprintf (mpPathName + strlen (mpPathName), "%s", "/event");
      NOTICE ("Directory's path: %s", mpPathName);
   }

   /* Allocate memory for mpBaseName */
   if (AM_UNLIKELY((mpBaseName = new char [strlen (pFileName)]) == NULL)) {
      ERROR ("Failed to allocate memory for tmp name.\n");
      delete[] copyName;
      return ME_NO_MEMORY;
   } else {
      /* Fetch base name from pFileName. */
      strcpy (copyName, pFileName);
      str = basename (copyName);
      strcpy (mpBaseName, str);
      NOTICE ("Base name: %s", mpBaseName);
   }

   /* Free memory. */
   delete[] copyName;

   /*
    * Check whether current directory used to store
    * video files contains event sub-directory.
    */
   if (access (mpPathName, F_OK) != 0) {
      /* Event sub-directory doesn't exist, create it! */
      if (mkdir (mpPathName, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
         ERROR ("Failed to create an event sub-directory!");
         return ME_ERROR;
      }
   }

   return CreateNextSplitFile ();
}

AM_ERR CTsFileWriter::SetSplitDuration (AM_U64 duration)
{
   return ME_OK;
}

AM_ERR CTsFileWriter::CreateNextSplitFile ()
{
   if (mbEventFlag) {
      snprintf (mpTmpName,
                strlen (mpFileName) + EXTRA_NAME_LEN,
                "%s/%s_event_%u.ts",
                mpPathName,
                mpBaseName,
                ++mEventCounter);
   } else {
      snprintf (mpTmpName,
                strlen (mpFileName) + EXTRA_NAME_LEN,
                "%s_%u.ts",
                mpFileName,
                ++mFileCounter);
   }

   if (mEOFMap == 0) {
      /* Ts muxer doesn't begin to run. */
      if (mpDataWriter->CreateFile (mpTmpName) != ME_OK) {
         ERROR ("Failed to create %s: %s\n",
               mpTmpName,
               strerror (errno));
         return ME_ERROR;
      }

      if (mpDataWriter->Setbuf (IO_TRANSFER_BLOCK_SIZE) != ME_OK) {
         INFO ("Failed to set IO buffer.\n");
      }

      NOTICE ("File create: %s\n", mpTmpName);
   } else if (mEOFMap == 3) {
      /*
       * Both audio and video data of previous splitted file
       * has been written and we need to close it. Then write
       * the data in cache into new file.
       */
      mpDataWriter->CloseFile ();
      if (mpDataWriter->CreateFile (mpTmpName) != ME_OK) {
         ERROR ("Failed to create %s: %s",
               mpTmpName,
               strerror (errno));
         return ME_ERROR;
      }

      AM_ENSURE_OK_ (mpDataWriter->WriteFile (mpData, mDataLen));
      mDataLen = 0;
   }

   return ME_OK;
}

AM_ERR CTsFileWriter::WriteData (AM_U8 *pData, int dataLen, int dataType)
{
   if (pData == NULL || dataLen <= 0) {
      ERROR ("Invalid parameter.\n");
      return ME_BAD_PARAM;
   }

   if (mpDataWriter == NULL) {
      ERROR ("File writer was not initialized.\n");
      return ME_BAD_STATE;
   }

   if (mEOFMap == 0) {
      /* Both audio and video stream don't reach splitting duration. */
      AM_ENSURE_OK_ (mpDataWriter->WriteFile (pData, dataLen));
   } else {
      /* Video will always reach EOF before Audio,
       * so when mEOFMap == VIDEO_STREAM, the following Video data should be
       * written to cache, and Audio data should be written to old file,
       * when both Video and Audio reached EOF, mEOFMap will be 0 again
       */
      if ((mEOFMap == dataType) && (dataType == VIDEO_STREAM)) {
         memcpy (mpData + mDataLen, pData, dataLen);
         mDataLen += dataLen;
      } else {
         mpDataWriter->WriteFile (pData, dataLen);
      }
   }

   return ME_OK;
}

AM_ERR CTsFileWriter::Deinit ()
{
   return ME_OK;
}

void CTsFileWriter::OnEOF (int streamType)
{
   mEOFMap |= streamType;

   if (mEOFMap == 0x3) {
      AM_ENSURE_OK_ (CreateNextSplitFile ());
      mEOFMap = 0;
   }
}

void CTsFileWriter::OnEOS (int streamType)
{
   if (streamType == AUDIO_STREAM) {
      mEOSMap |= 0x1 << 0;
   } else {
      mEOSMap |= 0x1 << 1;
   }

   if (mEOSMap == 0x3) {
      AM_ENSURE_OK_ (mpDataWriter->WriteFile (mpData, mDataLen));
      AM_ENSURE_OK_ (mpDataWriter->CloseFile ());
      mDataLen = 0;
   }
}

void CTsFileWriter::OnEvent ()
{
   mbEventFlag = true;
   AM_ENSURE_OK_ (mpDataWriter->CloseFile ());
   AM_ENSURE_OK_ (CreateNextSplitFile ());
   mbEventFlag = false;
}
