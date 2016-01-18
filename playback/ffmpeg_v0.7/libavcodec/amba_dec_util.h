
#ifndef __AMBA_DEC_UTIL__
#define __AMBA_DEC_UTIL__

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#define _config_ambadec_assert_

#ifdef _config_ambadec_assert_ 

#define ambadec_assert(expr) \
    do{ \
        if (!(expr)) { \
		printf("assert failed: file %s, %d line. \n", __FILE__, __LINE__); \
        } \
    } while (0)
    
#define ambadec_assert_ffmpeg(expr) \
    do{ \
        if (!(expr)) { \
		av_log(NULL,AV_LOG_ERROR,"assert failed: file %s, %d line. \n", __FILE__, __LINE__); \
        } \
    } while (0)
	
#else

#define ambadec_assert(expr)			//
#define ambadec_assert_ffmpeg(expr)			//

#endif

//thread related
typedef struct ctx_node_s
{
    void* p_ctx;
    struct ctx_node_s* p_pre;
    struct ctx_node_s* p_next;
}ctx_node_t;

typedef struct ambadec_queue_s
{
    int max_cnt;
    int current_cnt;
    ctx_node_t head;

    ctx_node_t* freelist;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_notfull;
    pthread_cond_t cond_notempty;

    int (*getcnt)(struct ambadec_queue_s* thiz);
    void (*lock)(struct ambadec_queue_s* thiz);
    void (*unlock)(struct ambadec_queue_s* thiz);
    
    void (*enqueue)(struct ambadec_queue_s* thiz, void* p_ctx);
    void*(*dequeue)(struct ambadec_queue_s* thiz);
}ambadec_queue_t;

ambadec_queue_t* ambadec_create_queue(int num);
void ambadec_destroy_queue(ambadec_queue_t* thiz);

typedef struct ctx_nodef_s
{
    void* p_ctx;
    unsigned int flag;
    struct ctx_nodef_s* p_pre;
    struct ctx_nodef_s* p_next;
}ctx_nodef_t;

typedef void*(*dualq_create_t)(int arg1, int arg2, int arg3, int arg4);
typedef void (*dualq_destroy_t)(void* context);

typedef struct ambadec_dualqueue_s
{
    int free_cnt,used_cnt;
    int arg1,arg2,arg3,arg4;
    ctx_nodef_t free_head,used_head;

//    ctx_nodef_t* freelist;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_notempty;//for free_queue
    
    void* (*get)(struct ambadec_dualqueue_s* thiz,int flag);
    void(*release)(struct ambadec_dualqueue_s* thiz,void* p_ctx,int flag);

    //context create and destroy
    dualq_create_t create;
    dualq_destroy_t destroy;
}ambadec_dualqueue_t;


ambadec_dualqueue_t* ambadec_create_dualqueue(int arg1,int arg2,int arg3,int arg4,dualq_create_t create,dualq_destroy_t destroy);
void ambadec_destroy_dualqueue(ambadec_dualqueue_t* thiz);

//fatal error, need exit process
#define _flag_cmd_exit_next_ 0x1
//error data, discard the current picture
#define _flag_cmd_error_data_ 0x2
//flush flag
#define _flag_cmd_flush_  0x3
//eos flag
#define _flag_cmd_eos_  0x4

typedef  void (*ambadec_triqueue_destroy_callback)(void* context);

//ready free used
typedef struct ambadec_triqueue_s
{
    int ready_cnt,free_cnt,used_cnt;
    ctx_nodef_t ready_head,free_head,used_head;
    ctx_nodef_t* free_cmd_list;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_notempty;//for ready_queue

    ctx_nodef_t* (*get_cmd)(struct ambadec_triqueue_s* thiz);
//    void (*free_cmd)(struct ambadec_triqueue_s* thiz,ctx_nodef_t* ptmp);
    
    ctx_nodef_t* (*get_free)(struct ambadec_triqueue_s* thiz);
    void (*put_ready)(struct ambadec_triqueue_s* thiz,ctx_nodef_t* p_ctx,int flag);

    //get from ready, release to free
    ctx_nodef_t* (*get)(struct ambadec_triqueue_s* thiz);//would be cmd or data
    void(*release)(struct ambadec_triqueue_s* thiz,void* p_ctx,int flag);
    void(*release_toready)(struct ambadec_triqueue_s* thiz,void* p_ctx,int flag);
    
    ambadec_triqueue_destroy_callback destroy;
}ambadec_triqueue_t;


ambadec_triqueue_t* ambadec_create_triqueue(ambadec_triqueue_destroy_callback destroy);
void ambadec_destroy_triqueue(ambadec_triqueue_t* thiz);
void ambadec_reset_triqueue(ambadec_triqueue_t* thiz);
void ambadec_print_triqueue_general_status(ambadec_triqueue_t* thiz);
void ambadec_print_triqueue_detailed_status(ambadec_triqueue_t* thiz);

//cnt based
typedef struct ambadec_pool_s
{
    int free_cnt,used_cnt;
    ctx_nodef_t free_head,used_head;
    
    pthread_mutex_t mutex;
    pthread_cond_t cond_notempty;//for free_queue

    void (*put)(struct ambadec_pool_s* thiz,void* p_ctx);

    //get from ready, release to free
    void* (*get)(struct ambadec_pool_s* thiz, int* inited);
    void (*rtn)(struct ambadec_pool_s* thiz,void* p_ctx);
    void(*inc_lock)(struct ambadec_pool_s* thiz,void* p_ctx);
    //return 1 if flag decrease to 0, need release 
    int (*dec_lock)(struct ambadec_pool_s* thiz,void* p_ctx);    
}ambadec_pool_t;

ambadec_pool_t* ambadec_create_pool(void);
void ambadec_destroy_pool(ambadec_pool_t* thiz);

//transpose matrix
void transpose_matrix(short* pel);


#endif
