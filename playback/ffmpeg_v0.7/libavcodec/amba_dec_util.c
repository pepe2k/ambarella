
#include "amba_dec_util.h"


static int _getcnt(ambadec_queue_t* thiz)
{
    ambadec_assert(thiz);
    return thiz->current_cnt;
}

static void _lock(ambadec_queue_t* thiz)
{
    ambadec_assert(thiz);
    pthread_mutex_lock(&thiz->mutex);
}

static void _unlock(ambadec_queue_t* thiz)
{
    ambadec_assert(thiz);
    pthread_mutex_unlock(&thiz->mutex);
}

static void _enqueue(ambadec_queue_t* thiz,void* p_ctx)
{
    ctx_node_t* ptmp=NULL;
    ambadec_assert(thiz);
//    ambadec_assert(p_ctx);
    
    pthread_mutex_lock(&thiz->mutex);
//    if(p_ctx)
    {
        while(thiz->current_cnt>=thiz->max_cnt)
        {
            pthread_cond_wait(&thiz->cond_notfull,&thiz->mutex);
        }
        
        if(!thiz->freelist)
        {
            ptmp=(ctx_node_t*)malloc(sizeof(ctx_node_t));
        }
        else
        {
            ptmp=thiz->freelist;
            thiz->freelist=thiz->freelist->p_next;
        }
        
        ptmp->p_ctx=p_ctx;
        ptmp->p_pre=&thiz->head;
        ptmp->p_next=thiz->head.p_next;
        thiz->head.p_next->p_pre=ptmp;
        thiz->head.p_next=ptmp;
        thiz->current_cnt++;
        pthread_cond_signal(&thiz->cond_notempty);
    }
    pthread_mutex_unlock(&thiz->mutex);
}

static void* _dequeue(ambadec_queue_t* thiz)
{
    ctx_node_t* ptmp=NULL;
    void* pctx;
    ambadec_assert(thiz);
    
    pthread_mutex_lock(&thiz->mutex);
    while(thiz->current_cnt<=0)
    {
        pthread_cond_wait(&thiz->cond_notempty,&thiz->mutex);
    }
    ptmp=thiz->head.p_pre;
    if(ptmp==&thiz->head)
    {
        printf("fatal error: ambadec_queue_t currupt in dequeue.\n");
        return NULL;
    }
    ptmp->p_pre->p_next=&thiz->head;
    thiz->head.p_pre=ptmp->p_pre;
    thiz->current_cnt--;
    
    ptmp->p_next=thiz->freelist;
    thiz->freelist=ptmp;
    pctx=ptmp->p_ctx;  
    
    pthread_cond_signal(&thiz->cond_notfull);
  
    pthread_mutex_unlock(&thiz->mutex);

    return pctx;
}

ambadec_queue_t* ambadec_create_queue(int num)
{
    ambadec_queue_t* thiz=malloc(sizeof(ambadec_queue_t));
    if(!thiz)
        return NULL;

    thiz->max_cnt=num;
    thiz->current_cnt=0;
    thiz->head.p_next=thiz->head.p_pre=&thiz->head;
    thiz->freelist=NULL;

    thiz->getcnt=_getcnt;
    thiz->lock=_lock;
    thiz->unlock=_unlock;
    thiz->enqueue=_enqueue;
    thiz->dequeue=_dequeue;
    
    pthread_mutex_init(&thiz->mutex,NULL);
    pthread_cond_init(&thiz->cond_notempty,NULL);
    pthread_cond_init(&thiz->cond_notfull,NULL);
    return thiz;
}

void ambadec_destroy_queue(ambadec_queue_t* thiz)
{
    ctx_node_t* ptmp=NULL, *ptmp1=NULL;
    ambadec_assert(thiz);
    
    ptmp1=thiz->head.p_next;
    while(ptmp1!=&thiz->head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        free(ptmp);
    }
    while(thiz->freelist)
    {
        ptmp=thiz->freelist;
        thiz->freelist=thiz->freelist->p_next;
        free(ptmp);
    }
    pthread_mutex_destroy(&thiz->mutex);
    pthread_cond_destroy(&thiz->cond_notempty);
    pthread_cond_destroy(&thiz->cond_notfull);
    free(thiz);
}

