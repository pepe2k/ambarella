/*
 * hls_ts_muxer.cpp
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 26/09/2012 [Created]
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
#include "mpeg_ts_defs.h"
#include "am_media_info.h"
#include "ts_builder.h"

#include "file_sink.h"
#include "ts_file_writer.h"
#include "ts_hls_writer.h"

#ifndef MAX_INDEX_ENTRY_LEN
#define MAX_INDEX_ENTRY_LEN 512
#endif

#ifndef MAX_IPV4_ADDRESS_LEN
#define MAX_IPV4_ADDRESS_LEN 15
#endif

#ifndef MAX_CHAR_IN_LINE
#define MAX_CHAR_IN_LINE 1024
#endif

#ifndef SPLIT_FILE_NUM_IN_M3U8
#define SPLIT_FILE_NUM_IN_M3U8 10
#endif

#ifndef LIGHTTPD_WORK_DIR
#define LIGHTTPD_WORK_DIR "/webSvr/web"
#endif

ITsDataWriter *CTsHlsWriter::Create ()
{
   CTsHlsWriter *result = new CTsHlsWriter ();
   if (result && result->Construct () != ME_OK) {
      delete result;
      result = NULL;
   }

   return result;
}

CTsHlsWriter::CTsHlsWriter ():
   inherited (),
   mpM3u8FileName (NULL),
   mpBaseFileName (NULL),
   mpDirectoryName (NULL),
   mpTmpM3u8Name (NULL),
   mpHostIPAddress (NULL),
   mSplitDuration (0)
{}

AM_ERR CTsHlsWriter::Construct ()
{
   if (inherited::Construct () != ME_OK) {
      ERROR ("Failed to construct parent.\n");
      return ME_ERROR;
   }

   if ((mpHostIPAddress = new char[MAX_IPV4_ADDRESS_LEN]) == NULL) {
      ERROR ("Failed to allocate memory to store ipv4 address.\n");
      return ME_ERROR;
   }

   mpHostIPAddress[0] = '\0';
   return ME_OK;
}

CTsHlsWriter::~CTsHlsWriter ()
{
  delete[] mpM3u8FileName;
  delete[] mpHostIPAddress;
  delete[] mpBaseFileName;
  delete[] mpDirectoryName;
  delete[] mpTmpM3u8Name;
}

AM_ERR CTsHlsWriter::SetMediaSink (const char *pFileName)
{
   /*
    * For original design, user can configure where ts segment
    * files are stored and maybe those files are configured in
    * sd card. For this situation, reading data from sd card
    * needs to consume extra time. So we set media sink of hls
    * to /tmp directory because writing data to this directory
    * is the same to write data to memory directly.
    *
    * We need to check whether /tmp/media sub-directory exist.
    */
   if (access ("/tmp/media", F_OK) != 0) {
      /* "/tmp/meida" sub-directory doesn't exist, create it! */
      if (mkdir ("/tmp/media", S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
         ERROR ("Failed to create an event sub-directory!");
         return ME_ERROR;
      }
   }

   if (inherited::SetMediaSink ("/tmp/media/ts_hls_file") != ME_OK) {
      ERROR ("Failed to initialize file name.\n");
      return ME_ERROR;
   }

   delete[] mpM3u8FileName;
   mpM3u8FileName = NULL;
   delete[] mpTmpM3u8Name;
   mpTmpM3u8Name = NULL;

   if ((mpM3u8FileName = new char[strlen (pFileName) + 9]) == NULL) {
      ERROR ("Failed to allocate memory to store m3u8 name.\n");
      return ME_NO_MEMORY;
   }

   if ((mpTmpM3u8Name = new char[strlen (pFileName) + 9]) == NULL) {
      ERROR ("Failed to allocate memory to store m3u8 name.\n");
      return ME_NO_MEMORY;
   }

   if (FetchHostIPAddress () != ME_OK) {
      ERROR ("Failed to fetch host ip address!");
      return ME_ERROR;
   }

   return CreateM3u8IndexFile ();
}

