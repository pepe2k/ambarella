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
#if AMF_RECORD_DIRECTIO

/* For large-file support */
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
/* For O_DIRECT */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fs.h>

#include "libavutil/avstring.h"
#include "avformat.h"
#include <fcntl.h>
#if HAVE_SETMODE
#include <io.h>
#endif
#include "os_support.h"
#include "url.h"


#define WRITE_SYNC_THRESHOLD  (1024 * 1024)
#define READ_SYNC_THRESHOLD (128 * 1024)

 typedef struct _node_t {
    int64_t write_pos;
    unsigned char *buf;
    int len;
    unsigned char *buf_base;
    struct _node_t *next;
}node_t;

typedef struct _write_queue_t {
    node_t *head;
    node_t *tail;
    pthread_mutex_t q_h_lock;
    pthread_mutex_t q_t_lock;

    volatile int task_exit_flag;
    volatile int last_errno;
    pthread_t  thread;
}write_queue_t;

typedef struct _FileContext{
    int fd;
    int f_bsize;
    int64_t seek_pos;

    int read_direct;
    unsigned char *read_buf;
    unsigned char *read_buf_base;
    int64_t file_size;
    int64_t read_pos;
    int64_t r_pos;
    int rcount;

    int write_direct;
    int64_t file_real_len;
    int64_t write_pos;
    write_queue_t queue;
    URLContext *h;
    char *wtmp_buf;
    char *wbuf;
    char *wbuf_base;
    int wcount;
} FileContext;

static int alloc_wbuf(FileContext *s,int size){
    s->wbuf_base = (char *)av_malloc(size + s->f_bsize);
    if(!s->wbuf_base){
        return -1;
    }
    s->wbuf = (char*) (((int)s->wbuf_base + s->f_bsize -1) & (~(s->f_bsize -1)));
    s->wcount = 0;
    return 0;
}

static void *write_data_task( void * arg);
static int write_queue_init(FileContext *s)
{
    write_queue_t *q = &s->queue;
    node_t *node = (node_t*)av_mallocz(sizeof(node_t));
    if(!node){
        return -1;
    }
    node->write_pos = -1;
    node->next = NULL;
    q->head = q->tail = node;
    pthread_mutex_init(&q->q_h_lock,NULL);
    pthread_mutex_init(&q->q_t_lock,NULL);

    //spawn write task
    q->task_exit_flag = 0;
    q->last_errno = 0;
    {
        pthread_attr_t attr;
        struct sched_param param;
        pthread_attr_init(&attr);
        pthread_attr_setschedpolicy(&attr, SCHED_RR);
        param.sched_priority = 80;
        pthread_attr_setschedparam(&attr, &param);
        if (pthread_create(&q->thread, &attr, write_data_task, (void*)s)) {
            pthread_attr_destroy(&attr);
            return -1;
        }
        pthread_attr_destroy(&attr);
    }
    return 0;
}

static int write_queue_deinit(write_queue_t * q){
    q->task_exit_flag = 1;
    pthread_join(q->thread,NULL);
    if(q->head)av_free(q->head);
    pthread_mutex_destroy(&q->q_h_lock);
    pthread_mutex_destroy(&q->q_t_lock);
    return 0;
}