static void* _dualq_get(ambadec_dualqueue_t* thiz,int flag)
{
    ctx_nodef_t* ptmp=NULL;
    void* pctx;
    ambadec_assert(thiz);
    
    pthread_mutex_lock(&thiz->mutex);
    if(thiz->free_cnt<=0 && thiz->create)
    {
        ptmp=malloc(sizeof(ctx_nodef_t));
        ptmp->p_ctx=thiz->create(thiz->arg1,thiz->arg2,thiz->arg3,thiz->arg4);
        thiz->free_cnt=0;
    }
    else
    {
        //need outside create
        if(!thiz->create)
        {
            while(thiz->free_cnt<=0)
            {
                pthread_cond_wait(&thiz->cond_notempty,&thiz->mutex);
            }
        }
        
        ptmp=thiz->free_head.p_pre;
        if(ptmp==&thiz->free_head)
        {
            printf("fatal error: ambadec_dualqueue_t currupt in get.\n");
            return NULL;
        }

        //remove from free queue
        ptmp->p_pre->p_next=&thiz->free_head;
        thiz->free_head.p_pre=ptmp->p_pre;
        thiz->free_cnt--;
    }
    
    //put it to used queue
    ptmp->p_pre=&thiz->used_head;
    ptmp->p_next=thiz->used_head.p_next;
    thiz->used_head.p_next->p_pre=ptmp;
    thiz->used_head.p_next=ptmp;
    thiz->used_cnt++;

    ptmp->flag=flag;
    pctx=ptmp->p_ctx;
    
    pthread_mutex_unlock(&thiz->mutex);

    return pctx;
}

static void _dualq_release(ambadec_dualqueue_t* thiz,void* p_ctx,int flag)
{
    ctx_nodef_t* ptmp=NULL;
    ambadec_assert(thiz);
//    ambadec_assert(p_ctx);
    
    pthread_mutex_lock(&thiz->mutex);
//    if(p_ctx)
    {       
        ptmp=thiz->used_head.p_next;
        //find p_ctx
        while(ptmp!=&thiz->used_head)
        {
            if(ptmp->p_ctx==p_ctx)
                break;
            ptmp=ptmp->p_next;
        }
        
        if(ptmp==&thiz->used_head)
        {
            //can't find, must have errors
            printf("fatal error: can't find p_ctx in ambadec_dualqueue_t when call release.\n");
            return;
        }

        ptmp->flag&=~flag;
        if(!ptmp->flag)
        {
            //remove from used queue
            ptmp->p_pre->p_next=&thiz->used_head;
            thiz->used_head.p_pre=ptmp->p_pre;
            thiz->used_cnt--;

            //put it to free queue
            ptmp->p_pre=&thiz->free_head;
            ptmp->p_next=thiz->free_head.p_next;
            thiz->free_head.p_next->p_pre=ptmp;
            thiz->free_head.p_next=ptmp;
            thiz->free_cnt++;
            if(!thiz->create)
                pthread_cond_signal(&thiz->cond_notempty);
        }        
    }
    pthread_mutex_unlock(&thiz->mutex);
}

ambadec_dualqueue_t* ambadec_create_dualqueue(int arg1,int arg2,int arg3,int arg4,dualq_create_t create,dualq_destroy_t destroy)
{
    if(!create || ! destroy)
    {
        printf("fatal error: create or destroy function is NULL.\n");
        return NULL;
    }
    ambadec_dualqueue_t* thiz=malloc(sizeof(ambadec_dualqueue_t));
    if(!thiz)
    {
        printf("fatal error: malloc fail in ambadec_create_dualqueue.\n");
        return NULL;
    }
    
    thiz->used_cnt=thiz->free_cnt=0;
    thiz->arg1=arg1;
    thiz->arg2=arg2;
    thiz->arg3=arg3;
    thiz->arg4=arg4;
    thiz->free_head.p_next=thiz->free_head.p_pre=&thiz->free_head;
    thiz->used_head.p_next=thiz->used_head.p_pre=&thiz->used_head;

    thiz->get=_dualq_get;
    thiz->release=_dualq_release;
    thiz->create=create;
    thiz->destroy=destroy;
    pthread_mutex_init(&thiz->mutex,NULL);
    pthread_cond_init(&thiz->cond_notempty,NULL);
    
    return thiz;
}

