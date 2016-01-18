/*******************************************************************************
 * am_file.cpp
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

#include "am_include.h"
#include "utilities/am_log.h"
#include "utilities/am_define.h"
#include "utilities/am_file.h"

#include <dirent.h>

AmFile::AmFile(const char *file):
  mFd(-1),
  mIsOpen(false),
  mFileName(NULL)
{
  if (file) {
    mFileName = amstrdup(file);
  }
}

AmFile::~AmFile()
{
  delete[] mFileName;
  if (mFd >= 0) {
    ::close(mFd);
  }
}

bool AmFile::seek(long offset, AmFileSeekPos where)
{
  bool ret = false;
  if (AM_LIKELY(mFd >= 0)) {
    int whence = 0;
    switch(where) {
      case AmFile::AM_FILE_SEEK_SET:
        whence = SEEK_SET;
        break;
      case AmFile::AM_FILE_SEEK_CUR:
        whence = SEEK_CUR;
        break;
      case AmFile::AM_FILE_SEEK_END:
        whence = SEEK_END;
        break;
    }
    ret = (-1 != lseek(mFd, offset, whence));
    if (AM_UNLIKELY(!ret)) {
      PERROR("lseek");
    }
  } else {
    ERROR("%s is not open!", mFileName);
  }

  return ret;
}

bool AmFile::open(AmFileMode mode)
{
  if (AM_LIKELY(!mIsOpen)) {
    int flags = -1;
    switch(mode) {
      case AM_FILE_READONLY:
        flags = O_RDONLY;
        break;
      case AM_FILE_WRITEONLY:
        flags = O_WRONLY;
        break;
      case AM_FILE_READWRITE:
        flags = O_RDWR;
        break;
      case AM_FILE_CREATE:
        flags = O_WRONLY|O_CREAT;
        break;
      default:
        ERROR("Unknown file mode!");
        break;
    }
    if (flags != -1) {
      if (mFileName) {
        if (AM_UNLIKELY((mFd = ::open(mFileName, flags)) < 0)) {
          ERROR("Failed to open %s: %s", mFileName, strerror(errno));
          mIsOpen = false;
        } else {
          mIsOpen = true;
        }
      } else {
        ERROR("File name not set!");
      }
    }
  }

  return mIsOpen;
}

uint64_t AmFile::size()
{
  struct stat fileStat;
  uint64_t size = 0;
  if (AM_UNLIKELY(stat(mFileName, &fileStat) < 0)) {
    ERROR("%s stat error: %s", mFileName, strerror(errno));
  } else {
    size = fileStat.st_size;
  }

  return size;
}

ssize_t AmFile::write(void *data, uint32_t len)
{
  ssize_t ret = -1;
  if ((mFd >= 0) && data) {
    ret = ::write(mFd, data, len);
  } else if (mFd < 0) {
    ERROR("File not open!");
  } else {
    ERROR("NULL pointer specified!");
  }

  return ret;
}

ssize_t AmFile::read(char *buf, uint32_t len)
{
  ssize_t ret = -1;
  if ((mFd >= 0) && buf) {
    ret = ::read(mFd, buf, len);
    if ((ret < 0) && (errno == EINTR)) {
      ret = ::read(mFd, buf, len);
    } else if (ret < 0) {
      ERROR("%s read error: %s", mFileName, strerror(errno));
    }
  } else if (mFd < 0) {
    ERROR("File not open!");
  } else {
    ERROR("NULL pointer specified!");
  }

  return ret;
}

bool AmFile::exists()
{
  bool ret = false;

  if (mFileName) {
    ret = (0 == access(mFileName, F_OK));
  }
  return ret;
}

bool AmFile::exists(const char *file)
{
  bool ret = false;
  if (file) {
    ret = (0 == access(file, F_OK));
  }
  return ret;
}

bool AmFile::create_path(const char *path)
{
  if (path) {
    if (access(path, F_OK) == 0) {
      return true;
    } else {
      char parent[256] = {0};
      int ret = sprintf(parent, "%s", path);
      parent[ret] = '\0';
      char *end = strrchr(parent, (int)'/');
      if (end && (end != parent)) {
        *end = '\0';
        if (create_path(parent)) {
          if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
            return true;
          } else if (errno == EEXIST) {
            return true;
          } else {
            return false;
          }
        } else {
          return false;
        }
      } else {
        if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
          return true;
        } else if (errno == EEXIST) {
          return true;
        } else {
          return false;
        }
      }
    }
  } else {
    ERROR("Cannot create NULL path!");
  }

  return false;
}

static int filter(const struct dirent* dir)
{
  bool ret = false;
  if (AM_LIKELY(dir && ('.' != dir->d_name[0]))) {
    const char *ext = strstr(dir->d_name, ".");
    ret = (ext && is_str_n_equal(ext, ".so", strlen(".so")));
  }
  return ((int)ret);
}

int AmFile::list_files(const char *dir, char**& list)
{
  int number = 0;

  list = NULL;
  if (AM_LIKELY(dir)) {
    struct dirent **namelist = NULL;
    number = scandir(dir, &namelist, filter, alphasort);
    if (AM_LIKELY(number > 0)) {
      list = new char*[number];
      if (AM_LIKELY(list)) {
        memset(list, 0, sizeof(char*));
        for (int i = 0; i < number; ++ i) {
          int len = strlen(dir) + strlen(namelist[i]->d_name) + 1;
          list[i] = new char[len];
          memset(list[i], 0, len);
          sprintf(list[i], "%s/%s", dir, namelist[i]->d_name);
          free(namelist[i]);
        }
      }
      free(namelist);
    } else if (number < 0) {
      PERROR("scandir");
    }
  }

  return number;
}
