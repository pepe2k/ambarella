/*
 * jpeg_file_writer.cpp
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
#include "jpeg_file_writer.h"

IJpegDataWriter *CJpegFileWriter::Create ()
{
   CJpegFileWriter *result = new CJpegFileWriter ();
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CJpegFileWriter::CJpegFileWriter ():
   mbOldTmpFile (false),
   mFileCounter (0),
   mMaxFileAmount (0),
   mpFileName (NULL),
   mpCurFileName (NULL),
   mpTmpFileName (NULL),
   mpDataWriter (NULL)
{}

CJpegFileWriter::~CJpegFileWriter()
{
  AM_DELETE (mpDataWriter);
  delete[] mpFileName;
  delete[] mpCurFileName;
  delete[] mpTmpFileName;
}

void *CJpegFileWriter::GetInterface (AM_REFIID refiid)
{
   if (refiid == IID_IJpegDataWriter) {
      return (IJpegDataWriter *)this;
   }

   ERROR ("No such interface.\n");
   return NULL;
}

void CJpegFileWriter::Delete ()
{
   delete this;
}

AM_ERR CJpegFileWriter::Construct ()
{
   if ((mpDataWriter = CFileWriter::Create ()) == NULL) {
      ERROR ("Failed to create an instance of CFilerWriter.\n");
      return ME_ERROR;
   }

   ((CFileWriter *)mpDataWriter)->SetEnableDirectIO (false);
   return ME_OK;
}

AM_ERR CJpegFileWriter::Init ()
{
   return ME_OK;
}

AM_ERR CJpegFileWriter::SetMediaSink (const char *pFileName)
{
   const char *fileName = "/tmp/";

   if (pFileName == NULL) {
      ERROR ("File's name should not be empty.\n");
      return ME_ERROR;
   }

   /*
    * Currently, as the file location for different stream has not
    * be splitted and the speed of writing sd card is not fast, so
    * we need to put jpeg file into memory.
    *
    * When the file location of different stream is seperated,
    * remove the definition of fileName and replace fileName with
    * pFileName in this routine.
    */
   if (mpFileName != NULL) {
      INFO ("File name: %s will be rewritten.\n", mpFileName);
      delete[] mpFileName;
      mpFileName = NULL;
   }

   if (mpCurFileName != NULL) {
      delete[] mpCurFileName;
      mpCurFileName = NULL;
   }

   if (mpTmpFileName != NULL) {
     delete[] mpTmpFileName;
   }

   if (AM_UNLIKELY(NULL == (mpFileName = amstrdup(fileName)))) {
     ERROR("Failed to duplicate file name string!");
     return ME_NO_MEMORY;
   }

   mpCurFileName = new char [strlen (fileName) + 10];
   if (AM_UNLIKELY(mpCurFileName == NULL)) {
      ERROR ("Failed to allocate memory for cur name.\n");
      return ME_NO_MEMORY;
   } else {
     memset(mpCurFileName, 0, sizeof(strlen(fileName) + 10));
   }

   mpTmpFileName = new char [strlen(fileName) + 10];
   if (AM_UNLIKELY(mpTmpFileName == NULL)) {
      ERROR ("Failed to allocate memory for cur name.\n");
      return ME_NO_MEMORY;
   } else {
     memset(mpTmpFileName, 0, sizeof(strlen(fileName) + 10));
     snprintf (mpTmpFileName, strlen(mpFileName) + 10,
           "%s_%s.jpeg", mpFileName, "tmp");
   }

   INFO ("File's name: %s\n", mpFileName);
   return CreateSymbolLinks ();
}

AM_ERR CJpegFileWriter::SetMaxFileAmount (AM_UINT fileAmount)
{
   mMaxFileAmount = fileAmount;
   NOTICE ("MaxFileAmount = %d", mMaxFileAmount);

   return ME_OK;
}

