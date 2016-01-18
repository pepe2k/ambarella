/**
 * am_types.h
 *
 * History:
 *	2007/11/5 - [Oliver Li] created file
 *	2009/12/2 - [Oliver Li] rewrite
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AM_TYPES_H__
#define __AM_TYPES_H__

#include "stdio.h"
#include "stdlib.h"

//-----------------------------------------------------------------------
//
//	general types
//
//-----------------------------------------------------------------------

#define AM_DEBUG

typedef int AM_INT;
typedef unsigned int AM_UINT;

typedef unsigned char AM_U8;
typedef unsigned short AM_U16;
typedef unsigned int AM_U32;

typedef signed char AM_S8;
typedef signed short AM_S16;
typedef signed int AM_S32;

typedef signed long long AM_S64;
typedef unsigned long long AM_U64;

typedef int AM_INTPTR;

typedef AM_U64	am_pts_t;

typedef unsigned char am_char_t;

typedef volatile int am_atomic_t;

typedef int AM_BOOL;
#define AM_TRUE		1
#define AM_FALSE	0

typedef AM_S64 am_file_off_t;


//-----------------------------------------------------------------------
//
//	error code
//
//-----------------------------------------------------------------------

enum AM_ERR
{
	ME_OK = 0,

	ME_PENDING,
	ME_ERROR,
	ME_CLOSED,
	ME_BUSY,
	ME_NO_IMPL,
	ME_OS_ERROR,
	ME_IO_ERROR,
	ME_TIMEOUT,
	ME_NO_MEMORY,
	ME_TOO_MANY,

	ME_NOT_EXIST,
	ME_NOT_SUPPORTED,
	ME_NO_INTERFACE,
	ME_BAD_STATE,
	ME_BAD_PARAM,
	ME_BAD_COMMAND,

	ME_BAD_FORMAT,

	ME_VIDEO_DATA_ERROR,
	ME_UDEC_ERROR,

	ME_TRYNEXT,

    ME_DEBUG,
};

#define DAMF_MAX_FILENAME_EXTENTION_LEN 40
#define DAMF_MAX_FILENAME_LEN 200

enum {
    //from dsp/itron define
    PredefinedPictureType_IDR = 1,
    PredefinedPictureType_I = 2,
    PredefinedPictureType_P = 3,
    PredefinedPictureType_B = 4,
};

//-----------------------------------------------------------------------
//
//	debug print, assert
//
//-----------------------------------------------------------------------

#include "stdio.h"

extern void AM_LoadGlobalCfg(const char* CfgfileName);
extern void AM_OpenGlobalFiles(void);
extern void AM_CloseGlobalFiles(void);
extern void AM_GlobalFilesSaveBegin(char* pchDataSource);
extern void AM_GlobalFilesSaveEnd(char* pchDataSource);
extern AM_UINT AM_LoadLogConfigFile(const char* CfgfileName);
extern void GeneratePresetYCbCr422_YCbCrAlphaCLUT(AM_U32* p_clut, AM_U8 transparent_index, AM_U8 reserve_transparent);

typedef enum{
    AM_PathSdcard = 0,
    AM_PathDump,
    AM_PathConfig,
    AM_TotalPathesCount,
} EAM_Path;

extern AM_ERR AM_SetPathes(char* dumpfile_path, char* configfile_path);
extern char* AM_GetPath(EAM_Path path_type);
extern AM_ERR AM_SetPath(EAM_Path path_type, char* string);

#ifdef AM_DEBUG

#include "assert.h"

#if PLATFORM_ANDROID
#include <cutils/log.h>
#elif PLATFORM_ANDROID_ITRON
#include <cutils/log.h>
#else
#define __NOLOG(format, args...)    (void)0
#undef LOGI
#undef LOGE
#undef LOGW
#undef LOGV
#undef LOGD
#define LOGI __NOLOG
#define LOGE __NOLOG
#define LOGW __NOLOG
#define LOGV __NOLOG
#define LOGD __NOLOG
#endif

typedef enum {
    LogNone = 0,
    LogErrorLevel,
    LogWarningLevel,
    LogInfoLevel,
    LogDebugLevel,
    LogVerboseLevel,
    LogAll
}ELogLevel;

typedef enum {
    LogBasicInfo = 0x1,
    LogPTS = 0x2,
    LogState = 0x4,
    LogPerformance = 0x8,
    LogDestructor = 0x10,
    LogBinaryData = 0x20,
    LogCMD = 0x40,
}ELogOption;

typedef enum {
    LogNoOutput = 0,
    LogConsole = 0x1,
    LogLogcat = 0x2,
    LogFile = 0x4,
    LogDumpTotalBinary = 0x8,
    LogDumpSeparateBinary = 0x10,
    LogForDebugTest = 0x20,
    LogDisableNewPath = 0x40,
}ELogOutput;

//module index, for log.config
typedef enum {
    //global settings
    LogGlobal = 0,

    LogModuleListStart,

    //playback related
    LogModulePBEngine = LogModuleListStart,//first line must not be modified
    LogModuleFFMpegDemuxer,
    LogModuleAmbaVideoDecoder,
    LogModuleVideoRenderer,
    LogModuleFFMpegDecoder,
    LogModuleAudioEffecter,
    LogModuleAudioRenderer,
    LogModuleGeneralDecoder,
    LogModuleVideoDecoderDSP,
    LogModuleVideoDecoderFFMpeg,
    LogModuleDSPHandler,
    LogModuleAudioDecoderHW,
    LogModuleAudioOutputALSA,
    LogModuleAudioOutputAndroid,

    //record related, add new modules, todo
    LogModuleRecordEngine,
    LogModuleVideoEncoder,
    LogModuleAudioInputFilter,
    LogModuleAudioEncoder,
    LogModuleFFMpegEncoder,
    LogModuleFFMpegMuxer,
    LogModuleItronStreammer,

    //editing related, add new modules, todo
    LogModuleVEEngine,
    LogModuleVideoEffecter,
    LogModuleVideoTranscoder,
    LogModuleVideoMemEncoder,

    //privatedata related
    LogModulePridataComposer,
    LogModulePridataParser,

    //streaming related
    LogModuleStreamingServerManager,
    LogModuleRTPTransmiter,

    //duplex related
    LogModuleAmbaVideoSink,

    LogModuleListEnd,
}ELogModule;

#define _AM_LOG_LEVEL_LOGCAT(level,format,args...)	 \
	if (level == LogInfoLevel) { \
		LOGI(format,##args); \
	}else if(level == LogErrorLevel) { \
		LOGE(format,##args); \
	} else if (level == LogWarningLevel) { \
		LOGW(format,##args); \
	} else if (level == LogDebugLevel) { \
		LOGD(format,##args); \
	} else { \
		LOGV(format,##args); \
	}

#ifdef AMDROID_DEBUG

#define _AM_LOG_LEVEL(level,format,args...)  do { \
	if (mLogLevel >= level) { \
		if (mLogOutput & LogConsole) { \
			fprintf(stdout, "%s", (const char*)mcLogTag[level]); \
			fprintf(stdout, format,##args);  \
		} \
		if (mLogOutput & LogFile) { \
                      if(g_GlobalCfg.mLogFd) { \
                            fprintf(g_GlobalCfg.mLogFd, "%s", (const char*)mcLogTag[level]); \
                            fprintf(g_GlobalCfg.mLogFd, format,##args);  \
                      } \
		} \
		if (mLogOutput & LogLogcat) { \
			_AM_LOG_LEVEL_LOGCAT(level,format,##args) \
		} \
	} \
} while (0)

#define _AM_LOG_OPTION(option,format,args...)  do { \
	if (mLogOption & option) { \
		if (mLogOutput & LogConsole) { \
			fprintf(stdout, format, ##args);  \
		} \
		if (mLogOutput & LogFile) { \
                      if(g_GlobalCfg.mLogFd) { \
			    fprintf(g_GlobalCfg.mLogFd, format, ##args);  \
                      } \
		} \
		if (mLogOutput & LogLogcat) { \
			LOGD(format,##args); \
		} \
	} \
} while (0)

#else

#define _AM_LOG_LEVEL(level,format,args...)  do { \
	if (mLogLevel >= level) { \
		if (mLogOutput & LogLogcat) { \
			_AM_LOG_LEVEL_LOGCAT(level,format,##args) \
		} \
		if (mLogOutput & LogFile) { \
                      if(g_GlobalCfg.mLogFd) { \
                            fprintf(g_GlobalCfg.mLogFd, "%s", (const char*)mcLogTag[level]); \
                            fprintf(g_GlobalCfg.mLogFd, format,##args); \
                      } \
		} \
		if (mLogOutput & LogConsole) { \
			fprintf(stdout, "%s", (const char*)mcLogTag[level]); \
			fprintf(stdout, format,##args); \
		} \
	} \
} while (0)

#define _AM_LOG_OPTION(option,format,args...)  do { \
	if (mLogOption & option) { \
		if (mLogOutput & LogLogcat) { \
			 LOGD(format,##args); \
		} \
		if (mLogOutput & LogFile) { \
                     if(g_GlobalCfg.mLogFd) { \
			    fprintf(g_GlobalCfg.mLogFd, format,##args); \
                     } \
		} \
		if (mLogOutput & LogConsole) { \
			fprintf(stdout, format,##args); \
		} \
	} \
} while (0)

#endif

#define AMLOG_VERBOSE(format, args...) _AM_LOG_LEVEL(LogVerboseLevel,format, ##args)
#define AMLOG_DEBUG(format, args...) _AM_LOG_LEVEL(LogDebugLevel, format, ##args)
#define AMLOG_INFO(format, args...) _AM_LOG_LEVEL(LogInfoLevel, format, ##args)
#define AMLOG_WARN(format, args...) _AM_LOG_LEVEL(LogWarningLevel, format, ##args)
#define AMLOG_ERROR(format, args...) _AM_LOG_LEVEL(LogErrorLevel, format, ##args)
#define AMLOG_PRINTF(format, args...) _AM_LOG_OPTION(LogBasicInfo, format, ##args)

#define AMLOG_PERFORMANCE(format, args...) _AM_LOG_OPTION(LogPerformance, format, ##args)
#define AMLOG_PTS(format, args...) _AM_LOG_OPTION(LogPTS, format, ##args)
#define AMLOG_STATE(format, args...) _AM_LOG_OPTION(LogState, format, ##args)
#define AMLOG_DESTRUCTOR(format, args...) _AM_LOG_OPTION(LogDestructor, format, ##args)
#define AMLOG_BINARY(format, args...) _AM_LOG_OPTION(LogBinaryData, format, ##args)
#define AMLOG_CMD(format, args...) _AM_LOG_OPTION(LogCMD, format, ##args)

//detailed msg for debug use
//#define AM_VERBOSE_LOG_MSG

#define _AM_LEVEL(level,format,args...)  do { \
	if (g_ModuleLogConfig[LogGlobal].log_level >= level) { \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogLogcat) { \
			_AM_LOG_LEVEL_LOGCAT(level,format,##args) \
		} \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogFile) { \
                      if(g_GlobalCfg.mLogFd) { \
                            fprintf(g_GlobalCfg.mLogFd, format,##args); \
                      } \
		} \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogConsole) { \
			fprintf(stdout, format,##args); \
		} \
	} \
} while (0)

#define _AM_OPTION(option,format,args...)  do { \
	if (g_ModuleLogConfig[LogGlobal].log_option & option) { \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogLogcat) { \
			_AM_LOG_LEVEL_LOGCAT(LogInfoLevel,format,##args) \
		} \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogFile) { \
                      if(g_GlobalCfg.mLogFd) { \
                            fprintf(g_GlobalCfg.mLogFd, format,##args); \
                      } \
		} \
		if (g_ModuleLogConfig[LogGlobal].log_output & LogConsole) { \
			fprintf(stdout, format,##args); \
		} \
	} \
} while (0)

#define AM_VERBOSE(format, args...) _AM_LEVEL(LogVerboseLevel,format, ##args)
#define AM_DBG(format, args...) _AM_LEVEL(LogDebugLevel, format, ##args)
#define AM_INFO(format, args...)	_AM_LEVEL(LogInfoLevel, "::: " format, ##args)
#define AM_WARNING(format, args...) _AM_LEVEL(LogWarningLevel, format, ##args)
#define AM_ERROR(format, args...)	_AM_LEVEL(LogErrorLevel, "!!! %s:%d,\n" format, __FILE__, __LINE__, ##args)
#define AM_PRINTF(format, args...)	_AM_OPTION(LogBasicInfo, ">>> " format, ##args)
#define AM_DESTRUCTOR(format, args...)	_AM_OPTION(LogDestructor, ">>> " format, ##args)

#else

#define AM_VERBOSE(format, args...) (void)0
#define AM_DBG(format, args...) (void)0
#define AM_INFO(format, args...)	(void)0
#define AM_WARNING(format, args...) (void)0
#define AM_ERROR(format, args...)	(void)0
#define AM_PRINTF(format, args...)	(void)0
#define AM_DESTRUCTOR(format, args...)	(void)0

#define _AM_NOLOG(format, args...)	(void)0

#define AMLOG_VERBOSE _AM_NOLOG
#define AMLOG_DEBUG _AM_NOLOG
#define AMLOG_PRINTF _AM_NOLOG
#define AMLOG_INFO _AM_NOLOG
#define AMLOG_WARN _AM_NOLOG
#define AMLOG_ERROR _AM_NOLOG

#define AMLOG_PERFORMANCE _AM_NOLOG
#define AMLOG_PTS _AM_NOLOG
#define AMLOG_STATE _AM_NOLOG
#define AMLOG_DESTRUCTOR _AM_NOLOG
#define AMLOG_BINARY _AM_NOLOG
#define AMLOG_CMD _AM_NOLOG
#endif

//critical error print should be ways true
#define AM_ASSERT(expr) do { \
    if (!(expr)) { \
        fprintf(stdout,"assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
        LOGE("assertion failed: %s\n\tAt %s : %d\n", #expr, __FILE__, __LINE__); \
    } \
}while (0)

#define AM_ASSERT_OK(_err) do { \
    if ((_err) != ME_OK) { \
        fprintf(stdout,"result != ME_OK at %s : %d\n", __FILE__, __LINE__); \
        LOGE("result != ME_OK at %s : %d\n", __FILE__, __LINE__); \
    } \
} while (0)

#define AM_PERROR(msg) do { \
    if (g_ModuleLogConfig[LogGlobal].log_output & LogFile) { \
        if(g_GlobalCfg.mLogFd) { \
            fprintf(g_GlobalCfg.mLogFd, "%s", msg); \
        } \
    } \
    LOGE("PERROR: %s, %d.\n", __FILE__, __LINE__); \
    LOGE(msg); \
    perror(msg); \
} while (0)

//debug print
#define AM_DEBUG_INFO(format, args...) (void)0


//-----------------------------------------------------------------------
//
//	macros
//
//-----------------------------------------------------------------------

// align must be power of 2
#ifndef ROUND_UP
#define ROUND_UP(_size, _align)		(((_size) + ((_align) - 1)) & ~((_align) - 1))
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(_size, _align)	((_size) & ~((_align) - 1))
#endif

#ifndef AM_MIN
#define AM_MIN(_a, _b)			((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef AM_MAX
#define AM_MAX(_a, _b)			((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_array)		(sizeof(_array) / sizeof(_array[0]))
#endif

#ifndef OFFSET
#define OFFSET(_type, _member)		((int)&((_type*)0)->member)
#endif

#ifndef PTR_ADD
#define PTR_ADD(_ptr, _size)		(void*)((char*)(_ptr) + (_size))
#endif


//-----------------------------------------------------------------------
//
//	IID
//
//-----------------------------------------------------------------------

struct AM_GUID
{
	AM_U32	x;
	AM_U16	s1;
	AM_U16	s2;
	AM_U8	c[8];
};

typedef const AM_GUID& AM_REFGUID;

extern const AM_GUID GUID_NULL;

typedef AM_GUID AM_IID;
typedef AM_GUID AM_CLSID;

typedef const AM_IID& AM_REFIID;
typedef const AM_CLSID& AM_REFCLSID;

inline int operator==(AM_REFGUID guid1, AM_REFGUID guid2)
{
	if (&guid1 == &guid2)
		return 1;
	return ((AM_U32*)&guid1)[0] == ((AM_U32*)&guid2)[0] &&
		((AM_U32*)&guid1)[1] == ((AM_U32*)&guid2)[1] &&
		((AM_U32*)&guid1)[2] == ((AM_U32*)&guid2)[2] &&
		((AM_U32*)&guid1)[3] == ((AM_U32*)&guid2)[3];
}

inline int operator!=(AM_REFGUID guid1, AM_REFGUID guid2)
{
	return !(guid1 == guid2);
}

#define AM_DEFINE_IID(name, x, s1, s2, c0, c1, c2, c3, c4, c5, c6, c7) \
	extern const AM_IID name = {x, s1, s2, {c0, c1, c2, c3, c4, c5, c6, c7}}

#define AM_DEFINE_GUID(name, x, s1, s2, c0, c1, c2, c3, c4, c5, c6, c7) \
	extern const AM_GUID name = {x, s1, s2, {c0, c1, c2, c3, c4, c5, c6, c7}}

//pb basic enum/type, put here?
enum {
    eVoutLCD = 0,
    eVoutHDMI,
    eVoutCnt
};

#define D_AVSYNC_ADJUST_INTERVEL 10

typedef enum {
    DEC_SOFTWARE = 1,
    DEC_HYBRID = 2,
    DEC_HARDWARE = 4,
    DEC_DEFAULT = DEC_HARDWARE,
    DEC_SPE_DEFAULT = DEC_HYBRID,
    DEC_OTHER_DEFAULT = DEC_SOFTWARE
} DECTYPE;

typedef enum {
    SCODEC_INVALID = -1,
    SCODEC_MPEG12,
    SCODEC_MPEG4,
    SCODEC_H264,
    SCODEC_VC1,
    SCODEC_RV40,
    SCODEC_NOT_STANDARD_MPEG4,
    SCODEC_OTHER,
    CODEC_NUM
} SCODECID;

typedef struct
{
    AM_UINT log_level, log_option, log_output;
} SLogConfig;

typedef struct
{
    FILE* mLogFd;
    FILE* mErrHandleFd;

    //tmp debug use
    AM_UINT mUseActivePBEngine;
    AM_UINT mUseActiveRecordEngine;
    AM_UINT mUseGeneralDecoder;
    AM_UINT mUsePrivateAudioDecLib;
    AM_UINT mUsePrivateAudioEncLib;
    AM_UINT mUseFFMpegMuxer2;
    AM_U16 mErrorCodeRecordLevel;//error code's log record level(if error code level larger than this value, record it)
    AM_U16 mUseNativeSEGFaultHandler;
    AM_U16 mbEnableOverlay;
}TGlobalCfg;

extern TGlobalCfg g_GlobalCfg;
extern SLogConfig g_ModuleLogConfig[LogModuleListEnd];

#define DSetModuleLogConfig(x) do { \
        SetLogLevelOption(g_ModuleLogConfig[x].log_level, g_ModuleLogConfig[x].log_option, g_ModuleLogConfig[x].log_output); \
    } while (0)

#endif