AM_ERR CTsHlsWriter::SetSplitDuration (AM_U64 splitDuration)
{
   mSplitDuration = splitDuration;
   NOTICE ("splitDuration = %d", mSplitDuration);
   return ME_OK;
}

void CTsHlsWriter::OnEOF (int streamType)
{
   mEOFMap |= streamType;

   if (mEOFMap == 0x3) {
      AM_ENSURE_OK_ (CreateNextSplitFile ());
      AM_ENSURE_OK_ (UpdateM3u8IndexFile ());
      mEOFMap = 0;
   }
}

void CTsHlsWriter::OnEOS (int streamType)
{
   inherited::OnEOS (streamType);
}

AM_ERR CTsHlsWriter::CreateM3u8IndexFile ()
{
   FILE *fp;
   int i, splitSec, baseNameLen;

   /*
    * Create live.m3u8 file and initialize it.
    *
    * Firstly, we need to get it's absolute path of
    * this index file. For our hls server, live.m3u8
    * is put in the parent directory of which those
    * splitted ts files locate.
    */
   i = strlen (mpFileName) - 1;
   while (i > 0 && mpFileName[i--] != '/');

   delete[] mpBaseFileName;
   mpBaseFileName = NULL;
   delete[] mpDirectoryName;
   mpDirectoryName = NULL;

   /* Initialize mpBaseFileName. */
   baseNameLen = strlen (mpFileName) - 1 - (i + 1);
   if ((mpBaseFileName = new char[baseNameLen + 1]) == NULL) {
      ERROR ("Failed to allocate memory for mpBaseFileName.");
      return ME_NO_MEMORY;
   }

   /* Initialize mpDirectoryName. */
   if ((mpDirectoryName = new char[i + 2]) == NULL) {
      ERROR ("Failed to allocate memory for mpDirectoryName.");
      return ME_NO_MEMORY;
   }

   memset (mpDirectoryName, 0, i + 2);
   memset (mpBaseFileName, 0, baseNameLen);
   memcpy (mpBaseFileName, mpFileName + i + 2, baseNameLen);
   memcpy (mpDirectoryName, mpFileName, i + 2);

   mpDirectoryName[i + 1] = '\0';
   mpBaseFileName[baseNameLen] = '\0';
   // NOTICE ("mpBaseFileName = %s", mpBaseFileName);
   // NOTICE ("mpDirectoryName = %s", mpDirectoryName);

   while (i > 0 && mpFileName[i--] != '/');
   strncpy (mpM3u8FileName, mpFileName, i + 1);
   strncpy (mpTmpM3u8Name, mpFileName, i + 1);
   strcpy (mpM3u8FileName + i + 1, "/live.m3u8");
   strcpy (mpTmpM3u8Name + i + 1, "/tmp.m3u8");
   // NOTICE ("mpM3u8FileName = %s", mpM3u8FileName);
   // NOTICE ("mpTmpM3u8Name = %s", mpTmpM3u8Name);

   if ((fp = fopen (mpM3u8FileName, "w+"))  == NULL) {
      ERROR ("Failed to open %s: %s\n",
            mpM3u8FileName,
            strerror (errno));
      return ME_ERROR;
   }

   if (mSplitDuration % 90000L == 0) {
      splitSec = (int)(mSplitDuration / 90000L);
   } else {
      splitSec = (int)(mSplitDuration * 2997 / 270000000LL);
   }

   NOTICE ("splitSec = %d", splitSec);

   fprintf (fp,
         "#EXTM3U\n"
         "#EXT-X-TARGETDURATION:%d\n"
         "#EXT-X-MEDIA-SEQUENCE:%d\n\n",
         splitSec,
         mFileCounter);

   fclose (fp);
   return CreateSymbolLinks ();
}

