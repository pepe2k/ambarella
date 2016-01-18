/*
 * include/ambas_stream_texture.h
 *
 * History:
 *    2011/01/05 - [Zhenwu Xue] Create
 *
 * Copyright (C) 2004-2012, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBAS_STREAM_TEXTURE_H
#define __AMBAS_STREAM_TEXTURE_H

#include <config.h>

typedef enum _PVRSRV_PIXEL_FORMAT_ {

	PVRSRV_PIXEL_FORMAT_UNKNOWN				=  0,
	PVRSRV_PIXEL_FORMAT_RGB565				=  1,
	PVRSRV_PIXEL_FORMAT_RGB555				=  2,
	PVRSRV_PIXEL_FORMAT_RGB888				=  3,
	PVRSRV_PIXEL_FORMAT_BGR888				=  4,
	PVRSRV_PIXEL_FORMAT_GREY_SCALE				=  8,
	PVRSRV_PIXEL_FORMAT_PAL12				= 13,
	PVRSRV_PIXEL_FORMAT_PAL8				= 14,
	PVRSRV_PIXEL_FORMAT_PAL4				= 15,
	PVRSRV_PIXEL_FORMAT_PAL2				= 16,
	PVRSRV_PIXEL_FORMAT_PAL1				= 17,
	PVRSRV_PIXEL_FORMAT_ARGB1555				= 18,
	PVRSRV_PIXEL_FORMAT_ARGB4444				= 19,
	PVRSRV_PIXEL_FORMAT_ARGB8888				= 20,
	PVRSRV_PIXEL_FORMAT_ABGR8888				= 21,
	PVRSRV_PIXEL_FORMAT_YV12				= 22,
	PVRSRV_PIXEL_FORMAT_I420				= 23,
	PVRSRV_PIXEL_FORMAT_IMC2				= 25,
	PVRSRV_PIXEL_FORMAT_XRGB8888				= 26,
	PVRSRV_PIXEL_FORMAT_XBGR8888				= 27,
	PVRSRV_PIXEL_FORMAT_BGRA8888				= 28,
	PVRSRV_PIXEL_FORMAT_XRGB4444				= 29,
	PVRSRV_PIXEL_FORMAT_ARGB8332				= 30,
	PVRSRV_PIXEL_FORMAT_A2RGB10				= 31,
	PVRSRV_PIXEL_FORMAT_A2BGR10				= 32,
	PVRSRV_PIXEL_FORMAT_P8					= 33,
	PVRSRV_PIXEL_FORMAT_L8					= 34,
	PVRSRV_PIXEL_FORMAT_A8L8				= 35,
	PVRSRV_PIXEL_FORMAT_A4L4				= 36,
	PVRSRV_PIXEL_FORMAT_L16					= 37,
	PVRSRV_PIXEL_FORMAT_L6V5U5				= 38,
	PVRSRV_PIXEL_FORMAT_V8U8				= 39,
	PVRSRV_PIXEL_FORMAT_V16U16				= 40,
	PVRSRV_PIXEL_FORMAT_QWVU8888				= 41,
	PVRSRV_PIXEL_FORMAT_XLVU8888				= 42,
	PVRSRV_PIXEL_FORMAT_QWVU16				= 43,
	PVRSRV_PIXEL_FORMAT_D16					= 44,
	PVRSRV_PIXEL_FORMAT_D24S8				= 45,
	PVRSRV_PIXEL_FORMAT_D24X8				= 46,


	PVRSRV_PIXEL_FORMAT_ABGR16				= 47,
	PVRSRV_PIXEL_FORMAT_ABGR16F				= 48,
	PVRSRV_PIXEL_FORMAT_ABGR32				= 49,
	PVRSRV_PIXEL_FORMAT_ABGR32F				= 50,
	PVRSRV_PIXEL_FORMAT_B10GR11				= 51,
	PVRSRV_PIXEL_FORMAT_GR88				= 52,
	PVRSRV_PIXEL_FORMAT_BGR32				= 53,
	PVRSRV_PIXEL_FORMAT_GR32				= 54,
	PVRSRV_PIXEL_FORMAT_E5BGR9				= 55,


	PVRSRV_PIXEL_FORMAT_RESERVED1				= 56,
	PVRSRV_PIXEL_FORMAT_RESERVED2				= 57,
	PVRSRV_PIXEL_FORMAT_RESERVED3				= 58,
	PVRSRV_PIXEL_FORMAT_RESERVED4				= 59,
	PVRSRV_PIXEL_FORMAT_RESERVED5				= 60,


	PVRSRV_PIXEL_FORMAT_R8G8_B8G8				= 61,
	PVRSRV_PIXEL_FORMAT_G8R8_G8B8				= 62,


	PVRSRV_PIXEL_FORMAT_NV11				= 63,
	PVRSRV_PIXEL_FORMAT_NV12				= 64,


	PVRSRV_PIXEL_FORMAT_YUY2				= 65,
	PVRSRV_PIXEL_FORMAT_YUV420				= 66,
	PVRSRV_PIXEL_FORMAT_YUV444				= 67,
	PVRSRV_PIXEL_FORMAT_VUY444				= 68,
	PVRSRV_PIXEL_FORMAT_YUYV				= 69,
	PVRSRV_PIXEL_FORMAT_YVYU				= 70,
	PVRSRV_PIXEL_FORMAT_UYVY				= 71,
	PVRSRV_PIXEL_FORMAT_VYUY				= 72,

	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_UYVY			= 73,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YUYV			= 74,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_YVYU			= 75,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_VYUY			= 76,
	PVRSRV_PIXEL_FORMAT_FOURCC_ORG_AYUV			= 77,


	PVRSRV_PIXEL_FORMAT_A32B32G32R32			= 78,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32F			= 79,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32_UINT			= 80,
	PVRSRV_PIXEL_FORMAT_A32B32G32R32_SINT			= 81,


	PVRSRV_PIXEL_FORMAT_B32G32R32				= 82,
	PVRSRV_PIXEL_FORMAT_B32G32R32F				= 83,
	PVRSRV_PIXEL_FORMAT_B32G32R32_UINT			= 84,
	PVRSRV_PIXEL_FORMAT_B32G32R32_SINT			= 85,


	PVRSRV_PIXEL_FORMAT_G32R32				= 86,
	PVRSRV_PIXEL_FORMAT_G32R32F				= 87,
	PVRSRV_PIXEL_FORMAT_G32R32_UINT				= 88,
	PVRSRV_PIXEL_FORMAT_G32R32_SINT				= 89,


	PVRSRV_PIXEL_FORMAT_D32F				= 90,
	PVRSRV_PIXEL_FORMAT_R32					= 91,
	PVRSRV_PIXEL_FORMAT_R32F				= 92,
	PVRSRV_PIXEL_FORMAT_R32_UINT				= 93,
	PVRSRV_PIXEL_FORMAT_R32_SINT				= 94,


	PVRSRV_PIXEL_FORMAT_A16B16G16R16			= 95,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16F			= 96,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_SINT			= 97,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_SNORM			= 98,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_UINT			= 99,
	PVRSRV_PIXEL_FORMAT_A16B16G16R16_UNORM			= 100,


	PVRSRV_PIXEL_FORMAT_G16R16				= 101,
	PVRSRV_PIXEL_FORMAT_G16R16F				= 102,
	PVRSRV_PIXEL_FORMAT_G16R16_UINT				= 103,
	PVRSRV_PIXEL_FORMAT_G16R16_UNORM			= 104,
	PVRSRV_PIXEL_FORMAT_G16R16_SINT				= 105,
	PVRSRV_PIXEL_FORMAT_G16R16_SNORM			= 106,


	PVRSRV_PIXEL_FORMAT_R16					= 107,
	PVRSRV_PIXEL_FORMAT_R16F				= 108,
	PVRSRV_PIXEL_FORMAT_R16_UINT				= 109,
	PVRSRV_PIXEL_FORMAT_R16_UNORM				= 110,
	PVRSRV_PIXEL_FORMAT_R16_SINT				= 111,
	PVRSRV_PIXEL_FORMAT_R16_SNORM				= 112,


	PVRSRV_PIXEL_FORMAT_X8R8G8B8				= 113,
	PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM			= 114,
	PVRSRV_PIXEL_FORMAT_X8R8G8B8_UNORM_SRGB			= 115,

	PVRSRV_PIXEL_FORMAT_A8R8G8B8				= 116,
	PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM			= 117,
	PVRSRV_PIXEL_FORMAT_A8R8G8B8_UNORM_SRGB			= 118,

	PVRSRV_PIXEL_FORMAT_A8B8G8R8				= 119,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UINT			= 120,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM			= 121,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_UNORM_SRGB			= 122,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_SINT			= 123,
	PVRSRV_PIXEL_FORMAT_A8B8G8R8_SNORM			= 124,


	PVRSRV_PIXEL_FORMAT_G8R8				= 125,
	PVRSRV_PIXEL_FORMAT_G8R8_UINT				= 126,
	PVRSRV_PIXEL_FORMAT_G8R8_UNORM				= 127,
	PVRSRV_PIXEL_FORMAT_G8R8_SINT				= 128,
	PVRSRV_PIXEL_FORMAT_G8R8_SNORM				= 129,


	PVRSRV_PIXEL_FORMAT_A8					= 130,
	PVRSRV_PIXEL_FORMAT_R8					= 131,
	PVRSRV_PIXEL_FORMAT_R8_UINT				= 132,
	PVRSRV_PIXEL_FORMAT_R8_UNORM				= 133,
	PVRSRV_PIXEL_FORMAT_R8_SINT				= 134,
	PVRSRV_PIXEL_FORMAT_R8_SNORM				= 135,


	PVRSRV_PIXEL_FORMAT_A2B10G10R10				= 136,
	PVRSRV_PIXEL_FORMAT_A2B10G10R10_UNORM			= 137,
	PVRSRV_PIXEL_FORMAT_A2B10G10R10_UINT			= 138,


	PVRSRV_PIXEL_FORMAT_B10G11R11				= 139,
	PVRSRV_PIXEL_FORMAT_B10G11R11F				= 140,


	PVRSRV_PIXEL_FORMAT_X24G8R32				= 141,
	PVRSRV_PIXEL_FORMAT_G8R24				= 142,
	PVRSRV_PIXEL_FORMAT_X8R24				= 143,
	PVRSRV_PIXEL_FORMAT_E5B9G9R9				= 144,
	PVRSRV_PIXEL_FORMAT_R1					= 145,

	PVRSRV_PIXEL_FORMAT_RESERVED6				= 146,
	PVRSRV_PIXEL_FORMAT_RESERVED7				= 147,
	PVRSRV_PIXEL_FORMAT_RESERVED8				= 148,
	PVRSRV_PIXEL_FORMAT_RESERVED9				= 149,
	PVRSRV_PIXEL_FORMAT_RESERVED10				= 150,
	PVRSRV_PIXEL_FORMAT_RESERVED11				= 151,
	PVRSRV_PIXEL_FORMAT_RESERVED12				= 152,
	PVRSRV_PIXEL_FORMAT_RESERVED13				= 153,
	PVRSRV_PIXEL_FORMAT_RESERVED14				= 154,
	PVRSRV_PIXEL_FORMAT_RESERVED15				= 155,
	PVRSRV_PIXEL_FORMAT_RESERVED16				= 156,
	PVRSRV_PIXEL_FORMAT_RESERVED17				= 157,
	PVRSRV_PIXEL_FORMAT_RESERVED18				= 158,
	PVRSRV_PIXEL_FORMAT_RESERVED19				= 159,
	PVRSRV_PIXEL_FORMAT_RESERVED20				= 160,


	PVRSRV_PIXEL_FORMAT_UBYTE4				= 161,
	PVRSRV_PIXEL_FORMAT_SHORT4				= 162,
	PVRSRV_PIXEL_FORMAT_SHORT4N				= 163,
	PVRSRV_PIXEL_FORMAT_USHORT4N				= 164,
	PVRSRV_PIXEL_FORMAT_SHORT2N				= 165,
	PVRSRV_PIXEL_FORMAT_SHORT2				= 166,
	PVRSRV_PIXEL_FORMAT_USHORT2N				= 167,
	PVRSRV_PIXEL_FORMAT_UDEC3				= 168,
	PVRSRV_PIXEL_FORMAT_DEC3N				= 169,
	PVRSRV_PIXEL_FORMAT_F16_2				= 170,
	PVRSRV_PIXEL_FORMAT_F16_4				= 171,


	PVRSRV_PIXEL_FORMAT_L_F16				= 172,
	PVRSRV_PIXEL_FORMAT_L_F16_REP				= 173,
	PVRSRV_PIXEL_FORMAT_L_F16_A_F16				= 174,
	PVRSRV_PIXEL_FORMAT_A_F16				= 175,
	PVRSRV_PIXEL_FORMAT_B16G16R16F				= 176,

	PVRSRV_PIXEL_FORMAT_L_F32				= 177,
	PVRSRV_PIXEL_FORMAT_A_F32				= 178,
	PVRSRV_PIXEL_FORMAT_L_F32_A_F32				= 179,


	PVRSRV_PIXEL_FORMAT_PVRTC2				= 180,
	PVRSRV_PIXEL_FORMAT_PVRTC4				= 181,
	PVRSRV_PIXEL_FORMAT_PVRTCII2				= 182,
	PVRSRV_PIXEL_FORMAT_PVRTCII4				= 183,
	PVRSRV_PIXEL_FORMAT_PVRTCIII				= 184,
	PVRSRV_PIXEL_FORMAT_PVRO8				= 185,
	PVRSRV_PIXEL_FORMAT_PVRO88				= 186,
	PVRSRV_PIXEL_FORMAT_PT1					= 187,
	PVRSRV_PIXEL_FORMAT_PT2					= 188,
	PVRSRV_PIXEL_FORMAT_PT4					= 189,
	PVRSRV_PIXEL_FORMAT_PT8					= 190,
	PVRSRV_PIXEL_FORMAT_PTW					= 191,
	PVRSRV_PIXEL_FORMAT_PTB					= 192,
	PVRSRV_PIXEL_FORMAT_MONO8				= 193,
	PVRSRV_PIXEL_FORMAT_MONO16				= 194,


	PVRSRV_PIXEL_FORMAT_C0_YUYV				= 195,
	PVRSRV_PIXEL_FORMAT_C0_UYVY				= 196,
	PVRSRV_PIXEL_FORMAT_C0_YVYU				= 197,
	PVRSRV_PIXEL_FORMAT_C0_VYUY				= 198,
	PVRSRV_PIXEL_FORMAT_C1_YUYV				= 199,
	PVRSRV_PIXEL_FORMAT_C1_UYVY				= 200,
	PVRSRV_PIXEL_FORMAT_C1_YVYU				= 201,
	PVRSRV_PIXEL_FORMAT_C1_VYUY				= 202,


	PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_UV			= 203,
	PVRSRV_PIXEL_FORMAT_C0_YUV420_2P_VU			= 204,
	PVRSRV_PIXEL_FORMAT_C0_YUV420_3P			= 205,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_UV			= 206,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_2P_VU			= 207,
	PVRSRV_PIXEL_FORMAT_C1_YUV420_3P			= 208,

	PVRSRV_PIXEL_FORMAT_A2B10G10R10F			= 209,
	PVRSRV_PIXEL_FORMAT_B8G8R8_SINT				= 210,
	PVRSRV_PIXEL_FORMAT_PVRF32SIGNMASK			= 211,

	PVRSRV_PIXEL_FORMAT_ABGR4444				= 212,
	PVRSRV_PIXEL_FORMAT_ABGR1555				= 213,
	PVRSRV_PIXEL_FORMAT_BGR565				= 214,

	PVRSRV_PIXEL_FORMAT_C0_4KYUV420_2P_UV			= 215,
	PVRSRV_PIXEL_FORMAT_C0_4KYUV420_2P_VU			= 216,
	PVRSRV_PIXEL_FORMAT_C1_4KYUV420_2P_UV			= 217,
	PVRSRV_PIXEL_FORMAT_C1_4KYUV420_2P_VU			= 218,
	PVRSRV_PIXEL_FORMAT_P208				= 219,
	PVRSRV_PIXEL_FORMAT_A8P8				= 220,

	PVRSRV_PIXEL_FORMAT_A4					= 221,
	PVRSRV_PIXEL_FORMAT_AYUV8888				= 222,
	PVRSRV_PIXEL_FORMAT_RAW256				= 223,
	PVRSRV_PIXEL_FORMAT_RAW512				= 224,
	PVRSRV_PIXEL_FORMAT_RAW1024				= 225,

	PVRSRV_PIXEL_FORMAT_FORCE_I32				= 0x7fffffff

} PVRSRV_PIXEL_FORMAT;

#ifndef IMG_UINT32
#define IMG_UINT32 unsigned int
#endif

#ifndef IMG_CHAR
#define IMG_CHAR char
#endif

#ifndef IMG_SYS_PHYADDR
typedef struct {IMG_UINT32 uiAddr;} IMG_SYS_PHYADDR;
#endif

#ifndef IMG_CPU_VIRTADDR
typedef void *	IMG_CPU_VIRTADDR;
#endif

#define MAX_BUFFER_DEVICE_NAME_SIZE				50

typedef struct BUFFER_INFO_TAG
{
	IMG_UINT32 			ui32BufferCount;
	IMG_UINT32			ui32BufferDeviceID;
	PVRSRV_PIXEL_FORMAT		pixelformat;
	IMG_UINT32			ui32ByteStride;
	IMG_UINT32			ui32Width;
	IMG_UINT32			ui32Height;
	IMG_UINT32			ui32Flags;
	IMG_CHAR			szDeviceName[MAX_BUFFER_DEVICE_NAME_SIZE];
} BUFFER_INFO;

typedef enum _BCE_ERROR_
{
	BCE_OK                             =  0,
	BCE_ERROR_GENERIC                  =  1,
	BCE_ERROR_OUT_OF_MEMORY            =  2,
	BCE_ERROR_TOO_FEW_BUFFERS          =  3,
	BCE_ERROR_INVALID_PARAMS           =  4,
	BCE_ERROR_INIT_FAILURE             =  5,
	BCE_ERROR_CANT_REGISTER_CALLBACK   =  6,
	BCE_ERROR_INVALID_DEVICE           =  7,
	BCE_ERROR_DEVICE_REGISTER_FAILED   =  8,
	BCE_ERROR_NO_PRIMARY               =  9
} BCE_ERROR;

typedef void *					BCE_HANDLE;

typedef struct _PVRSRV_SYNC_DATA_
{

	IMG_UINT32				ui32WriteOpsPending;
	volatile IMG_UINT32			ui32WriteOpsComplete;


	IMG_UINT32				ui32ReadOpsPending;
	volatile IMG_UINT32			ui32ReadOpsComplete;


	IMG_UINT32				ui32LastOpDumpVal;
	IMG_UINT32				ui32LastReadOpDumpVal;

} PVRSRV_SYNC_DATA;

typedef struct STREAM_TEXTURE_BUFFER_TAG
{
	unsigned long				ulBufferDeviceID;
	unsigned long				ulBufferID;

	unsigned long           		ulSize;
	BCE_HANDLE              		hMemHandle;

#ifdef CONFIG_SGX_STREAMING_BUFFER_PREALLOCATED
	IMG_SYS_PHYADDR				sSysAddr;
	IMG_SYS_PHYADDR         		sPageAlignSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_KMALLOC
	IMG_SYS_PHYADDR				sSysAddr;
	IMG_SYS_PHYADDR         		sPageAlignSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_VMALLOC
	IMG_SYS_PHYADDR				*psSysAddr;
#endif

#ifdef CONFIG_SGX_STREAMING_BUFFER_ATT
	IMG_SYS_PHYADDR				*psSysAddr;
#endif

	IMG_CPU_VIRTADDR        		sCPUVAddr;
	PVRSRV_SYNC_DATA        		*psSyncData;

	struct STREAM_TEXTURE_BUFFER_TAG	*psNext;
} STREAM_TEXTURE_BUFFER;

typedef struct STREAM_TEXTURE_DEVINFO_TAG
{
	unsigned long           		ulDeviceID;
	BUFFER_INFO             		sBufferInfo;

	STREAM_TEXTURE_BUFFER       		*psSystemBuffer;
	unsigned long           		ulNumBuffers;

	unsigned long           		ulRefCount;
}  STREAM_TEXTURE_DEVINFO;

typedef struct STREAM_TEXTURE_FB2INFO_TAG
{
	unsigned long           		width;
	unsigned long           		height;
	PVRSRV_PIXEL_FORMAT			format;
	void *					buffer[64];	//Physical Addr Array
	unsigned int				n_buffer;
	unsigned int				size;
}  STREAM_TEXTURE_FB2INFO;

#define PVRSRV_BC_FLAGS_YUVCSC_CONFORMANT_RANGE		(0 << 0)
#define PVRSRV_BC_FLAGS_YUVCSC_FULL_RANGE		(1 << 0)

#define PVRSRV_BC_FLAGS_YUVCSC_BT601			(0 << 1)
#define PVRSRV_BC_FLAGS_YUVCSC_BT709			(1 << 1)

enum STREAM_TEXTURE_IOCTL_ENUM
{
	STREAM_TEXTURE_CREATE_INPUT_DEVICE,
	STREAM_TEXTURE_DESTROY_INPUT_DEVICE,
	STREAM_TEXTURE_GET_INPUT_DEVICE,
	STREAM_TEXTURE_SET_INPUT_BUFFER,
	STREAM_TEXTURE_GET_INPUT_BUFFER,
	STREAM_TEXTURE_MAP_INPUT_BUFFER,

	STREAM_TEXTURE_CREATE_OUTPUT_DEVICE,
	STREAM_TEXTURE_DESTROY_OUTPUT_DEVICE,
	STREAM_TEXTURE_GET_OUTPUT_DEVICE,
	STREAM_TEXTURE_SET_OUTPUT_BUFFER,
	STREAM_TEXTURE_GET_OUTPUT_BUFFER,
	STREAM_TEXTURE_MAP_OUTPUT_BUFFER,
	STREAM_TEXTURE_GET_DISPLAY_OFFSET,
};

#endif