void ambadec_destroy_dualqueue(ambadec_dualqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL, *ptmp1=NULL;
    ambadec_assert(thiz);
    
    ptmp1=thiz->free_head.p_next;
    while(ptmp1!=&thiz->free_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        if(thiz->destroy)
            thiz->destroy(ptmp->p_ctx);
        free(ptmp);
    }
    
    ptmp1=thiz->used_head.p_next;    
    while(ptmp1!=&thiz->used_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        if(thiz->destroy)
            thiz->destroy(ptmp->p_ctx);
        free(ptmp);
    }
    
    pthread_mutex_destroy(&thiz->mutex);
    pthread_cond_destroy(&thiz->cond_notempty);
    free(thiz);
}

static ctx_nodef_t* _triq_get_free_cmd(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL;    
    ambadec_assert(thiz);
    if(!thiz)
    {
        printf("error: NULL pointer in _triq_get_free_cmd.\n");
        return NULL;
    }
    
    pthread_mutex_lock(&thiz->mutex);
    if(thiz->free_cmd_list)
    {
        ptmp=thiz->free_cmd_list;
        thiz->free_cmd_list=thiz->free_cmd_list->p_next;
    }
    else
    {
        ptmp=malloc(sizeof(ctx_nodef_t));
        ptmp->p_ctx=NULL;
    }
    pthread_mutex_unlock(&thiz->mutex);
    ambadec_assert(!ptmp->p_ctx);
    ptmp->p_ctx=NULL;
    return ptmp;
}

/*
static void _triq_free_cmd(ambadec_triqueue_t* thiz,ctx_nodef_t* ptmp)
{
    ambadec_assert(thiz);
    ambadec_assert(ptmp);
    if(!thiz || !ptmp)
    {
        printf("error: NULL pointer in _triq_free_cmd.\n");
        return ;
    }
    
    pthread_mutex_lock(&thiz->mutex);
    ptmp->p_next=thiz->free_cmd_list;
    thiz->free_cmd_list=ptmp;
    pthread_mutex_unlock(&thiz->mutex);
}
*/

static ctx_nodef_t* _triq_get_free(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL;    
    ambadec_assert(thiz);
    if(!thiz)
    {
        printf("error: NULL pointer in _triq_get_free.\n");
        return NULL;
    }
    
    pthread_mutex_lock(&thiz->mutex);

    if(thiz->free_cnt<=0)
    {
        ptmp=malloc(sizeof(ctx_nodef_t));
        pthread_mutex_unlock(&thiz->mutex);
        ptmp->p_ctx=0;
        return ptmp;
    }
    ambadec_assert(thiz->free_cnt>0);
    
    ptmp=thiz->free_head.p_pre;
    ambadec_assert(ptmp!=&thiz->free_head);

/*    if(ptmp==&thiz->free_head)
    {
        printf("fatal error: ambadec_triqueue_t currupt in _triq_get_free.\n");
        pthread_mutex_unlock(&thiz->mutex);
        return NULL;
    }*/

    //remove from free queue
    ptmp->p_pre->p_next=&thiz->free_head;
    thiz->free_head.p_pre=ptmp->p_pre;
    thiz->free_cnt--;
    
    pthread_mutex_unlock(&thiz->mutex);    

    return ptmp;
}

static void _triq_put_ready(ambadec_triqueue_t* thiz,ctx_nodef_t* p_node,int flag)
{
    ambadec_assert(thiz);
    if(!thiz || !p_node)
    {
        printf("error: NULL pointer in _triq_put_ready.\n");
        return;
    }
    
    pthread_mutex_lock(&thiz->mutex);
    //put to ready queue
    p_node->flag=flag;
    p_node->p_next=thiz->ready_head.p_next;
    p_node->p_pre=&thiz->ready_head;
    thiz->ready_head.p_next->p_pre=p_node;
    thiz->ready_head.p_next=p_node;

    thiz->ready_cnt++;   
    pthread_cond_signal(&thiz->cond_notempty);

    pthread_mutex_unlock(&thiz->mutex);    
}