AM_ERR CTsHlsWriter::UpdateM3u8IndexFile ()
{
   char indexEntry[MAX_INDEX_ENTRY_LEN];
   FILE *oldFp, *newFp;
   int lineNum;

   oldFp = fopen(mpM3u8FileName, "r");
   newFp = fopen(mpTmpM3u8Name, "w");

   if (AM_UNLIKELY(!oldFp || !newFp)) {
     if (oldFp) {
       fclose(oldFp);
     } else {
       ERROR ("Failed to open %s: %s\n",
              mpM3u8FileName,
              strerror (errno));
     }
     if (newFp) {
       fclose(newFp);
     } else {
       ERROR ("Failed to open %s: %s\n",
              mpTmpM3u8Name,
              strerror (errno));
     }
     return ME_ERROR;
   }

   if (mFileCounter > SPLIT_FILE_NUM_IN_M3U8 + 1) {
      /*
       * The first ten splitted files have been generated
       * and we need to delete the first entry and append
       * the newest one into the m3u8 index file.
       */
      DeleteAndAppendEntry (oldFp, newFp);
   } else {
      /*
       * There are just less than ten splitted file have
       * been generated and we can simply append the newest
       * one into the m3u8 index file.
       */
      lineNum = 0;
      while (fgets (indexEntry, sizeof (indexEntry), oldFp) != NULL) {
         if (++lineNum == 3) {
            snprintf (indexEntry, sizeof (indexEntry),
               "#EXT-X-MEDIA-SEQUENCE:%d\n", mFileCounter - 1);
         }

         fputs (indexEntry, newFp);
      }

      AppendEntry (newFp);
   }

   fclose (oldFp);
   fclose (newFp);

   if (remove (mpM3u8FileName) < 0) {
      ERROR ("Failed to remove %s: %s\n",
            mpM3u8FileName,
            strerror (errno));
      return ME_ERROR;
   }

   if (rename (mpTmpM3u8Name, mpM3u8FileName) < 0) {
      ERROR ("Failed to rename %s to %s: %s\n",
            mpTmpM3u8Name,
            mpM3u8FileName,
            strerror (errno));
      return ME_ERROR;
   }

   return ME_OK;
}

void CTsHlsWriter::DeleteAndAppendEntry (FILE *oldFp, FILE *newFp)
{
   int currentLineNum = 0;
   char indexEntry[MAX_INDEX_ENTRY_LEN];

   while (fgets (indexEntry, MAX_INDEX_ENTRY_LEN, oldFp) != NULL) {
      if (++currentLineNum == 3) {
         snprintf (indexEntry, sizeof (indexEntry),
               "#EXT-X-MEDIA-SEQUENCE:%d\n", mFileCounter - 1);

         fputs (indexEntry, newFp);
         continue;
      }

      if (currentLineNum == 5 || currentLineNum == 6) {
         /*
          * Those two lines are the index of first splitted
          * file and we need to skip it during the process
          * of updating live.m3u8.
          */
         continue;
      }

      fputs (indexEntry, newFp);
   }

   AppendEntry (newFp);

   /* Remove oldest splitted file */
   snprintf (indexEntry, sizeof (indexEntry), "%s_%u.ts",
         mpFileName, mFileCounter - SPLIT_FILE_NUM_IN_M3U8 - 1);

   if (remove (indexEntry) < 0) {
      NOTICE ("Failed to remove %s: %s",
            indexEntry,
            strerror (errno));
   }
}

void CTsHlsWriter::AppendEntry (FILE *fp)
{
   char indexEntry[MAX_INDEX_ENTRY_LEN];
   int splitSec;

   if (mSplitDuration % 90000L == 0) {
      splitSec = (int)(mSplitDuration / 90000L);
   } else {
      splitSec = (int)(mSplitDuration * 2997 / 270000000LL);
   }

   snprintf (indexEntry, sizeof (indexEntry),
         "#EXTINF:%d,\n", splitSec);
   fputs (indexEntry, fp);

   snprintf (indexEntry, sizeof (indexEntry),
         "http://%s/media/%s_%u.ts\n",
         mpHostIPAddress, mpBaseFileName, mFileCounter - 1);
   fputs (indexEntry, fp);
}

