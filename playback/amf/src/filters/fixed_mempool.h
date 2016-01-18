#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#ifndef FIXED_MEM_POOL_H_
#define  FIXED_MEM_POOL_H_

class CFixedMemPool{
public:
       struct MemBlock{
           unsigned char *buf;
           void *internal;
       };
       static CFixedMemPool* Create(int blockSize, int nReservedSlots){
           CFixedMemPool *result = new CFixedMemPool();
           if (result && result->Construct(blockSize, nReservedSlots) != 0) {
               delete result;
               result = NULL;
           }
           return result;
       }
       void Delete(){delete this;}
private:
       CFixedMemPool():
           mBlockSize(0),
           mNodeSize(0),
           mpFreeList(NULL),
           mpReservedMemory(NULL)
       {}

       int Construct(int blockSize, int nReservedSlots){
           mBlockSize = (blockSize + 15)/16 * 16;
           mListSize = (sizeof(List) + 15)/16 * 16;
           mNodeSize = (mListSize + mBlockSize);
           mpReservedMemory = (unsigned char*)malloc( mNodeSize * nReservedSlots);
           if (mpReservedMemory == NULL)
               return -1;
           pthread_mutex_init(&mMutex,NULL);
           // reserved nodes, keep in free-list
           List *pNode = (List*)(mpReservedMemory);
           for (; nReservedSlots > 0; nReservedSlots--) {
               pNode->bAllocated = false;
               pNode->pNext = mpFreeList;
               mpFreeList = pNode;
               pNode = (List*)((unsigned char*)pNode + mNodeSize);
           }
           return 0;
       }
       ~CFixedMemPool(){
           if(mpFreeList){
               mpFreeList->Delete();
           }
           free(mpReservedMemory);
           pthread_mutex_destroy(&mMutex);
      }
public:
      int mallocBlock(MemBlock &block){
          pthread_mutex_lock(&mMutex);
          List *pNode = AllocNode();
          if (pNode == NULL){
              pthread_mutex_unlock(&mMutex);
              return -1;
          }
          pthread_mutex_unlock(&mMutex);
          block.buf = (unsigned char *)((unsigned char*)pNode +mListSize);
          block.internal = (void*)pNode;
          return 0;
      }
      void freeBlock(MemBlock &block){
           List *pNode = (List *)block.internal;
           if(pNode){
               if(!pNode->bAllocated){
                   pthread_mutex_lock(&mMutex);
                   pNode->pNext = mpFreeList;
                   mpFreeList = pNode;
                   pthread_mutex_unlock(&mMutex);
              }else{
                   free((void*)pNode);
              }
          }
      }
private:
    struct List {
        List *pNext;
        bool bAllocated;
        void Delete(){}
    };
    List* AllocNode(){
        if (mpFreeList) {
            List *pNode = mpFreeList;
            mpFreeList = mpFreeList->pNext;
            pNode->pNext = NULL;
            return pNode;
        }
        List *pNode = (List*)malloc(mNodeSize);
        if (pNode == NULL)
            return NULL;
        pNode->pNext = NULL;
        pNode->bAllocated = true;
        return pNode;
    }
private:
    pthread_mutex_t mMutex;
    int mBlockSize;
    int mListSize;
    int mNodeSize;
    List *mpFreeList;
    unsigned char *mpReservedMemory;
};

#endif//FIXED_MEM_POOL_H_