static int write_queue_enqueue(write_queue_t * q,unsigned char *buf,unsigned char *buf_base,int size, int64_t write_pos)
{
    node_t *node;
    if(q->last_errno){
        return -1;
    }
    node = (node_t*)av_mallocz(sizeof(node_t));
    if(!node){
        return -1;
    }
    //av_log(NULL, AV_LOG_WARNING, "write_queue_enqueue  --- size = %d,pos = %lld\n",size,seek_pos);

    node->buf_base = buf_base;
    node->buf  = buf;
    node->len = size;
    node->write_pos = write_pos;

   pthread_mutex_lock(&q->q_t_lock);
   q->tail->next = node;
   q->tail = node;
   pthread_mutex_unlock(&q->q_t_lock);
   return 0;
}
static int  write_file(FileContext *s,node_t *node)
{
    //android does not support pread64()/pwrite64() now,
    //    use lseek64()/read() and lseek64()/write().
    //Read-Modify-Write
    int64_t  file_pos = node->write_pos & (~(s->f_bsize - 1));
    int align_offset = (int)(node->write_pos - file_pos);
    if(align_offset && (align_offset + node->len <= s->f_bsize)){
        memcpy(s->wtmp_buf,node->buf + align_offset,node->len);
        lseek64(s->fd,file_pos,SEEK_SET);
        read(s->fd,node->buf,s->f_bsize);
        memcpy(node->buf + align_offset,s->wtmp_buf,node->len);
        node->len += align_offset;
    }else {
        if(align_offset){
            memcpy(s->wtmp_buf,node->buf + align_offset,s->f_bsize - align_offset);
            lseek64(s->fd,file_pos,SEEK_SET);
            read(s->fd,node->buf,s->f_bsize);
            memcpy(node->buf + align_offset,s->wtmp_buf,s->f_bsize - align_offset);
            node->len += align_offset;
        }
        align_offset = node->len  & (s->f_bsize -1);
        if(align_offset && (node->len + file_pos < s->file_real_len)){
            memcpy(s->wtmp_buf,node->buf + node->len  - align_offset,align_offset);
            lseek64(s->fd,file_pos +node->len - align_offset,SEEK_SET);
            read(s->fd,node->buf + node->len - align_offset,s->f_bsize);
            memcpy(node->buf + node->len  - align_offset,s->wtmp_buf,align_offset);
        }
    }
    s->file_real_len = (s->file_real_len < file_pos + node->len) ? (file_pos + node->len):(s->file_real_len);
    lseek64(s->fd,file_pos,SEEK_SET);
    if(write(s->fd,node->buf,(node->len + s->f_bsize -1) &(~(s->f_bsize -1))) < 0){
        av_log(s->h, AV_LOG_WARNING, "write_file error   --- count = %d, pos %lld,errno = %d\n",node->len,file_pos,errno);
        return errno;
    }
    //av_log(s->h, AV_LOG_WARNING, "write_queue_dequeue_write  --- pos = %lld\n",file_pos);
    return 0;
}
static int write_queue_dequeue_write(write_queue_t * q, URLContext *h)
{
     node_t *node,*new_head;
     pthread_mutex_lock(&q->q_h_lock);
     node = q->head;
     new_head = node->next;
     if(new_head == NULL){
         pthread_mutex_unlock(&q->q_h_lock);
         return 0;
     }
     if(new_head->buf){
         if(h){
             FileContext *s = (FileContext*)h->priv_data;
             q->last_errno = write_file(s,new_head);
         }
         av_free(new_head->buf_base);
     }
     new_head->buf_base = NULL;
     new_head->buf = NULL;
     new_head->len = 0;
     new_head->write_pos = -1;
     q->head = new_head;
     pthread_mutex_unlock(&q->q_h_lock);
     av_free(node);
     return 1;
}

static inline void my_usleep(int usec){
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = usec;
    select(0, NULL, NULL, NULL, &timeout);
}

static int getFileName(int fd, char *filename,int size)
{
    char proc_str[256];
    snprintf(proc_str,sizeof(proc_str),"/proc/%d/fd/%d",getpid(),fd);
    int bytes = readlink(proc_str,filename, size - 1);
    if(bytes < 0){
        return bytes;
    }
    filename[bytes] = '\0';
    return bytes;
}

static void *write_data_task( void * arg)
{
    FileContext *s = (FileContext*)arg;
    write_queue_t *q = &s->queue;
    int fd = s->fd;

    const int filename_size = 4096;
    char *filename = av_malloc(sizeof(char)* filename_size);
    if(!filename){
        return (void*)NULL;
    }
    if(getFileName(fd,filename,filename_size) > 0){
        av_log(s->h, AV_LOG_WARNING, "write_data_task start, filename[%s]\n",filename);
    }else{
        av_log(s->h, AV_LOG_WARNING, "write_data_task start, fd[%d]\n",fd);
    }

    s->file_real_len = 0;
    for(;;) {
        if(q->task_exit_flag){
            break;
        }
        while(write_queue_dequeue_write(q,s->h));
        my_usleep(1000); //give up cpu
    }
    while(write_queue_dequeue_write(q,s->h));
    close(fd);
    av_log(s->h, AV_LOG_WARNING, "write_data_task exit, file_size = %lld\n",s->file_real_len);
    //android does not support truncate64 now
    //    how to handle it?
    truncate(filename,s->file_real_len);
    av_free(filename);
    return (void*)NULL;
}