static ctx_nodef_t* _triq_get(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL;
    ambadec_assert(thiz);

    pthread_mutex_lock(&thiz->mutex);
    while(thiz->ready_cnt<=0)
    {
        pthread_cond_wait(&thiz->cond_notempty,&thiz->mutex);
    }
    ptmp=thiz->ready_head.p_pre;
    //remove from ready queue
    ptmp->p_pre->p_next=&thiz->ready_head;
    thiz->ready_head.p_pre=ptmp->p_pre;
    thiz->ready_cnt--;

    if(!ptmp->p_ctx)
    {    
        //cmd, put it to free_cmd_list
        ptmp->p_next=thiz->free_cmd_list;
        thiz->free_cmd_list=ptmp;
    }
    else
    {    
        //put it to used queue
        ptmp->p_pre=&thiz->used_head;
        ptmp->p_next=thiz->used_head.p_next;
        thiz->used_head.p_next->p_pre=ptmp;
        thiz->used_head.p_next=ptmp;
        thiz->used_cnt++;
    }
    
    pthread_mutex_unlock(&thiz->mutex);

    return ptmp;
}

static void _triq_release(ambadec_triqueue_t* thiz,void* p_ctx,int flag)
{
    ctx_nodef_t* ptmp=NULL;
    ambadec_assert(thiz);
    ambadec_assert(p_ctx);
    
    pthread_mutex_lock(&thiz->mutex);
    if(p_ctx)
    {       
        ptmp=thiz->used_head.p_next;
        //find p_ctx
        while(ptmp!=&thiz->used_head)
        {
            if(ptmp->p_ctx==p_ctx)
            {
                ptmp->flag&=~flag;
                if(!ptmp->flag)
                {
                    //remove from used queue
                    ptmp->p_pre->p_next=&thiz->used_head;
                    thiz->used_head.p_pre=ptmp->p_pre;
                    thiz->used_cnt--;

//                    thiz->destroy(ptmp->p_ctx);
                    
                    //put it to free queue
                    ptmp->p_pre=&thiz->free_head;
                    ptmp->p_next=thiz->free_head.p_next;
                    thiz->free_head.p_next->p_pre=ptmp;
                    thiz->free_head.p_next=ptmp;
                    thiz->free_cnt++;
                }
                pthread_mutex_unlock(&thiz->mutex);
                return;
            }
            ptmp=ptmp->p_next;
        }
        
        //can't find, in ready queue?
        ptmp=thiz->ready_head.p_next;
        while(ptmp!=&thiz->ready_head)
        {
            if(ptmp->p_ctx==p_ctx)
            {
                ptmp->flag&=~flag;
                if(!ptmp->flag)
                {
                    //remove from used queue
                    ptmp->p_pre->p_next=&thiz->ready_head;
                    thiz->ready_head.p_pre=ptmp->p_pre;
                    thiz->ready_cnt--;

                    thiz->destroy(ptmp->p_ctx);
                    
                    //put it to free queue
                    ptmp->p_pre=&thiz->free_head;
                    ptmp->p_next=thiz->free_head.p_next;
                    thiz->free_head.p_next->p_pre=ptmp;
                    thiz->free_head.p_next=ptmp;
                    thiz->free_cnt++;
                }
                pthread_mutex_unlock(&thiz->mutex);
                return;
            }
            ptmp=ptmp->p_next;
        }   
    }
    
    //can't find in used and ready queue, must have errors
    pthread_mutex_unlock(&thiz->mutex);
    printf("fatal error: can't find p_ctx in ambadec_dualqueue_t when call release.\n");
    return;  
}

static void _triq_release_to_ready(ambadec_triqueue_t* thiz,void* p_ctx,int flag)
{
    ctx_nodef_t* ptmp=NULL;
    ambadec_assert(thiz);
    ambadec_assert(p_ctx);
    
    pthread_mutex_lock(&thiz->mutex);
    if(p_ctx)
    {       
        ptmp=thiz->used_head.p_next;
        //find p_ctx
        while(ptmp!=&thiz->used_head)
        {
            if(ptmp->p_ctx==p_ctx)
            {
                ptmp->flag&=~flag;
                if(!ptmp->flag)
                {
                    //remove from used queue
                    ptmp->p_pre->p_next = ptmp->p_next;
                    ptmp->p_next->p_pre = ptmp->p_pre;
                    thiz->used_cnt--;

//                    thiz->destroy(ptmp->p_ctx);

                    //put it to ready queue
                    ptmp->p_pre=&thiz->ready_head;
                    ptmp->p_next=thiz->ready_head.p_next;
                    thiz->ready_head.p_next->p_pre=ptmp;
                    thiz->ready_head.p_next=ptmp;
                    thiz->ready_cnt++;
                    pthread_cond_signal(&thiz->cond_notempty);
                }
                pthread_mutex_unlock(&thiz->mutex);
                return;
            }
            ptmp=ptmp->p_next;
        }
           
    }
    
    //can't find in used and ready queue, must have errors
    pthread_mutex_unlock(&thiz->mutex);
    printf("fatal error: can't find p_ctx in ambadec_triqueue_t when call release_to_ready.\n");
    return;  
}

