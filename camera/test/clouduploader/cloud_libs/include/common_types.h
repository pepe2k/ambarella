/**
 * common_types.h
 *
 * History:
 *  2012/12/07 - [Zhi He] create file
 *
 * Copyright (C) 2012, the ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the ambarella Inc.
 */

#ifndef __COMMON_TYPES_H__
#define __COMMON_TYPES_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  General types
//
//-----------------------------------------------------------------------

typedef int TInt;
typedef unsigned int TUint;

typedef unsigned char TU8;
typedef unsigned short TU16;
typedef unsigned int TU32;

typedef signed char TS8;
typedef signed short TS16;
typedef signed int TS32;

typedef signed long long TS64;
typedef unsigned long long TU64;

typedef long TLong;
typedef unsigned long TULong;

typedef TS64	    TTime;
typedef char TChar;
typedef unsigned char TUChar;

typedef TS64 TFileSize;

typedef volatile int TAtomic;

typedef TULong TMemSize;
typedef TULong TPointer;

//-----------------------------------------------------------------------
//
// Macros
//
//-----------------------------------------------------------------------

// align must be power of 2
#ifndef DROUND_UP
#define DROUND_UP(_size, _align)    (((_size) + ((_align) - 1)) & ~((_align) - 1))
#endif

#ifndef DROUND_DOWN
#define DROUND_DOWN(_size, _align)  ((_size) & ~((_align) - 1))
#endif

#ifndef DCAL_MIN
#define DCAL_MIN(_a, _b)    ((_a) < (_b) ? (_a) : (_b))
#endif

#ifndef DCAL_MAX
#define DCAL_MAX(_a, _b)    ((_a) > (_b) ? (_a) : (_b))
#endif

#ifndef DARRAY_SIZE
#define DARRAY_SIZE(_array) (sizeof(_array) / sizeof(_array[0]))
#endif

#ifndef DPTR_OFFSET
#define DPTR_OFFSET(_type, _member) ((int)&((_type*)0)->member)
#endif

#ifndef DPTR_ADD
#define DPTR_ADD(_ptr, _size)   (void*)((char*)(_ptr) + (_size))
#endif

#define DCAL_BITMASK(x) (1 << x)
#define DREAD_BE32(x) (((*((TU8*)x))<<24) | ((*((TU8*)x + 1))<<16) | ((*((TU8*)x + 2))<<8) | (*((TU8*)x + 3)))
#define DREAD_BE64(x) (((*((TU8*)x))<<56) | ((*((TU8*)x + 1))<<48) | ((*((TU8*)x + 2))<<40) | ((*((TU8*)x + 3))<<32) | ((*((TU8*)x + 4))<<24) | ((*((TU8*)x + 5))<<16) | ((*((TU8*)x + 6))<<8) | (*((TU8*)x + 7)))

#define DLikely(x)   __builtin_expect(!!(x),1)
#define DUnlikely(x)   __builtin_expect(!!(x),0)

//-----------------------------------------------------------------------
//
//  Error code
//
//-----------------------------------------------------------------------

typedef enum
{
    EECode_OK = 0,

    EECode_NotInitilized = 0x001,
    EECode_NotRunning = 0x002,

    EECode_Error = 0x003,
    EECode_Closed = 0x004,
    EECode_Busy = 0x005,
    EECode_OSError = 0x006,
    EECode_IOError = 0x007,
    EECode_TimeOut = 0x008,
    EECode_TooMany = 0x009,
    EECode_OutOfCapability = 0x00a,
    EECode_TimeTick = 0x00b,

    EECode_NoMemory = 0x00c,
    EECode_NoPermission = 0x00d,
    EECode_NoImplement = 0x00e,
    EECode_NoInterface = 0x00f,

    EECode_NotExist = 0x010,
    EECode_NotSupported = 0x011,

    EECode_BadState = 0x012,
    EECode_BadParam = 0x013,
    EECode_BadCommand = 0x014,
    EECode_BadFormat = 0x015,
    EECode_BadMsg = 0x016,
    EECode_BadSessionNumber = 0x017,

    EECode_TryAgain = 0x018,

    EECode_DataCorruption = 0x019,
    EECode_DataMissing = 0x01a,

    EECode_InternalMemoryBug = 0x01b,
    EECode_InternalLogicalBug = 0x01c,
    EECode_InternalParamsBug = 0x01d,

    EECode_NetSendHeader_Fail = 0x100,
    EECode_NetSendPayload_Fail = 0x101,
    EECode_NetReceiveHeader_Fail = 0x102,
    EECode_NetReceivePayload_Fail = 0x103,

    EECode_NetConnectFail = 0x104,
    EECode_ServerReject_NoSuchChannel = 0x105,
    EECode_ServerReject_ChannelIsBusy = 0x106,
    EECode_ServerReject_BadRequestFormat = 0x107,
    EECode_ServerReject_CorruptedProtocol = 0x108,
    EECode_ServerReject_Unknown = 0x109,

    EECode_NetSocketSend_Error = 0x110,
    EECode_NetSocketRecv_Error = 0x111,
} EECode;

