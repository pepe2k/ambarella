/*
 * default memory allocator for libavutil
 * Copyright (c) 2002 Fabrice Bellard
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

/**
 * @file
 * default memory allocator for libavutil
 */

#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif

#include "avutil.h"
#include "mem.h"


/* here we can use OS-dependent allocation functions */
#undef free
#undef malloc
#undef realloc
#define MALLOC_PREFIX
#ifdef MALLOC_PREFIX

#define malloc         AV_JOIN(MALLOC_PREFIX, malloc)
#define memalign       AV_JOIN(MALLOC_PREFIX, memalign)
#define posix_memalign AV_JOIN(MALLOC_PREFIX, posix_memalign)
#define realloc        AV_JOIN(MALLOC_PREFIX, realloc)
#define free           AV_JOIN(MALLOC_PREFIX, free)

void *malloc(size_t size);
void *memalign(size_t align, size_t size);
int   posix_memalign(void **ptr, size_t align, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

#endif /* MALLOC_PREFIX */

/* You can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically. */

#ifdef PROJECT_NVR_MEM_CHECK
    #define MEM_FUNC_NAME(a)  do_##a
#else
    #define MEM_FUNC_NAME(a)  a
#endif

void *MEM_FUNC_NAME(av_malloc)(size_t size)
{
    void *ptr = NULL;
#if CONFIG_MEMALIGN_HACK
    long diff;
#endif

    /* let's disallow possible ambiguous cases */
    if(size > (INT_MAX-16) )
        return NULL;

#if CONFIG_MEMALIGN_HACK
    ptr = malloc(size+16);
    if(!ptr)
        return ptr;
    diff= ((-(long)ptr - 1)&15) + 1;
    ptr = (char*)ptr + diff;
    ((char*)ptr)[-1]= diff;
#elif HAVE_POSIX_MEMALIGN
    if (posix_memalign(&ptr,16,size))
        ptr = NULL;
#elif HAVE_MEMALIGN
    ptr = memalign(16,size);
    /* Why 64?
       Indeed, we should align it:
         on 4 for 386
         on 16 for 486
         on 32 for 586, PPro - K6-III
         on 64 for K7 (maybe for P3 too).
       Because L1 and L2 caches are aligned on those values.
       But I don't want to code such logic here!
     */
     /* Why 16?
        Because some CPUs need alignment, for example SSE2 on P4, & most RISC CPUs
        it will just trigger an exception and the unaligned load will be done in the
        exception handler or it will just segfault (SSE2 on P4).
        Why not larger? Because I did not see a difference in benchmarks ...
     */
     /* benchmarks with P3
        memalign(64)+1          3071,3051,3032
        memalign(64)+2          3051,3032,3041
        memalign(64)+4          2911,2896,2915
        memalign(64)+8          2545,2554,2550
        memalign(64)+16         2543,2572,2563
        memalign(64)+32         2546,2545,2571
        memalign(64)+64         2570,2533,2558

        BTW, malloc seems to do 8-byte alignment by default here.
     */
#else
    ptr = malloc(size);
#endif
    return ptr;
}

void *MEM_FUNC_NAME(av_realloc)(void *ptr, size_t size)
{
#if CONFIG_MEMALIGN_HACK
    int diff;
#endif

    /* let's disallow possible ambiguous cases */
    if(size > (INT_MAX-16) )
        return NULL;

#if CONFIG_MEMALIGN_HACK
    //FIXME this isn't aligned correctly, though it probably isn't needed
    if(!ptr) return av_malloc(size);
    diff= ((char*)ptr)[-1];
    return (char*)realloc((char*)ptr - diff, size + diff) + diff;
#else
    return realloc(ptr, size);
#endif
}

void MEM_FUNC_NAME(av_free)(void *ptr)
{
#if CONFIG_MEMALIGN_HACK
    if (ptr)
        free((char*)ptr - ((char*)ptr)[-1]);
#else
    free(ptr);
#endif
}

#ifdef PROJECT_NVR_MEM_CHECK

#include <pthread.h>
#include <stdarg.h>
#define PTR_LE(left,right) ((left) < (right))
#define PTR_GE(left,right) ((left) > (right))
#define PTR_EQ(left,right)  ((left) == (right))
#define PTR_LE_EQ(left,right)  ((left) <= (right))
#define PTR_GE_EQ(left,right)  ((left) >= (right))

typedef struct BLOCKINFO{
    struct BLOCKINFO *next;
    void *ptr;
    size_t size;
}blockinfo_t;

static void do_assert(char *strfile,unsigned int line){
    fflush(stdout);
    fprintf(stderr,"\nAssertion failed: %s, line %u\n",strfile,line);
    fflush(stderr);
    abort();
}

static void do_print(const char *fmt, ...){
    va_list p;
    va_start(p, fmt);
    vprintf(fmt,p);
    va_end(p);
}

#define MY_ASSERT(f) do{\
    if(f) NULL;\
    else do_assert(__FILE__,__LINE__);\
}while(0)