ambadec_triqueue_t* ambadec_create_triqueue(ambadec_triqueue_destroy_callback destroy)
{
    ambadec_triqueue_t* thiz=(ambadec_triqueue_t*)malloc(sizeof(ambadec_triqueue_t));

    if(!thiz)
    {
        printf("fatal error: malloc fail in ambadec_create_triqueue.\n");
        return NULL;
    }
    memset(thiz,0x0,sizeof(ambadec_triqueue_t));    
    //thiz->used_cnt=thiz->free_cnt=thiz->ready_cnt=0;
    
    thiz->ready_head.p_next=thiz->ready_head.p_pre=&thiz->ready_head;
    thiz->free_head.p_next=thiz->free_head.p_pre=&thiz->free_head;
    thiz->used_head.p_next=thiz->used_head.p_pre=&thiz->used_head;
    
    thiz->get_cmd=_triq_get_free_cmd;
//    thiz->free_cmd=_triq_free_cmd;
    thiz->get_free=_triq_get_free;
    thiz->put_ready=_triq_put_ready;
    thiz->get=_triq_get;
    thiz->release=_triq_release;
    thiz->release_toready=_triq_release_to_ready;
    thiz->destroy=destroy;
//    thiz->destroy=NULL;
    pthread_mutex_init(&thiz->mutex,NULL);
    pthread_cond_init(&thiz->cond_notempty,NULL);
    
    return thiz;
}

void ambadec_reset_triqueue(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL;
    ctx_nodef_t* ptmp_next_save=NULL;
    ambadec_assert(thiz);
    
    ptmp=thiz->used_head.p_next;
    while(ptmp!=&thiz->used_head)
    {
        ptmp->p_pre->p_next = ptmp->p_next;
        ptmp->p_next->p_pre = ptmp->p_pre;
        thiz->used_cnt--;
        
//        if(thiz->destroy && ptmp->p_ctx)
//            thiz->destroy(ptmp->p_ctx);
        //save next node first
        ptmp_next_save = ptmp->p_next;
        
        //put it to free queue
        ptmp->p_pre=&thiz->free_head;
        ptmp->p_next=thiz->free_head.p_next;
        thiz->free_head.p_next->p_pre=ptmp;
        thiz->free_head.p_next=ptmp;
        thiz->free_cnt++;

        //next node op then
        ptmp=ptmp_next_save;
    }
    
    ptmp=thiz->ready_head.p_next;    
    while(ptmp!=&thiz->ready_head)
    {
        //remove from ready queue
        ptmp->p_pre->p_next = ptmp->p_next;
        ptmp->p_next->p_pre = ptmp->p_pre;
        thiz->ready_cnt--;
//        if(thiz->destroy && ptmp->p_ctx)
//            thiz->destroy(ptmp->p_ctx);

        //save next node first
        ptmp_next_save = ptmp->p_next;

        if (ptmp->p_ctx) {
            //put it to free queue
            ptmp->p_pre=&thiz->free_head;
            ptmp->p_next=thiz->free_head.p_next;
            thiz->free_head.p_next->p_pre=ptmp;
            thiz->free_head.p_next=ptmp;
            thiz->free_cnt++;
        } else {
            //put it to free list
            ptmp->p_next = thiz->free_cmd_list;
            thiz->free_cmd_list = ptmp;
        }

        //next node op then
        ptmp=ptmp_next_save;
    }

}

