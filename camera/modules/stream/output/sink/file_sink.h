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
#ifndef __FILE_SINK_H__
#define __FILE_SINK_H__

extern const AM_IID IID_IFileReader;
extern const AM_IID IID_IFileWriter;

#ifndef IO_SIZE_KB
#define IO_SIZE_KB (1024)
#endif

#ifndef IO_TRANSFER_BLOCK_SIZE
#define IO_TRANSFER_BLOCK_SIZE (512 * IO_SIZE_KB)
#endif

#ifndef MAX_FILE_NAME_LEN
#define MAX_FILE_NAME_LEN (256)
#endif

//-----------------------------------------------------------------------
//
// IFileReader
//
//-----------------------------------------------------------------------
class IFileReader: public IInterface
{
public:
   DECLARE_INTERFACE(IFileReader, IID_IFileReader);
   virtual AM_ERR OpenFile(const char *pFileName) = 0;
   virtual AM_ERR CloseFile() = 0;
   virtual am_file_off_t GetFileSize() = 0;
   virtual am_file_off_t TellFile() = 0;
   virtual AM_ERR SeekFile(am_file_off_t offset) = 0;
   virtual AM_ERR AdvanceFile(am_file_off_t offset) = 0;
   virtual int ReadFile(void *pBuffer, AM_UINT size) = 0;
};


//-----------------------------------------------------------------------
//
// CFileReader
//
//-----------------------------------------------------------------------
class CFileReader: public CObject, public IFileReader
{
   typedef CObject inherited;

public:
   static CFileReader *Create();

private:
   CFileReader(): mpMutex(NULL), mpFile(NULL) {}
   AM_ERR Construct();
   virtual ~CFileReader();

public:
   // IInterface
   virtual void *GetInterface(AM_REFIID refiid);
   virtual void Delete() { inherited::Delete(); }

   // IFileReader
   virtual AM_ERR OpenFile(const char *pFileName);
   virtual AM_ERR CloseFile();
   virtual am_file_off_t GetFileSize();
   virtual am_file_off_t TellFile();
   virtual AM_ERR SeekFile(am_file_off_t offset);
   virtual AM_ERR AdvanceFile(am_file_off_t offset);
   virtual int ReadFile(void *pBuffer, AM_UINT size);

private:
   void __CloseFile();

private:
   CMutex *mpMutex;
   void *mpFile;
};

//-----------------------------------------------------------------------
//
// IFileWriter
//
//-----------------------------------------------------------------------
class IFileWriter: public IInterface
{
public:
   DECLARE_INTERFACE(IFileWriter, IID_IFileWriter);
   virtual AM_ERR CreateFile(const char *pFileName) = 0;
   virtual AM_ERR CloseFile() = 0;
   virtual AM_ERR FlushFile () = 0;
   virtual AM_ERR Setbuf (AM_UINT) = 0;
   virtual AM_ERR SeekFile(am_file_off_t offset, AM_UINT whence) = 0;
   virtual AM_ERR WriteFile(const void *pBuffer, AM_UINT size) = 0;
};

//-----------------------------------------------------------------------
//
// CFileWriter
//
//-----------------------------------------------------------------------
class CFileWriter: public CObject, public IFileWriter
{
   typedef CObject inherited;

public:
   static CFileWriter* Create();

public:
   CFileWriter():
     mFd(-1),
     mBufSize(0),
     mDataSize(0),
     mWriteCnt(0),
     mWriteDataSize(0),
     mWriteAvgSpeed(0),
     mHistMinSpeed(0xFFFFFFFF),
     mHistMaxSpeed(0),
     mWriteTime(0),
     mpCustomBuf(NULL),
     mpDataWriteAddr (NULL),
     mpDataSendAddr (NULL),
     mIODirectSet (false),
     mEnableDirectIO (false)
   {
   }

   AM_ERR Construct();
   virtual ~CFileWriter();

public:
   // IInterface
   virtual void *GetInterface(AM_REFIID refiid)
   {
      if (refiid == IID_IFileWriter)
         return (IFileWriter*)this;
      return inherited::GetInterface(refiid);
   }
   virtual void Delete() { inherited::Delete(); }

   // IFileWriter
   virtual AM_ERR CreateFile(const char *pFileName);
   virtual AM_ERR CloseFile();
   virtual AM_ERR FlushFile ();
   virtual AM_ERR Setbuf (AM_UINT);
   virtual AM_ERR SeekFile(am_file_off_t offset, AM_UINT whence = SEEK_SET);
   virtual AM_ERR WriteFile(const void *pBuffer, AM_UINT size);
   virtual void SetEnableDirectIO (bool enable) { mEnableDirectIO = enable; }

private:
   AM_ERR __CloseFile();
   AM_ERR SetFileFlag (int, bool);
   ssize_t Write(const void *buf, size_t nbyte);

private:
   int mFd;
   AM_UINT mBufSize;
   AM_UINT mDataSize;
   AM_UINT mWriteCnt;
   AM_U64  mWriteDataSize;
   AM_U64  mWriteAvgSpeed;
   AM_U64  mHistMinSpeed;
   AM_U64  mHistMaxSpeed;
   AM_U64  mWriteTime;

   AM_U8 *mpCustomBuf;
   AM_U8 *mpDataWriteAddr;
   AM_U8 *mpDataSendAddr;

   bool mIODirectSet;
   bool mEnableDirectIO;
};

#endif

