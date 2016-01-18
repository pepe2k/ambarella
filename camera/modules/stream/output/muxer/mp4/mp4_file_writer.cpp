/*
 * mp4_file_writer.cpp
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
#include "am_include.h"
#include "am_mw_packet.h"
#include "am_queue.h"
#include "am_base.h"
#include "am_base_packet.h"
#include "am_muxer_info.h"
#include "output_record_if.h"
#include "am_media_info.h"

#include "file_sink.h"
#include "mp4_file_writer.h"
#include <libgen.h>

#ifndef EXTRA_NAME_LEN
#define EXTRA_NAME_LEN 60
#endif

IMp4DataWriter *CMp4FileWriter::Create ()
{
   CMp4FileWriter *result = new CMp4FileWriter ();
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CMp4FileWriter::CMp4FileWriter ():
   mEventMap (0),
   mFileCounter (0),
   mEventFileCounter (0),
   mpFileName (NULL),
   mpTmpName (NULL),
   mpPathName (NULL),
   mpBaseName (NULL),
   mpWriter (NULL)
{

}

CMp4FileWriter::~CMp4FileWriter()
{
   AM_DELETE (mpWriter);

   delete[] mpFileName;
   delete[] mpTmpName;
   delete[] mpPathName;
   delete[] mpBaseName;
}

void *CMp4FileWriter::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IMp4DataWriter) {
      return (IMp4DataWriter*)this;
   }

   ERROR ("No such interface.\n");
   return NULL;
}

void CMp4FileWriter::Delete ()
{
   delete this;
}

AM_ERR CMp4FileWriter::Construct ()
{
   if ((mpWriter = CFileWriter::Create ()) == NULL) {
      ERROR ("Failed to create an instance of CFilerWriter.\n");
      return ME_ERROR;
   }

   ((CFileWriter *)mpWriter)->SetEnableDirectIO (false);

   return ME_OK;
}

AM_ERR CMp4FileWriter::Init ()
{
   return ME_OK;
}

AM_ERR CMp4FileWriter::SetMediaSink (const char *pFileName)
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

   if (AM_UNLIKELY((mpTmpName = new char [strlen (pFileName) \
               + EXTRA_NAME_LEN]) == NULL)) {
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

AM_ERR CMp4FileWriter::SetSplitDuration (AM_U64 duration)
{
   return ME_OK;
}

AM_ERR CMp4FileWriter::CreateNextSplitFile ()
{
   if (mEventMap) {
      snprintf (mpTmpName,
                strlen (mpFileName) + EXTRA_NAME_LEN,
                "%s/%s_event_%u.mp4",
                mpPathName,
                mpBaseName,
                ++mEventFileCounter);
   } else {
      snprintf (mpTmpName,
                strlen (mpFileName) + EXTRA_NAME_LEN,
                "%s_%u.mp4",
                mpFileName,
                ++mFileCounter);
   }

   /* Mp4 muxer doesn't begin to run. */
   if (mpWriter->CreateFile (mpTmpName) != ME_OK) {
      ERROR ("Failed to create %s: %s\n",
            mpTmpName,
            strerror (errno));
      return ME_ERROR;
   }

   if (mpWriter->Setbuf (IO_TRANSFER_BLOCK_SIZE) != ME_OK) {
      INFO ("Failed to set IO buffer.\n");
   }

   NOTICE ("File create: %s\n", mpTmpName);


   return ME_OK;
}

AM_ERR CMp4FileWriter::WriteData (AM_U8 *pData, int dataLen)
{
   if (pData == NULL || dataLen <= 0) {
      ERROR ("Invalid parameter.\n");
      return ME_BAD_PARAM;
   }

   return mpWriter->WriteFile (pData, dataLen);
}

AM_ERR CMp4FileWriter::SeekData(am_file_off_t offset, AM_UINT whence)
{
   return mpWriter->SeekFile (offset, whence);
}

AM_ERR CMp4FileWriter::Deinit ()
{
   if (mpWriter) {
      AM_ENSURE_OK_ (mpWriter->CloseFile ());
   }

   return ME_OK;
}

void CMp4FileWriter::OnEOF ()
{
   if (mpWriter) {
      AM_ENSURE_OK_ (mpWriter->CloseFile ());
      AM_ENSURE_OK_ (CreateNextSplitFile ());
   }
}

void CMp4FileWriter::OnEvent ()
{
   if (mpWriter) {
      mEventMap = 1 << 0;
      AM_ENSURE_OK_ (mpWriter->CloseFile ());
      AM_ENSURE_OK_ (CreateNextSplitFile ());
      mEventMap = 0;
   }

}

void CMp4FileWriter::OnEOS ()
{
   if (mpWriter) {
      AM_ENSURE_OK_ (mpWriter->CloseFile ());
   }
}