void ambadec_destroy_triqueue(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL, *ptmp1=NULL;
    ambadec_assert(thiz);
    
    ptmp1=thiz->ready_head.p_next;
    while(ptmp1!=&thiz->ready_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        if(thiz->destroy && ptmp->p_ctx)
            thiz->destroy(ptmp->p_ctx);
        free(ptmp);
    }
    
    ptmp1=thiz->used_head.p_next;    
    while(ptmp1!=&thiz->used_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        if(thiz->destroy && ptmp->p_ctx)
            thiz->destroy(ptmp->p_ctx);
        free(ptmp);
    }

    ptmp1=thiz->free_head.p_next;    
    while(ptmp1!=&thiz->free_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        if(thiz->destroy && ptmp->p_ctx)
            thiz->destroy(ptmp->p_ctx);
        free(ptmp);
    }
    
    ptmp1=thiz->free_cmd_list;    
    while(ptmp1)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        free(ptmp);
    }
    
    pthread_mutex_destroy(&thiz->mutex);
    pthread_cond_destroy(&thiz->cond_notempty);
    free(thiz);
}

void ambadec_print_triqueue_general_status(ambadec_triqueue_t* thiz)
{
    ambadec_assert(thiz);
    if (!thiz) {
        fprintf(stderr, "NULL pointer in ambadec_print_triqueue_general_status.\n");
        return;
    }

    //print queue's node count
    fprintf(stderr, " general: ready_cnt %d, used_cnt %d, free_cnt %d, triqueue %p.\n", thiz->ready_cnt, thiz->used_cnt, thiz->free_cnt, thiz);
}

void ambadec_print_triqueue_detailed_status(ambadec_triqueue_t* thiz)
{
    ctx_nodef_t* ptmp=NULL;//, *ptmp1=NULL
    ambadec_assert(thiz);
    if (!thiz) {
        fprintf(stderr, "NULL pointer in ambadec_print_triqueue_detailed_status.\n");
        return;
    }

    fprintf(stderr, " Detailed in ready queue:\n");
    ptmp=thiz->ready_head.p_next;
    while(ptmp!=&thiz->ready_head)
    {
        fprintf(stderr, "   node %p, flag 0x%x, payload %p.\n", ptmp, ptmp->flag, ptmp->p_ctx);
        ptmp = ptmp->p_next;
    }

    fprintf(stderr, " Detailed in used queue:\n");
    ptmp=thiz->used_head.p_next;
    while(ptmp!=&thiz->used_head)
    {
        fprintf(stderr, "   node %p, flag 0x%x, payload %p.\n", ptmp, ptmp->flag, ptmp->p_ctx);
        ptmp = ptmp->p_next;
    }

    fprintf(stderr, " Detailed in free queue:\n");
    ptmp=thiz->free_head.p_next;
    while(ptmp!=&thiz->free_head)
    {
        fprintf(stderr, "   node %p, flag 0x%x, payload %p.\n", ptmp, ptmp->flag, ptmp->p_ctx);
        ptmp = ptmp->p_next;
    }

    fprintf(stderr, " Detailed in free list:\n");
    ptmp=thiz->free_cmd_list;
    while(ptmp)
    {
        fprintf(stderr, "   node %p, flag 0x%x, payload %p.\n", ptmp, ptmp->flag, ptmp->p_ctx);
        ptmp = ptmp->p_next;
    }
}

//#define _lod_internal_pool_

static void _pool_put(ambadec_pool_t* thiz,void* p_ctx)
{
    ctx_nodef_t* p_node=malloc(sizeof(ctx_nodef_t));
    p_node->p_ctx=p_ctx;
    p_node->flag=-1;
    
    #ifdef _lod_internal_pool_
    static int cnt=0;
    printf("_pool_put in %d, pctx=%x, free_cnt=%d,used_cnt=%d.\n",cnt++,p_ctx,thiz->free_cnt,thiz->used_cnt);
    #endif
    
    //put to free queue
    pthread_mutex_lock(&thiz->mutex);    
    p_node->p_next=thiz->free_head.p_next;
    p_node->p_pre=&thiz->free_head;
    thiz->free_head.p_next->p_pre=p_node;
    thiz->free_head.p_next=p_node;
    thiz->free_cnt++;
    pthread_cond_signal(&thiz->cond_notempty);

    pthread_mutex_unlock(&thiz->mutex);
    
    #ifdef _lod_internal_pool_
    printf("_pool_put end,free_cnt=%d,used_cnt=%d.\n",thiz->free_cnt,thiz->used_cnt);
    #endif
}

