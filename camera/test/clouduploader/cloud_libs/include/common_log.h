/**
 * common_log.h
 *
 * History:
 *	2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2012, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __COMMON_LOG_H__
#define __COMMON_LOG_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN


#define DCONFIG_ENALDE_DEBUG_LOG

//-----------------------------------------------------------------------
//
//  log system
//
//-----------------------------------------------------------------------

#define DMAX_LOGTAG_NAME_LEN 64

enum ELogLevel {
    ELogLevel_None = 0,
    ELogLevel_Fatal,
    ELogLevel_Error,
    ELogLevel_Warn,
    ELogLevel_Notice,
    ELogLevel_Info,
    ELogLevel_Debug,
    ELogLevel_Verbose,

    //last element
    ELogLevel_TotalCount,
};

enum ELogOption {
    ELogOption_State = 0,
    ELogOption_Congifuration,
    ELogOption_Flow,
    ELogOption_PTS,
    ELogOption_Resource,
    ELogOption_Timing,

    ELogOption_BinaryHeader,
    ELogOption_WholeBinary2WholeFile,
    ELogOption_WholeBinary2SeparateFile,

    //last element
    ELogOption_TotalCount,
};

enum ELogOutput {
    ELogOutput_Console = 0,
    ELogOutput_File,
    ELogOutput_ModuleFile,
    ELogOutput_AndroidLogcat,

    //binady dump
    ELogOutput_BinaryTotalFile,
    ELogOutput_BinarySeperateFile,

    //last element
    ELogOutput_TotalCount,
};

extern volatile TUint* const pgConfigLogLevel;
extern volatile TUint* const pgConfigLogOption;
extern volatile TUint* const pgConfigLogOutput;

extern const TChar gcConfigLogLevelTag[ELogLevel_TotalCount][DMAX_LOGTAG_NAME_LEN];
extern const TChar gcConfigLogOptionTag[ELogOption_TotalCount][DMAX_LOGTAG_NAME_LEN];
extern const TChar gcConfigLogOutputTag[ELogOutput_TotalCount][DMAX_LOGTAG_NAME_LEN];

extern FILE* gpLogOutputFile;
extern FILE* gpSnapshotOutputFile;

//-----------------------------------------------------------------------
//
//  log macros
//
//-----------------------------------------------------------------------
//#define _DLOG_GLOBAL_MUTEX_NAME AUTO_LOCK(gpLogMutex)
#define _DLOG_GLOBAL_MUTEX_NAME

#define _DLOGCOLOR_HEADER_FATAL "\033[40;31m\033[1m"
#define _DLOGCOLOR_HEADER_ERROR "\033[40;31m\033[1m"
#define _DLOGCOLOR_HEADER_WARN "\033[40;33m\033[1m"
#define _DLOGCOLOR_HEADER_NOTICE "\033[40;35m\033[1m"
#define _DLOGCOLOR_HEADER_INFO "\033[40;37m\033[1m"
#define _DLOGCOLOR_HEADER_DETAIL "\033[40;32m"

#define _DLOGCOLOR_HEADER_OPTION "\033[40;36m"

#define _DLOGCOLOR_TAIL_FATAL "\033[0m"
#define _DLOGCOLOR_TAIL_ERROR "\033[0m"
#define _DLOGCOLOR_TAIL_WARN "\033[0m"
#define _DLOGCOLOR_TAIL_NOTICE "\033[0m"
#define _DLOGCOLOR_TAIL_INFO "\033[0m"
#define _DLOGCOLOR_TAIL_DETAIL "\033[0m"

#define _DLOGCOLOR_TAIL_OPTION "\033[0m"

#define _NOLOG(format, args...)	(void)0

#define _LOG_LEVEL_MODULE(level, header, tail, format, args...)  do { \
    if (DUnlikely(mConfigLogLevel >= level)) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(header "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
            printf(format, ##args);  \
            printf(tail); \
        } \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
            if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
            if (DUnlikely(gpSnapshotOutputFile)) { \
                fprintf(gpSnapshotOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(gpSnapshotOutputFile, format,##args);  \
                fflush(gpSnapshotOutputFile); \
            } \
        } \
        if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
            if (DUnlikely(mpLogOutputFile)) { \
                fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(mpLogOutputFile, format,##args);  \
            } else if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_LEVEL(level, header, tail, format, args...)  do { \
    if (DUnlikely((*pgConfigLogLevel) >= level)) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(header "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
            printf(format, ##args);  \
            printf(tail); \
        } \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
            if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
            if (DUnlikely(gpSnapshotOutputFile)) { \
                fprintf(gpSnapshotOutputFile, "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
                fprintf(gpSnapshotOutputFile, format,##args);  \
                fflush(gpSnapshotOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_LEVEL_MODULE_TRACE(level, header, tail, format, args...)  do { \
    if (DLikely(mConfigLogLevel >= level)) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(header "%s\t\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
            printf(format, ##args);  \
            printf(tail); \
            printf("\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
            if (DUnlikely(mpLogOutputFile)) { \
                fprintf(mpLogOutputFile, "%s\t\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
                fprintf(mpLogOutputFile, format,##args);  \
                fprintf(mpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
            } else if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
            if (DUnlikely(gpSnapshotOutputFile)) { \
                fprintf(gpSnapshotOutputFile, "%s\t\t[%s,%d]\t", (const TChar*)gcConfigLogLevelTag[level], GetModuleName(), GetModuleIndex()); \
                fprintf(gpSnapshotOutputFile, format,##args);  \
                fprintf(gpSnapshotOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpSnapshotOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_LEVEL_TRACE(level, header, tail, format, args...)  do { \
    if (DLikely((*pgConfigLogLevel) >= level)) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(header "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
            printf(format, ##args);  \
            printf(tail); \
            printf("\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
            if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
            if (DUnlikely(gpSnapshotOutputFile)) { \
                fprintf(gpSnapshotOutputFile, "%s\t\t", (const TChar*)gcConfigLogLevelTag[level]); \
                fprintf(gpSnapshotOutputFile, format,##args);  \
                fprintf(gpSnapshotOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpSnapshotOutputFile); \
            } \
        } \
    } \
} while (0)


#define _LOG_OPTION_MODULE(option, format, args...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(_DLOGCOLOR_HEADER_OPTION "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
            printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        } \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
            if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
        } \
        if (DUnlikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_ModuleFile))) { \
            if (DUnlikely(mpLogOutputFile)) { \
                fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(mpLogOutputFile, format,##args);  \
            } else if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], (const char*)GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_OPTION(option, format, args...)  do { \
    if (DUnlikely((*pgConfigLogOption) & DCAL_BITMASK(option))) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(_DLOGCOLOR_HEADER_OPTION "%s\t\t", (const TChar*)gcConfigLogOptionTag[option]); \
            printf(format _DLOGCOLOR_TAIL_OPTION, ##args);  \
        } \
        if (DLikely((*pgConfigLogOutput) & DCAL_BITMASK(ELogOutput_File))) { \
            if(gpLogOutputFile) { \
                fprintf(gpLogOutputFile, "%s\t\t", (const TChar*)gcConfigLogOptionTag[option]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_OPTION_MODULE_TRACE(option, format, args...)  do { \
    if (DUnlikely(mConfigLogOption & DCAL_BITMASK(option))) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(_DLOGCOLOR_HEADER_OPTION "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
            printf(format, ##args);  \
            printf("\t\t\t\t\t\t[trace]: file %s, function %s, line %d" _DLOGCOLOR_TAIL_OPTION "\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if (DLikely(mConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
            if (DUnlikely(mpLogOutputFile)) { \
                fprintf(mpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
                fprintf(mpLogOutputFile, format,##args);  \
                fprintf(mpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
            } else if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t[%s,%d]\t", (const TChar*)gcConfigLogOptionTag[option], GetModuleName(), GetModuleIndex()); \
                fprintf(gpLogOutputFile, format,##args);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#define _LOG_OPTION_TRACE(option, format, args...)  do { \
    if (DUnlikely(gConfigLogOption & DCAL_BITMASK(option))) { \
        _DLOG_GLOBAL_MUTEX_NAME; \
        if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_Console))) { \
            printf(_DLOGCOLOR_HEADER_OPTION "%s\t\t", (const TChar*)gcConfigLogOptionTag[option]); \
            printf(format, ##args);  \
            printf("\t\t\t\t\t\t[trace]: file %s, function %s, line %d" _DLOGCOLOR_TAIL_OPTION "\n", __FILE__, __FUNCTION__, __LINE__);  \
        } \
        if (DLikely(gConfigLogOutput & DCAL_BITMASK(ELogOutput_File))) { \
            if (DLikely(gpLogOutputFile)) { \
                fprintf(gpLogOutputFile, "%s\t\t", (const TChar*)gcConfigLogOptionTag[option]); \
                fprintf(gpLogOutputFile, format,##args);  \
                fprintf(gpLogOutputFile, "\t\t\t\t\t\t[trace]: file %s, function %s, line %d\n", __FILE__, __FUNCTION__, __LINE__);  \
                fflush(gpLogOutputFile); \
            } \
        } \
    } \
} while (0)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_VERBOSE(format, args...)       _LOG_LEVEL_MODULE(ELogLevel_Verbose, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL , format, ##args)
#define LOGM_DEBUG(format, args...)         _LOG_LEVEL_MODULE(ELogLevel_Debug, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOGM_INFO(format, args...)          _LOG_LEVEL_MODULE(ELogLevel_Info, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#else
#define LOGM_VERBOSE        _NOLOG
#define LOGM_DEBUG          _NOLOG
#define LOGM_INFO           _NOLOG
#endif

#define LOGM_NOTICE(format, args...)        _LOG_LEVEL_MODULE_TRACE(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#define LOGM_WARN(format, args...)          _LOG_LEVEL_MODULE_TRACE(ELogLevel_Warn, _DLOGCOLOR_HEADER_WARN, _DLOGCOLOR_TAIL_WARN, format, ##args)
#define LOGM_ERROR(format, args...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Error, _DLOGCOLOR_HEADER_ERROR, _DLOGCOLOR_TAIL_ERROR, format, ##args)
#define LOGM_FATAL(format, args...)         _LOG_LEVEL_MODULE_TRACE(ELogLevel_Fatal, _DLOGCOLOR_HEADER_FATAL, _DLOGCOLOR_TAIL_FATAL, format , ##args)
#define LOGM_ALWAYS(format, args...)         _LOG_LEVEL_MODULE(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_VERBOSE(format, args...)        _LOG_LEVEL(ELogLevel_Verbose, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOG_DEBUG(format, args...)          _LOG_LEVEL(ELogLevel_Debug, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#define LOG_INFO(format, args...)           _LOG_LEVEL(ELogLevel_Info, _DLOGCOLOR_HEADER_DETAIL, _DLOGCOLOR_TAIL_DETAIL, format, ##args)
#else
#define LOG_VERBOSE         _NOLOG
#define LOG_DEBUG           _NOLOG
#define LOG_INFO            _NOLOG
#endif

#define LOG_NOTICE(format, args...)         _LOG_LEVEL_TRACE(ELogLevel_Notice, _DLOGCOLOR_HEADER_NOTICE, _DLOGCOLOR_TAIL_NOTICE, format, ##args)
#define LOG_WARN(format, args...)           _LOG_LEVEL_TRACE(ELogLevel_Warn, _DLOGCOLOR_HEADER_WARN, _DLOGCOLOR_TAIL_WARN, format, ##args)
#define LOG_ERROR(format, args...)          _LOG_LEVEL_TRACE(ELogLevel_Error, _DLOGCOLOR_HEADER_ERROR, _DLOGCOLOR_TAIL_ERROR, format, ##args)
#define LOG_FATAL(format, args...)          _LOG_LEVEL_TRACE(ELogLevel_Fatal, _DLOGCOLOR_HEADER_FATAL, _DLOGCOLOR_TAIL_FATAL, format, ##args)
#define LOG_ALWAYS(format, args...)         _LOG_LEVEL(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOGM_STATE(format, args...)         _LOG_OPTION_MODULE(ELogOption_State, format, ##args)
#define LOGM_CONFIGURATION(format, args...)         _LOG_OPTION_MODULE(ELogOption_Congifuration, format, ##args)
#define LOGM_FLOW(format, args...)          _LOG_OPTION_MODULE(ELogOption_Flow, format, ##args)
#define LOGM_PTS(format, args...)           _LOG_OPTION_MODULE(ELogOption_PTS, format, ##args)
#define LOGM_RESOURCE(format, args...)      _LOG_OPTION_MODULE(ELogOption_Resource, format, ##args)
#define LOGM_TIMING(format, args...)        _LOG_OPTION_MODULE(ELogOption_Timing, format, ##args)
#define LOGM_BINARY(format, args...)        _LOG_OPTION_MODULE(ELogOption_BinaryHeader, format, ##args)
#else
#define LOGM_STATE          _NOLOG
#define LOGM_CONFIGURATION          _NOLOG
#define LOGM_FLOW           _NOLOG
#define LOGM_PTS            _NOLOG
#define LOGM_RESOURCE       _NOLOG
#define LOGM_TIMING         _NOLOG
#define LOGM_BINARY         _NOLOG
#endif

#ifdef DCONFIG_ENALDE_DEBUG_LOG
#define LOG_STATE(format, args...)          _LOG_OPTION(ELogOption_State, format, ##args)
#define LOG_CONFIGURATION(format, args...)          _LOG_OPTION(ELogOption_Congifuration, format, ##args)
#define LOG_FLOW(format, args...)           _LOG_OPTION(ELogOption_Flow, format, ##args)
#define LOG_PTS(format, args...)            _LOG_OPTION(ELogOption_PTS, format, ##args)
#define LOG_RESOURCE(format, args...)       _LOG_OPTION(ELogOption_Resource, format, ##args)
#define LOG_TIMING(format, args...)         _LOG_OPTION(ELogOption_Timing, format, ##args)
#define LOG_BINARY(format, args...)         _LOG_OPTION(ELogOption_BinaryHeader, format, ##args)
#else
#define LOG_STATE           _NOLOG
#define LOG_CONFIGURATION           _NOLOG
#define LOG_FLOW            _NOLOG
#define LOG_PTS             _NOLOG
#define LOG_RESOURCE        _NOLOG
#define LOG_TIMING          _NOLOG
#define LOG_BINARY          _NOLOG
#endif

#define LOGM_PRINTF(format, args...)          _LOG_LEVEL_MODULE(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)
#define LOG_PRINTF(format, args...)          _LOG_LEVEL(ELogLevel_None, _DLOGCOLOR_HEADER_INFO, _DLOGCOLOR_TAIL_INFO, format, ##args)

#define ASSERT(expr) do { \
    if (DUnlikely(!(expr))) { \
        printf(_DLOGCOLOR_HEADER_FATAL "assertion failed: %s\n\tAt %s : %s: %d\n" _DLOGCOLOR_TAIL_FATAL, #expr, __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile,"assertion failed: %s\n\tAt %s : %s: %d\n", #expr, __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

#define ASSERT_OK(code) do { \
    if (DUnlikely(EECode_OK != code)) { \
        printf(_DLOGCOLOR_HEADER_FATAL "ERROR code %s\n\tAt %s : %s: %d\n" _DLOGCOLOR_TAIL_FATAL, gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
        if (gpLogOutputFile) { \
            fprintf(gpLogOutputFile, "ERROR code %s\n\tAt %s : %s: %d\n", gfGetErrorCodeString(code), __FILE__, __FUNCTION__, __LINE__); \
            fflush(gpLogOutputFile); \
        } \
    } \
} while (0)

//module index, for log.config
typedef enum {
    //global settings
    ELogGlobal = 0,

    ELogModuleListStart,

    //engines
    ELogModuleGenericEngine = ELogModuleListStart, //first line must not be modified
    ELogModuleStreamingServerManager,
    ELogModuleCloudServerManager,

    //filters
    ELogModuleDemuxerFilter,
    ELogModuleAudioCapturerFilter,
    ELogModuleVideoDecoderFilter,
    ELogModuleAudioDecoderFilter,
    ELogModuleVideoRendererFilter,
    ELogModuleAudioRendererFilter,
    ELogModuleVideoPostPFilter,
    ELogModuleAudioPostPFilter,
    ELogModuleMuxerFilter,
    ELogModuleStreamingTransmiterFilter,
    ELogModuleVideoEncoderFilter,
    ELogModuleAudioEncoderFilter,
    ELogModuleConnecterPinmuxer,
    ELogModuleCloudConnecterServerAgent,
    ELogModuleCloudConnecterClientAgent,
    ELogModuleFlowController,

    //modules
    ELogModulePrivateRTSPDemuxer,
    ELogModulePrivateRTSPScheduledDemuxer,
    ELogModuleUploadingReceiver,
    ELogModuleAmbaDecoder,
    ELogModuleAmbaVideoPostProcessor,
    ELogModuleAmbaVideoRenderer,
    ELogModuleAmbaEncoder,
    ELogModuleStreamingTransmiter,
    ELogModuleFFMpegDemuxer,
    ELogModuleFFMpegMuxer,
    ELogModuleFFMpegAudioDecoder,
    ELogModuleFFMpegVideoDecoder,
    ELogModuleAudioRenderer,
    ELogModuleAudioInput,
    ELogModuleAACAudioDecoder,
    ELogModuleAACAudioEncoder,
    ELogModuleCustomizedAudioDecoder,
    ELogModuleCustomizedAudioEncoder,
    ELogModuleNativeTSMuxer,
    ELogModuleNativeTSDemuxer,
    ELogModuleRTSPStreamingServer,
    ELogModuleCloudServer,

    //dsp platform
    ELogModuleDSPPlatform,
    ELogModuleDSPDec,
    ELogModuleDSPEnc,

    //scheduler
    ELogModuleRunRobinScheduler,
    ELogModulePreemptiveScheduler,

    ELogModuleListEnd,
} ELogModule;

typedef struct
{
    TUint log_level, log_option, log_output;
} SLogConfig;

extern SLogConfig gmModuleLogConfig[ELogModuleListEnd];

#define DSET_MODULE_LOG_CONFIG(x) do { \
        SetLogLevel(gmModuleLogConfig[x].log_level); \
        SetLogOption(gmModuleLogConfig[x].log_option); \
        SetLogOutput(gmModuleLogConfig[x].log_output); \
    } while (0)

#define DLOG_MASK_TO_SNAPSHOT (1 << 4)
#define DLOG_MASK_TO_REMOTE (1 << 5)

extern EECode gfGetLogLevelString(ELogLevel level, const TChar*& loglevel_string);
extern EECode gfGetLogModuleString(ELogModule module, const TChar*& module_string);
extern EECode gfGetLogModuleIndex(const TChar* module_name, TUint& index);

extern void gfOpenLogFile(TChar* name);
extern void gfCloseLogFile();

extern void gfOpenSnapshotFile(TChar* name);
extern void gfCloseSnapshotFile();

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