/* standard file protocol */
static int get_read_size(FileContext *s,int align,int size_to_be_read){
    int64_t max_read_size;
    int read_size;

    max_read_size = s->file_size - s->read_pos;
    if(max_read_size > size_to_be_read){
        max_read_size = size_to_be_read;
    }
    read_size = (int)max_read_size;
    if(read_size + align > READ_SYNC_THRESHOLD){
        read_size = READ_SYNC_THRESHOLD - align;
    }
    return read_size;
}
static int file_read(URLContext *h, const unsigned char *buf, int size)
{
    FileContext *s = (FileContext*)h->priv_data;
    int fd = s->fd;

    if(s->read_direct){
        const char *buf_ptr = buf;
        int size_to_be_read = size;
        int size_read = 0;

        if(s->seek_pos != -1){
            s->read_pos = s->seek_pos;
            av_log(s->h, AV_LOG_WARNING, "file_read,  seekTo %lld, size = %d\n",s->read_pos,size);
            s->seek_pos = -1;
            //s->r_pos = -1;
        }
        //av_log(s->h, AV_LOG_WARNING, "file_read,  seekTo %lld, size = %d\n",s->read_pos,size);

        while(size_to_be_read){
            //use the cached data first
            if((s->r_pos != -1) && (s->rcount) && (s->read_pos >= s->r_pos && s->read_pos < s->r_pos + s->rcount)){
                int cached_len = s->r_pos + s->rcount - s->read_pos;
                if(cached_len > size_to_be_read){
                    cached_len = size_to_be_read;
                }
                memcpy(buf_ptr,s->read_buf + (s->read_pos - s->r_pos),cached_len);
                size_read += cached_len;
                buf_ptr += cached_len;
                size_to_be_read -= cached_len;
                s->read_pos += cached_len;
            }
            if(!size_to_be_read)
                break;

            int align = s->read_pos & (s->f_bsize -1);
            int read_size = get_read_size(s,align,READ_SYNC_THRESHOLD);
            if(!read_size){
                break;
            }
            int align_read_size = (read_size + align + s->f_bsize -1) & (~(s->f_bsize -1));
            //android does not support  pread64 now.
            //   use  lseek64()/read
            lseek64(fd,s->read_pos - align,SEEK_SET);
            int bytes_read = read(fd,s->read_buf,align_read_size);
            if(bytes_read < 0){
                return -1;
            }else if(bytes_read == 0){
                break;//EOF
            }
            //usleep(1000);//give up cpu

            s->r_pos  = s->read_pos - align;
            s->rcount = bytes_read;

            if(bytes_read - align > size_to_be_read){
                read_size = size_to_be_read;
            }
            s->read_pos += read_size;
            memcpy(buf_ptr,s->read_buf + align,read_size);
            buf_ptr+= read_size;
            size_to_be_read -= read_size;
            size_read += read_size;
        }
        return size_read;
    }
    return read(fd, (unsigned char *)buf, size);
}

static int file_write(URLContext *h, const unsigned char *buf, int size)
{
    FileContext *s = (FileContext*)h->priv_data;
    int fd = s->fd;
    if(s->write_direct){
        unsigned char *buf_ptr = (unsigned char *)buf;
        int remain_size = size;
        int wsize,woffset;
        int max_size;
        int wbuf_size = WRITE_SYNC_THRESHOLD;
        if(s->seek_pos != -1){
            if(s->wbuf){
                if(write_queue_enqueue(&s->queue,s->wbuf,s->wbuf_base,s->wcount,s->write_pos) < 0){
                    av_free(s->wbuf_base);
                    s->wbuf = NULL;
                    return -1;
                }
                s->wbuf = NULL;
            }
            s->write_pos = s->seek_pos;
            if(s->write_pos != 0){
                woffset = s->write_pos & (s->f_bsize - 1);
                if(alloc_wbuf(s,((size + woffset + s->f_bsize -1)&(~(s->f_bsize -1)))) < 0){
                    av_log(h, AV_LOG_WARNING, "file.c -- file_write --seek alloc wbuf failed\n");
                    return -1;
                }
                memcpy(s->wbuf + woffset,buf_ptr,size);
                s->wcount = size;
                if(write_queue_enqueue(&s->queue,s->wbuf,s->wbuf_base,s->wcount,s->write_pos) < 0){
                    av_free(s->wbuf_base);
                    s->wbuf = NULL;
                    av_log(h, AV_LOG_WARNING, "file.c -- file_write --seek enqueue failed\n");
                    return -1;
                }
                s->write_pos +=  s->wcount;
                s->wbuf = NULL;
                s->wbuf_base = NULL;
                s->wcount = 0;
                remain_size = 0;
            }
            s->seek_pos = -1;
        }

        while(remain_size){
            if(!s->wbuf){
                if(alloc_wbuf(s,wbuf_size) < 0){
                    av_log(h, AV_LOG_WARNING, "file.c -- file_write --error----1\n");
                    return -1;
                }
            }

            wsize = remain_size;
            woffset = s->write_pos & (s->f_bsize - 1);
            max_size = wbuf_size - (woffset + s->wcount);
            if(wsize > max_size){
                wsize = max_size;
            }
            memcpy(s->wbuf + woffset + s->wcount,buf_ptr,wsize);
            s->wcount += wsize;
            if(woffset +  s->wcount == wbuf_size){
                if(write_queue_enqueue(&s->queue,s->wbuf,s->wbuf_base,s->wcount,s->write_pos) < 0){
                    av_free(s->wbuf_base);
                    s->wbuf = NULL;
                    av_log(h, AV_LOG_WARNING, "file.c -- file_write --error----2\n");
                    return -1;
                }
                s->write_pos += wbuf_size - woffset;
                s->wbuf = NULL;
                s->wbuf_base = NULL;
                s->wcount = 0;
            }
            remain_size -= wsize;
            buf_ptr += wsize;
        }
        return size;
    }
    return write(fd, buf, size);
}