static void* _pool_get(ambadec_pool_t* thiz,int* inited)
{
    ctx_nodef_t* ptmp=NULL;
    void* pctx;
    ambadec_assert(thiz);
    
    #ifdef _lod_internal_pool_
    static int cnt=0;
    printf("_pool_get in %d, free_cnt=%d,used_cnt=%d.\n",cnt++,thiz->free_cnt,thiz->used_cnt);
    #endif
    
    pthread_mutex_lock(&thiz->mutex);
    while(thiz->free_cnt<=0)
    {
        pthread_cond_wait(&thiz->cond_notempty,&thiz->mutex);
    }

    ptmp=thiz->free_head.p_pre;
    thiz->free_head.p_pre=ptmp->p_pre;
    ptmp->p_pre->p_next=&thiz->free_head;
    thiz->free_cnt--;
    
    ptmp->p_pre=&thiz->used_head;
    ptmp->p_next=thiz->used_head.p_next;
    thiz->used_head.p_next->p_pre=ptmp;
    thiz->used_head.p_next=ptmp;
    thiz->used_cnt++;   
    
    pctx=ptmp->p_ctx;
    if(ptmp->flag==-1)
    {
        *inited=1;//need initialize
    }
    ptmp->flag=0;
    pthread_mutex_unlock(&thiz->mutex);
    
    #ifdef _lod_internal_pool_
    printf("_pool_get end,get ctx=%x, free_cnt=%d,used_cnt=%d.\n",pctx,thiz->free_cnt,thiz->used_cnt);
    #endif
    
    return pctx;    
}

void _pool_rtn(struct ambadec_pool_s* thiz,void* p_ctx)
{
    ctx_nodef_t* ptmp=NULL;
    ambadec_assert(thiz);
    ambadec_assert(p_ctx);

    pthread_mutex_lock(&thiz->mutex);
    if(p_ctx)
    {
        ptmp=thiz->used_head.p_next;
        //find p_ctx
        while(ptmp!=&thiz->used_head)
        {
            if(ptmp->p_ctx==p_ctx)
            {
                //remove from used list
                ptmp->p_pre->p_next=ptmp->p_next;
                ptmp->p_next->p_pre=ptmp->p_pre;
                thiz->used_cnt--;

                //put to free list
                ptmp->p_next=thiz->free_head.p_next;
                ptmp->p_pre=&thiz->free_head;
                thiz->free_head.p_next->p_pre=ptmp;
                thiz->free_head.p_next=ptmp;
                thiz->free_cnt++;
                ptmp->flag=0;
                pthread_cond_signal(&thiz->cond_notempty);
                pthread_mutex_unlock(&thiz->mutex);
                return;
            }
            ptmp=ptmp->p_next;
        }

    }

    //can't find in used queue, must have errors
    pthread_mutex_unlock(&thiz->mutex);
    printf("fatal error: can't find p_ctx in ambadec_pool_t when call _pool_rtn.\n");
    return;
}

static void _pool_inc_lock(ambadec_pool_t* thiz,void* p_ctx)
{
    ctx_nodef_t* ptmp;
    ambadec_assert(thiz);
    
    #ifdef _lod_internal_pool_
    static int cnt=0;
    printf("_pool_inc_lock in %d, p_ctx=%x, free_cnt=%d,used_cnt=%d.\n",cnt++,p_ctx,thiz->free_cnt,thiz->used_cnt);
    #endif    
    
    pthread_mutex_lock(&thiz->mutex);
    ptmp=thiz->used_head.p_next;
    while(ptmp!=&thiz->used_head)
    {
        if(p_ctx==ptmp->p_ctx)
        {
            ptmp->flag++;
            break;
        }
        ptmp=ptmp->p_next;
    }
    
    pthread_mutex_unlock(&thiz->mutex);

    return ;    
}