static blockinfo_t *s_record_head = NULL;
static pthread_mutex_t s_record_lock = PTHREAD_MUTEX_INITIALIZER;

static blockinfo_t *get_blockinfo(void*ptr){
    blockinfo_t *info;
    for(info = s_record_head; info != NULL; info = info->next){
        void *start = info->ptr;
        void *end = (void*)((unsigned char*)info->ptr + info->size -1);
        if(PTR_GE_EQ(ptr,start) && PTR_LE_EQ(ptr,end)){
            break;
        }
    }
    MY_ASSERT(info != NULL);
    return info;
}

static int create_blockinfo(void *ptr,size_t size){
    blockinfo_t *info;
    MY_ASSERT(ptr != NULL && size != 0);
    info = (blockinfo_t*)MEM_FUNC_NAME(av_malloc)(sizeof(blockinfo_t));
    if(info != NULL){
        info->ptr = ptr;
        info->size = size;
        pthread_mutex_lock(&s_record_lock);
        info->next = s_record_head;
        s_record_head = info;
        pthread_mutex_unlock(&s_record_lock);
        return 0;
    }
    return -1;
}
static void free_blockinfo(void *ptr){
    blockinfo_t *info,*info_prev = NULL;
    pthread_mutex_lock(&s_record_lock);
    for(info = s_record_head;info != NULL;info = info->next){
        if(PTR_EQ(info->ptr,ptr)){
            if(info_prev == NULL){
                s_record_head = info->next;
            }else{
                info_prev ->next = info->next;
            }
            break;
        }
        info_prev = info;
    }
    pthread_mutex_unlock(&s_record_lock);
    MY_ASSERT(info != NULL);
    MEM_FUNC_NAME(av_free)(info);
}

static void update_blockinfo(void *ptr_old,void *ptr_new,size_t size_new){
    blockinfo_t *info;
    MY_ASSERT((ptr_new != NULL) && (size_new != 0));
    pthread_mutex_lock(&s_record_lock);
    info = get_blockinfo(ptr_old);
    MY_ASSERT(PTR_EQ(ptr_old,info->ptr));
    info->ptr = ptr_new;
    info->size = size_new;
    pthread_mutex_unlock(&s_record_lock);
}

size_t ff_get_mem_allocated_size(){
    size_t total_mem_size = 0;
    blockinfo_t *info;
    pthread_mutex_lock(&s_record_lock);
    for(info = s_record_head;info != NULL;info = info->next){
        total_mem_size += info->size;
    }
    pthread_mutex_unlock(&s_record_lock);
    return total_mem_size;
}
size_t ff_sizeof_block(void * ptr){
   blockinfo_t *info;
   size_t size;
   pthread_mutex_lock(&s_record_lock);
   info = get_blockinfo(ptr);
   size = info->size;
   pthread_mutex_unlock(&s_record_lock);
   return size;
}

void *av_malloc(size_t size){
    void *ptr;
    MY_ASSERT(size != 0);
    ptr =  MEM_FUNC_NAME(av_malloc)(size);
    if(ptr){
        memset(ptr,0xa3,size);
        if(create_blockinfo(ptr,size) != 0){
            MEM_FUNC_NAME(av_free)(ptr);
            ptr = NULL;
        }
    }
    MY_ASSERT(ptr);
    return ptr;
}

void *av_realloc(void *ptr, size_t size){
    if(!ptr){
        return av_malloc(size);
    }else{
        void *ptr_new;
        //do_print("av_realloc -- %p\n",ptr);
        size_t size_old = ff_sizeof_block(ptr);
        ptr_new =  MEM_FUNC_NAME(av_realloc)(ptr,size);
        if(ptr_new){
            update_blockinfo(ptr,ptr_new,size);
            if(size > size_old){
                memset((char*)ptr_new + size_old,0xa3,size - size_old);
            }
        }
        MY_ASSERT(ptr_new);
        return ptr_new;
    }
}
void av_free(void *ptr){
    if(ptr){
        //do_print("av_free -- %p\n",ptr);
        memset(ptr,0xa5,ff_sizeof_block(ptr));
        free_blockinfo(ptr);
    }
    MEM_FUNC_NAME(av_free)(ptr);
}
#endif


void av_freep(void *arg)
{
    void **ptr= (void**)arg;
    av_free(*ptr);
    *ptr = NULL;
}

void *av_mallocz(size_t size)
{
    void *ptr = av_malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

char *av_strdup(const char *s)
{
    char *ptr= NULL;
    if(s){
        int len = strlen(s) + 1;
        ptr = av_malloc(len);
        if (ptr)
            memcpy(ptr, s, len);
    }
    return ptr;
}