static int file_get_handle(URLContext *h)
{
    FileContext *s = (FileContext*)h->priv_data;
    return s->fd;
}

#if CONFIG_FILE_PROTOCOL

static int file_open(URLContext *h, const char *filename, int flags)
{
    int access;
    int fd;
    av_strstart(filename, "file:", &filename);
    FileContext *s = (FileContext*)av_mallocz(sizeof(FileContext));
    if(!s){
        return AVERROR(ENOMEM);
    }
    h->priv_data = s;

    if ((flags & AVIO_FLAG_WRITE) && (flags & AVIO_FLAG_READ)) {
        access = O_CREAT | O_TRUNC | O_RDWR;
    } else if (flags & AVIO_FLAG_WRITE) {
        access = O_CREAT | O_TRUNC | O_RDWR;
    } else {
        access = O_RDONLY;
    }
    access |= O_LARGEFILE;
#ifdef O_BINARY
    access |= O_BINARY;
#endif

    fd = open(filename, access, 0666);
    if (fd == -1){
        av_free(s);
        return AVERROR(errno);
    }
    s->fd = fd;

    //check  O_DIRECT
    s->read_direct = 0;
    s->write_direct = 0;
    if ((flags & AVIO_FLAG_WRITE) && (flags & AVIO_FLAG_READ)){
        //rw, not use O_DIRECT
    }else{
        struct statfs disk_info;
        if(statfs(filename,&disk_info) < 0){
            av_log(h, AV_LOG_WARNING, "file_open, filename[%s],statfs failed,errno = %d\n",filename,errno);
        }else  if(disk_info.f_type == 0x4d44/*MSDOS_SUPER_MAGIC*/){
            s->f_bsize = disk_info.f_bsize;
            if(flags & AVIO_FLAG_READ){
               s->read_direct = 1;
            }
            if(flags & AVIO_FLAG_WRITE){
                s->write_direct = 1;
            }
        }else{
            av_log(h, AV_LOG_WARNING, "file_open, filename[%s],statfs ok, not MSDOS_SUPER_MAGIC, f_type = 0x%x, f_bsize = %d\n",filename,disk_info.f_type,disk_info.f_bsize);
        }
    }
    if(s->read_direct){
        int fcntl_flags_ =  fcntl(fd,F_GETFL,0);
        if(!fcntl(fd,F_SETFL, fcntl_flags_ | O_DIRECT)){
            struct stat st;
            if(fstat(fd, &st) < 0){
                av_free(s);
                close(fd);
                return AVERROR(errno);
            }
            s->file_size = st.st_size;
            s->seek_pos = -1;
            s->read_buf_base = (char *)av_malloc(READ_SYNC_THRESHOLD + s->f_bsize );
            if(!s->read_buf_base){
                close(fd);
                av_free(s);
               return AVERROR(ENOMEM);
            }
            s->read_buf = (char*) (((int)s->read_buf_base + s->f_bsize -1) & (~(s->f_bsize -1)));
            s->rcount = 0;
            s->r_pos = -1;
            av_log(h, AV_LOG_WARNING, "file_open  --- read_direct ok, file_size = %lld\n",s->file_size);
        }else{
            s->read_direct = 0;
            av_log(h, AV_LOG_WARNING, "file_open  --- read_direct failed -- use normal read\n");
        }
        return 0;
    }

    if(s->write_direct){
        int fcntl_flags_ =  fcntl(fd,F_GETFL,0);
        if(!fcntl(fd,F_SETFL, fcntl_flags_ | O_DIRECT)){
            s->wtmp_buf = (char *)av_malloc(s->f_bsize );
            if(!s->wtmp_buf){
                av_free(s);
                close(fd);
                return AVERROR(ENOMEM);
            }
            s->seek_pos = 0;
            s->write_pos = 0;
            s->h = h;
            av_log(h, AV_LOG_WARNING, "file_open, filename[%s]\n",filename);
            return write_queue_init(s);
        }else{
            s->write_direct = 0;
            av_log(h, AV_LOG_WARNING, "file_open  --- write_direct failed -- use normal write\n");
        }
    }
    return 0;
}

