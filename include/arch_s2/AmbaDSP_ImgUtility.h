/*-------------------------------------------------------------------------------------------------------------------*\
 *  @FileName       :: AmbaDSP_ImgUtility.h
 *
 *  @Copyright      :: Copyright (C) 2012 Ambarella Corporation. All rights reserved.
 *
 *                     No part of this file may be reproduced, stored in a retrieval system,
 *                     or transmitted, in any form, or by any means, electronic, mechanical, photocopying,
 *                     recording, or otherwise, without the prior consent of Ambarella Corporation.
 *
 *  @Description    :: Definitions & Constants for Ambarella DSP Liveview APIs
 *
 *  @History        ::
 *      Date        Name        Comments
 *      12/12/2012  Steve Chen  Created
 *
 *  $LastChangedDate: 2013-11-08 18:21:53 +0800 (週五, 08 十一月 2013) $
 *  $LastChangedRevision: 7252 $ 
 *  $LastChangedBy: hfwang $ 
 *  $HeadURL: http://ambtwsvn2/svn/DSC_Platform/trunk/SoC/A9/DSP/inc/AmbaDSP_ImgUtility.h $
\*-------------------------------------------------------------------------------------------------------------------*/

#ifndef _AMBA_DSP_IMG_UTIL_H_
#define _AMBA_DSP_IMG_UTIL_H_

#include "AmbaDSP_ImgDef.h"

#define AMBA_DSP_IMG_MAX_PIPE_NUM         8

typedef struct _AMBA_DSP_IMG_PIPE_INFO_s_ {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CtxBufNum;
    UINT8               CfgBufNum;
    UINT8               Reserved;
} AMBA_DSP_IMG_PIPE_INFO_s;

#if 0
typedef struct _AMBA_DSP_IMG_MEM_INFO_s_ {
    UINT32  Addr;
    UINT32  Size;
} AMBA_DSP_IMG_MEM_INFO_s;
#endif

typedef struct _AMBA_DSP_IMG_ARCH_INFO_s_ {
    UINT8                    *pWorkBuf;
    UINT32                   BufSize;
    UINT32                   PipeNum;
    AMBA_DSP_IMG_PIPE_INFO_s *pPipeInfo[AMBA_DSP_IMG_MAX_PIPE_NUM];
} AMBA_DSP_IMG_ARCH_INFO_s;

typedef struct _AMBA_DSP_IMG_CTX_INFO_s_ {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CtxId;    
    UINT8               Reserved;
    UINT8               Reserved1;
} AMBA_DSP_IMG_CTX_INFO_s;

typedef struct _AMBA_DSP_IMG_CFG_INFO_s_ {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CfgId;
    UINT8               Reserved;
    UINT8               Reserved1;
} AMBA_DSP_IMG_CFG_INFO_s;

typedef enum _AMBA_DSP_IMG_CFG_STATE_e_ {
    AMBA_DSP_IMG_CFG_STATE_IDLE = 0x00,
    AMBA_DSP_IMG_CFG_STATE_INIT,
    AMBA_DSP_IMG_CFG_STATE_PREEXE,
    AMBA_DSP_IMG_CFG_STATE_POSTEXE,
} AMBA_DSP_IMG_CFG_STATE_e;

typedef enum _AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e_{
    AMBA_DSP_IMG_CFG_EXE_QUICK = 0x00,
    AMBA_DSP_IMG_CFG_EXE_FULLCOPY,
    AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY,
} AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e;


typedef struct _AMBA_DSP_IMG_CFG_STATUS_s_ {
    UINT32                   Addr;           /* Config buffer address. */
    AMBA_DSP_IMG_CFG_STATE_e State;
    AMBA_DSP_IMG_ALGO_MODE_e AlgoMode;
    UINT8                    Reserved;
    UINT8                    Reserved1;
} AMBA_DSP_IMG_CFG_STATUS_s;

typedef struct _AMBA_DSP_IMG_SIZE_INFO_s_ {
    UINT16  WidthIn;
    UINT16  HeightIn;
    UINT16  WidthMain;
    UINT16  HeightMain;
    UINT16  WidthPrevA;
    UINT16  HeightPrevA;
    UINT16  WidthPrevB;
    UINT16  HeightPrevB;
    UINT16  WidthScrn;
    UINT16  HeightScrn;
    UINT16  WidthQvRaw;
    UINT16  HeightQvRaw;
    UINT16  Reserved;
    UINT16  Reserved1;
} AMBA_DSP_IMG_SIZE_INFO_s;