AM_ERR CTsHlsWriter::FetchHostIPAddress ()
{
   int i;
   FILE *fp;
   char buf[MAX_CHAR_IN_LINE];
   char cmd[] = "ifconfig | grep \"inet addr\"";
   char *addressBegin;

   /*
    * Temporary approach to fetch wlan0 ip address by using
    * ifconfig and grep command.
    */
   DEBUG ("cmd = %s\n", cmd);
   if ((fp = popen (cmd, "r")) == NULL) {
      ERROR ("Failed to create a pipe with hls writer.");
      return ME_ERROR;
   }

   while (fgets (buf, MAX_CHAR_IN_LINE, fp) != NULL) {
      INFO ("Contents read from pipe: %s", buf);
      for (i = 0; buf[i] != ':'; ++i);

      /* Check whether current address is loop address or not. */
      if (buf[i + 1] == '1' &&
          buf[i + 2] == '2'
          && buf[i + 3] == '7') {
         continue;
      }

      addressBegin = buf + i + 1;
      for (i = 0; addressBegin[i] != ' '; ++ i) {
         mpHostIPAddress[i] = addressBegin[i];
      }

      mpHostIPAddress[i] = '\0';
   }

   pclose (fp);

   if (mpHostIPAddress[0] == '\0') {
      ERROR ("Failed to get IP address of wlan0");
      return ME_ERROR;
   } else {
      NOTICE ("IP address of wlan0 is: %s", mpHostIPAddress);
      return ME_OK;
   }
}

/*
 * To avoid writing NAND repeatly, we need to create two
 * symbol link file:
 *
 * /webSvr/web/live.m3u8 --> real m3u8 index file
 * /webSvr/web/media     --> real directory containing ts segments
 */
AM_ERR CTsHlsWriter::CreateSymbolLinks ()
{
   int status;
   char cmd[MAX_INDEX_ENTRY_LEN];

   if (access (LIGHTTPD_WORK_DIR"/live.m3u8", F_OK) == 0) {
      status = system ("rm -rf "LIGHTTPD_WORK_DIR"/live.m3u8");
      if (WEXITSTATUS (status)) {
         ERROR ("Failed to remove %s: %s",
               LIGHTTPD_WORK_DIR"/live.m3u8",
               strerror (errno));
         return ME_ERROR;
      }
   }

   if (access (LIGHTTPD_WORK_DIR"/media", F_OK) == 0) {
      status = system ("rm -rf "LIGHTTPD_WORK_DIR"/media");
      if (WEXITSTATUS (status)) {
         ERROR ("Failed to remove %s: %s",
               LIGHTTPD_WORK_DIR"/media",
               strerror (errno));
         return ME_ERROR;
      }
   }

   snprintf (cmd, sizeof (cmd), "ln -s %s %s",
        mpM3u8FileName, LIGHTTPD_WORK_DIR"/live.m3u8");
   status = system (cmd);
   if (WEXITSTATUS (status)) {
      ERROR ("Failed to create an symbol %s --> %s: %s",
            LIGHTTPD_WORK_DIR"/live.m3u8",
            mpM3u8FileName,
            strerror (errno));
      return ME_ERROR;
   }

   snprintf (cmd, sizeof (cmd), "ln -s %s %s",
        mpDirectoryName, LIGHTTPD_WORK_DIR"/media");
   status = system (cmd);
   if (WEXITSTATUS (status)) {
      ERROR ("Failed to create an symbol %s --> %s: %s",
            LIGHTTPD_WORK_DIR"/media",
            mpDirectoryName,
            strerror (errno));
      return ME_ERROR;
   }

   return ME_OK;
}
