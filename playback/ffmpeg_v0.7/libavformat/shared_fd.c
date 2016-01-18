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
//#define LOG_NDEBUG 0
//#define ANDROID_DEBUG
//#define LOG_TAG "openedfile"

#include "libavutil/avstring.h"
#include "avformat.h"
#include <fcntl.h>
#if HAVE_SETMODE
#include <io.h>
#endif
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "os_support.h"


/* opend file handle in ,,standard file protocol */

typedef struct SFDContext
{
    int sfd;
    int64_t sfd_offset;
    int64_t sfd_position;
    int64_t sfd_size;
}SFDContext;

static int file_open(URLContext *h, const char *filename, int flags)
{
//FFLOGD("1");
    long long size,offset;
    int fd;
    int64_t len;
    SFDContext* s;

    if(sscanf(filename, "sharedfd://%d:%lld:%lld", &fd, &offset, &size) != 3)
    {
        return -1;
    }
//    FFLOGD("2");

    s = av_mallocz(sizeof(SFDContext));
    if(!s)
        return AVERROR(ENOMEM);
    h->priv_data = s;
    //set Shared fd context
    s->sfd = fd;
    s->sfd_offset = offset;
    s->sfd_position = 0;
    //get the file size
    len = lseek64(fd, 0, SEEK_END);
    lseek64(fd, 0, SEEK_SET);
    len -= offset;
    s->sfd_size = len < size ? len : size;

    return 0;
}

static int file_read(URLContext *h, const unsigned char *buf, int size)
{
//FFLOGD("3");

    int readsize;
    long long curpos;
    SFDContext* s = h->priv_data;
    lseek64(s->sfd, s->sfd_offset + s->sfd_position, SEEK_SET);
    if(s->sfd_position + size > s->sfd_size)
        size = s->sfd_size - s->sfd_position;
    readsize = read(s->sfd, (unsigned char *)buf, size);
    //restore position
    curpos = lseek64(s->sfd, 0, SEEK_CUR);
    if(curpos >= 0)
        s->sfd_position = curpos - s->sfd_offset;
    return readsize;
}

/*XXX:CC: may be wrong, but a shared fd should need write func?*/
static int file_write(URLContext *h, const unsigned char *buf, int size)
{
    int writesize;
    SFDContext* s = h->priv_data;
    lseek64(s->sfd, s->sfd_offset + s->sfd_position, SEEK_SET);
    writesize = write(s->sfd, (unsigned char *)buf, size);
    s->sfd_position += writesize;
    s->sfd_size += writesize;
    return writesize;
}

/* XXX: use lseek, this will return the file size, for ic->file_size */
//cc: no seek, change the shared file's positon. it will affect the read func.
static int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
    SFDContext* s = h->priv_data;
    int64_t newpos = s->sfd_position;
    if (whence == SEEK_CUR) newpos = newpos + pos;
    else if (whence == SEEK_SET) newpos = pos;
    else if (whence == SEEK_END) newpos = s->sfd_size - pos;
    else if (whence == AVSEEK_SIZE) return s->sfd_size;
    if (newpos > s->sfd_size)
        newpos = s->sfd_size;
    s->sfd_position = newpos;
    return 0;
    //return lseek64(fd, pos, whence);
}

/*CC: I don't think we need to close a shared fd!*/
static int file_close(URLContext *h)
{
    SFDContext* s = h->priv_data;
    av_free(s);
    //return close(fd);
    return 0;
}

static int file_get_handle(URLContext *h)
{
    SFDContext* s = h->priv_data;
    return s->sfd;
}

URLProtocol ff_shared_fd_protocol = {
    .name                   = "sharedfd",
    .url_open               = file_open,
    .url_read               = file_read,
    .url_write              = file_write,
    .url_seek               = file_seek,
    .url_close              = file_close,
    .url_get_file_handle    = file_get_handle,
};

