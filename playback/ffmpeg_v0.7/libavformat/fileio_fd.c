/*
 * Buffered file io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/avstring.h"
#include "avformat.h"
//#include <fcntl.h>
#if HAVE_SETMODE
//#include <io.h>
#endif
//#include <unistd.h>
//#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
//#include "os_support.h"


/* opend file handle in ,,standard file protocol */

static int file_open(URLContext *h, const char *filename, int flags)
{
    long long size,offset;
    int fd;

    sscanf(filename, "fileio://%d:%lld:%lld", &fd, &offset, &size);

    h->priv_data = (FILE*) fd;
    return 0;
}

static int file_read(URLContext *h, const unsigned char *buf, int size)
{
    FILE* fd = (FILE*) h->priv_data;
    return fread((unsigned char *)buf,1,size,fd);
}

static int file_write(URLContext *h, const unsigned char *buf, int size)
{
    FILE* fd = (FILE*) h->priv_data;
    return fwrite((unsigned char *)buf,1,size,fd);
}

/* XXX: use llseek */
static int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
    FILE* fd = (FILE*) h->priv_data;
    return fseek(fd, pos, whence);
}

static int file_close(URLContext *h)
{
    FILE* fd = (FILE*) h->priv_data;
    return fclose(fd);
}

static int file_get_handle(URLContext *h)
{
    return (int) h->priv_data;
}

URLProtocol ff_fileio_fd_protocol = {
    .name                   = "fileio",
    .url_open               = file_open,
    .url_read               = file_read,
    .url_write              = file_write,
    .url_seek               = file_seek,
    .url_close              = file_close,
    .url_get_file_handle    = file_get_handle,
};
