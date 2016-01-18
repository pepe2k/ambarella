/**
 * file_sink.cpp
 *
 * History:
 *    2012/02/14 - [Jay Zhang] created file
 *    2012/09/19 - [Hanbo Xiao] modified file
 *
 * Copyright (C) 2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "am_types.h"
#include "am_new.h"
#include "osal.h"
#include "am_queue.h"
#include "am_if.h"
#include "am_mw.h"
#include "am_base.h"
#include "file_sink.h"

#define WRITE_STATISTICS_THRESHHOLD 100
//-----------------------------------------------------------------------
//
// CFileReader
//
//-----------------------------------------------------------------------
CFileReader *CFileReader::Create()
{
   CFileReader *result = new CFileReader;
   if (result && result->Construct() != ME_OK) {
      delete result;
      result = NULL;
   }
   return result;
}

AM_ERR CFileReader::Construct()
{
  return ((mpMutex = CMutex::Create(false)) == NULL) ? ME_ERROR : ME_OK;
}

CFileReader::~CFileReader()
{
   __CloseFile();
   AM_DELETE(mpMutex);
}

void *CFileReader::GetInterface(AM_REFIID refiid)
{
   if (refiid == IID_IFileReader)
      return (IFileReader*)this;
   return inherited::GetInterface(refiid);
}

AM_ERR CFileReader::OpenFile(const char *pFileName)
{
   AUTO_LOCK(mpMutex);

   __CloseFile();

   mpFile = ::fopen(pFileName, "rb");
   if (mpFile == NULL) {
      AM_PERROR(pFileName);
      return ME_IO_ERROR;
   }

   return ME_OK;
}

AM_ERR CFileReader::CloseFile()
{
   AUTO_LOCK(mpMutex);
   __CloseFile();
   return ME_OK;
}

void CFileReader::__CloseFile()
{
   if (mpFile) {
      ::fclose((FILE*)mpFile);
      mpFile = NULL;
   }
}

am_file_off_t CFileReader::GetFileSize()
{
   AUTO_LOCK(mpMutex);

   if (::fseeko((FILE*)mpFile, 0, SEEK_END) < 0) {
      AM_PERROR("fseeko");
      return 0;
   }

   unsigned long long size = ::ftello((FILE*)mpFile);
   return size;
}

am_file_off_t CFileReader::TellFile(void)
{
   AUTO_LOCK(mpMutex);

   unsigned long long size = ::ftello((FILE*)mpFile);
   NOTICE ("size = %lld", size);
   return size;
}

AM_ERR CFileReader::SeekFile(am_file_off_t offset)
{
   AUTO_LOCK(mpMutex);

   if (::fseeko((FILE*)mpFile, offset, SEEK_SET) < 0) {
      AM_PERROR("fseeko");
      return ME_IO_ERROR;
   }
   return ME_OK;
}

AM_ERR CFileReader::AdvanceFile(am_file_off_t offset)
{
   AUTO_LOCK(mpMutex);

   if (::fseeko((FILE*)mpFile, offset, SEEK_CUR) < 0) {
      AM_PERROR("fseeko");
      return ME_IO_ERROR;
   }
   return ME_OK;
}

int CFileReader::ReadFile(void *pBuffer, AM_UINT size)
{
   int readSize;
   AUTO_LOCK(mpMutex);

   if ((AM_UINT)(readSize = ::fread(pBuffer, 1, size, (FILE*)mpFile)) != size) {
      if (feof((FILE*)mpFile)) {
         AM_NOTICE("Reached end-of-file!");
      } else if (ferror((FILE*)mpFile)) {
         AM_PERROR("fread");
         return -1;
      }
   }

   return readSize;
}

//-----------------------------------------------------------------------
//
// CFileWriter
//
//-----------------------------------------------------------------------
CFileWriter* CFileWriter::Create()
{
   CFileWriter *result = new CFileWriter;
   if (result && result->Construct() != ME_OK) {
      delete result;
      result = NULL;
   }
   return result;
}

AM_ERR CFileWriter::Construct()
{
   return ME_OK;
}

CFileWriter::~CFileWriter()
{
   if (!(mFd < 0)) {
      __CloseFile();
   }

   delete[] mpCustomBuf;
}

AM_ERR CFileWriter::CreateFile(const char *pFileName)
{
   if (!(mFd < 0)) {
      __CloseFile();
   }

   if (pFileName == NULL) {
      AM_ERROR ("File name is empty\n");
      return ME_ERROR;
   }

   if ((mFd = ::open (pFileName, O_WRONLY | O_CREAT | O_TRUNC,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
      AM_ERROR ("Can't open file %s: %s\n", pFileName, strerror (errno));
      return ME_ERROR;
   }
   INFO("File %s is created successfully!", pFileName);

   mDataSize = 0;
   mpDataWriteAddr = mpCustomBuf;
   mpDataSendAddr = mpCustomBuf;

   return ME_OK;
}

AM_ERR CFileWriter::CloseFile()
{
   return (mFd >= 0) ? __CloseFile() : ME_OK;
}

AM_ERR CFileWriter::WriteFile (const void *buf, AM_UINT len)
{
   AM_UINT remainData = len;
   AM_U8*  dataAddr   = (AM_U8 *)buf;

   if (AM_UNLIKELY(mFd < 0)) {
      AM_ERROR ("File is not open.\n");
      return ME_ERROR;
   }

   if (len == 0) {
      return ME_OK;
   }

   if (NULL == mpCustomBuf) { // No IO Buffer
      int ret = -1;
      while (remainData > 0) {
        if ((ret = Write(dataAddr, remainData)) > 0) {
          remainData -= ret;
          dataAddr += ret;
        } else {
          AM_PERROR("Write file error");
          break;
        }
      }
   } else { // Use IO Buffer
      AM_UINT bufferSize = 0;
      AM_UINT dataSize   = 0;
      while (remainData > 0) {
         bufferSize = mBufSize + mpCustomBuf - mpDataWriteAddr;
         dataSize = (remainData <= bufferSize ? remainData : bufferSize);

         memcpy(mpDataWriteAddr, dataAddr, dataSize);
         dataAddr      += dataSize;
         mDataSize += dataSize;
         remainData    -= dataSize;
         mpDataWriteAddr += dataSize;
         if ((AM_U32)(mpDataWriteAddr - mpCustomBuf) >= mBufSize) {
            mpDataWriteAddr = mpCustomBuf;
         }
         while (mDataSize >= IO_TRANSFER_BLOCK_SIZE) {
            AM_INT writeRet = Write(mpDataSendAddr, IO_TRANSFER_BLOCK_SIZE);
            if (AM_LIKELY(writeRet == IO_TRANSFER_BLOCK_SIZE)) {
               mDataSize -= IO_TRANSFER_BLOCK_SIZE;
               mpDataSendAddr += IO_TRANSFER_BLOCK_SIZE;
               if ((AM_U32)(mpDataSendAddr - mpCustomBuf) >= mBufSize) {
                  mpDataSendAddr = mpCustomBuf;
               }
            } else {
               AM_ERROR("Write file error: %d!", writeRet);
               return ME_ERROR;
            }
         }
      }
   }

   return ((remainData == 0) ? ME_OK : ME_ERROR);
}

AM_ERR CFileWriter::FlushFile ()
{
   AM_ERR err = ME_ERROR;
   if ((mDataSize > 0) && mpDataSendAddr && mpCustomBuf) {
      if (((mDataSize % 512) != 0) && mIODirectSet && mEnableDirectIO) {
         mIODirectSet = !(ME_OK == SetFileFlag((int)O_DIRECT, false));
      }
      while (mDataSize > 0) {
         AM_UINT dataRemain = mDataSize;
         AM_INT ret = -1;
         if (mpDataWriteAddr > mpDataSendAddr) {
            dataRemain = (mpDataWriteAddr - mpDataSendAddr); //mDataSize
         } else if (mpDataWriteAddr < mpDataSendAddr) {
            dataRemain = (mpCustomBuf + mBufSize - mpDataSendAddr);
         } else {
            break;
         }
         if (AM_UNLIKELY((ret = Write(mpDataSendAddr, dataRemain)) !=
                  (AM_INT)mDataSize)) {
            err = ME_ERROR;
            AM_PERROR("Write file error");
         } else {
            mDataSize -= dataRemain;
            mpDataSendAddr += dataRemain;
            if ((AM_U32)(mpDataSendAddr - mpCustomBuf) >= mBufSize) {
               mpDataSendAddr = mpCustomBuf;
            }
            err = ME_OK;
         }
      }
      if (mEnableDirectIO &&
            mpCustomBuf &&
            ((mBufSize % 512) == 0) &&
            !mIODirectSet) {
         mIODirectSet = (ME_OK == SetFileFlag((int)O_DIRECT, true));
      }
   } else {
      err = ME_OK;
   }
   mDataSize = 0;
   mpDataWriteAddr = mpCustomBuf;
   mpDataSendAddr  = mpCustomBuf;

   return err;
}

AM_ERR CFileWriter::Setbuf (AM_UINT size)
{
   if (AM_UNLIKELY(mFd < 0)) {
      ERROR ("Bad file descriptor!");
      return ME_ERROR;
   }

   if (mpCustomBuf && ((0 == size) || (size != mBufSize))) {
      delete[] mpCustomBuf;
      mpCustomBuf = NULL;
      mpDataWriteAddr = NULL;
      mpDataSendAddr = NULL;
      mDataSize = 0;
      mBufSize = 0;
   }

   if (size == 0) {
      return ME_OK;
   }

   if (AM_LIKELY(!mpCustomBuf && (size > 0) &&
                 ((size % IO_TRANSFER_BLOCK_SIZE) == 0))) {
      DEBUG ("File write buffer is set to %d KB\n", size/1024);
      mpCustomBuf = new AM_U8[size];
      mpDataWriteAddr = mpCustomBuf;
      mpDataSendAddr = mpCustomBuf;
      mDataSize = 0;
      mBufSize = (mpCustomBuf ? size : 0);
   } else if ((size % IO_TRANSFER_BLOCK_SIZE) != 0) {
      ERROR("Buffer size should be multiples of %uKB!",
            (IO_TRANSFER_BLOCK_SIZE / IO_SIZE_KB));
   }

   if (mEnableDirectIO && mpCustomBuf &&
         ((size % 512) == 0) && !mIODirectSet) {
      mIODirectSet = (ME_OK == SetFileFlag(((int)O_DIRECT), true));
   }

   return (mpCustomBuf ? ME_OK : ME_ERROR);
}

AM_ERR CFileWriter::SetFileFlag (int flag, bool enable)
{
   int fileFlags = 0;
   if (mFd < 0) {
      return ME_ERROR;
   }
   if (flag & ~(O_APPEND | O_ASYNC | O_NONBLOCK | O_DIRECT | O_NOATIME)) {
      AM_ERROR("Unsupported file flag!");
      return ME_ERROR;
   }
   fileFlags = fcntl(mFd, F_GETFL);
   if (enable) {
      fileFlags |= flag;
   } else {
      fileFlags &= ~flag;
   }
   if (fcntl(mFd, F_SETFL, fileFlags) < 0) {
      AM_ERROR("Failed to %s file flags!\n", (enable ? "enable" : "disable"));
      return ME_ERROR;
   }

   return ME_OK;
}

ssize_t CFileWriter::Write(const void *buf, size_t nbyte)
{
  size_t    remainSize = nbyte;
  ssize_t       retval = 0;
  AM_UINT     writeCnt = 0;
  struct timeval start = {0, 0};
  struct timeval   end = {0, 0};

  if (AM_UNLIKELY(gettimeofday(&start, NULL) < 0)) {
    PERROR("gettimeofday");
  }

  do {
    retval = ::write(mFd, buf, remainSize);
    if (AM_LIKELY(retval > 0)) {
      remainSize -= retval;
    } else if (AM_UNLIKELY((errno != EAGAIN) && (errno != EINTR))) {
      PERROR("write");
      break;
    }
  } while ((++ writeCnt < 5) && (remainSize > 0));

  ++ mWriteCnt;
  mWriteDataSize += (nbyte - remainSize);

  if (AM_LIKELY(remainSize < nbyte)) {
    if (AM_UNLIKELY(gettimeofday(&end, NULL) < 0)) {
      PERROR("gettimeofday");
    } else {
      AM_U64 timediff = ((end.tv_sec * 1000000 + end.tv_usec) -
          (start.tv_sec * 1000000 + start.tv_usec));
      mWriteTime += timediff;

      if (AM_UNLIKELY(timediff >= 200000)) {
        STAT("Write %u bytes data, takes %llu ms",
             (nbyte - remainSize), (timediff / 1000));
      }

      if (AM_UNLIKELY(mWriteCnt >= WRITE_STATISTICS_THRESHHOLD)) {
        mWriteAvgSpeed = mWriteDataSize * 1000000 / mWriteTime / 1024;
        mHistMinSpeed  = (mWriteAvgSpeed < mHistMinSpeed)  ?
            mWriteAvgSpeed : mHistMinSpeed;
        mHistMaxSpeed  = (mWriteAvgSpeed > mHistMaxSpeed)  ?
            mWriteAvgSpeed : mHistMaxSpeed;
        STAT("\nWrite %u times, total data %llu bytes, takes %llu ms"
             "\n       Average speed %llu KB/sec"
             "\nHistorical min speed %llu KB/sec"
             "\nHistorical max speed %llu KB/sec",
             WRITE_STATISTICS_THRESHHOLD,
             mWriteDataSize, mWriteTime / 1000,
             mWriteAvgSpeed, mHistMinSpeed, mHistMaxSpeed);
        mWriteDataSize = 0;
        mWriteCnt = 0;
        mWriteTime = 0;
      }
    }
  }

  return (retval < 0) ? retval : (nbyte - remainSize);
}

AM_ERR CFileWriter::SeekFile(am_file_off_t offset, AM_UINT whence)
{
   if (AM_UNLIKELY(mFd < 0)) {
      AM_ERROR ("Bad file descriptor!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(FlushFile () != ME_OK)) {
      AM_ERROR ("Failed to sync file!");
      return ME_ERROR;
   }

   if (AM_UNLIKELY(::lseek (mFd, offset, whence) < 0)) {
      AM_PERROR ("Failed to seek");
      return ME_IO_ERROR;
   }

   return ME_OK;
}

AM_ERR CFileWriter::__CloseFile()
{
   if (AM_UNLIKELY(mFd < 0)) {
      AM_ERROR ("Bad file descriptor@");
      return ME_BAD_PARAM;
   }

   FlushFile ();
   if (AM_UNLIKELY(::close (mFd) != 0)) {
      mFd = -1;
      switch (errno) {
      case EBADF: return ME_BAD_PARAM;
      case EIO:   return ME_IO_ERROR;
      case EINTR: return ME_BUSY;
      default:   return ME_ERROR;
      }
   }

   mFd = -1;
   mIODirectSet = false;

   return ME_OK;
}