typedef struct _AMBA_DSP_IMG_LISO_DEBUG_MODE_s_ {
    UINT8 Step;
    UINT8 Mode;
    UINT8 TileIdx;
} AMBA_DSP_IMG_DEBUG_MODE_s;

/*---------------------------------------------------------------------------*\
 * Defined in AmbaDSP_ImgUtility.c
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
 * Pipeline related api 
\*---------------------------------------------------------------------------*/
int AmbaDSP_ImgQueryArchMemSize(AMBA_DSP_IMG_ARCH_INFO_s *pArchInfo);
int AmbaDSP_ImgInitArch(AMBA_DSP_IMG_ARCH_INFO_s *pArchInfo);
int AmbaDSP_ImgDeInitArch(void);

/*---------------------------------------------------------------------------*\
 * Context related api
\*---------------------------------------------------------------------------*/
/* To initialize the context. */
int   AmbaDSP_ImgInitCtx(
                    UINT8                       InitMode,
                    UINT8                       DefblcEnb,
                    UINT8			FinalSharpenEnb,
                    AMBA_DSP_IMG_CTX_INFO_s     *pDestCtx,
                    AMBA_DSP_IMG_CTX_INFO_s     *pSrcCtx);

/*---------------------------------------------------------------------------*\
 * Config related api 
\*---------------------------------------------------------------------------*/
/* To initialize the config buffer layout to certain algo mode settings. */
int AmbaDSP_ImgInitCfg(AMBA_DSP_IMG_CFG_INFO_s *pCfgInfo, AMBA_DSP_IMG_ALGO_MODE_e AlgoMode);

/* To pre-execute the config from CtxId using AlgoMode and FuncMode method. */
int AmbaDSP_ImgPreExeCfg(
                  AMBA_DSP_IMG_MODE_CFG_s     *pMode,
                  AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e ExeMode);   
                    /* 0: fast execute. Point to content in context memory buffer. */
                    /* 1: full copy execute. Copy content from context memory to config memory. */

/* To post-execute the config from CtxId using AlgoMode and FuncMode method. */
int AmbaDSP_ImgPostExeCfg(int fd_iav,
                  AMBA_DSP_IMG_MODE_CFG_s     *pMode,
                  AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e ExeMode,
                  UINT32 boot);   
                    /* 0: fast execute. Point to content in context memory buffer. */
                    /* 1: full copy execute. Copy content from context memory to config memory. */
int AmbaDSP_ImgSetSizeInfo(
                    AMBA_DSP_IMG_MODE_CFG_s     *pMode,
                    AMBA_DSP_IMG_SIZE_INFO_s    *pSizeInfo);

/* To Get the config status, including address and size. */
int AmbaDSP_ImgGetCfgStatus(AMBA_DSP_IMG_CFG_INFO_s *pCfgInfo, AMBA_DSP_IMG_CFG_STATUS_s *pStatus);

/* set the debug mode for Iso config*/
int AmbaDSP_ImgSetDebugMode(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEBUG_MODE_s *pDebugMode);
/*To get the debug mode for Iso config*/
int AmbaDSP_ImgGetDebugMode(AMBA_DSP_IMG_MODE_CFG_s *pMode, AMBA_DSP_IMG_DEBUG_MODE_s *pDebugMode);

typedef void* (*img_malloc)(u32 size);
typedef void (*img_free)(void *ptr);
typedef void* (*img_memset)(void *s, int c, u32 n);
typedef void* (*img_memcpy)(void *dest, const void *src, u32 n);
typedef int (*img_ioctl)(int handle, int request, ...);
typedef int (*img_print)(const char*format, ...);

typedef struct mem_op_s{
	img_malloc	_malloc;
	img_free		_free;
	img_memset	_memset;
	img_memcpy	_memcpy;
}mem_op_t;

void img_dsp_register_mem(mem_op_t* op);
void img_dsp_register_ioc(img_ioctl p_func);
void img_dsp_register_print(img_print p_func);

#endif  /* _AMBA_DSP_IMG_UTIL_H_ */