extern const TChar* gfGetErrorCodeString(EECode code);

typedef enum
{
    EECodeHint_GenericType = 0,

    EECodeHint_BadSocket = 0x001,
    EECodeHint_SocketError = 0x002,
    EECodeHint_SendHeaderFail = 0x003,
    EECodeHint_SendPayloadFail = 0x004,
    EECodeHint_ReceiveHeaderFail = 0x005,
    EECodeHint_ReceivePayloadFail = 0x006,

    EECodeHint_BadFileDescriptor = 0x007,
    EECodeHint_BadFileIOError = 0x008,
} EECodeHintType;

class CIRectInt
{
public:
    EECode QueryRectPosition(TInt& offset_x, TInt& offset_y);
    EECode QueryRectCenter(TInt& center_x, TInt& center_y);
    EECode QueryRectSize(TInt& size_x, TInt& size_y);

public:
    EECode SetRect(TInt offset_x, TInt offset_y, TInt size_x, TInt size_y);
    EECode SetRectPosition(TInt offset_x, TInt offset_y);
    EECode SetRectSize(TInt offset_x, TInt offset_y);

    EECode MoveRect(TInt offset_x, TInt offset_y);
    EECode MoveRectCenter(TInt offset_x, TInt offset_y);

private:
    TInt mOffsetX, mOffsetY;
    TInt mSizeX, mSizeY;
};

typedef struct {
    TU16 year;
    TU8 month;
    TU8 day;

    TU8 hour;
    TU8 minute;
    TU8 seconds;

    TU8 weekday : 4;
    TU8 specify_weekday : 1;
    TU8 specify_hour : 1;
    TU8 specify_minute : 1;
    TU8 specify_seconds : 1;
} SDateTime;

typedef struct {
    TS32 days;

    TU8 hour;
    TU8 minute;
    TU8 second;

    TU8 calculate_hour_minute_seconds;

    TU64 totoal_hours;
    TU64 total_minutes;
    TU64 total_seconds;
} SDateTimeGap;

void gfDateCalculateGap(SDateTime& current_date, SDateTime& start_date, SDateTimeGap& gap);
void gfDateNextDay(SDateTime& date, TU32 days);
void gfDatePreviousDay(SDateTime& date, TU32 days);

typedef struct {
    TU16 year;
    TU8 month;
    TU8 day;

    TU8 hour;
    TU8 minute;
    TU8 second;
    TU8 frames;
} SStorageTime;

#define DINVALID_VALUE_TAG 0xFEDCFEFE
#define DINVALID_VALUE_TAG_64 0xFEDCFEFEFEDCFEFELL

enum {
    //from dsp define
    EPredefinedPictureType_IDR = 1,
    EPredefinedPictureType_I = 2,
    EPredefinedPictureType_P = 3,
    EPredefinedPictureType_B = 4,
};

typedef enum {
    ERunTimeConfigType_XML = 0x0,
    ERunTimeConfigType_SimpleINI = 0x1,
} ERunTimeConfigType;

typedef enum {
    EIPCAgentType_Invalid = 0x0,
    EIPCAgentType_UnixDomainSocket = 0x1,
    EIPCAgentType_Socket = 0x2,
} EIPCAgentType;

#define BE_16(x) (((TU8 *)(x))[0] <<  8 | ((TU8 *)(x))[1])

#define DBEW32(x, p) do { \
    p[0] = (x >> 24) & 0xff; \
    p[1] = (x >> 16) & 0xff; \
    p[2] = (x >> 8) & 0xff; \
    p[3] = x & 0xff; \
} while(0)

#define DBEW16(x, p) do { \
    p[0] = (x >> 8) & 0xff; \
    p[1] = x & 0xff; \
} while(0)

#define DBER32(x, p) do { \
    x = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; \
} while(0)

#define DBER16(x, p) do { \
    x = (p[0] << 8) | p[1]; \
} while(0)

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