static int _pool_dec_lock(ambadec_pool_t* thiz,void* p_ctx)
{
    ctx_nodef_t* ptmp;
    ambadec_assert(thiz);
    
    #ifdef _lod_internal_pool_
    static int cnt=0;
    printf("_pool_dec_lock in %d, p_ctx=%x, free_cnt=%d,used_cnt=%d.\n",cnt++,p_ctx,thiz->free_cnt,thiz->used_cnt);
    #endif  
    
    pthread_mutex_lock(&thiz->mutex);
    ptmp=thiz->used_head.p_next;
    while(ptmp!=&thiz->used_head)
    {
        if(p_ctx==ptmp->p_ctx)
        {
            if(ptmp->flag==1)
            {
                ptmp->flag = 0;
                //remove from used list
                ptmp->p_pre->p_next=ptmp->p_next;
                ptmp->p_next->p_pre=ptmp->p_pre;
                thiz->used_cnt--;
                
                //put to free list
                ptmp->p_next=thiz->free_head.p_next;
                ptmp->p_pre=&thiz->free_head;
                thiz->free_head.p_next->p_pre=ptmp;
                thiz->free_head.p_next=ptmp;
                thiz->free_cnt++;
                pthread_cond_signal(&thiz->cond_notempty);

                pthread_mutex_unlock(&thiz->mutex);
                
                #ifdef _lod_internal_pool_
                printf("_pool_dec_lock end free, free_cnt=%d,used_cnt=%d.\n",thiz->free_cnt,thiz->used_cnt);
                #endif  

                return 1;
            }else if (ptmp->flag > 1) {
                ptmp->flag--;
            } else {
                printf("_pool_dec_lock end free, error ptmp->flag=%d, reset it to 0.\n", ptmp->flag);
                ptmp->flag = 0;
            }
            break;
        }
        ptmp=ptmp->p_next;
    }
    
    pthread_mutex_unlock(&thiz->mutex);
    
    #ifdef _lod_internal_pool_
    printf("_pool_dec_lock end no free, free_cnt=%d,used_cnt=%d.\n",thiz->free_cnt,thiz->used_cnt);
    #endif  
                
    return 0;    
}
    
ambadec_pool_t* ambadec_create_pool()
{
    ambadec_pool_t* thiz=malloc(sizeof(ambadec_pool_t));
    if(!thiz)
    {
        printf("fatal error: malloc fail in ambadec_create_triqueue.\n");
        return NULL;
    }
    
    thiz->used_cnt=thiz->free_cnt=0;
    thiz->free_head.p_next=thiz->free_head.p_pre=&thiz->free_head;
    thiz->used_head.p_next=thiz->used_head.p_pre=&thiz->used_head;
    
    thiz->put=_pool_put;
    thiz->get=_pool_get;
    thiz->rtn=_pool_rtn;
    thiz->inc_lock=_pool_inc_lock;
    thiz->dec_lock=_pool_dec_lock;    
    
    pthread_mutex_init(&thiz->mutex,NULL);
    pthread_cond_init(&thiz->cond_notempty,NULL);
    
    return thiz;
}

void ambadec_destroy_pool(ambadec_pool_t* thiz)
{
    ctx_nodef_t* ptmp=NULL, *ptmp1=NULL;
    ambadec_assert(thiz);
    
    ptmp1=thiz->used_head.p_next;    
    while(ptmp1!=&thiz->used_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        free(ptmp);
    }

    ptmp1=thiz->free_head.p_next;    
    while(ptmp1!=&thiz->free_head)
    {
        ptmp=ptmp1;
        ptmp1=ptmp1->p_next;
        free(ptmp);
    }
    
    pthread_mutex_destroy(&thiz->mutex);
    pthread_cond_destroy(&thiz->cond_notempty);
    free(thiz);    
}

typedef short(* block_array)[8][8];

void transpose_matrix(short* pel)
{
    block_array pmatrix=(block_array)pel;
    int i=0,j=0;
    short exchange;
    
    for(i=1;i<8;i++)
    {
        for(j=0;j<i;j++)
        {
            exchange=(*pmatrix)[i][j];
            (*pmatrix)[i][j]=(*pmatrix)[j][i];
            (*pmatrix)[j][i]=exchange;
        }
    }
}

/*
void print_m(short* p)
{
        int i,j;
        block_array block=(block_array)p;
        printf("\n");
        for(i=0;i<8;i++)
        {
                for(j=0;j<8;j++)
                {
                        printf("  %d ",(*block)[i][j]);
                }
                printf("\n");
        }
}
*/

