/*******************************************************************************
 * am_file.h
 *
 * Histroy:
 *  2012-3-7 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMFILE_H_
#define AMFILE_H_

class AmFile
{
  public:
    enum AmFileMode {
      AM_FILE_CREATE,
      AM_FILE_WRITEONLY,
      AM_FILE_READONLY,
      AM_FILE_READWRITE
    };
    enum AmFileSeekPos {
      AM_FILE_SEEK_SET,
      AM_FILE_SEEK_CUR,
      AM_FILE_SEEK_END
    };
  public:
    AmFile(const char *file);
    AmFile() :
      mFd(-1),
      mIsOpen(false),
      mFileName(NULL){}
    virtual ~AmFile();

  public:
    const char* name()
    {
      return (const char*)mFileName;
    }
    bool is_open()
    {
      return mIsOpen;
    }
    void set_file_name(const char *file)
    {
      delete[] mFileName;
      mFileName = (file ? amstrdup(file) : NULL);
    }
    bool seek(long offset, AmFileSeekPos where);
    bool open(AmFileMode mode);
    uint64_t size();
    void close()
    {
      if (mFd >= 0) {
        ::close(mFd);
      }
      mFd = -1;
      mIsOpen = false;
    }
    int handle() {return mFd;}
    ssize_t write(void *data, uint32_t len);
    ssize_t read(char *buf, uint32_t len);
    bool exists();
    static bool exists(const char *file);
    static bool create_path(const char *path);
    static int list_files(const char *dir, char**& list);

  private:
    int   mFd;
    bool  mIsOpen;
    char *mFileName;
};


#endif /* AMFILE_H_ */