AM_ERR CJpegFileWriter::CreateSymbolLinks ()
{
   int i, status;
   char* webName = NULL;
   char* cmd = NULL;

   /*
    * To avoid looking whether symbol links exist or not during
    * middleware working, we precreate some symbol links like
    * the following.
    *
    * /webSvr/web/snap/testx.jpg ---> test_streamx_x.jpeg
    */
   if ((webName = new char[strlen (mpFileName) + 10]) == NULL) {
      ERROR ("Failed to allocate memory!");
      return ME_ERROR;
   }

   if ((cmd = new char[strlen (mpFileName) + 20]) == NULL) {
      ERROR ("Failed to allocate memory!");
      delete[] webName;
      return ME_ERROR;
   }

   for (i = 0; i < mMaxFileAmount; i++) {
      snprintf (mpCurFileName, strlen(mpFileName) + 10,
                "%s_%u.jpeg", mpFileName, i);
      snprintf (cmd, strlen(mpFileName) + 20, "touch %s", mpCurFileName);
      snprintf (webName, strlen (mpFileName) + 10,
                "%s%u.jpg", "/webSvr/web/snap/snap", i);

      if (access (mpCurFileName, F_OK) != 0) {
         status = system (cmd);
         if (WEXITSTATUS (status)) {
            ERROR ("Failed to create %s: %s",
                  mpCurFileName,
                  strerror (errno));
            delete[] cmd;
            delete[] webName;
            return ME_ERROR;
         }
      }

      if (access ("/webSvr/web/snap", F_OK) == 0) {
         if (access (webName, F_OK) != 0) {
            /* Symbol link does't exist, we create it. */
            if (symlink (mpCurFileName, webName) < 0) {
               ERROR ("Failed to create symbol: %s ---> %s: %s",
                     webName,
                     mpCurFileName,
                     strerror (errno));
               delete[] cmd;
               delete[] webName;
               return ME_ERROR;
            }

         }
      } else {
         NOTICE ("Warning: /webSvr/web/snap does't exist");
      }
   }

   delete[] cmd;
   delete[] webName;

   return ME_OK;
}

AM_ERR CJpegFileWriter::WriteData (AM_U8 *pData, int dataLen)
{
   if (pData == NULL || dataLen <= 0) {
      ERROR ("Invalid parameter.\n");
      return ME_BAD_PARAM;
   }

   if (mpDataWriter == NULL) {
      ERROR ("File writer was not initialized.\n");
      return ME_BAD_STATE;
   }

   if (mbOldTmpFile) {
      /* Rename xxx_tmp.jpeg to xxx_x.jpeg unsuccessfully, retry. */
      if (rename (mpTmpFileName, mpCurFileName) < 0) {
         NOTICE ("Failed to rename %s to %s: %s\n",
               mpTmpFileName,
               mpCurFileName,
               strerror (errno));
         remove (mpTmpFileName);
      }

      mbOldTmpFile = false;
   }

   if (mpDataWriter->CreateFile (mpTmpFileName) != ME_OK) {
      ERROR ("Failed to create %s: %s\n",
            mpTmpFileName,
            strerror (errno));
      return ME_ERROR;
   }

   AM_ENSURE_OK_ (mpDataWriter->WriteFile (pData, dataLen));
   AM_ENSURE_OK_ (mpDataWriter->CloseFile ());

   mFileCounter = (mFileCounter + 1) % mMaxFileAmount;
   snprintf (mpCurFileName, strlen (mpFileName) + 10,
         "%s_%d.jpeg", mpFileName, mFileCounter);
   NOTICE ("mpCurFileName = %s", mpCurFileName);

   /* Try to rename xxx_tmp.jpeg to xxx_x.jpeg */
   if (rename (mpTmpFileName, mpCurFileName) < 0) {
      NOTICE ("Failed to rename %s to %s: %s\n",
            mpTmpFileName,
            mpCurFileName,
            strerror (errno));
      mbOldTmpFile = true;
   }

   return ME_OK;
}

AM_ERR CJpegFileWriter::Deinit ()
{
   if (mpDataWriter) {
      AM_ENSURE_OK_ (mpDataWriter->CloseFile ());
   }

   return ME_OK;
}