/* XXX: use llseek */
static int64_t file_seek(URLContext *h, int64_t pos, int whence)
{
    FileContext *s = (FileContext*)h->priv_data;
    int fd = s->fd;
    if (whence == AVSEEK_SIZE) {
        struct stat st;
        int ret = fstat(fd, &st);
        return ret < 0 ? AVERROR(errno) : st.st_size;
    }
    if(s->read_direct){
        //TODO, now only support whence == SEEK_SET
        if(whence == SEEK_SET){
            s->seek_pos = pos;
            return pos;
        }else{
            av_log(h, AV_LOG_WARNING, "file_seek  whence not SEEK_SET, whence = %d\n",whence);
            return -1;
        }
    }
    if(s->write_direct){
        //TODO, now only support whence == SEEK_SET
        if(whence == SEEK_SET){
            s->seek_pos = pos;
            return pos;
        }else{
            av_log(h, AV_LOG_WARNING, "file_seek  whence not SEEK_SET, whence = %d\n",whence);
            return -1;
        }
    }
    return lseek64(fd, pos, whence);
}

static int file_close(URLContext *h)
{
    FileContext *s = (FileContext*)h->priv_data;
    int fd = s->fd;

    if(s->read_direct){
        av_free(s->read_buf_base);
    }

    if(s->write_direct){
        if(s->wbuf){
            if(write_queue_enqueue(&s->queue,s->wbuf,s->wbuf_base,s->wcount,s->write_pos) < 0){
                av_free(s->wbuf_base);
            }
            s->wbuf = NULL;
        }
        write_queue_deinit(&s->queue);
        av_free((void*)s->wtmp_buf);
        av_free((void*)s);
        return 0;
    }

    av_free((void*)s);
    return close(fd);
}

static int file_check(URLContext *h, int mask)
{
    struct stat st;
    int ret = stat(h->filename, &st);
    if (ret < 0)
        return AVERROR(errno);

    ret |= st.st_mode&S_IRUSR ? mask&AVIO_FLAG_READ  : 0;
    ret |= st.st_mode&S_IWUSR ? mask&AVIO_FLAG_WRITE : 0;

    return ret;
}

URLProtocol ff_file_protocol = {
    .name                = "file",
    .url_open            = file_open,
    .url_read            = file_read,
    .url_write           = file_write,
    .url_seek            = file_seek,
    .url_close           = file_close,
    .url_get_file_handle = file_get_handle,
    .url_check           = file_check,
};

#endif /* CONFIG_FILE_PROTOCOL */

#if CONFIG_PIPE_PROTOCOL

static int pipe_open(URLContext *h, const char *filename, int flags)
{
    int fd;
    char *final;
    av_strstart(filename, "pipe:", &filename);

    FileContext *s = (FileContext*)av_mallocz(sizeof(FileContext));
    if(!s){
        return AVERROR(ENOMEM);
    }

    fd = strtol(filename, &final, 10);
    if((filename == final) || *final ) {/* No digits found, or something like 10ab */
        if (flags & AVIO_FLAG_WRITE) {
            fd = 1;
        } else {
            fd = 0;
        }
    }
#if HAVE_SETMODE
    setmode(fd, O_BINARY);
#endif
    s->fd = fd;
    h->priv_data = (void*)s;
    h->is_streamed = 1;
    return 0;
}

URLProtocol ff_pipe_protocol = {
    .name                = "pipe",
    .url_open            = pipe_open,
    .url_read            = file_read,
    .url_write           = file_write,
    .url_get_file_handle = file_get_handle,
    .url_check           = file_check,
};

#endif /* CONFIG_PIPE_PROTOCOL */

#endif //AMF_RECORD_DIRECTIO
